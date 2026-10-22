// Link this translation unit before lvref-static-sentinel.cpp to reproduce
// global references being constructed ahead of their shared sentinel.
#include "lvref.h"
struct Item {
    static int live;
    Item() { ++live; }
    ~Item() { --live; }
};
int Item::live = 0;
LVRef<Item> earlyGlobal;
int main() {
    earlyGlobal.Clear();
    {
        LVRef<Item> empty;
        LVRef<Item> copy(empty);
        earlyGlobal = copy;
        empty.Clear();
    }
    {
        LVRef<Item> owner(new Item());
        LVRef<Item> copy(owner);
        owner.Clear();
        if (Item::live != 1) return 1;
        copy.Clear();
        if (Item::live != 0) return 2;
    }
    return 0;
}
