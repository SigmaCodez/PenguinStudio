#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Prepare a policy from an independently verified public baseline, without root."""
import argparse
import hashlib
import json
from pathlib import Path
import installer

p = argparse.ArgumentParser()
p.add_argument('baseline', type=Path)
p.add_argument('--sha256', required=True)
p.add_argument('--uid', required=True, type=int)
p.add_argument('--incoming', required=True, type=Path)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
with a.baseline.open('rb') as stream:
    digest = hashlib.file_digest(stream, 'sha256').hexdigest()
installer.require(digest == a.sha256, 'Baseline checksum differs from independently verified release')
control = installer.members(a.baseline, True)
installer.require(set(control) == {'control', 'md5sums', 'triggers'}, 'Unexpected baseline control files')
metadata = installer.fields(control['control']['text'])
installer.require(metadata['Package'] == 'penguin-studio' and metadata['Architecture'] == 'amd64', 'Wrong baseline')
version = metadata.pop('Version')
metadata.pop('Installed-Size')
payload = installer.members(a.baseline, require_root=False)
installer.require(payload[installer.WORKER]['kind'] == 'file', 'GPU worker missing')
a.output.write_text(json.dumps({'package': 'penguin-studio', 'baseline_sha256': digest,
    'version_prefix': version, 'uid': a.uid, 'incoming': str(a.incoming.resolve()),
    'fields': metadata, 'control': control, 'payload': payload}, indent=2) + '\n')
print('Prepared policy: only the GPU worker may change; all other payload files are pinned.')
