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
