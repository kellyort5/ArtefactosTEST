#!/usr/bin/env python3
"""Compose entrypoint with actionable preflight and WSL/Windows path handling."""
import argparse
import subprocess
import sys
from common import compose

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('action', choices=['config', 'up', 'stop', 'restart', 'ps', 'logs'])
args = parser.parse_args()
try:
    commands = {'up': ['up', '-d'], 'restart': ['restart', 'mosquitto'], 'config': ['config'],
                'stop': ['stop'], 'ps': ['ps'], 'logs': ['logs', '--tail', '50', 'mosquitto']}
    compose(*commands[args.action])
except (RuntimeError, subprocess.CalledProcessError, OSError) as error:
    print(str(error), file=sys.stderr)
    sys.exit(1)
