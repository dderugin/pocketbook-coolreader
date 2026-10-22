#!/usr/bin/env python3
"""Run ARM regressions using the configured build and extracted firmware runtime."""
from pathlib import Path
import os, re, shlex, subprocess
root = Path(__file__).resolve().parent.parent
os.chdir(root)
build = root / 'work/build-touch-hd3-fw6'
sdk = Path(os.environ.get('POCKETBOOK_SDK', root / 'work/sdk/SDK-B288-6.10'))
compiler = sdk / 'usr/bin/arm-obreey-linux-gnueabi-g++'
qemu = root / 'work/host/usr/bin/qemu-arm'
runtime = root / 'work/firmware/runtime'
out = root / 'work/arm-regressions'
out.mkdir(exist_ok=True)
makeflags = (build / 'crengine/CMakeFiles/crengine.dir/flags.make').read_text()
def flags(name):
    return shlex.split(re.search(r'^' + name + r' = (.*)$', makeflags, re.M).group(1))
options = flags('CXX_DEFINES') + flags('CXX_INCLUDES') + flags('CXX_FLAGS')
libraries = [build / p for p in ['crengine/libcrengine.a', 'tinydict/libtinydict.a', 'thirdparty/libjpeg/libjpeg.a', 'thirdparty/freetype/libfreetype.a', 'thirdparty/chmlib/libchmlib.a', 'thirdparty/antiword/libantiword.a']]
for name, sources, engine, args in [
    ('reference-initialization', ['tests/lvref-static-global.cpp', 'tests/lvref-static-sentinel.cpp'], False, []),
    ('css-null-ancestor', ['tests/css-selector-null-ancestor.cpp'], True, []),
    ('skin-language-symbol', ['tests/skin-language-symbol.cpp'], True, []),
    ('document-render', ['tests/document-render-startup.cpp'], True, [root / 'work/firmware/ebrmain/cramfs/fonts/DejaVuSans.ttf', root / 'work/firmware/ebrmain/cramfs/fonts/DejaVuSans-Bold.ttf']),
]:
    binary = out / name
    command = [compiler] + options + sources
    if engine:
        command += ['-Wl,--exclude-libs,ALL'] + libraries + ['-linkview', '-lpng', '-lz', '-lbz2', '-lcurl', '-ldl', '-lm']
    subprocess.run([str(x) for x in command + ['-o', binary]], check=True)
    subprocess.run([str(x) for x in [qemu, '-L', runtime, '-E', 'LD_LIBRARY_PATH=' + str(runtime / 'lib'), binary] + args], check=True, timeout=30)
    print('PASS:', name, flush=True)
