LVRef static initialization regression (Linux x86_64, PocketBook B288 SDK)

From the repository root:

work/sdk/SDK-B288-6.10/usr/bin/arm-obreey-linux-gnueabi-g++ \
  -std=gnu++14 -DPOCKETBOOK_FW6=1 -DLDOM_USE_OWN_MEM_MAN=0 \
  -Icrengine/include tests/lvref-static-global.cpp \
  tests/lvref-static-sentinel.cpp -o work/lvref-regression
work/host/usr/bin/qemu-arm -L work/firmware/runtime work/lvref-regression

Exit 0 means success. Link order intentionally constructs a global LVRef before
its shared sentinel would have been dynamically initialized. Without the fix,
the test frees the static sentinel and aborts. It also checks copy ownership
and that the owned object is destroyed exactly once.

work/firmware/runtime contains the runtime libraries extracted for verification.
A compatible ARM sysroot can be supplied to qemu-arm instead.

Run all current ARM regressions after building:
python3 tools/test-touch-hd3.py
This also checks absent CSS ancestors/null nodes, skin language selection
(including the Chinese font fallback), and startup/Cyrillic FB2 layout.

Reliability review regressions
------------------------------
The build script now runs twelve ARM/QEMU regressions before packaging. The new
cases exercise dictionary UTF-8 buffer boundaries and redirect guards; XML
history/bookmark round trips, failed and short writes, atomic replacement and
backup retention; bounded archive fragments and UTF-8/legacy ZIP entry names,
including stored/deflated data-descriptor entries.

CR3_BUILD_DIR can select a separate build directory. Reports are written to
<build>/test-results.json and bind the tests to the staged binary SHA-256,
source commit/content/dirty state and compiler flags. A failed run removes the
old report. tools/test-build-metadata.py checks that failed/missing/stale test
results cannot be reused. No automated report claims physical-reader testing.

The default package revision is dev. Set CR3_REVISION explicitly to label a new
candidate. Link outputs stay unstripped; debug information is saved separately,
and a stripped staging copy with a GNU debug link goes into the ZIP. Repeating
the build without source changes must retain useful debug sections.

Package assets
--------------
tools/test-package-assets.py <staging-directory> runs before a ZIP is created.
It checks the tested runtime asset hashes, all compiled language catalogs,
the unpacked contents of all four skins and both launchers. Generated ZIP
metadata and translation binaries are not stored as duplicate source assets.
Styles, dictionaries, hyphenation patterns and manuals live in cr3gui/data.

Captured reading defaults
-------------------------
The document-render group also loads data/touch-hd3/cr3-defaults.ini, checks
that font/layout validation retains the captured settings, and renders an FB2
using the target firmware's DroidSans.ttf. It verifies that saved user font
size and margins override packaged defaults while unspecified values survive.
The history-persistence group saves properties through the UI's read/write
memory-stream and atomic-file path, creates a new profile directory and reads
back the font, size, margin and Unicode values. This reproduces the previously
silent settings-save failure before the serializer fix.
Packaging compares the installed defaults file with its repository source.

The skin-language-symbol group also renders the settings menu with the PB631
skin at Touch HD 3 portrait and landscape sizes. It checks that the header
matches a menu row, the Back button responds throughout a full-height target
three times as wide, touches outside it miss, the first item stays below it,
and other menus retain their skin height.
It also checks pressed feedback and exact normal-state restoration after release,
and saves full portrait/landscape menu previews in <build>/arm-regressions/*.pgm.

Dependency upgrades
-------------------
The dependency-compatibility group checks runtime versions and decodes baseline,
progressive and grayscale JPEG plus RGBA PNG through the engine's custom stream
adapters. Existing archive/history groups exercise the upgraded zlib paths.
The web-tls group uses a temporary local HTTPS server and two ephemeral CAs.
It verifies text/binary downloads, rejection of untrusted certificates and wrong
hostnames, blocked file:// redirects, failed output creation and curl callback
byte counts. OpenSSL CLI is required to generate the local test certificates.
Neither TLS tests nor the application build require access to public servers.
All tests link the same private archives as the application, using the generated
dependency-link.json. Firmware library fallbacks cannot satisfy version checks.

CREngine compatibility baseline
------------------------------
Document tests now use explicit UTF-32 interfaces and named in-memory documents.
They also check chapter/bookmark helpers, supplementary Unicode and EPUB loading,
CSS and image/page painting. The history group reads unversioned older PocketBook
XML and preserves its metadata and saved pointer text. Actual pointer/DOM-version
conversion and on-reader behavior still need dedicated migration validation.

Reading-state migration
-----------------------
The reading-state-migration group checks real legacy pointers, including the
last reading position with zero/one/two explicit bookmarks, range endpoints and
annotation text, across memory streams, files and ZIP members. Migrated history
is serialized and reopened against the modern DOM. Invalid pointers leave
history unchanged; metadata scans skip conversion and a failed modern rewind
rolls back the migrated history and restores the requested rendering settings.

Margin compatibility
--------------------
The document-render group checks every margin value offered by the PocketBook
menu survives preference validation. It paints a long FB2 paragraph before and
after applying a margin-only preferences delta, without an explicit second
render, and checks all four text bounds and effective margins.

Standby frontlight regression
----------------------------
standby-frontlight uses the production FW6 standby helper with mocked InkView
light APIs. It covers manually disabled lights with remembered brightness,
on/off restoration across repeated cycles and duplicate notifications,
automatic brightness changes, and unavailable frontlight hardware. It verifies
that standby never writes the brightness setting. Physical frontlight and
cover-screen behaviour still require verification on a reader.

Touch settings navigation
-------------------------
The skin-language-symbol group checks outlined Back/Previous/Next targets of
equal size, full-height paging hit regions, the non-interactive page counter,
pressed feedback and first/middle/last/single-page enabled states. The footer
reserves its own space below the menu rows in portrait and landscape. Other
menus retain their skin's original header and scroll layout.
