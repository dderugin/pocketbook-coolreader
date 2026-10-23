from pathlib import Path
import os,shutil,json
root=Path('/src');build=Path(os.environ['CR3_BUILD_DIR']);revision=os.environ['CR3_REVISION']
for source,suffix in [(build/'test-results.json','tests.json'),(build/'cr3gui/cr3-pb.debug','debug')]:
    shutil.copy2(source,root/'dist'/('coolreader-touch-hd3-'+revision+'.'+suffix))
report=json.loads((build/'test-results.json').read_text())
print('Local build complete: '+str(root/'dist'),flush=True)
print('ARM test groups passed: '+str(len(report['passed'])),flush=True)
