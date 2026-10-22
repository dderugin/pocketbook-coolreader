#!/usr/bin/env python3
"""Run ARM regressions using the configured build and extracted firmware runtime."""
from pathlib import Path
import argparse, datetime, io, json, os, re, shlex, subprocess, zipfile
import http.server, ssl, threading
from build_metadata import sha256, source_metadata
root = Path(__file__).resolve().parent.parent
os.chdir(root)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', type=Path, default=root / 'work/build-touch-hd3-fw6')
parser.add_argument('--application', type=Path)
args = parser.parse_args()
build = args.build.resolve()
application = args.application or build / 'package/cr3-pb.app'
report_path = build / 'test-results.json'
report_path.unlink(missing_ok=True)
source_before = source_metadata(root)
sdk = Path(os.environ.get('POCKETBOOK_SDK', root / 'work/sdk/SDK-B288-6.10'))
compiler = sdk / 'usr/bin/arm-obreey-linux-gnueabi-g++'
qemu = root / 'work/host/usr/bin/qemu-arm'
runtime = root / 'work/firmware/runtime'
out = build / 'arm-regressions'
out.mkdir(exist_ok=True)
makeflags = (build / 'crengine/CMakeFiles/crengine.dir/flags.make').read_text()
def flags(name):
    return shlex.split(re.search(r'^' + name + r' = (.*)$', makeflags, re.M).group(1))
options = flags('CXX_DEFINES') + flags('CXX_INCLUDES') + flags('CXX_FLAGS')
dependencies = json.loads((build / 'dependency-link.json').read_text())
libraries = [build / p for p in ['crengine/libcrengine.a', 'tinydict/libtinydict.a', 'thirdparty/chmlib/libchmlib.a', 'thirdparty/antiword/libantiword.a']]
libraries += [Path(p) for p in dependencies['archives']]
class StreamingBuffer(io.BytesIO):
    def seekable(self): return False
    def seek(self, *args): raise io.UnsupportedOperation('streaming ZIP')
for name, entry, method, streaming in [
    ('utf8-stored.zip', 'Тест/章节-é.xhtml', zipfile.ZIP_STORED, False),
    ('utf8-deflated.zip', 'Тест/章节-é.xhtml', zipfile.ZIP_DEFLATED, False),
    ('descriptor-stored.zip', 'chapter.xhtml', zipfile.ZIP_STORED, True),
    ('descriptor-deflated.zip', 'chapter.xhtml', zipfile.ZIP_DEFLATED, True),
    ('ascii.zip', 'chapter.xhtml', zipfile.ZIP_DEFLATED, False),
]:
    target = StreamingBuffer() if streaming else io.BytesIO()
    with zipfile.ZipFile(target, 'w', method) as z: z.writestr(entry, b'payload')
    (out / name).write_bytes(target.getvalue())
legacy = io.BytesIO()
legacy_name = 'Тест.xhtml'.encode('cp866')
placeholder = 'x' * len(legacy_name)
with zipfile.ZipFile(legacy, 'w') as z:
    info = zipfile.ZipInfo(placeholder); info.create_system = 0
    z.writestr(info, b'payload')
(out / 'legacy.zip').write_bytes(legacy.getvalue().replace(placeholder.encode(), legacy_name))
# Replace a UTF-8 character with an invalid leading byte, preserving header lengths.
bad = (out / 'utf8-stored.zip').read_bytes().replace('Т'.encode(), b'\xc0\x80')
(out / 'invalid-utf8.zip').write_bytes(bad)
# A private local CA makes HTTPS checks independent of the public network.
class TLSHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def handle(self):
        try:
            super().handle()
        except (ConnectionResetError, ssl.SSLEOFError):
            # Negative certificate tests deliberately abort the TLS connection.
            pass
    def do_GET(self):
        if self.path == '/redirect-file':
            self.send_response(302)
            self.send_header('Location', 'file:///etc/passwd')
            self.send_header('Content-Length', '0')
            self.end_headers()
        else:
            self.send_response(200)
            self.send_header('Content-Length', str(len(b'TLS verified\n')))
            self.end_headers()
            self.wfile.write(b'TLS verified\n')
    def log_message(self, *args): pass
