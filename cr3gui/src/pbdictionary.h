#ifndef PB_DICTIONARY_H_INCLUDED
#define PB_DICTIONARY_H_INCLUDED
#include "lvstring.h"
#include "lvutf8.h"
#include <regex.h>
#include <set>
#include <string>

namespace PbDictionary {
class RedirectPatterns {
    regex_t patterns[2];
    bool valid[2];
public:
    RedirectPatterns() {
        const char *rules[] = {
            "^[\n\t ]+\\([^\\)]+\\)[\t ]+(to|of|see|from)[\t ]+([a-z]+)[\\. \t\n]+$",
            "^[\n\t ]+\\([^\\)]+\\)[\t ]+([a-z\\.]+[ ])?(to|of|see|from)[\t ]+([a-z]+)[\\. \n\t]+$"
        };
        for (int i = 0; i < 2; ++i) valid[i] = regcomp(&patterns[i], rules[i], REG_ICASE | REG_EXTENDED) == 0;
    }
    ~RedirectPatterns() {
        for (int i = 0; i < 2; ++i) if (valid[i]) regfree(&patterns[i]);
    }
    lString32 find(const char *translation) const {
        if (!translation) return lString32();
        size_t length = strlen(translation);
        for (int i = 0; i < 2; ++i) {
            regmatch_t matches[5];
            if (!valid[i] || regexec(&patterns[i], translation, 5, matches, 0)) continue;
            int capture = i == 0 ? 2 : 3;
            regoff_t first = matches[capture].rm_so, last = matches[capture].rm_eo;
            if (first >= 0 && last > first && static_cast<size_t>(last) <= length)
                return Utf8ToUnicode(lString8(translation + first, last - first));
        }
        return lString32();
    }
private:
    RedirectPatterns(const RedirectPatterns &);
    RedirectPatterns &operator=(const RedirectPatterns &);
};
inline lString32 redirectFor(const char *translation) {
    static RedirectPatterns patterns;
    return patterns.find(translation);
}

class RedirectChain {
    std::set<std::string> visited;
    static std::string key(lString32 word) {
        word.lowercase();
        return UnicodeToUtf8(word).c_str();
    }
public:
    bool canFollow(const lString32 &word) const {
        return !word.empty() && visited.size() < 16 && !visited.count(key(word));
    }
    void add(const lString32 &word) { visited.insert(key(word)); }
};
}
#endif
