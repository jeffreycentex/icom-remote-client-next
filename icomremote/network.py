"""Bounded control connections with explicit shutdown and revocation."""

import socket
import threading


class ManagedConnections:
    max_clients = 16
    client_timeout = 30.0

    def __init__(self, *args, **kwargs):
        self._connections = set()
        self._connections_lock = threading.RLock()
        self.closing = threading.Event()
        super().__init__(*args, **kwargs)

    def process_request(self, request, client_address):
        with self._connections_lock:
            if self.closing.is_set() or len(self._connections) >= self.max_clients:
                self.shutdown_request(request)
                return
            request.settimeout(self.client_timeout)
            self._connections.add(request)
        try:
            super().process_request(request, client_address)
        except BaseException:
            with self._connections_lock:
                self._connections.discard(request)
            raise

    def shutdown_request(self, request):
        with self._connections_lock:
            self._connections.discard(request)
        super().shutdown_request(request)

    def close_clients(self):
        self.closing.set()
        with self._connections_lock:
            clients = tuple(self._connections)
        for request in clients:
            try:
                request.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            request.close()