for name in ('trusted', 'untrusted'):
    subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes',
                    '-days', '1', '-subj', '/CN=Dependency regression ' + name,
                    '-addext', 'subjectAltName=IP:127.0.0.1',
                    '-keyout', str(out / (name + '.key')),
                    '-out', str(out / (name + '.crt'))],
                   check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), TLSHandler)
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(out / 'trusted.crt', out / 'trusted.key')
server.socket = context.wrap_socket(server.socket, server_side=True)
threading.Thread(target=server.serve_forever, daemon=True).start()
tls_url = 'https://127.0.0.1:' + str(server.server_port)
readelf = sdk / 'usr/bin/arm-obreey-linux-gnueabi-readelf'
symbols = subprocess.check_output([str(readelf), '--dyn-syms', '--wide', str(application)], text=True)
if re.search(r'\b(?:FT_|jpeg_|png_|curl_|deflate|inflate|zlibVersion)\w*\b', symbols):
    raise SystemExit('Application must not import/export firmware copies of private library APIs')
needed = subprocess.check_output([str(readelf), '-d', str(application)], text=True)
if re.search(r'NEEDED.*\[lib(?:png|z\.|curl)', needed):
    raise SystemExit('Application must link private image/compression/network libraries')
passed = []
for name, sources, engine, arguments in [
    ('reference-initialization', ['tests/lvref-static-global.cpp', 'tests/lvref-static-sentinel.cpp'], False, []),
    ('css-null-ancestor', ['tests/css-selector-null-ancestor.cpp'], True, []),
    ('skin-language-symbol', ['tests/skin-language-symbol.cpp'], True, [root / 'work/firmware/ebrmain/cramfs/fonts/DejaVuSans.ttf', root / 'work/firmware/ebrmain/cramfs/fonts/DejaVuSans-Bold.ttf', build / 'cr3gui/pb631fw5.cr3skin', out]),
    ('document-render', ['tests/document-render-startup.cpp'], True, [root / 'work/firmware/ebrmain/cramfs/fonts/DejaVuSans.ttf', root / 'work/firmware/ebrmain/cramfs/fonts/DejaVuSans-Bold.ttf', root / 'work/firmware/ebrmain/cramfs/fonts/DroidSans.ttf', root / 'data/touch-hd3/cr3-defaults.ini', root / 'cr3gui/data/hyph/']),
    ('dependency-compatibility', ['tests/dependency-compatibility.cpp'], True, [out]),
    ('web-tls', ['tests/web-tls.cpp', 'cr3gui/src/web.cpp'], True, [tls_url, out / 'trusted.crt', out / 'untrusted.crt', out]),
    ('dictionary-safety', ['tests/dictionary-safety.cpp'], True, []),
    ('history-persistence', ['tests/history-persistence.cpp'], True, [out]),
    ('archive-boundaries', ['tests/archive-boundaries.cpp'], True, [out]),
    ('atomic-faults', ['tests/atomic-faults.cpp'], True, [out]),
]:
    binary = out / name
    command = [compiler] + options + sources
    if engine:
        command += ['-Wl,--exclude-libs,ALL', '-Wl,--start-group'] + libraries + ['-Wl,--end-group', '-linkview'] + ['-l' + name for name in dependencies['runtime']]
    subprocess.run([str(x) for x in command + ['-o', binary]], check=True)
    subprocess.run([str(x) for x in [qemu, '-L', runtime, '-E', 'LD_LIBRARY_PATH=' + str(runtime / 'lib'), binary] + arguments], check=True, timeout=30)
    if name == 'atomic-faults':
        preload = out / 'atomic-fsync-failure.so'
        subprocess.run([str(compiler).replace('-g++', '-gcc'), '-shared', '-fPIC',
                        'tests/atomic-fsync-failure.c', '-o', str(preload)], check=True)
        subprocess.run([str(x) for x in [qemu, '-L', runtime, '-E',
                       'LD_LIBRARY_PATH=' + str(runtime / 'lib'), '-E',
                       'LD_PRELOAD=' + str(preload), binary, out, 'sync-failure']],
                       check=True, timeout=30)
    passed.append(name)
    print('PASS:', name, flush=True)
server.shutdown()
server.server_close()
if source_metadata(root) != source_before:
    raise SystemExit('Sources changed while tests were running')
report = {
    'schema': 1, 'status': 'passed', 'passed': passed,
    'finished_at': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'binary_sha256': sha256(application), 'source': source_before,
    'configuration_sha256': sha256(build / 'crengine/CMakeFiles/crengine.dir/flags.make'),
    'environment': 'ARM/QEMU with extracted PocketBook firmware runtime; no physical device test',
}
report_path.write_text(json.dumps(report, indent=2) + '\n')
