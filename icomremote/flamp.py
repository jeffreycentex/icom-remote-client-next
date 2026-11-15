# SPDX-License-Identifier: GPL-3.0-or-later
"""Receive-only FLAMP framing, file assembly, and bounded asynchronous saving.

Protocol reference is FLAMP by Dave Freese W1HKJ and Robert Stiles KK5VD,
GPL-3.0-or-later, https://github.com/w1hkj/flamp.
"""

import base64
from collections import OrderedDict
from dataclasses import dataclass, field
import lzma
from pathlib import Path
import queue
import re
import struct
import threading
import time

from .sinks import _exclusive_file

MAX_FILE = 32 * 1024 * 1024
MAX_STORAGE = 64 * 1024 * 1024
MAX_DICTIONARY = 64 * 1024 * 1024
MAX_FRAME = 65536
MAX_BLOCKS = 65536
MAX_TRANSFERS = 32
FRAME_LIMITS = {b"FILE": 1024, b"SIZE": 64, b"ID": 1024, b"DESC": 4096,
                b"PROG": 256, b"CNTL": 256, b"DTTM": 128, b"DATA": MAX_FRAME}
HEADER = re.compile(rb"<(FILE|SIZE|DATA|ID|DESC|PROG|CNTL|DTTM) ([0-9]{1,6}) ([0-9A-Fa-f]{4})>")
IDENTIFIER = re.compile(rb"{([0-9A-Fa-f]{4})(?::([0-9]{1,6}))?}(.*)", re.S)
ESCAPES = {ord(":"): ord(":"), **{ord(str(n)): n for n in range(10)},
           ord("A"): 10, ord("B"): 13, ord("C"): 94, ord("D"): 127, ord("E"): 255}


def crc16(data):
    crc = 0xffff
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xa001 if crc & 1 else 0)
    return crc


def safe_filename(name, identifier):
    name = name.replace("\\", "/").rsplit("/", 1)[-1]
    name = re.sub(r'[<>:"/\\|?*\x00-\x1f\x7f]', "_", name).strip(" .")
    name = name.encode("utf-8")[:200].decode("utf-8", errors="ignore")
    stem = name.split(".", 1)[0].upper()
    if stem in {"CON", "PRN", "AUX", "NUL", *(f"COM{n}" for n in range(1, 10)),
                *(f"LPT{n}" for n in range(1, 10))}:
        name = "_" + name
    return name or "flamp_%s.dat" % identifier


