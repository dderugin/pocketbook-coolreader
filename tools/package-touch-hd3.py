#!/usr/bin/env python3
"""Package the FW6 binary with upstream data without modifying source assets."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parent.parent
build = Path(sys.argv[1]).resolve()
binary = build / 'cr3gui/cr3-pb.app'
if not binary.is_file():
    raise SystemExit(f'Missing build: {binary}')
stage = root / 'work/package-touch-hd3'
if stage.exists():
    shutil.rmtree(stage)
share = stage / 'system/share/cr3'
shutil.copytree(root / 'data/common/system/share/cr3', share)
shutil.copy2(root / 'data/firmware-specific/pro5/system/share/cr3/devices.ini', share)
for name in ('bin', 'skins', 'i18n', 'keymaps'):
    (share / name).mkdir(exist_ok=True)
shutil.copy2(binary, share / 'bin/cr3-pb.app')
for skin in ('default', 'pb62x', 'pb626fw5', 'pb631fw5'):
    shutil.copy2(build / f'cr3gui/{skin}.cr3skin', share / f'skins/{skin}.cr3skin')
for mo in sorted((build / 'i18n').glob('*/LC_MESSAGES/cr3.mo')):
    shutil.copy2(mo, share / f'i18n/{mo.parent.parent.name}.mo')
if not list((share / 'i18n').glob('*.mo')):
    raise SystemExit('No compiled translations found')
shutil.copytree(root / 'cr3gui/data/devices/pb360/keymaps', share / 'keymaps', dirs_exist_ok=True)
launcher = stage / 'system/bin/cr3-pb.app'
launcher.parent.mkdir(parents=True, exist_ok=True)
shutil.copy2(root / 'data/touch-hd3/cr3-pb.app', launcher)
apps = stage / 'applications'
apps.mkdir(exist_ok=True)
shutil.copy2(root / 'data/touch-hd3/CoolReader.app', apps / 'CoolReader.app')
shutil.copy2(root / 'data/touch-hd3/Welcome.fb2', share / 'Welcome.fb2')
launcher.chmod(0o755)
shutil.copy2(root / 'LICENSE', stage / 'LICENSE')
shutil.copy2(root / 'README-touch-hd3.txt', stage / 'README-touch-hd3.txt')
# This is intentionally a template: never overwrite a reader's custom config.
shutil.copy2(root / 'data/touch-hd3/extensions.cfg.example', stage / 'extensions.cfg.example')
manifest = {
    'application': 'CoolReader 0.0.6-17-62.1 + Touch HD 3 FW6 compatibility changes',
    'device': 'PocketBook Touch HD 3 / PB632 / B288 / ARM Cortex-A7 softfp',
    'target_firmware': 'U632.6.11.2001 (2026-09-01)',
    'sdk': 'SDK-B288-6.10, GCC 13.3.0 / glibc 2.39',
    'revision': 5,
    'startup_fix': 'rename the skin language reference to crCurrentLang; retain prior CSS, reference initialization and library isolation fixes',
    'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
    'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'validation': 'Four ARM regressions; PB632 foreground drawing with Welcome and the previously failing Amnesia FB2; user confirmed display and physical page buttons',
}
(stage / 'build-info.json').write_text(json.dumps(manifest, indent=2) + '\n')
dest = root / 'dist'
dest.mkdir(exist_ok=True)
archive = dest / 'coolreader-touch-hd3-fw6.11.2001.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for p in sorted(stage.rglob('*')):
        if p.is_file():
            z.write(p, p.relative_to(stage))
checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
(archive.with_suffix('.zip.sha256')).write_text(f'{checksum}  {archive.name}\n')
print(archive)
print(checksum)
