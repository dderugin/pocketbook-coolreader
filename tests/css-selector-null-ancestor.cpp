#include "crengine.h"
#include "lvstsheet.h"
#include <stdio.h>
lString16 pbSkinFileName;
lString16 openedCacheFile;
lString16 currentCacheDir;
int testSelectors() {
    setbuf(stdout, NULL);
    ldomDocument doc;
    ldomNode *root = doc.getRootNode();
    ldomNode *child = root->insertChildElement(doc.getElementNameIndex(L"p"));
    LVCssSelectorRule missingAncestor(cssrt_ancessor);
    missingAncestor.setId(doc.getElementNameIndex(L"missing"));
    const ldomNode *node = child;
    puts("Checking selector whose ancestor is absent");
    if (missingAncestor.check(node)) return 1;
    puts("PASS: absent ancestor stops at document root");
    node = NULL;
    LVCssSelectorRule nullRule(cssrt_parent);
    if (nullRule.check(node)) return 2;
    puts("PASS: null node safely rejected");
    return 0;
}

int main() {
    InitFontManager(lString8::empty_str);
    int result = testSelectors();
    ShutdownFontManager();
    return result;
}
