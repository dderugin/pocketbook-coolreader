#!/usr/bin/env python3
"""Ensure source cleanup preserves every runtime asset from the tested package."""
from pathlib import Path
import gettext
import hashlib
import json
import sys
import zipfile

root = Path(__file__).resolve().parent.parent
stage = Path(sys.argv[1])
expected = json.loads((root / 'tests/package-assets.json').read_text())
for name, checksum in expected.items():
    path = stage / name
    if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != checksum:
        raise SystemExit(f'Missing or changed runtime asset: {name}')
translations = stage / 'system/share/cr3/i18n'
languages = {p.stem[4:] for p in (root / 'cr3gui/po').glob('cr3_*.po')}
if {p.stem for p in translations.glob('*.mo')} != languages:
    raise SystemExit('Missing or unexpected compiled translations')
for path in translations.glob('*.mo'):
    with path.open('rb') as stream:
        gettext.GNUTranslations(stream)
for skin in ('default', 'pb62x', 'pb626fw5', 'pb631fw5'):
    sources = root / 'cr3gui/data/devices/pb360/skins' / skin
    with zipfile.ZipFile(stage / f'system/share/cr3/skins/{skin}.cr3skin') as archive:
        names = {p.name for p in sources.iterdir() if p.is_file()}
        if set(archive.namelist()) != names:
            raise SystemExit(f'Skin contents differ: {skin}')
        for name in names:
            if archive.read(name) != (sources / name).read_bytes():
                raise SystemExit(f'Skin asset differs: {skin}/{name}')
for relative in ('applications/CoolReader.app', 'system/bin/cr3-pb.app',
                 'system/share/cr3/cr3-defaults.ini'):
    if (stage / relative).read_bytes() != (root / 'data/touch-hd3' / Path(relative).name).read_bytes():
        raise SystemExit(f'Packaged defaults or launcher differs: {relative}')
print(f'PASS: {len(expected)} runtime assets, {len(languages)} translations, 4 skins, launchers and defaults')
