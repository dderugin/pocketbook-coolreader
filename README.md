# CoolReader for PocketBook Touch HD 3

CoolReader for **Touch HD 3 / PB632**, targeting firmware **6.11.2001**.

[Downloads](https://coolreader.nimbushub.eu/) · [Installation](https://coolreader.nimbushub.eu/installation.html) · [Published R5](https://github.com/dderugin/pocketbook-coolreader/releases/tag/touch-hd3-fw6-r5)

R5 was verified on the physical reader for book display and page-turn buttons.
The current source also contains dictionary, persistence, archive and build
verification fixes. New builds are candidates until tested on the reader;
`build-info.json` records automated results separately from device validation.
Other devices and firmware versions have not been verified by this fork.

New installations use the reading preferences captured from the Touch HD 3:
Droid Sans MTI Arabic at size 49, 100% line spacing, margins of 2 px on the
left/top/bottom and 0 px on the right, and Russian/English hyphenation.
All 154 preferences are stored in `data/touch-hd3/cr3-defaults.ini` and packaged
as `system/share/cr3/cr3-defaults.ini`. The font comes with the target firmware.
Saved user or book settings take precedence; upgrades preserve
`system/config/cr3/cr3.ini`. Settings saves, pinch zoom and normal exit now
retain preferences correctly.

## Install

Extract the release ZIP and merge its **system** and **applications** folders
into the reader’s USB storage, preserving existing settings. Safely eject it,
then open **Applications → CoolReader**. For Library’s **Open with** menu,
follow the [file-association instructions](https://coolreader.nimbushub.eu/installation.html#file-associations).

See [the detailed guide](README-touch-hd3.txt) for backups and troubleshooting.

## Build and test

Use Linux with CMake, Make, gettext, zip, Python 3 and the B288 SDK described in
the detailed guide. Keep SDKs, extracted firmware and host tools under `work/`.

```sh
export POCKETBOOK_SDK="$PWD/work/sdk/SDK-B288-6.10"
bash tools/build-touch-hd3.sh
```

The script builds the ARM application, keeps debug symbols, runs eight ARM/QEMU
regression groups and checks packaging evidence before producing
`dist/coolreader-touch-hd3-fw6.11.2001-dev.zip` and its SHA-256 file.
Set `CR3_REVISION` to label a candidate and `CR3_BUILD_DIR` to use another build
directory. The QEMU tests need the compatible firmware runtime under
`work/firmware/runtime`; see [tests/README.txt](tests/README.txt).

Published binaries belong in GitHub Releases. Generated packages, translations,
SDKs and firmware images are excluded from Git. Packaged styles, hyphenation
patterns, Chinese dictionaries and manuals come from the single asset tree in
`cr3gui/data`; translations are compiled from `cr3gui/po`.

## Source layout

| Directory | Purpose |
| --- | --- |
| `crengine/` | Document parsing, layout and rendering engine |
| `cr3gui/` | PocketBook UI, translations and shared runtime assets |
| `data/touch-hd3/` | Launchers, welcome book and device configuration |
| `thirdparty/`, `tinydict/` | Rendering and document-format dependencies |
| `tools/`, `tests/` | Build, package and regression tooling |
| `docs/` | Static download site and installation page |
| `deploy/nginx/` | Direct nginx hosting configuration and runbook |

CMake builds the PocketBook frontend (`CRGUI_PB`). Historical desktop frontends,
the obsolete Jinke metadata plugin, older SDK scripts and prebuilt OTA packages
are not maintained in this fork.

## Origin and license

Based on [blchinezu/pocketbook-coolreader](https://github.com/blchinezu/pocketbook-coolreader)
and the original CoolReader work by Vadim Lopatin and contributors. Older
PocketBook builds and their documentation remain available from that upstream
project and Git history.

CoolReader is licensed under [GPL version 2](LICENSE). Third-party components
retain their own notices and licenses in their source directories.
