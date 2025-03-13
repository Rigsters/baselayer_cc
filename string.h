#ifndef __STRING_H__
#define __STRING_H__

#include <cstdio>
#include <cassert>


//
// Str and StrLst
//


struct Str {
    char *str = NULL;
    u32 len = 0;
};


struct StrLst {
    char *str;
    u32 len;
    StrLst *next;
    StrLst *first;

    Str GetStr() {
        return Str { str, len };
    }
};


char *StrZeroTerm(MArena *a, Str s) {
    char * result = (char*) ArenaAlloc(a, s.len + 1);
    _memcpy(result, s.str, s.len);
    result[s.len] = 0;
    return result;
}
Str StrLiteral(MArena *a, const char *lit) {
    Str s;
    s.len = 0;
    while (*(lit + s.len) != '\0') {
        ++s.len;
    }
    s.str = (char*) ArenaAlloc(a, s.len);
    _memcpy(s.str, lit, s.len);

    return s;
}
// TODO: get variadic
void StrPrint(const char *format, Str s) {
    assert(1 == 0 && "not implemented");

    //u32 format_str_max_len = 255;
    //char buff[s.len + format_str_max_len];
    //sprintf(buff, "%.*s", s.len, s.str);
    //printf(format, buff);
}
inline void StrPrint(Str s) {
    printf("%.*s", s.len, s.str);
}
inline void StrPrint(const char *aff, Str s, const char *suf) {
    printf("%s%.*s%s", aff, s.len, s.str, suf);
}
inline void StrPrint(StrLst s) {
    printf("%.*s", s.len, s.str);
}
inline void StrPrint(Str *s) {
    printf("%.*s", s->len, s->str);
}
inline void StrPrint(StrLst *s) {
    printf("%.*s", s->len, s->str);
}
bool StrEqual(Str a, Str b) {
    u32 i = 0;
    u32 len = MinU32(a.len, b.len);
    while (i < len) {
        if (a.str[i] != b.str[i]) {
            return false;
        }
        ++i;
    }

    return a.len == b.len;
}
bool StrContainsChar(Str s, char c) {
    for (u32 i = 0; i < s.len; ++i) {
        if (c == s.str[i]) {
            return true;
        }
    }
    return false;
}
Str StrCat(MArena *arena, Str a, Str b) {
    Str cat;
    cat.len = a.len + b.len;
    cat.str = (char*) ArenaAlloc(arena, cat.len);
    _memcpy(cat.str, a.str, a.len);
    _memcpy(cat.str + a.len, b.str, b.len);

    return cat;
}
u32 StrListLen(StrLst *lst, u32 limit = -1) {
    if (lst == NULL) {
        return 0;
    }
    u32 cnt = 0;
    while (lst && cnt < limit) {
        cnt++;
        lst = lst->next;
    }
    return cnt;
}
void StrLstPrint(StrLst *lst, const char *sep = "\n") {
    while (lst != NULL) {
        StrPrint(lst); // TODO: fix
        printf("%s", sep);
        lst = lst->next;
    }
}
StrLst *_StrLstAllocNext(MArena *a_dest) {
    static StrLst def;
    StrLst *lst = (StrLst*) ArenaPush(a_dest, &def, sizeof(StrLst));
    lst->str = (char*) ArenaAlloc(a_dest, 0);
    return lst;
}
StrLst *StrSplit(MArena *a_dest, Str base, char split) {
    StrLst *next = _StrLstAllocNext(a_dest);
    StrLst *first = next;
    StrLst *node = next;

    u32 i = 0;
    u32 j = 0;
    while (i < base.len) {
        // seek
        j = 0;
        while (i + j < base.len && base.str[i + j] != split) {
            j++;
        }

        // copy
        if (j > 0) {
            if (node->len > 0) {
                next = _StrLstAllocNext(a_dest);
                node->next = next;
                node->first = first;
                node = next;
            }

            node->len = j;
            ArenaAlloc(a_dest, j);
            _memcpy(node->str, base.str + i, j);
        }

        // iter
        i += j + 1;
    }
    return first;
}
StrLst *StrSplitSpacesKeepQuoted(MArena *a_dest, Str base) {
    char space = ' ';
    char quote = '"';

    u32 qcnt = 0;
    bool e1 = false;
    bool e2 = false;
    for (u32 i = 0; i < base.len; ++i) {
        if (base.str[i] == quote) {
            qcnt++;

            // check whether the quotation thing is padded with space on left/right side (uneven / even)
            bool qts_are_wrapped_by_spaces =
                (qcnt % 2 == 1) && (i == 0 || base.str[i - 1] == ' ') ||
                (qcnt % 2 == 0) && (i == base.len - 1 || base.str[i + 1] == ' ');

            if (!qts_are_wrapped_by_spaces) {
                e1 = true;
            }
        }
    }
    e2 = qcnt % 2 == 1;

    bool debug_print = false;
    if (e1 || e2) {
        if (debug_print) printf("FAIL: qcnt: %u\n", qcnt);
        if (debug_print) printf("FAIL: fail %d %d\n", e1, e2);
        return NULL;
    }
    else {
        if (debug_print) printf("able to collapse\n");
    }

    char split = space;
    StrLst *next = _StrLstAllocNext(a_dest);
    StrLst *first = next;
    StrLst *node = next;

    u32 i = 0;
    u32 j = 0;
    while (i < base.len) {
        // seek
        j = 0;
        while (i + j < base.len && (base.str[i + j] != split) ) {
            char c = base.str[i + j];

            if (c == quote) {
                i++;
                if (split == space) {
                    split = quote;
                }
                else {
                    split = space;
                }
            }

            j++;
        }

        // copy
        if (j > 0) {
            if (node->len > 0) {
                next = _StrLstAllocNext(a_dest);
                node->next = next;
                node->first = first;
                node = next;
            }

            node->len = j;
            ArenaAlloc(a_dest, j);
            _memcpy(node->str, base.str + i, j);
        }

        // iter
        split = space;
        i += j + 1;
    }
    return first;
}
Str StrJoin(MArena *a, StrLst *strs) {
    Str join;
    join.str = (char*) ArenaOpen(a);
    join.len = 0;

    while (strs != NULL) {
        _memcpy(join.str + join.len, strs->str, strs->len);
        join.len += strs->len;
        strs = strs->next;
    }

    ArenaClose(a, join.len);
    return join;
}
Str StrJoinInsertChar(MArena *a, StrLst *strs, char insert) {
    Str join;
    join.str = (char*) ArenaOpen(a);
    join.len = 0;

    while (strs != NULL) {
        _memcpy(join.str + join.len, strs->str, strs->len);
        join.len += strs->len;
        strs = strs->next;

        if (strs != NULL) {
            join.str[join.len] = insert;
            ++join.len;
        }
    }

    ArenaClose(a, join.len);
    return join;
}
Str StrTrim(MArena *a, Str s, char t) {
    if (s.len) {
        if (s.str[0] == t) {
            s.str++;
            s.len -=1;
        }
        if (s.str[s.len-1] == t) {
            s.len -=1;
        }
    }
    return s;
}


