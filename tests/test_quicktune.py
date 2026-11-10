"""Quick-tune file parsing."""

import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from icomremote import quicktune  # noqa: E402


class TestQuickTune(unittest.TestCase):
    def test_parse(self):
        entries, problems = quicktune.parse(
            "# name, kHz\n\n40mFT8, 7074.00\n  20m FT8 ,14074\nNet, 3573.5\n")
        self.assertEqual(entries, [("40mFT8", 7_074_000), ("20m FT8", 14_074_000), ("Net", 3_573_500)])
        self.assertEqual(problems, [])

    def test_bad_lines_skipped_and_reported(self):
        entries, problems = quicktune.parse(
            "nocomma 7074\n, 7074\nX, abc\nY, 0\nAVeryLongName, 7074\nZ, 7.074e3\n")
        self.assertEqual(entries, [("AVeryLongN", 7_074_000), ("Z", 7_074_000)])
        self.assertEqual(len(problems), 5)
        self.assertIn("cut to 10", problems[4])

    def test_max_fifteen(self):
        entries, problems = quicktune.parse("".join("B%d, %d\n" % (i, 7000 + i) for i in range(17)))
        self.assertEqual(len(entries), 15)
        self.assertEqual(entries[-1], ("B14", 7_014_000))
        self.assertEqual(len(problems), 1)

    def test_load(self):
        d = tempfile.mkdtemp()
        p = os.path.join(d, quicktune.FILE_NAME)
        self.assertEqual(quicktune.load(p), (None, []))          # no file: no buttons
        with open(p, "w", encoding="utf-8-sig") as f:           # BOM from Windows Notepad
            f.write("40mFT8, 7074.00\r\n")
        self.assertEqual(quicktune.load(p), ([("40mFT8", 7_074_000)], []))


if __name__ == "__main__":
    unittest.main()
