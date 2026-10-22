#include "lvxmlutils.h"
#include "crengine.h"
#include "lvdocview.h"
#include "testutil.h"
#include "../cr3gui/src/settings.h"
#include <stdio.h>
#include <string>
lString32 pbSkinFileName;
lString32 openedCacheFile;
lString32 currentCacheDir;
DECL_DEF_CR_FONT_SIZES;
static int testRendering(const char *defaultsFile, const char *epubFile, const char *fb2Css) {
    LVDocView view(8);
    view.setDefaultFontFace(lString8("DejaVu Sans"));
    view.Resize(1072, 1448);
    view.createDefaultDocument(lString32("Startup test"), lString32("No book loaded yet"));
    view.Render();
    if (view.getPageCount() < 1) return 3;
    puts("PASS: startup placeholder document rendered");
    std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><FictionBook xmlns=\"http://www.gribuser.ru/xml/fictionbook/2.0\"><description><title-info><book-title>Test</book-title><lang>ru</lang></title-info></description><body><section><title><p>Test</p></title>";
    for (int i=0; i<50; ++i)
        xml += "<p>Привет! Проверка чтения книги. Text for paragraph layout and page rendering.</p>";
    xml += "</section></body></FictionBook>";
    LVStreamRef stream = LVCreateMemoryStream(const_cast<char *>(xml.data()), xml.size(), true);
    if (!view.LoadDocument(stream, U"regression.fb2")) return 4;
    view.Render();
    if (view.getPageCount() < 1) return 5;
    printf("PASS: Cyrillic FB2 rendered, %d pages\n", view.getPageCount());
    CHECK(!view.getPageHeaderPages(0, view.getPageCount(), true).empty());
    CHECK(!view.getCurrentChapterName().empty());
    CHECK(view.getSectionBoundsPages().length() >= 1);
    CHECK(view.currentPageIsBookmarked() == NULL);
    CHECK(!view.removeCurrentPageShortcutBookmark());
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
    // Legacy PocketBook modes must keep the status line visible at Top (0)
    // and hidden at Off (2), both through settings and the direct key toggle.
    CHECK(defaults->getIntDef(PROP_STATUS_LINE, -1) == 0);
    CHECK(view.getPageheaderPosition() == 1 && view.getPageHeaderHeight() > 0);
    CHECK(view.getPageHeaderInfo() & PGHDR_PAGE_NUMBER);
    CRPropRef status = LVCreatePropsContainer();
    status->setInt(PROP_STATUS_LINE, 2);
    view.propsApply(status);
    CHECK(view.getPageheaderPosition() == 0 && view.getPageHeaderHeight() == 0);
    status->setInt(PROP_STATUS_LINE, 0);
    view.propsApply(status);
    CHECK(view.getPageheaderPosition() == 1 && view.getPageHeaderHeight() > 0);
    lvRect header;
    view.getPageHeaderRectangle(1, header);
    CHECK(header.height() > 0 && header.top < view.GetHeight() / 2);
    view.setStatusMode(2, true, true, true, true, true, true, true);
    CHECK(view.getPageHeaderHeight() == 0);
    view.setStatusMode(0, true, true, true, true, true, true, true);
    CHECK(view.getPageHeaderHeight() > 0);
    view.setStatusMode(1, true, true, true, true, true, true, true);
    CHECK(view.getPageheaderPosition() == 2 && view.getPageHeaderHeight() > 0);
    view.setStatusMode(0, true, true, true, true, true, true, true);
    CRPropRef fresh = LVCreatePropsContainer();
    view.propsUpdateDefaults(fresh);
    CHECK(fresh->getIntDef(PROP_STATUS_LINE, -1) == 0);
    puts("PASS: legacy PocketBook status modes, settings and key toggle");
    // Saved preferences override the package; missing keys retain defaults.
    LVStreamRef user = LVCreateStringStream(lString8("crengine.font.size=32\ncrengine.page.margin.left=20\n"));
    CHECK(preferences->loadFromStream(user.get()));
    CHECK(preferences->getIntDef(PROP_FONT_SIZE) == 32);
    CHECK(preferences->getIntDef(PROP_PAGE_MARGIN_LEFT) == 20);
    CHECK(preferences->getStringDef(PROP_FONT_FACE) == defaults->getStringDef(PROP_FONT_FACE));
    CHECK(preferences->getStringDef(PROP_PAGE_MARGIN_RIGHT) == defaults->getStringDef(PROP_PAGE_MARGIN_RIGHT));
    puts("PASS: reader defaults render with firmware font; user preferences take precedence");
    CRPropRef legacyWeight = LVCreatePropsContainer();
    legacyWeight->setInt(PROP_FONT_WEIGHT_EMBOLDEN, 1);
    view.propsApply(legacyWeight);
    CHECK(view.propsGetCurrent()->getIntDef(PROP_FONT_BASE_WEIGHT) == 700);
    legacyWeight->setInt(PROP_FONT_WEIGHT_EMBOLDEN, 0);
    view.propsApply(legacyWeight);
    CHECK(view.propsGetCurrent()->getIntDef(PROP_FONT_BASE_WEIGHT) == 400);
    CHECK(Utf8ToUnicode("😀").length() == 1);
    // Exercise the shipped stylesheet: upstream now requires explicit CSS
    // hints to collect FB2 notes/comments into the referencing page footer.
    for (int enabled = 0; enabled <= 1; ++enabled) {
        LVDocView notes(8);
        notes.setDefaultFontFace(lString8("DejaVu Sans"));
        notes.Resize(1072, 1448);
        notes.setStyleSheet(UnicodeToUtf8(LVReadTextFile(lString32(fb2Css))));
        CRPropRef notePrefs = LVClonePropsContainer(preferences);
        notePrefs->setBool(PROP_FOOTNOTES, enabled != 0);
        notes.propsApply(notePrefs);
        std::string fb2 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><FictionBook xmlns=\"http://www.gribuser.ru/xml/fictionbook/2.0\" xmlns:l=\"http://www.w3.org/1999/xlink\"><description><title-info><book-title>Footnotes</book-title></title-info></description><body><section><p>Text with note <a l:href=\"#n1\" type=\"note\">1</a> and comment <a l:href=\"#c1\" type=\"note\">2</a>.</p></section></body><body name=\"notes\"><section id=\"n1\"><title><p>1</p></title><p>Footnote at the bottom.</p></section></body><body name=\"comments\"><section id=\"c1\"><title><p>2</p></title><p>Comment at the bottom.</p></section></body></FictionBook>";
        LVStreamRef noteStream = LVCreateMemoryStream(const_cast<char *>(fb2.data()), fb2.size(), true);
        CHECK(notes.LoadDocument(noteStream, U"footnotes.fb2"));
        notes.Render();
        int fragments = 0;
        LVRendPageList *pages = notes.getPageList();
        for (int i = 0; i < pages->length(); ++i)
            fragments += (*pages)[i]->footnotes.length();
        CHECK(enabled ? fragments == 2 : fragments == 0);
        CHECK(!notes.getPageImage(0).isNull());
    }
    puts("PASS: FB2 notes/comments appear in page footers; disabling footnotes is respected");
    LVDocView epub(8);
    epub.setDefaultFontFace(lString8("DejaVu Sans"));
    epub.Resize(1072, 1448);
    CHECK(epub.LoadDocument(epubFile));
    epub.Render();
    CHECK(epub.getPageCount() > 0 && epub.getTitle() == lString32("EPUB regression"));
    LVDocImageRef painted = epub.getPageImage(0);
    CHECK(!painted.isNull() && painted->getDrawBuf()->GetWidth() > 0);
    puts("PASS: UTF-32 supplementary characters and EPUB CSS/image page painting");
    return 0;
}
int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    if (argc < 8) return 1;
    InitFontManager(lString8::empty_str);
    if (!fontMan->RegisterFont(lString8(argv[1])) || !fontMan->RegisterFont(lString8(argv[2]))) return 2;
    if (!fontMan->RegisterFont(lString8(argv[3]))) return 2;
    HyphMan::initDictionaries(lString32(argv[5]));
    int result = testRendering(argv[4], argv[6], argv[7]);
    HyphMan::uninit();
    ShutdownFontManager();
    return result;
}
