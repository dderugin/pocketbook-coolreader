#include "crengine.h"
#include "lvstreamfragment.h"
#include "hist.h"
#include "testutil.h"
#include <cstring>
#include <string>
lString32 pbSkinFileName, openedCacheFile, currentCacheDir;
class ShortWriter : public LVStream {
public:
    lvsize_t total;
    bool fail;
    ShortWriter(bool failure) : total(0), fail(failure) {}
    lvopen_mode_t GetMode() { return LVOM_WRITE; }
    lverror_t Seek(lvoffset_t,lvseek_origin_t,lvpos_t*) { return LVERR_FAIL; }
    lverror_t SetSize(lvsize_t) { return LVERR_FAIL; }
    lverror_t Read(void*,lvsize_t,lvsize_t*) { return LVERR_FAIL; }
    bool Eof() { return false; }
    lverror_t Write(const void*,lvsize_t count,lvsize_t* written) {
        lvsize_t size = fail ? 0 : (count < 3 ? count : 3);
        if (written) *written = size;
        total += size;
        return fail ? LVERR_FAIL : LVERR_OK;
    }
};
class BadSource : public LVStreamFragment {
public:
    BadSource() : LVStreamFragment(LVCreateStringStream(lString8("new content")), 0, 11) {}
    lverror_t Read(void*,lvsize_t,lvsize_t *count) { if (count) *count = 0; return LVERR_FAIL; }
};
static lString8 readFile(lString32 path) {
    LVStreamRef source = LVOpenFileStream(path.c_str(), LVOM_READ);
    if (source.isNull()) return lString8();
    char bytes[256] = {}; lvsize_t count = 0;
    source->Read(bytes, 255, &count);
    return lString8(bytes, count);
}
int main(int argc, char **argv) {
    CHECK(argc >= 2);
    CRFileHist history;
    CRFileHistRecord *record = new CRFileHistRecord();
    lString32 text = Utf8ToUnicode("A < B && C > D &amp; — Привет 😀");
    record->setTitle(text); record->setAuthor(text); record->setSeries(text);
    record->setFileName(text + lString32(".fb2"));
    record->setFilePath(lString32("/mnt/ext1/A&amp;B/<folder>/")); record->setFileSize(123);
    record->getLastPos()->setStartPos(lString32("/FictionBook/body/section/p.12"));
    record->getLastPos()->setTitleText(text); record->getLastPos()->setPosText(text); record->getLastPos()->setCommentText(text);
    history.getRecords().add(record);
    LVStreamRef memory = LVCreateMemoryStream(NULL,0,false,LVOM_WRITE);
    CHECK(history.saveToStream(memory.get())); memory->SetPos(0);
    CRFileHist restored; CHECK(restored.loadFromStream(memory)); CHECK(restored.getRecords().length() == 1);
    CRFileHistRecord *copy = restored.getRecords()[0];
    CHECK(copy->getTitle() == text && copy->getAuthor() == text && copy->getSeries() == text);
    CHECK(copy->getFileName() == record->getFileName() && copy->getFilePath() == record->getFilePath());
    CHECK(copy->getLastPos()->getStartPos() == record->getLastPos()->getStartPos());
    CHECK(copy->getLastPos()->getTitleText() == text && copy->getLastPos()->getPosText() == text && copy->getLastPos()->getCommentText() == text);
    // Older PocketBook history has no doc-dom-version element.
    memory->SetPos(0);
    std::string legacyXml(memory->GetSize(), '\0');
    lvsize_t actual = 0;
    CHECK(memory->Read(&legacyXml[0], legacyXml.size(), &actual) == LVERR_OK && actual == legacyXml.size());
    size_t domStart = legacyXml.find("<doc-dom-version>");
    CHECK(domStart != std::string::npos);
    size_t domEnd = legacyXml.find("</doc-dom-version>", domStart);
    CHECK(domEnd != std::string::npos);
    legacyXml.erase(domStart, domEnd + strlen("</doc-dom-version>") - domStart);
    CRFileHist legacyHistory;
    CHECK(legacyHistory.loadFromStream(LVCreateStringStream(lString8(legacyXml.c_str()))));
    CHECK(legacyHistory.getRecords().length() == 1);
    CHECK(legacyHistory.getRecords()[0]->getDOMversion() == 20171225);
    CHECK(legacyHistory.getRecords()[0]->getLastPos()->getStartPos() == record->getLastPos()->getStartPos());
    LVStreamRef readOnly = LVCreateStringStream(lString8("unchanged"));
    CHECK(!history.saveToStream(readOnly.get()));
    ShortWriter shortWriter(false), failure(true);
    CHECK(history.saveToStream(&shortWriter)); CHECK(shortWriter.total == memory->GetSize());
    CHECK(!history.saveToStream(&failure));
    CRPropRef props = LVCreatePropsContainer(); props->setString("test", text);
    CHECK(!props->saveToStream(&failure));
    CHECK(!props->saveToStream(readOnly.get()));
    // Exercise the same serialization/atomic-save path as the settings UI.
    LVStreamRef settings = LVCreateMemoryStream(NULL, 0, false, LVOM_WRITE);
    CHECK(settings->GetMode() == LVOM_READWRITE);
    props->setString("font.face.default", "PT Serif");
    props->setInt("crengine.font.size", 32);
    props->setInt("crengine.page.margin.left", 20);
    CHECK(props->saveToStream(settings.get()));
    lString32 config = Utf8ToUnicode(argv[1]) + lString32("/new-profile/cr3/cr3.ini");
    CHECK(LVWriteFileAtomic(config, settings));
    CRPropRef loaded = LVCreatePropsContainer();
    LVStreamRef configStream = LVOpenFileStream(config.c_str(), LVOM_READ);
    CHECK(!configStream.isNull() && loaded->loadFromStream(configStream.get()));
    CHECK(loaded->getStringDef("test") == text);
    CHECK(loaded->getStringDef("font.face.default") == lString32("PT Serif"));
    CHECK(loaded->getIntDef("crengine.font.size") == 32);
    CHECK(loaded->getIntDef("crengine.page.margin.left") == 20);
    lString32 file = Utf8ToUnicode(argv[1]) + lString32("/history-state");
    CHECK(LVWriteFileAtomic(file, LVCreateStringStream(lString8("old"))));
    CHECK(!LVWriteFileAtomic(file, LVStreamRef(new BadSource())));
    CHECK(readFile(file) == "old");
    CHECK(LVWriteFileAtomic(file, LVCreateStringStream(lString8("new"))));
    CHECK(readFile(file) == "new" && readFile(file + lString32(".bak")) == "old");
    puts("PASS: XML round trip, rejected writes, short writes and atomic replacement/backup");
}