//
// string list builder functions [another take]


StrLst *StrLstPut(MArena *a, char *str, StrLst *after = NULL) {
    StrLst _ = {};
    StrLst *lst = (StrLst*) ArenaPush(a, &_, sizeof(StrLst));
    lst->len = _strlen(str);
    lst->str = (char*) ArenaAlloc(a, lst->len);

    for (u32 i = 0; i < lst->len; ++i) {
        lst->str[i] = str[i];
    }
    if (after != NULL) {
        assert(after->first != NULL && "don't allow first pointer to not be set during StrLstPut");
        after->next = lst;
        lst->first = after->first;
    }
    else {
        lst->first = lst;
    }
    return lst;
}
void StrLstPrint(StrLst lst) {
    StrLst *iter = &lst;
    do {
        StrPrint(iter);
        printf("\n");
    }
    while ((iter = iter->next) != NULL);
}
StrLst *StrLstPop(StrLst *pop, StrLst *prev) {
    if (pop == NULL) {
        return NULL;
    }
    else if (pop->first == NULL && pop->next == NULL) {
        return NULL;
    }

    // pop is first
    else if (pop == pop->first) { 
        assert(prev == NULL);

        StrLst *newfirst = pop->next;
        StrLst *iter = newfirst;
        while (iter) {
            iter->first = newfirst;
            iter = iter->next;
        }
        pop->first = newfirst; // this line is just a safeguard in case someone uses first on the item
        return newfirst;
    }

    else if (prev != NULL) {
        assert(prev->next == pop);

        // pop is in the middle
        if (pop->next) {
            prev->next = pop->next;
        }

        // pop is the last
        else {
            prev->next = NULL;
        }
    }

    return pop->first;
}


