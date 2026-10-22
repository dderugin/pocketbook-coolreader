#include "crengine.h"
#include "lvdocview.h"
#include "testutil.h"
#include "../cr3gui/src/settings.h"
#include <stdio.h>
#include <string>
lString16 pbSkinFileName;
lString16 openedCacheFile;
lString16 currentCacheDir;
DECL_DEF_CR_FONT_SIZES;
static int testRendering(const char *defaultsFile) {
    LVDocView view(8);
    view.setDefaultFontFace(lString8("DejaVu Sans"));
    view.Resize(1072, 1448);
    view.createDefaultDocument(lString16("Startup test"), lString16("No book loaded yet"));
    view.Render();
    if (view.getPageCount() < 1) return 3;
    puts("PASS: startup placeholder document rendered");
    std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><FictionBook xmlns=\"http://www.gribuser.ru/xml/fictionbook/2.0\"><description><title-info><book-title>Test</book-title><lang>ru</lang></title-info></description><body><section><title><p>Test</p></title>";
    for (int i=0; i<50; ++i)
        xml += "<p>Привет! Проверка чтения книги. Text for paragraph layout and page rendering.</p>";
    xml += "</section></body></FictionBook>";
    LVStreamRef stream = LVCreateMemoryStream(const_cast<char *>(xml.data()), xml.size(), true);
    if (!view.LoadDocument(stream)) return 4;
    view.Render();
    if (view.getPageCount() < 1) return 5;
    printf("PASS: Cyrillic FB2 rendered, %d pages\n", view.getPageCount());
    // A fresh installation must retain the captured font/layout after validation
    // and render successfully with the font supplied by the target firmware.
    CRPropRef defaults = LVCreatePropsContainer();
    LVStreamRef config = LVOpenFileStream(defaultsFile, LVOM_READ);
    CHECK(!config.isNull() && defaults->loadFromStream(config.get()));
    CRPropRef preferences = LVClonePropsContainer(defaults);
    LVArray<int> sizes(cr_font_sizes, sizeof(cr_font_sizes) / sizeof(int));
    view.setFontSizes(sizes, true);
    view.propsUpdateDefaults(preferences);
    const char *keys[] = {PROP_FONT_FACE, PROP_FONT_SIZE, PROP_INTERLINE_SPACE,
                         PROP_PAGE_MARGIN_LEFT, PROP_PAGE_MARGIN_RIGHT,
                         PROP_PAGE_MARGIN_TOP, PROP_PAGE_MARGIN_BOTTOM,
                         PROP_HYPHENATION_DICT};
    for (unsigned i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i)
        CHECK(preferences->getStringDef(keys[i]) == defaults->getStringDef(keys[i]));
    view.propsApply(preferences);
    view.Render();
    CHECK(view.getPageCount() > 0 && view.getFontSize() == defaults->getIntDef(PROP_FONT_SIZE));
    // Saved preferences override the package; missing keys retain defaults.
    LVStreamRef user = LVCreateStringStream(lString8("crengine.font.size=32\ncrengine.page.margin.left=20\n"));
    CHECK(preferences->loadFromStream(user.get()));
    CHECK(preferences->getIntDef(PROP_FONT_SIZE) == 32);
    CHECK(preferences->getIntDef(PROP_PAGE_MARGIN_LEFT) == 20);
    CHECK(preferences->getStringDef(PROP_FONT_FACE) == defaults->getStringDef(PROP_FONT_FACE));
    CHECK(preferences->getStringDef(PROP_PAGE_MARGIN_RIGHT) == defaults->getStringDef(PROP_PAGE_MARGIN_RIGHT));
    puts("PASS: reader defaults render with firmware font; user preferences take precedence");
    return 0;
}
int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    if (argc < 6) return 1;
    InitFontManager(lString8::empty_str);
    if (!fontMan->RegisterFont(lString8(argv[1])) || !fontMan->RegisterFont(lString8(argv[2]))) return 2;
    if (!fontMan->RegisterFont(lString8(argv[3]))) return 2;
    HyphMan::initDictionaries(lString16(argv[5]));
    int result = testRendering(argv[4]);
    HyphMan::uninit();
    ShutdownFontManager();
    return result;
}
