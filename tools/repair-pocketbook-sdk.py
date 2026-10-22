#!/usr/bin/env python3
"""Run from repo root after SDK extraction. First generate the archive listing:
work/host/usr/lib/7zip/7z l -slt work/sdk/SDK-B288-6.10.7z > work/sdk-list.txt
This restores only skipped symlinks whose resolved targets stay inside work/sdk.
"""
from pathlib import Path
import re,subprocess,os
base=Path('work/sdk').resolve(); entries=[]
for block in Path('work/sdk-list.txt').read_text().split('\n\n'):
 d=dict(line.split(' = ',1) for line in block.splitlines() if ' = ' in line)
 if d.get('Attributes','').startswith('A l'):
  p=base/d['Path']
  if p.is_file() and not p.is_symlink() and p.stat().st_size==0 and int(d['Size'])>0:entries.append(d)
if not entries:
 print('SDK links are already intact')
 raise SystemExit(0)
Path('work/sdk-link-files.txt').write_text('\n'.join(d['Path'] for d in entries)+'\n')
data=subprocess.check_output(['work/host/usr/lib/7zip/7z','x','-so','work/sdk/SDK-B288-6.10.7z','-i@work/sdk-link-files.txt'])
off=0
for d in entries:
 n=int(d['Size']);target=data[off:off+n].decode();off+=n;p=base/d['Path']
 resolved=(p.parent/target).resolve()
 if resolved.is_relative_to(base):
  p.unlink();p.symlink_to(target)
 else:print('Skipped external target:',p,target)
assert off==len(data),(off,len(data))
print('Restored internal SDK symlinks:',len(entries))