//
//  Automated arena signatures


static MArena _g_a_strings;
static MArena *g_a_strings;
MArena *StringCreateArena() {
    if (g_a_strings == NULL) {
        _g_a_strings = ArenaCreate();
        g_a_strings = &_g_a_strings;
    }
    return g_a_strings;
}
void StringInit() {
    StringCreateArena();
}
void StringSetGlobalArena(MArena *a) {
    g_a_strings = a;
}
MArena *StringGetGlobalArena() {
    return g_a_strings;
}
MArena *InitStrings() {
    return StringCreateArena();
}


//
//  Wrappers without any arena arg, these just expand on the currently set arena


inline
char *StrZeroTerm(Str s) {
    return StrZeroTerm(g_a_strings, s);
}
inline
Str StrLiteral(const char *literal) {
    return StrLiteral(g_a_strings, literal);
}
inline
Str StrInline(const char *literal) {
    return Str { (char*) literal, _strlen((char*) literal) };
}
inline
Str StrLiteral(char *literal) {
    return StrLiteral(g_a_strings, (const char*) literal);
}
#define StrL StrLiteral
inline
bool StrEqual(Str a, const char *lit) {
    Str b = StrLiteral(lit);
    return StrEqual(a, b);
}
inline
Str StrAlloc(MArena *a_dest, u32 len) {
    char *buff = (char*) ArenaAlloc(a_dest, len);
    return Str { buff, len };
}
inline
Str StrAlloc(u32 len) {
    char *buff = (char*) ArenaAlloc(g_a_strings, len);
    return Str { buff, len };
}
inline
Str StrCat(Str a, Str b) {
    return StrCat(g_a_strings, a, b);
}
inline
Str StrCat(Str a, char *b) {
    return StrCat(g_a_strings, a, StrLiteral(b));
}
inline
Str StrCat(Str a, const char *b) {
    return StrCat(g_a_strings, a, StrLiteral(b));
}
inline
Str StrCat(const char *a, Str b) {
    return StrCat(g_a_strings, StrLiteral(a), b);
}
inline
StrLst *StrSplit(Str base, char split) {
    return StrSplit(g_a_strings, base, split);
}
inline
StrLst *StrSplitSpacesKeepQuoted(Str base) {
    return StrSplitSpacesKeepQuoted(g_a_strings, base);
}
inline
StrLst *StrSplitLines(Str base) {
    return StrSplit(g_a_strings, base, '\n');
}
inline
StrLst *StrSplitWords(Str base) {
    return StrSplit(g_a_strings, base, ' ');
}
inline
Str StrJoin(StrLst *strs) {
    return StrJoin(g_a_strings, strs);
}
inline
Str StrTrim(Str s, char t) {
    return StrTrim(g_a_strings, s, t);
}
inline
Str StrJoinInsertChar(StrLst *strs, char insert) {
    return StrJoinInsertChar(g_a_strings, strs, insert);
}
inline
StrLst *StrLstPut(char *str, StrLst *after = NULL) {
    return StrLstPut(g_a_strings, str, after);
}
StrLst *StrLstPut(Str str, StrLst *after = NULL) {
    return StrLstPut(StrZeroTerm(str), after);
}


#endif
