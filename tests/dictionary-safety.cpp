#include "../cr3gui/src/pbdictionary.h"
#include "testutil.h"
#include <string>
int main() {
    char buffer[256];
    for (int length : {255, 256, 300}) {
        std::string source(length, 'a');
        LVCopyUtf8(buffer, sizeof(buffer), source.c_str());
        CHECK(strlen(buffer) == static_cast<size_t>(length < 256 ? length : 255));
    }
    std::string russian;
    for (int i = 0; i < 150; ++i) russian += "я";
    LVCopyUtf8(buffer, sizeof(buffer), russian.c_str());
    CHECK(strlen(buffer) == 254 && LVIsValidUtf8(buffer, strlen(buffer)));
    LVCopyUtf8(buffer, 1, "book"); CHECK(buffer[0] == 0);
    LVCopyUtf8(buffer, sizeof(buffer), NULL); CHECK(buffer[0] == 0);
    CHECK(!LVIsValidUtf8("\xc0\x80", 2));
    CHECK(!LVIsValidUtf8("\xed\xa0\x80", 3));
    CHECK(!LVIsValidUtf8("\xf4\x90\x80\x80", 4));
    CHECK(PbDictionary::redirectFor(NULL).empty());
    std::string longWord(300, 'a');
    std::string article = " (n.) see " + longWord + ".\n";
    CHECK(PbDictionary::redirectFor(article.c_str()) == lString32(longWord.c_str()));
    for (int i = 0; i < 1000; ++i) {
        CHECK(PbDictionary::redirectFor(" (n.) see apple.\n") == lString32("apple"));
        CHECK(PbDictionary::redirectFor("not a redirect").empty());
    }
    PbDictionary::RedirectChain chain;
    chain.add(lString32("A")); CHECK(!chain.canFollow(lString32("a")));
    CHECK(chain.canFollow(lString32("B"))); chain.add(lString32("B"));
    CHECK(!chain.canFollow(lString32("A")));
    for (int i = 2; i < 16; ++i) { lString32 next = lString32::itoa(i); CHECK(chain.canFollow(next)); chain.add(next); }
    CHECK(!chain.canFollow(lString32("seventeenth")));
    puts("PASS: dictionary bounds, UTF-8, redirects and cycle/hop guards");
}
