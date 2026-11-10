"""Headless GUI smoke test (skipped when PySide6 isn't installed).

Runs the real main window offscreen against the fake radio: connect, then
drive frequency, mode, DATA and filter controls and check that both the
radio and the window follow. Settings go to a temporary file, not yours.
"""

import os
import sys
import tempfile
import time
import unittest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from PySide6.QtCore import QSettings, Qt
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication
    HAVE_QT = True
except ImportError:
    HAVE_QT = False

from fake_rsba1 import FakeRadio  # noqa: E402
from icomremote.session import ConnectionSpec  # noqa: E402


@unittest.skipUnless(HAVE_QT, "PySide6 not installed")
class TestGui(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance() or QApplication([])

    def setUp(self):
        from icomremote.gui import MainWindow
        self.tmp = tempfile.mkdtemp()
        settings = QSettings(os.path.join(self.tmp, "test.ini"), QSettings.IniFormat)
        settings.setValue("rig_enable", False)            # don't grab port 4532
        self.radio = FakeRadio(mode=0x01, data=False).start()
        self.quicktune = os.path.join(self.tmp, "quicktune.txt")     # not your real one
        self.w = MainWindow(settings=settings, quicktune_path=self.quicktune)
        from icomremote.restore import RestoreStore
        self.w.radio._store = RestoreStore(self.tmp)     # keep restore records out of your config dir
        self.w.show()

    def tearDown(self):
        self.w.close()
        self.radio.stop()
        self.w.deleteLater()                 # destroy Qt objects now, not at interpreter exit
        self.pump_until(lambda: False, 0.2)
        self.w = None

    def pump_until(self, pred, timeout=8.0):
        end = time.time() + timeout
        while time.time() < end:
            self.app.processEvents()
            if pred():
                return True
            time.sleep(0.02)
        return False

    def test_operating_controls_stay_on_main_tab(self):
        radio_page = self.w.tabs.widget(0)
        audio_page = self.w.tabs.widget(1)
        for control in (self.w.freq, self.w.ptt_btn, self.w.voice_arm,
                        self.w.import_freq_btn, *self.w.quicktune_buttons):
            self.assertTrue(radio_page.isAncestorOf(control))
        for control in (self.w.device_combo, self.w.tx_combo, self.w.tx_gain):
            self.assertTrue(audio_page.isAncestorOf(control))

    def test_switching_tabs_releases_and_disarms_voice(self):
        from unittest.mock import patch
        self.w.voice_arm.setEnabled(True)
        self.w.voice_arm.setChecked(True)
        self.w._space_down = True
        with patch.object(self.w.service, "release_ptt") as release:
            self.w.tabs.setCurrentIndex(1)
            release.assert_called_once_with(self.w._space_owner, reason=None)
        self.assertFalse(self.w.voice_arm.isChecked())
        self.assertFalse(self.w._space_down)

    def test_connection_failure_remains_visible_after_stopped(self):
        from unittest.mock import patch
        from icomremote.session import ConnectionSpec
        p = self.radio.ports
        self.w.service.connect(ConnectionSpec("127.0.0.1", "beer", "wrong", p["control"], p["serial"], p["audio"]))
        self.assertTrue(self.pump_until(lambda: self.w.radio.failed and not self.w.radio.running))
        self.w._tick()
        self.assertEqual(self.w.state_badge.text(), "CONNECTION FAILED")
        self.assertTrue(self.w.connection_error.isVisible())
        self.assertIn("password", self.w.connection_error.text().lower())
        self.assertEqual(self.w.connect_btn.text(), "Connect")
        self.w.tabs.setCurrentIndex(1)
        self.assertTrue(self.w.connection_error.isVisible())
        self.assertEqual(self.w.state_badge.text(), "CONNECTION FAILED")

    def test_connected_badge_and_disconnect(self):
        from icomremote.session import ConnectionSpec
        p = self.radio.ports
        self.w.service.connect(ConnectionSpec("127.0.0.1", "beer", "beerbeer", p["control"], p["serial"], p["audio"]))
        self.assertTrue(self.pump_until(lambda: self.w.state_badge.text() == "CONNECTED"))
        self.assertFalse(self.w.connection_error.isVisible())
        self.w._connect_clicked()
        self.assertTrue(self.pump_until(lambda: self.w.state_badge.text() == "DISCONNECTED"))

    def test_password_and_token_saved_on_close_and_restored(self):
        from icomremote.gui import MainWindow

        self.w.password.setText("fixture-password")
        self.w.remember_pw.setChecked(True)
        self.w.api_token.setText("fixture-token")
        self.w._save_settings()

        settings = QSettings(self.w.settings.fileName(), QSettings.IniFormat)
        other = MainWindow(settings=settings, quicktune_path=self.quicktune)
        try:
            self.assertEqual(other.password.text(), "fixture-password")
            self.assertEqual(other.api_token.text(), "fixture-token")
            other.remember_pw.setChecked(False)
            other.api_token.clear()
        finally:
            other.close()
            other.deleteLater()
        self.assertFalse(settings.contains("password"))
        self.assertEqual(settings.value("api_token"), "")

    def test_dsp_reads_and_changes_radio_settings(self):
        from icomremote.session import ConnectionSpec

        w = self.w
        p = self.radio.ports
        w.service.connect(ConnectionSpec("127.0.0.1", "beer", "beerbeer",
                                         p["control"], p["serial"], p["audio"]))
        dsp_tab = next(i for i in range(w.tabs.count()) if w.tabs.tabText(i) == "DSP")
        w.tabs.setCurrentIndex(dsp_tab)
        self.assertTrue(self.pump_until(lambda: w.dsp_notch.isEnabled()))
        w.dsp_widgets["nr"].click()
        w.dsp_widgets["nr_level"].setValue(4)
        w.dsp_widgets["nb_level"].setValue(67)
        self.assertTrue(self.pump_until(lambda: w._dsp_state.get("nr_level") == 72
                                       and w._dsp_state.get("nb_level") == 171))
        self.assertEqual(self.radio.dsp["nr"], 1)
        self.assertEqual(self.radio.dsp["nb"], 0)
        for mode in ("auto_notch", "manual_notch"):
            index = w.dsp_notch.findData(mode)
            w.dsp_notch.setCurrentIndex(index)
            w.dsp_notch.activated.emit(index)
            self.assertTrue(self.pump_until(lambda: w.dsp_notch.currentData() == mode))
        self.assertTrue(w.dsp_widgets["position"].isEnabled())
        w.dsp_widgets["position"].setValue(128)
        w.dsp_widgets["width"].activated.emit(2)
        self.assertTrue(self.pump_until(lambda: w._dsp_state.get("position") == 128
                                       and w._dsp_state.get("width") == 2))
        self.assertEqual(self.radio.dsp["auto_notch"], 0)

    def test_log_view_updates_and_exports_full_file(self):
        from pathlib import Path
        from unittest.mock import patch

        w = self.w
        w.log_path = Path(self.tmp) / "active.log"
        original = b"old entry\n" * 30_000 + b"latest entry\n"
        w.log_path.write_bytes(original)
        log_tab = w.tabs.count() - 1
        self.assertEqual(w.tabs.tabText(log_tab), "Log")
        w.tabs.setCurrentIndex(log_tab)
        self.assertTrue(self.pump_until(lambda: w.log_export_btn.isEnabled()))
        self.assertIn("latest entry", w.log_view.toPlainText())
        self.assertLess(len(w.log_view.toPlainText()), len(original))
        with w.log_path.open("ab") as log:
            log.write(b"new entry\n")
        self.assertTrue(self.pump_until(lambda: "new entry" in w.log_view.toPlainText()))
        output = Path(self.tmp) / "export.log"
        with patch("icomremote.gui.QFileDialog.getSaveFileName", return_value=(str(output), "")):
            w.log_export_btn.click()
        self.assertEqual(output.read_bytes(), original + b"new entry\n")

    def test_low_power_tune_times_out_and_restores_power(self):
        import numpy as np
        from icomremote.session import ConnectionSpec

        w = self.w
        p = self.radio.ports
        self.radio.data = True
        original = self.radio.rf_power
        w.service.connect(ConnectionSpec("127.0.0.1", "beer", "beerbeer",
                                         p["control"], p["serial"], p["audio"]))
        self.assertTrue(self.pump_until(lambda: w.low_tune_btn.isEnabled() and w.radio._mods_ready is True and w.radio.rig.data))
        w.low_tune_btn.click()
        self.assertTrue(self.pump_until(lambda: self.radio.ptt and self.radio.tx_audio_while_ptt))
        self.assertEqual(self.radio.rf_power, 13)
        self.assertTrue(self.pump_until(lambda: not w.radio.low_tuning, timeout=12))
        duration = self.radio.ptt_log[1][0] - self.radio.ptt_log[0][0]
        self.assertAlmostEqual(duration, 10, delta=0.3)
        samples = np.frombuffer(bytes(self.radio.tx_audio), dtype="<i2")
        spectrum = abs(np.fft.rfft(samples)) ** 2
        frequencies = np.fft.rfftfreq(len(samples), 1 / 48000)
        in_band = abs(frequencies - 1000) <= 25
        self.assertGreater(spectrum[in_band].sum() / spectrum.sum(), 0.99)
        self.assertFalse(self.radio.ptt)
        self.assertEqual(self.radio.rf_power_writes[-2:], [13, original])
        self.assertEqual(self.radio.rf_power, original)
        self.assertEqual(self.radio.tune_started, [])

        w.low_tune_btn.click()
        self.assertTrue(self.pump_until(lambda: self.radio.ptt))
        w.low_tune_btn.click()
        self.assertTrue(self.pump_until(lambda: not w.radio.low_tuning))
        self.assertFalse(self.radio.ptt)
        self.assertEqual(self.radio.rf_power, original)

    def test_connect_and_control(self):
        w = self.w
        self.assertFalse(w.freq.isEnabled())               # disabled until connected
        p = self.radio.ports
        w.radio.connect(ConnectionSpec("127.0.0.1", "beer", "beerbeer", p["control"], p["serial"], p["audio"]))
        self.assertTrue(self.pump_until(lambda: w.freq.isEnabled() and w.freq.value() == 14074.0))
        self.assertTrue(w.mode_buttons["USB"].isChecked())
        self.assertFalse(w.data_btn.isChecked())
        self.assertTrue(self.pump_until(lambda: w.width_label.text() == "3.0 kHz"))
        self.assertIn("IC-7300MK2", w.info_label.text())
        self.assertTrue(self.pump_until(lambda: w.smeter.format() == "S9"))

        w.freq.setValue(7074.0)                             # like pressing Enter after typing
        self.assertTrue(self.pump_until(lambda: self.radio.freq_hz == 7_074_000))

        w.data_btn.click()
        self.assertTrue(self.pump_until(lambda: self.radio.data and w.data_btn.isChecked()))

        w.mode_buttons["LSB"].click()
        self.assertTrue(self.pump_until(lambda: self.radio.mode == 0x00 and self.radio.data))
        self.assertTrue(self.pump_until(lambda: w.mode_buttons["LSB"].isChecked()))

        w.filter_buttons[2].click()
        self.assertTrue(self.pump_until(lambda: self.radio.filt == 2 and w.width_label.text() == "2.4 kHz"))

        self.radio.push_freq(3_573_000)                     # VFO turned on the radio
        self.assertTrue(self.pump_until(lambda: w.freq.value() == 3573.0))

        # ---- recording
        w.rec_dir.setText(self.tmp)
        w.rec_btn.click()
        self.assertTrue(self.pump_until(lambda: w.rec_label.text().startswith("Recording")))
        self.assertTrue(self.pump_until(lambda: (w.radio.recording_seconds() or 0) > 1.0))
        w.rec_btn.click()
        self.assertTrue(w.rec_label.text().startswith("Saved icom_"), w.rec_label.text())
        self.assertTrue([f for f in os.listdir(self.tmp) if f.endswith(".wav")])

        # ---- transmit
        from test_tx import FakeSource
        self.assertTrue(self.pump_until(lambda: "LAN (was USB" in w.data_mod_label.text()))
        self.assertTrue(w.restore_btn.isEnabled())
        w.radio._tx_source_factory = FakeSource
        w.radio.set_tx_device("fake")
        w.mode_buttons["USB"].click()
        self.assertTrue(self.pump_until(lambda: self.radio.mode == 0x01))
        w.data_btn.click()                                  # DATA off -> plain USB
        self.assertTrue(self.pump_until(lambda: not self.radio.data and not w.data_btn.isChecked()))
        w.ptt_btn.click()                                   # refused: not a data mode
        self.assertTrue(self.pump_until(lambda: "only allowed in data modes" in w.statusBar().currentMessage()))
        self.assertFalse(w.ptt_btn.isChecked())
        self.assertFalse(self.radio.ptt)
        w.data_btn.click()
        self.assertTrue(self.pump_until(lambda: self.radio.data and w.radio._tx_source is not None))
        w.ptt_btn.click()
        self.assertTrue(self.pump_until(lambda: self.radio.ptt and w.tx_indicator.text() == "TX"),
                        "not keyed; status: %r / %r" % (w.statusBar().currentMessage(), w.conn_label.text()))
        self.assertTrue(self.pump_until(lambda: w.tx_level.value() > 0))
        self.assertTrue(self.pump_until(lambda: "SWR 1.5" in w.txmeter_label.text()))  # Po/SWR on TX
        self.assertIn("Output", w.txmeter_label.text())
        self.assertIn("#f1c40f", w.swr_dot.styleSheet())                              # 1.5: yellow
        w.ptt_btn.click()
        self.assertTrue(self.pump_until(lambda: not self.radio.ptt and w.tx_indicator.text() == "RX"))
        self.assertTrue(self.pump_until(lambda: "#9e9e9e" in w.swr_dot.styleSheet()))  # grey on RX
        self.assertIn("SWR 1.5", w.txmeter_label.text())                              # last reading kept

        # ---- RF power and TUNE
        self.assertTrue(self.pump_until(lambda: w.rf_power_label.text() == "50 %"))  # 128/255 from the radio
        w.rf_power.setValue(25)
        self.assertTrue(self.pump_until(lambda: self.radio.rf_power == 64))
        w.tune_btn.click()
        self.assertTrue(self.pump_until(lambda: self.radio.tuner == 2 and "tuning" in w.tuner_label.text()))
        self.assertFalse(w.tune_btn.isEnabled())
        self.assertTrue(self.pump_until(lambda: w.tuner_label.text() == "Tuner: on" and w.tune_btn.isEnabled()))

        w._connect_clicked()                                # Disconnect
        self.assertTrue(self.pump_until(lambda: not w.freq.isEnabled()
                                        and w.connect_btn.text() == "Connect"))
        self.assertEqual(self.radio.menu[b"\x00\x85"], 0x01)      # DATA MOD put back to USB


    def test_api_drives_window(self):
        import socket
        from icomremote.ctl import ApiClient
        w = self.w
        p = self.radio.ports
        w.radio.connect(ConnectionSpec("127.0.0.1", "beer", "beerbeer", p["control"], p["serial"], p["audio"]))
        self.assertTrue(self.pump_until(lambda: w.freq.isEnabled() and w.freq.value() == 14074.0))
        with socket.socket() as sk:
            sk.bind(("127.0.0.1", 0))
            port = sk.getsockname()[1]
        w.api_port.setValue(port)
        w.api_enable.setChecked(True)
        self.assertTrue(w.api_status.text().startswith("Listening"), w.api_status.text())
        cl = ApiClient(port=port)
        code, r = cl.tune(freq_khz=7110, mode="LSB")
        self.assertEqual(code, 200, r)
        self.assertTrue(self.pump_until(lambda: w.freq.value() == 7110.0 and w.mode_buttons["LSB"].isChecked()
                                        and w.data_btn.isChecked()))
        w.rec_dir.setText(self.tmp)
        w.rec_dir.editingFinished.emit()
        code, r = cl.record_start(name="api_test")
        self.assertEqual(code, 200, r)
        self.assertEqual(os.path.dirname(r["recording"]["path"]), self.tmp)
        self.assertTrue(self.pump_until(lambda: w.rec_btn.isChecked() and w.rec_label.text().startswith("Recording")))
        code, r = cl.record_stop()
        self.assertTrue(r["was_recording"])
        self.assertTrue(self.pump_until(lambda: not w.rec_btn.isChecked()
                                        and w.rec_label.text().startswith("Saved api_test")))
        w.api_enable.setChecked(False)
        self.assertEqual(w.api_status.text(), "Off")

    def test_quicktune_buttons(self):
        w = self.w
        self.assertFalse(w.quicktune_box.isVisible())                 # no file: no box
        with open(self.quicktune, "w") as f:
            f.write("# name, kHz\n40mFT8, 7074.00\n20mFT8, 14074\nWay too long name, 3573\n")
            f.writelines("Preset %d, %d\n" % (i, 7000 + i) for i in range(4, 16))
        self.assertTrue(self.pump_until(lambda: w.quicktune_box.isVisible(), 3))   # picked up while running
        shown = [b.text() for b in w.quicktune_buttons if b.isVisible()]
        self.assertEqual(shown, ["40mFT8", "20mFT8", "Way too lo"]
                         + ["Preset %d" % i for i in range(4, 16)])
        self.assertFalse(w.quicktune_buttons[0].isEnabled())           # not connected yet
        p = self.radio.ports
        w.radio.connect(ConnectionSpec("127.0.0.1", "beer", "beerbeer", p["control"], p["serial"], p["audio"]))
        self.assertTrue(self.pump_until(lambda: w.quicktune_buttons[0].isEnabled()))
        mode = (self.radio.mode, self.radio.data, self.radio.filt)
        w.quicktune_buttons[0].click()
        self.assertTrue(self.pump_until(lambda: self.radio.freq_hz == 7_074_000 and w.freq.value() == 7074.0))
        w.quicktune_buttons[14].click()
        self.assertTrue(self.pump_until(lambda: self.radio.freq_hz == 7_015_000))
        time.sleep(0.3)
        self.assertEqual((self.radio.mode, self.radio.data, self.radio.filt), mode)   # frequency only
        os.remove(self.quicktune)
        self.assertTrue(self.pump_until(lambda: not w.quicktune_box.isVisible(), 3))

    def test_voice_space_bar(self):
        from test_tx import FakeSource
        w = self.w
        w.radio._tx_source_factory = FakeSource
        p = self.radio.ports
        w.radio.connect(ConnectionSpec("127.0.0.1", "beer", "beerbeer", p["control"], p["serial"], p["audio"]))
        self.assertTrue(self.pump_until(lambda: w.voice_arm.isEnabled() and
                                        w.radio.mods["data_off_mod"]["value"] == 0x05))
        self.assertIn("DATA OFF MOD: LAN (was MIC", w.data_mod_label.text())
        w.radio.set_tx_device("fake-voice", 1.0)             # the one 'Audio from' input
        self.assertTrue(self.pump_until(lambda: "data" in w.radio._tx_sources and w.tx_level.value() > 0))
        w.activateWindow()
        self.pump_until(lambda: w.isActiveWindow(), 2)
        if not w.isActiveWindow():
            self.skipTest("this Qt platform can't make the window active")

        def space(press=True):
            (QTest.keyPress if press else QTest.keyRelease)(w, Qt.Key_Space)
            self.pump_until(lambda: False, 0.3)

        w.host.setFocus()                                    # not armed: Space types, no PTT
        space(True)
        space(False)
        self.assertFalse(self.radio.ptt)

        w.voice_arm.click()
        self.assertTrue(w.voice_arm.isChecked())
        self.assertFalse(w.host.hasFocus())                  # arming takes focus out of the text field
        space(True)                                          # armed, plain USB: voice
        self.assertTrue(self.pump_until(lambda: self.radio.ptt and w.tx_indicator.text() == "TX"))
        self.assertEqual(w.radio._tx_kind, "voice")
        # Lose the immediate RX readback after PTT off.
        self.radio.drop_next_rx_ptt_read = True
        space(False)
        self.assertTrue(self.pump_until(lambda: w.tx_indicator.text() == "RX"))
        self.assertFalse(self.radio.ptt)
        self.assertFalse(self.radio.drop_next_rx_ptt_read)
        w.service.set_frequency(7_100_000)
        w.service.set_filter(2)
        self.assertTrue(self.pump_until(lambda: self.radio.freq_hz == 7_100_000 and self.radio.filt == 2))

        w.host.setFocus()                                    # armed: Space is PTT even in a text field
        space(True)
        self.assertTrue(self.pump_until(lambda: self.radio.ptt))
        space(False)
        self.assertTrue(self.pump_until(lambda: not self.radio.ptt))

        space(True)                                          # disarming while keyed unkeys
        self.assertTrue(self.pump_until(lambda: self.radio.ptt))
        w.voice_arm.click()
        self.assertTrue(self.pump_until(lambda: not self.radio.ptt))
        space(False)

        w.voice_arm.click()
        w._connect_clicked()                                 # disconnect disarms
        self.assertTrue(self.pump_until(lambda: w.connect_btn.text() == "Connect"))
        self.assertFalse(w.voice_arm.isChecked())
        self.assertEqual(self.radio.menu[b"\x00\x84"], 0x00)     # DATA OFF MOD back to MIC


def tearDownModule():
    """Shut Qt down before Python finalizes: on CI (all three OSes) the test
    process crashed at exit after all tests passed when Qt objects were left
    for interpreter teardown."""
    if not HAVE_QT:
        return
    import gc
    app = QApplication.instance()
    if app is not None:
        app.closeAllWindows()
        app.processEvents()
        app.quit()
    gc.collect()


if __name__ == "__main__":
    unittest.main()
