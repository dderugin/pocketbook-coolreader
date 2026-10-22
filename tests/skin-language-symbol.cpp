#include "crskin.h"
#include "lvfntman.h"
#include "crgui.h"
#include "testutil.h"
#include <cstdio>
#include <string>
#include <vector>

// This is the application's language string. Firmware exports currentLang()
// as a function, so skin code must never import that symbol as a string.
lString32 crCurrentLang;
lString32 openedCacheFile;
lString32 currentCacheDir;
lString32 pbSkinFileName;

class TouchScreen : public CRGUIScreenBase {
public:
    TouchScreen() : CRGUIScreenBase(1072, 1448, false) {
        _canvas = LVRef<LVDrawBuf>(createCanvas(1072, 1448));
    }
    LVDrawBuf *createCanvas(int width, int height) { return new LVGrayDrawBuf(width, height, 8); }
    void update(const lvRect &, bool) {}
    bool isTouchSupported() { return true; }
};

class SettingsMenu : public CRMenu {
public:
    SettingsMenu(CRGUIWindowManager *wm) : CRMenu(wm, NULL, 1, lString32("Settings"),
            LVImageSourceRef(), LVFontRef(), LVFontRef()) {
        setFullscreen(true);
        setSkinName(lString32("#settings"));
        for (int i = 0; i < 15; ++i)
            addItem(new CRMenuItem(this, i + 10, lString32("Reading preference"),
                                  LVImageSourceRef(), LVFontRef()));
    }
    bool backHit(int x, int y) {
        lvPoint point(x, y);
        for (int i = 0; i < _controls.length(); ++i)
            if (dynamic_cast<CRButtonControl *>(_controls[i]) && _controls[i]->hitTest(point))
                return true;
        return false;
    }
    bool pressBack(bool pressed) {
        lvPoint point(1, 1);
        for (int i = 0; i < _controls.length(); ++i)
            if (dynamic_cast<CRButtonControl *>(_controls[i]) && _controls[i]->hitTest(point))
                return _controls[i]->setSelected(pressed);
        return false;
    }
};

static int checkSettingsHeader(const char *skinFile, const char *previewDirectory) {
    TouchScreen screen;
    CRGUIWindowManager wm(&screen);
    CHECK(wm.loadSkin(lString32(skinFile)));
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
        int backWidth = title.height() * 3;
        CHECK(menu.backHit(title.left + backWidth - 1, title.top + 1));
        CHECK(menu.backHit(title.left + backWidth - 1, title.bottom - 1));
        CHECK(!menu.backHit(title.left + backWidth, title.top + 1));
        CHECK(!menu.backHit(title.left + 1, title.bottom));
        LVRef<LVDrawBuf> canvas = screen.getCanvas();
        std::vector<lUInt32> normal;
        for (int y = title.top; y < title.bottom; ++y)
            for (int x = title.left; x < title.left + backWidth; ++x)
                normal.push_back(canvas->GetPixel(x, y));
        CHECK(menu.pressBack(true));
        CHECK(canvas->GetPixel(title.left + 1, title.top + 1) != normal[backWidth + 1]);
        CHECK(menu.pressBack(false));
        unsigned pixel = 0;
        for (int y = title.top; y < title.bottom; ++y)
            for (int x = title.left; x < title.left + backWidth; ++x)
                CHECK(canvas->GetPixel(x, y) == normal[pixel++]);
        std::string path = std::string(previewDirectory) +
                           (rotated ? "/settings-landscape.pgm" : "/settings-portrait.pgm");
        FILE *preview = fopen(path.c_str(), "wb");
        CHECK(preview != NULL);
        fprintf(preview, "P5\n%d %d\n255\n", title.width(), title.height());
        for (int y = title.top; y < title.bottom; ++y)
            for (int x = title.left; x < title.right; ++x)
                fputc(canvas->GetPixel(x, y), preview);
        CHECK(fclose(preview) == 0);
        printf("PASS: %s settings header %d px, rows %d px; Back target %d x %d px\n",
               rotated ? "landscape" : "portrait", title.height(), menu.getItemHeight(),
               backWidth, title.height());
    }
    menu.setSkinName(lString32("#main"));
    menu.reconfigure(CRGUI_CONFIGURE_FLAG_SCREEN_ORIENTATION);
    lvRect title;
    CHECK(menu.getTitleRect(title) && title.height() == 75);
    return 0;
}

int main(int argc, char **argv) {
    CHECK(argc >= 5);
    InitFontManager(lString8());
    crCurrentLang = lString32("en");
    {
        CRRectSkin skin;
        skin.setFontFace(lString32("DejaVu Sans"));
        if (skin.getFontFace() != "DejaVu Sans") return 1;
        crCurrentLang = lString32("zh");
        skin.setFontFace(lString32("DejaVu Sans"));
        if (skin.getFontFace() != "AR PL New Sung") return 2;
    }
    crCurrentLang = lString32("en");
    CHECK(fontMan->RegisterFont(lString8(argv[1])));
    CHECK(fontMan->RegisterFont(lString8(argv[2])));
    int result = checkSettingsHeader(argv[3], argv[4]);
    ShutdownFontManager();
    CHECK(result == 0);
    puts("PASS: skin reads the application language string, including Chinese fallback");
    return 0;
}
