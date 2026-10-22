#include "crengine.h"
#include "lvutf8.h"
#include "testutil.h"
#include <cstring>
#include <limits>
lString16 pbSkinFileName, openedCacheFile, currentCacheDir;
int main(int argc, char **argv) {
    CHECK(argc == 2);
    LVStreamFragment fragment(LVCreateStringStream(lString8("ABCDE")),1,2);
    char bytes[128] = {}; lvsize_t count = 99;
    CHECK(fragment.Read(bytes,4,&count) == LVERR_OK && count == 2 && !memcmp(bytes,"BC",2));
    CHECK(fragment.Read(bytes,4,&count) == LVERR_OK && count == 0);
    CHECK(fragment.Seek(-1,LVSEEK_END,NULL) == LVERR_OK && fragment.GetPos() == 1);
    CHECK(fragment.Read(bytes,1,&count) == LVERR_OK && bytes[0] == 'C');
    CHECK(fragment.Seek(-3,LVSEEK_END,NULL) != LVERR_OK);
    CHECK(fragment.Seek(1,LVSEEK_END,NULL) != LVERR_OK);
    CHECK(fragment.Seek(std::numeric_limits<lvoffset_t>::min(),LVSEEK_CUR,NULL) != LVERR_OK);
    CHECK(fragment.Seek(0,LVSEEK_SET,NULL) == LVERR_OK && fragment.GetPos() == 0);
    LVStreamFragment empty(LVCreateStringStream(lString8("hidden")),0,0);
    CHECK(empty.Read(bytes,1,&count) == LVERR_OK && count == 0);
    const char *archives[] = {"utf8-stored.zip", "utf8-deflated.zip", "descriptor-stored.zip", "descriptor-deflated.zip", "legacy.zip", "ascii.zip"};
    for (int i = 0; i < 6; ++i) {
        lString16 file = Utf8ToUnicode(argv[1]) + lString16("/") + lString16(archives[i]);
        LVContainerRef arc = LVOpenArchieve(LVOpenFileStream(file.c_str(),LVOM_READ)); CHECK(!arc.isNull());
        lString16 name = i < 2 ? Utf8ToUnicode("Тест/章节-é.xhtml") : i == 4 ? Utf8ToUnicode("Тест.xhtml") : lString16("chapter.xhtml");
        LVStreamRef entry = arc->OpenStream(name.c_str(),LVOM_READ); CHECK(!entry.isNull());
        memset(bytes,0,sizeof(bytes)); count=0;
        CHECK(entry->Read(bytes,sizeof(bytes)-1,&count) == LVERR_OK);
        CHECK(count == 7 && !memcmp(bytes,"payload",7));
    }
    lString16 invalid = Utf8ToUnicode(argv[1]) + lString16("/invalid-utf8.zip");
    CHECK(LVOpenArchieve(LVOpenFileStream(invalid.c_str(),LVOM_READ)).isNull());
    puts("PASS: fragment bounds/seeks, UTF-8/legacy ZIP names and data descriptors");
}
