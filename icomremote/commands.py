"""Bounded application commands; callbacks from workers must never submit here."""

from concurrent.futures import Future, TimeoutError
from functools import wraps
import queue
import threading


class CommandQueue:
    def __init__(self):
        self._queue = queue.Queue(64)
        self._lock = threading.Lock()
        self._closed = False
        self._thread = threading.Thread(target=self._run, name="icom-commands", daemon=True)
        self._thread.start()

    def call(self, fn, *args, **kwargs):
        if threading.current_thread() is self._thread:
            return fn(*args, **kwargs)
        future = Future()
        with self._lock:
            if self._closed:
                raise RuntimeError("application service has shut down")
            try:
                self._queue.put_nowait((future, fn, args, kwargs))
            except queue.Full:
                raise RuntimeError("too many pending control commands") from None
        try:
            return future.result(timeout=30)
        except TimeoutError:
            future.cancel()  # a command still waiting in the queue must not run later
            raise RuntimeError("control command timed out") from None

    def _run(self):
        while True:
            item = self._queue.get()
            if item is None:
                return
            future, fn, args, kwargs = item
            if not future.set_running_or_notify_cancel():
                continue
            try:
                future.set_result(fn(*args, **kwargs))
            except BaseException as exc:
                future.set_exception(exc)

    def close(self):
        with self._lock:
            self._closed = True
            while True:
                try:
                    item = self._queue.get_nowait()
                except queue.Empty:
                    break
                if item is not None:
                    item[0].cancel()
            self._queue.put_nowait(None)
        if threading.current_thread() is not self._thread:
            self._thread.join(5)


def command(fn):
    @wraps(fn)
    def submit(self, *args, **kwargs):
        return self._commands.call(fn, self, *args, **kwargs)
    return submit
