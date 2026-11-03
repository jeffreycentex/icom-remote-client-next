"""Optional native credential storage. Never fall back to plaintext files."""
import hashlib
import sys


class SecretStore:
    SERVICE = "Icom Remote Next"
    display_name = {"darwin": "macOS Keychain", "win32": "Windows Credential Manager"}.get(sys.platform, "the system credential store")

    def _backend(self):
        try:
            import keyring
            selected = keyring.get_keyring()
            native = ("keyring.backends.macOS", "keyring.backends.Windows",
                      "keyring.backends.SecretService", "keyring.backends.kwallet",
                      "keyring.backends.libsecret")
            # Linux may expose several providers through a chainer. Choose one
            # native provider explicitly; never delegate to an insecure fallback.
            if type(selected).__module__ == "keyring.backends.chainer":
                candidates = selected.backends
            else:
                candidates = (selected,)
            for backend in candidates:
                module = type(backend).__module__
                if not any(module == name or module.startswith(name + ".") for name in native):
                    continue
                try:
                    if backend.priority > 0:
                        return backend
                except Exception:
                    continue
            raise RuntimeError("a native system credential store is unavailable")
        except ImportError:
            raise RuntimeError("install keyring to remember credentials securely") from None

    @staticmethod
    def identifier(*parts):
        return hashlib.sha256("\0".join(parts).encode("utf-8")).hexdigest()

    def get(self, identifier):
        return self._backend().get_password(self.SERVICE, identifier) or ""

    def set(self, identifier, value):
        self._backend().set_password(self.SERVICE, identifier, value)

    def delete(self, identifier):
        backend = self._backend()
        if backend.get_password(self.SERVICE, identifier) is not None:
            backend.delete_password(self.SERVICE, identifier)
