#include "crengine.h"
#include "testutil.h"
#include <sys/resource.h>
#include <signal.h>
#include <string>
#include <cstdio>
lString16 pbSkinFileName, openedCacheFile, currentCacheDir;
static lString8 readFile(const lString16 &path) {
    LVStreamRef stream = LVOpenFileStream(path.c_str(),LVOM_READ);
    if (stream.isNull()) return lString8();
    char bytes[32]={}; lvsize_t size=0;
    stream->Read(bytes,31,&size); return lString8(bytes,size);
}
int main(int argc,char **argv) {
    CHECK(argc >= 2);
    lString16 path=Utf8ToUnicode(argv[1])+lString16("/fault-state");
    // Seed without the atomic helper: the fsync-failure subprocess injects EIO.
    FILE *seed=fopen(UnicodeToUtf8(path).c_str(),"wb"); CHECK(seed);
    CHECK(fwrite("old",1,3,seed)==3); CHECK(fclose(seed)==0);
    if (argc > 2) {
        CHECK(!LVWriteFileAtomic(path,LVCreateStringStream(lString8("replacement"))));
    } else {
        struct rlimit previous, limited;
        CHECK(getrlimit(RLIMIT_FSIZE,&previous)==0);
        limited=previous; limited.rlim_cur=3;
        struct sigaction ignore={}, oldAction;
        ignore.sa_handler=SIG_IGN; sigemptyset(&ignore.sa_mask);
        CHECK(sigaction(SIGXFSZ,&ignore,&oldAction)==0);
        CHECK(setrlimit(RLIMIT_FSIZE,&limited)==0);
        bool result=LVWriteFileAtomic(path,LVCreateStringStream(lString8("replacement too large")));
        CHECK(setrlimit(RLIMIT_FSIZE,&previous)==0);
        CHECK(sigaction(SIGXFSZ,&oldAction,NULL)==0);
        CHECK(!result);
    }
    CHECK(readFile(path)=="old");
    puts("PASS: failed write/fsync preserves the previous live file");
}
