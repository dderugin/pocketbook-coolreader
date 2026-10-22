#include "crskin.h"
#include "lvfntman.h"
#include <cstdio>

// This is the application's language string. Firmware exports currentLang()
// as a function, so skin code must never import that symbol as a string.
lString16 crCurrentLang;
lString16 openedCacheFile;
lString16 currentCacheDir;

int main() {
    InitFontManager(lString8());
    crCurrentLang = lString16("en");
    {
        CRRectSkin skin;
        skin.setFontFace(lString16("DejaVu Sans"));
        if (skin.getFontFace() != "DejaVu Sans") return 1;
        crCurrentLang = lString16("zh");
        skin.setFontFace(lString16("DejaVu Sans"));
        if (skin.getFontFace() != "AR PL New Sung") return 2;
    }
    ShutdownFontManager();
    puts("PASS: skin reads the application language string, including Chinese fallback");
    return 0;
}
