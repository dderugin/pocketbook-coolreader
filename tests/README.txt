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
The build script now runs eight ARM/QEMU regressions before packaging. The new
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
and saves portrait/landscape header previews in <build>/arm-regressions/*.pgm.
