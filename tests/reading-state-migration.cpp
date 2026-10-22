#include "crengine.h"
#include "lvdocview.h"
#include "lvrend.h"
#include "lvstreamfragment.h"
#include "testutil.h"
#include <string>
lString32 pbSkinFileName, openedCacheFile, currentCacheDir;
// Fail exactly when the migrated history has been staged and the loader
// rewinds to parse the modern DOM, rather than failing a parser's normal seeks.
class FailModernRewind : public LVStreamFragment {
    CRFileHistRecord *record;
public:
    FailModernRewind(LVStreamRef source, CRFileHistRecord *rec)
        : LVStreamFragment(source, 0, source->GetSize()), record(rec) {}
    lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *position) {
        if (record->getDOMversion() == gDOMVersionCurrent) return LVERR_FAIL;
        return LVStreamFragment::Seek(offset, origin, position);
    }
};
static int run(const char *font, const char *file, const char *zip) {
    CHECK(fontMan->RegisterFont(lString8(font)));
    lString32 path = Utf8ToUnicode(file);
    LVStreamRef source = LVOpenFileStream(file, LVOM_READ);
    CHECK(!source.isNull());
    LVDocView old(8);
    old.setDefaultFontFace(lString8("DejaVu Sans"));
    old.Resize(1072, 1448);
    CRPropRef prefs = LVCreatePropsContainer();
    prefs->setInt(PROP_REQUESTED_DOM_VERSION, 20171225);
    prefs->setInt(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_LEGACY);
    old.propsApply(prefs);
    CHECK(old.LoadDocument(file));
    old.Render();
    ldomNode *paragraph = old.getDocument()->nodeFromXPath(U"/FictionBook/body/section/p[2]");
    CHECK(paragraph && paragraph->getChildCount() > 0);
    ldomXPointer saved(paragraph->getChildNode(0), 5);
    lString32 original = saved.toString();
    CHECK(!original.empty());
    for (int mode = 0; mode < 3; ++mode) {
        lString32 content = mode == 2 ? Utf8ToUnicode(zip) + U"@/migration.fb2" : path;
        for (int count = 0; count <= 2; ++count) {
            LVDocView view(8);
            view.setDefaultFontFace(lString8("DejaVu Sans"));
            view.Resize(1072, 1448);
            CRFileHistRecord *record = view.getHistory()->savePosition(content, source->GetSize(), U"Test", U"", U"", saved);
            record->setDOMversion(20171225);
            for (int i = 0; i < count; ++i) {
                CRBookmark *bmk = new CRBookmark(saved);
                bmk->setType(i == 0 ? bmkt_pos : bmkt_comment);
                if (i == 1) bmk->setEndPos(original);
                bmk->setCommentText(U"Preserve annotation");
                record->getBookmarks().add(bmk);
            }
            if (mode == 0) {
                source->SetPos(0);
                CHECK(view.LoadDocument(source, content.c_str()));
            } else CHECK(view.LoadDocument(content.c_str()));
            view.restorePosition();
            view.Render();
            CHECK(record->getDOMversion() == gDOMVersionCurrent);
            CHECK(view.getDocument()->getDOMVersionRequested() == gDOMVersionCurrent);
            CHECK(view.propsGetCurrent()->getIntDef(PROP_REQUESTED_DOM_VERSION, gDOMVersionCurrent) == gDOMVersionCurrent);
            CHECK(record->getBookmarks().length() == count);
            for (int i = -1; i < count; ++i) {
                CRBookmark *bmk = i < 0 ? record->getLastPos() : record->getBookmarks()[i];
                ldomXPointer ptr = view.getDocument()->createXPointer(bmk->getStartPos());
                CHECK(!ptr.isNull() && ptr.getNode()->isText());

                CHECK(ptr.getNode()->getText() == saved.getNode()->getText());
                CHECK(ptr.getOffset() == saved.getOffset());
                if (i == 1) CHECK(bmk->getEndPos() == bmk->getStartPos());
                if (i >= 0) CHECK(bmk->getCommentText() == lString32(U"Preserve annotation"));
            }
            CHECK(!view.getBookmark().isNull());
            // Persist migrated pointers and reopen with no conversion needed.
            LVStreamRef history = LVCreateMemoryStream(NULL, 0, false, LVOM_WRITE);
            CHECK(view.getHistory()->saveToStream(history.get()));
            history->SetPos(0);
            LVDocView reopened(8);
            reopened.setDefaultFontFace(lString8("DejaVu Sans"));
            reopened.Resize(1072, 1448);
            CHECK(reopened.getHistory()->loadFromStream(history));
            source->SetPos(0);
            CHECK(reopened.LoadDocument(source, content.c_str()));
            CRFileHistRecord *again = reopened.getHistory()->getRecord(content, source->GetSize());
            CHECK(again && again->getDOMversion() == gDOMVersionCurrent);
            ldomXPointer resumed = reopened.getDocument()->createXPointer(again->getLastPos()->getStartPos());
            CHECK(!resumed.isNull() && resumed.getNode()->getText() == saved.getNode()->getText());
            CHECK(resumed.getOffset() == 5);
        }
    }
    // Metadata-only scans must not migrate history. Failed reloads must roll
    // history back and restore both DOM and rendering preferences.
    for (int failure = 0; failure < 2; ++failure) {
        LVDocView view(8);
        view.setDefaultFontFace(lString8("DejaVu Sans"));
        CRFileHistRecord *rec = view.getHistory()->savePosition(path, source->GetSize(), U"Test", U"", U"", saved);
        rec->setDOMversion(20171225);
        int flags = view.propsGetCurrent()->getIntDef(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_DEFAULT);
        source->SetPos(0);
        if (failure) {
            LVStreamRef broken(new FailModernRewind(source, rec));
            CHECK(!view.LoadDocument(broken, path.c_str()));
        } else CHECK(view.LoadDocument(source, path.c_str(), true));
        CHECK(rec->getDOMversion() == 20171225);
        CHECK(rec->getLastPos()->getStartPos() == original);
        CHECK(view.propsGetCurrent()->getIntDef(PROP_REQUESTED_DOM_VERSION, gDOMVersionCurrent) == gDOMVersionCurrent);
        CHECK(view.propsGetCurrent()->getIntDef(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_DEFAULT) == flags);
    }
    // A broken annotation must leave every position and the version unchanged.
    CRFileHistRecord damaged;
    damaged.setDOMversion(20171225);
    damaged.getLastPos()->setStartPos(original);
    CRBookmark *bad = new CRBookmark();
    bad->setStartPos(U"/missing/node.999");
    damaged.getBookmarks().add(bad);
    CHECK(!damaged.convertBookmarks(old.getDocument(), gDOMVersionCurrent));
    CHECK(damaged.getDOMversion() == 20171225);
    CHECK(damaged.getLastPos()->getStartPos() == original);
    CHECK(bad->getStartPos() == lString32(U"/missing/node.999"));
    CHECK(old.getDocument()->getDOMVersionRequested() == 20171225);
    puts("PASS: saved position and 0/1/2 bookmarks migrate across stream/file/ZIP loads and reopening; invalid pointers preserve history");
    return 0;
}
int main(int argc, char **argv) {
    if (argc != 4) return 1;
    InitFontManager(lString8::empty_str);
    int result = run(argv[1], argv[2], argv[3]);
    ShutdownFontManager();
    return result;
}
