"""Native provider selection must never permit a plaintext fallback."""
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from icomremote.secrets import SecretStore


def backend(module):
    return type("Backend", (), {"__module__": module, "priority": 5})()


class TestSecretProviders(unittest.TestCase):
    def test_windows_native_provider(self):
        native = backend("keyring.backends.Windows")
        with patch.dict(sys.modules, {"keyring": SimpleNamespace(get_keyring=lambda: native)}):
            self.assertIs(SecretStore()._backend(), native)

    def test_linux_chainer_selects_native_not_plaintext(self):
        plaintext = backend("keyrings.alt.file")
        native = backend("keyring.backends.SecretService")
        chain = backend("keyring.backends.chainer")
        chain.backends = [plaintext, native]
        with patch.dict(sys.modules, {"keyring": SimpleNamespace(get_keyring=lambda: chain)}):
            self.assertIs(SecretStore()._backend(), native)

    def test_kwallet_provider(self):
        native = backend("keyring.backends.kwallet")
        with patch.dict(sys.modules, {"keyring": SimpleNamespace(get_keyring=lambda: native)}):
            self.assertIs(SecretStore()._backend(), native)

    def test_plaintext_only_or_disabled_is_refused(self):
        for module in ("keyrings.alt.file", "keyring.backends.null", "keyring.backends.fail"):
            candidate = backend(module)
            with self.subTest(module=module), patch.dict(sys.modules, {"keyring": SimpleNamespace(get_keyring=lambda: candidate)}):
                with self.assertRaises(RuntimeError):
                    SecretStore()._backend()


if __name__ == "__main__":
    unittest.main()
