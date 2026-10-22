#ifndef CR_TESTUTIL_H
#define CR_TESTUTIL_H
#include <cstdio>
#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); return 1; } } while (0)
#endif
