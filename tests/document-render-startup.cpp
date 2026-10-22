#include "crengine.h"
#include "lvdocview.h"
#include <stdio.h>
#include <string>
lString16 pbSkinFileName;
lString16 openedCacheFile;
lString16 currentCacheDir;
static int testRendering() {
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
    return 0;
}
int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    if (argc < 3) return 1;
    InitFontManager(lString8::empty_str);
    if (!fontMan->RegisterFont(lString8(argv[1])) || !fontMan->RegisterFont(lString8(argv[2]))) return 2;
    int result = testRendering();
    ShutdownFontManager();
    return result;
}
