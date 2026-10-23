#!/usr/bin/env python3
"""Download the pinned SDK and firmware; resume partial transfers and verify SHA-256."""
from pathlib import Path
import hashlib,json,subprocess,sys

def sha256(path):
    value=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):value.update(chunk)
    return value.hexdigest()

if __name__=='__main__':
    cache=Path(sys.argv[1]);cache.mkdir(parents=True,exist_ok=True)
    inputs=json.loads(Path(__file__).with_name('inputs.json').read_text())
    for entry in inputs.values():
        target=cache/entry['filename']
        if target.is_file():
            if sha256(target)!=entry['sha256']:raise SystemExit('Checksum mismatch: '+str(target))
            print('Verified cached '+target.name,flush=True);continue
        partial=target.with_name(target.name+'.part')
        subprocess.run(['curl','--fail','--location','--retry','3','--continue-at','-',entry['url'],'--output',str(partial)],check=True)
        if sha256(partial)!=entry['sha256']:raise SystemExit('Checksum mismatch: '+str(partial))
        partial.replace(target)
