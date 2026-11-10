"""Quick-tune buttons: up to 15 named frequencies read from a text file.

File: <settings dir>/quicktune.txt (see restore.default_dir). One button per
line, "name, frequency in kHz"; blank lines and lines starting with # are
skipped. Names longer than 10 characters are cut. No file = no buttons.

    # name, kHz
    40mFT8, 7074.00
"""

import logging
import os

from .restore import default_dir

log = logging.getLogger(__name__)

FILE_NAME = "quicktune.txt"
MAX_BUTTONS = 15
MAX_LABEL = 10


def default_path():
    return os.path.join(default_dir(), FILE_NAME)


def parse(text):
    """-> ([(label, hz)], [problem, ...]). Bad lines are skipped and reported."""
    entries, problems = [], []
    for n, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        name, sep, khz = line.rpartition(",")
        name = name.strip()
        if not sep or not name:
            problems.append("line %d: expected 'name, kHz': %r" % (n, line))
            continue
        try:
            hz = int(round(float(khz.strip()) * 1000))
        except ValueError:
            problems.append("line %d: %r is not a frequency in kHz" % (n, khz.strip()))
            continue
        if hz <= 0:
            problems.append("line %d: frequency must be above 0" % n)
            continue
        if len(name) > MAX_LABEL:
            problems.append("line %d: name %r cut to %d characters" % (n, name, MAX_LABEL))
            name = name[:MAX_LABEL]
        if len(entries) == MAX_BUTTONS:
            problems.append("line %d: more than %d buttons; the rest are ignored" % (n, MAX_BUTTONS))
            break
        entries.append((name, hz))
    return entries, problems


def load(path):
    """-> (entries, problems), or (None, []) when the file doesn't exist."""
    try:
        with open(path, encoding="utf-8-sig") as f:
            text = f.read()
    except FileNotFoundError:
        return None, []
    except OSError as e:
        return None, ["cannot read %s: %s" % (path, e)]
    entries, problems = parse(text)
    for p in problems:
        log.warning("%s %s", FILE_NAME, p)
    return entries, problems
