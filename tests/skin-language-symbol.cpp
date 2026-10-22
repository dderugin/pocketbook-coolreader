#include "crskin.h"
#include "lvfntman.h"
#include "crgui.h"
#include "testutil.h"
#include <cstdio>

// This is the application's language string. Firmware exports currentLang()
// as a function, so skin code must never import that symbol as a string.
lString16 crCurrentLang;
lString16 openedCacheFile;
lString16 currentCacheDir;
lString16 pbSkinFileName;

class TouchScreen : public CRGUIScreenBase {
public:
    TouchScreen() : CRGUIScreenBase(1072, 1448, false) {}
    void update(const lvRect &, bool) {}
    bool isTouchSupported() { return true; }
};

class SettingsMenu : public CRMenu {
public:
    SettingsMenu(CRGUIWindowManager *wm) : CRMenu(wm, NULL, 1, lString16("Settings"),
            LVImageSourceRef(), LVFontRef(), LVFontRef()) {
        setFullscreen(true);
        setSkinName(lString16("#settings"));
        for (int i = 0; i < 15; ++i)
            addItem(new CRMenuItem(this, i + 10, lString16("Reading preference"),
                                  LVImageSourceRef(), LVFontRef()));
    }
    bool backHit(int x, int y) {
        lvPoint point(x, y);
        for (int i = 0; i < _controls.length(); ++i)
            if (dynamic_cast<CRButtonControl *>(_controls[i]) && _controls[i]->hitTest(point))
                return true;
        return false;
    }
};

static int checkSettingsHeader(const char *skinFile) {
    TouchScreen screen;
    CRGUIWindowManager wm(&screen);
    CHECK(wm.loadSkin(lString16(skinFile)));
    SettingsMenu menu(&wm);
    for (int rotated = 0; rotated < 2; ++rotated) {
        wm.reconfigure(rotated ? 1448 : 1072, rotated ? 1072 : 1448,
                       rotated ? CR_ROTATE_ANGLE_90 : CR_ROTATE_ANGLE_0);
        menu.reconfigure(CRGUI_CONFIGURE_FLAG_SCREEN_ORIENTATION);
        lvRect title, client;
        CHECK(menu.getTitleRect(title) && menu.getClientRect(client));
        CHECK(abs(title.height() - menu.getItemHeight()) <= 1);
        CHECK(title.height() > 75 && client.top == title.bottom);
        menu.flush();
        CHECK(menu.backHit(title.left + 1, title.top + 1));
        CHECK(menu.backHit(title.left + title.height() - 1, title.bottom - 1));
        CHECK(!menu.backHit(title.left + 1, title.bottom));
        printf("PASS: %s settings header %d px, rows %d px; full-height Back target\n",
               rotated ? "landscape" : "portrait", title.height(), menu.getItemHeight());
    }
    menu.setSkinName(lString16("#main"));
    menu.reconfigure(CRGUI_CONFIGURE_FLAG_SCREEN_ORIENTATION);
    lvRect title;
    CHECK(menu.getTitleRect(title) && title.height() == 75);
    return 0;
}

int main(int argc, char **argv) {
    CHECK(argc >= 4);
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
    crCurrentLang = lString16("en");
    CHECK(fontMan->RegisterFont(lString8(argv[1])));
    CHECK(fontMan->RegisterFont(lString8(argv[2])));
    int result = checkSettingsHeader(argv[3]);
    ShutdownFontManager();
    CHECK(result == 0);
    puts("PASS: skin reads the application language string, including Chinese fallback");
    return 0;
}
