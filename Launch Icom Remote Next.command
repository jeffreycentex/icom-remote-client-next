#!/bin/zsh
set -e
cd -- "${0:A:h}"
if [[ ! -x .venv/bin/python ]]; then
  print "Install the local environment as described in README.md first."
  exit 1
fi
exec .venv/bin/python run_icomremote.py