def _decode_binary(body, kind):
    count_text, separator, encoded = body.partition(b"\n")
    if not separator or not count_text.isdigit() or len(count_text) > 10:
        raise ValueError("Invalid BASE%d byte count" % kind)
    count = int(count_text)
    if count > MAX_FILE:
        raise ValueError("Encoded file exceeds 32 MiB")
    unpacked = bytearray()
    index = 0
    while index < len(encoded):
        byte = encoded[index]
        if byte == ord(":"):
            index += 1
            if index == len(encoded) or encoded[index] not in ESCAPES:
                raise ValueError("Invalid binary escape")
            byte = ESCAPES[encoded[index]]
        unpacked.append(byte)
        index += 1
    if kind == 256:
        if len(unpacked) != count:
            raise ValueError("BASE256 byte count mismatch")
        return bytes(unpacked)
    if len(unpacked) != ((count + 6) // 7) * 8 or any(byte >= 128 for byte in unpacked):
        raise ValueError("Invalid BASE128 data")
    result = bytearray()
    for start in range(0, len(unpacked), 8):
        value = 0
        for byte in unpacked[start:start + 8]:
            value = (value << 7) | byte
        result.extend(value.to_bytes(7, "big"))
    return bytes(result[:count])


def decode_file(payload):
    prefix = suffix = b""
    for kind in (64, 128, 256):
        begin = ("[b%d:start]" % kind).encode()
        end = ("\n[b%d:end]" % kind).encode()
        start = payload.find(begin)
        if start < 0:
            continue
        finish = payload.find(end, start + len(begin))
        if finish < 0:
            raise ValueError("Incomplete binary envelope")
        prefix, suffix = payload[:start], payload[finish + len(end):]
        body = payload[start + len(begin):finish]
        if kind == 64:
            body = body.translate(None, bytes(range(33)))
            payload = base64.b64decode(body, validate=True)
        else:
            payload = _decode_binary(body, kind)
        break
    if payload.startswith(b"\x01LZMA"):
        if len(payload) < 14:
            raise ValueError("Truncated LZMA header")
        size = struct.unpack(">I", payload[5:9])[0]
        dictionary = struct.unpack("<I", payload[10:14])[0]
        if size > MAX_FILE or dictionary > MAX_DICTIONARY:
            raise ValueError("LZMA output exceeds 32 MiB or dictionary exceeds 64 MiB")
        properties = payload[9]
        lc = properties % 9
        lp = properties // 9 % 5
        pb = properties // 45
        filters = [{"id": lzma.FILTER_LZMA1, "dict_size": max(4096, dictionary),
                    "lc": lc, "lp": lp, "pb": pb}]
        decoder = lzma.LZMADecompressor(format=lzma.FORMAT_RAW, filters=filters)
        payload = decoder.decompress(payload[14:], max_length=size + 1)
        if len(payload) != size:
            raise ValueError("LZMA output size mismatch")
    result = prefix + payload + suffix
    if len(result) > MAX_FILE:
        raise ValueError("Decoded file exceeds 32 MiB")
    return result


@dataclass
class Transfer:
    identifier: str
    file_header: bytes | None = None
    filename: str = ""
    sender: str = ""
    size: tuple | None = None
    blocks: dict = field(default_factory=dict)
    received_bytes: int = 0
    payload: bytes | None = None
    error: str = ""
    save_error: str = ""
    saved_path: str = ""

    def info(self):
        count = self.size[1] if self.size else None
        missing = [number for number in range(1, count + 1) if number not in self.blocks] if count else []
        if self.error:
            status = self.error
        elif self.save_error:
            status = "Save failed: " + self.save_error
        elif self.saved_path:
            status = "Saved"
        elif self.payload is not None:
            status = "Ready to save"
        elif self.size is None or not self.filename:
            status = "Waiting for metadata"
        else:
            status = "Receiving"
        display = ", ".join(str(number) for number in missing[:64])
        if len(missing) > 64:
            display += ", …"
        return {"id": self.identifier, "filename": self.filename or "(waiting for FILE)",
                "sender": self.sender, "received": len(self.blocks), "total": count,
                "missing": display, "missing_count": len(missing), "status": status,
                "ready": self.payload is not None and not self.error,
                "path": self.saved_path}


class FlampReceiver:
    def __init__(self):
        self.transfers = OrderedDict()
        self.buffer = bytearray()
        self.stored_bytes = 0
        self.bad_crc = 0

    def reset_stream(self):
        self.buffer.clear()

    def clear(self, identifier=None):
        keys = [identifier] if identifier else list(self.transfers)
        for key in keys:
            transfer = self.transfers.pop(key, None)
            if transfer is not None:
                self.stored_bytes -= transfer.received_bytes + len(transfer.payload or b"")
        if identifier is None:
            self.reset_stream()

    def feed(self, data):
        # Decoder batches are bounded. Trim unframed noise as soon as it arrives.
        for start in range(0, len(data), 8192):
            self.buffer.extend(data[start:start + 8192])
            self._parse()

    def _parse(self):
        while self.buffer:
            header = HEADER.search(self.buffer)
            if header is None:
                del self.buffer[:-48]
                return
            if header.start():
                del self.buffer[:header.start()]
                continue
            kind, length, expected = header.groups()
            length = int(length)
            if length > FRAME_LIMITS[kind]:
                del self.buffer[0]
                continue
            end = header.end() + length
            if len(self.buffer) < end:
                return
            body = bytes(self.buffer[header.end():end])
            if crc16(body) != int(expected, 16):
                self.bad_crc += 1
                del self.buffer[0]
                continue
            del self.buffer[:end]
            self._record(kind, body)

    def _record(self, kind, body):
        if kind not in (b"FILE", b"SIZE", b"DATA", b"ID"):
            return
        match = IDENTIFIER.fullmatch(body)
        if match is None:
            return
        identifier, number, content = match.groups()
        identifier = identifier.decode().upper()
        transfer = self.transfers.get(identifier)
        if transfer is None:
            if len(self.transfers) >= MAX_TRANSFERS:
                raise ValueError("FLAMP transfer limit reached. Clear old transfers to receive more.")
            transfer = self.transfers[identifier] = Transfer(identifier)
        if transfer.error:
            return
        if kind == b"FILE":
            timestamp, separator, name = content.partition(b":")
            if not separator or not timestamp or len(timestamp) > 32 or not name:
                return
            if transfer.file_header is not None and transfer.file_header != content:
                transfer.error = "Conflicting FILE metadata. Clear this transfer to restart."
                return
            transfer.file_header = content
            transfer.filename = safe_filename(name.decode("utf-8", errors="replace"), identifier)
        elif kind == b"SIZE":
            fields = content.split()
            if len(fields) != 3 or not all(value.isdigit() and len(value) <= 10 for value in fields):
                return
            size, count, block_size = map(int, fields)
            if (size > MAX_FILE or count > MAX_BLOCKS or not 1 <= block_size <= MAX_FRAME - 16
                    or count != (size + block_size - 1) // block_size):
                transfer.error = "Invalid or oversized SIZE metadata"
                return
            metadata = (size, count, block_size)
            if transfer.size is not None and transfer.size != metadata:
                transfer.error = "Conflicting SIZE metadata. Clear this transfer to restart."
                return
            transfer.size = metadata
            for block in list(transfer.blocks):
                if not self._valid_block(transfer, block, transfer.blocks[block]):
                    removed = len(transfer.blocks.pop(block))
                    transfer.received_bytes -= removed
                    self.stored_bytes -= removed
        elif kind == b"ID":
            transfer.sender = content[:512].decode("utf-8", errors="replace")
        elif kind == b"DATA":
            if number is None or not 1 <= int(number) <= MAX_BLOCKS:
                return
            block = int(number)
            if not self._valid_block(transfer, block, content):
                return
            previous = transfer.blocks.get(block)
            if previous is not None:
                if previous != content:
                    transfer.error = "Conflicting data block %d. Clear this transfer to restart." % block
                return
            if self.stored_bytes + len(content) > MAX_STORAGE:
                raise ValueError("FLAMP storage limit reached. Clear old transfers to receive more.")
            transfer.blocks[block] = content
            transfer.received_bytes += len(content)
            self.stored_bytes += len(content)
        self._complete(transfer)

    @staticmethod
    def _valid_block(transfer, number, content):
        if transfer.size is None:
            return True
        size, count, block_size = transfer.size
        expected = block_size if number < count else size - (count - 1) * block_size
        return 1 <= number <= count and len(content) == expected

    def _complete(self, transfer):
        if (transfer.payload is not None or transfer.error or transfer.size is None
                or not transfer.filename or len(transfer.blocks) != transfer.size[1]):
            return
        encoded = b"".join(transfer.blocks[number] for number in range(1, transfer.size[1] + 1))
        try:
            payload = decode_file(encoded)
            if self.stored_bytes + len(payload) > MAX_STORAGE:
                raise ValueError("FLAMP storage limit reached. Clear old transfers to receive more.")
        except (ValueError, lzma.LZMAError) as error:
            transfer.error = "Decode failed: " + str(error)
            return
        transfer.payload = payload
        self.stored_bytes += len(payload)

    def save(self, identifier, directory):
        transfer = self.transfers[identifier]
        if transfer.payload is None or transfer.error:
            raise ValueError("This transfer is incomplete or invalid")
        folder = Path(directory).expanduser().resolve()
        folder.mkdir(parents=True, exist_ok=True)
        candidate, output = _exclusive_file(str(folder / transfer.filename))
        try:
            with output:
                output.write(transfer.payload)
        except OSError:
            Path(candidate).unlink(missing_ok=True)
            raise
        transfer.saved_path = candidate
        transfer.save_error = ""
        return candidate


class FlampWorker:
    """One owner for framing, transfer state and disk writes."""

    def __init__(self, on_event):
        self.receiver = FlampReceiver()
        self._emit = on_event
        self._queue = queue.Queue(maxsize=256)
        self._closed = threading.Event()
        self._directory = None
        self._auto_save = False
        self._overflow = False
        self._thread = threading.Thread(target=self._run, name="flamp-files", daemon=True)
        self._thread.start()

    def submit(self, kind, value=None):
        if self._closed.is_set():
            return
        try:
            self._queue.put_nowait((kind, value))
        except queue.Full:
            raise RuntimeError("FLAMP work queue is full")

    def feed(self, data):
        try:
            self.submit("data", data)
        except RuntimeError:
            if not self._overflow:
                self._overflow = True
                self._emit("flamp_status", "FLAMP backlog is full. Repeated blocks can fill missed frames.")

    def _publish(self):
        self._emit("flamp_transfers", [transfer.info() for transfer in self.receiver.transfers.values()])

    def _save(self, identifier):
        transfer = self.receiver.transfers[identifier]
        try:
            self.receiver.save(identifier, self._directory)
        except (OSError, ValueError) as error:
            transfer.save_error = str(error)

    def _run(self):
        dirty = False
        published = time.monotonic()
        while True:
            try:
                kind, value = self._queue.get(timeout=0.1)
            except queue.Empty:
                kind, value = "idle", None
            if kind == "close":
                self._publish()
                return
            try:
                if kind == "data":
                    self.receiver.feed(value)
                    dirty = True
                elif kind == "configure":
                    if self._directory != value[0]:
                        for transfer in self.receiver.transfers.values():
                            transfer.save_error = ""
                    self._directory, self._auto_save = value
                    dirty = True
                elif kind == "save":
                    if self._directory is None:
                        raise ValueError("Choose a FLAMP receive folder first")
                    self._save(value)
                    dirty = True
                elif kind == "clear":
                    self.receiver.clear(value)
                    dirty = True
                elif kind == "reset":
                    self.receiver.reset_stream()
                    self._overflow = False
                elif kind == "flush":
                    dirty = True
                if dirty and self._auto_save and self._directory is not None:
                    for transfer in self.receiver.transfers.values():
                        if transfer.payload is not None and not transfer.error and not transfer.saved_path and not transfer.save_error:
                            self._save(transfer.identifier)
            except (OSError, ValueError, KeyError) as error:
                self._emit("flamp_status", str(error))
                dirty = True
            if dirty and (time.monotonic() - published >= 0.1 or kind in ("flush", "save", "clear", "configure")):
                self._publish()
                published = time.monotonic()
                dirty = False

    def close(self):
        if self._closed.is_set():
            return
        self._closed.set()
        self._queue.put(("close", None), timeout=5)
        self._thread.join(timeout=10)
        if self._thread.is_alive():
            raise RuntimeError("FLAMP file worker did not stop")
