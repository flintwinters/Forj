#include <stdlib.h>
#include "vect.c"
#include <stdio.h>
#include <sys/types.h>

typedef long long Word;
typedef struct Vect Vect;
typedef struct Atom Atom;
typedef union data data;
typedef enum form form;
typedef struct Error Error;
typedef Error (*Func)(Atom* D, Atom* d, Atom* e, Atom* r);

union data {
    Word w;
    Func f;
    Vect* v;
    Atom* a;
};
enum form {
    atoms, // this points to more atoms (a stack)
    links, // this points to more atoms, but working scope will be forwarded to the inner stack.
    execs, // convenience `atoms` type that just displays differently.  Functionally identical to `atoms`
    words, // literal number value
    funcs, // pointer to a c function
    vects, // pointer to a dynamic array (a string)
    dots,  // indicates this is a `..` object, signaling execution
    ends   // structural only.  Placeholder type pointed to by empty `atoms`
};

struct Atom {
    Atom* n;
    data d;
    form f;  // shape of s;
    short r; // reference counter
    bool e;  // 'end'.  True means n points to the parent
};

struct Error {
    data d;
    char* msg;
};

Atom* Global;
Atom* Threads;
Atom* Library;

Word  asW(Atom* a) {return (a && a->f == words) ? a->d.w : 0;}
Func  asF(Atom* a) {return (a && a->f == funcs) ? a->d.f : 0;}
Vect* asV(Atom* a) {return (a && a->f == vects) ? a->d.v : 0;}
bool  isA(Atom* a) {return (a->f == atoms || a->f == links || a->f == execs);}
Atom* asA(Atom* a) {return (a && isA(a)) ? a->d.a : 0;}

Error pass(data d)    {return (Error) {d, 0};}
Error passA(Atom* a)  {return (Error) {(data) a, 0};}
Atom* str(char* c);
Atom* addstrch(Atom* s, char* c);
Atom* addstr(Atom* s1, Atom* s2);
Atom* inttostr(int a, int b);
Error makeErr(char* msg, Word line) {
    Atom* s = str("\e[4mError on line: ");
    addstr(s, inttostr(line, 10));
    addstrch(s, "\n");
    addstrch(s, msg);
    return (Error) {(data) s, s->d.v->v};
}
#define fail(msg) makeErr(msg, __LINE__)
// #define fail(msg) \
//     fprintf(stderr, RED "\e[4mError: %d %s\n" RESET, __LINE__, msg); \
//     abort(); \
//     (Error) {__LINE__, msg};

#define xfail(a, s) \
    if (a->f != s) { \
        fprintf(stderr, RED "\e[4mError: %s\n" RESET, fail(#a " is not " #s).msg); \
        abort(); \
    }

#define wordfail(a) xfail(a, words)
#define funcfail(a) xfail(a, funcs)
#define vectfail(a) xfail(a, vects)
#define atomfail(a)  \
    if (!isA(a)) { \
        fprintf(stderr, RED "\e[4mError: %s\n" RESET, fail(#a " is not an atom or link").msg); \
        abort(); \
    }

// Creates a new, zero-initialized atom with no references.
Atom* new(form f) {
    Atom* a = malloc(sizeof(Atom));
    *a = (Atom) {0, 0, f, 0, true};
    return a;
}
Atom* newraw(void* a) {
    Atom* m = malloc(sizeof(Atom));
    cpymem((char*) m, a, sizeof(Atom));
    return m;
}

bool isend(Atom* a) {return a->e;}
bool isempty(Atom* a) {
    return !asA(a) || (isend(asA(a)) && asA(a) && asA(a)->f == ends);
}
Atom* tail(Atom* a) {
    while (a && !isend(a)) {a = a->n;} return a;
}
Atom* get(Atom* a, int i) {
    if (i == -1) {return tail(a);}
    while (i--) {a = a->n;}
    return a;
}

Atom* traverselinks(Atom* d) {   
    if (!asA(d) || asA(d)->f != links) {return d;}
    return traverselinks(asA(d));
}

// Deletes a reference to an atom.
// If the atom reaches zero references, free its memory.
// Also frees vects:
//    exactly one atom at all times.
Atom* del(Atom* a) {
    if (!a) {return 0;}
    if (--a->r) {return a;}

    freevect(asV(a));
    if (!isend(a)) {del(a->n);}
    if (del(asA(a))) {
        // If a->d was referenced by something else,
        // and a owns it, the parent points must be corrected.
        Atom* n = tail(asA(a));
        if (n->n == a) {n->n = 0;}
    }
    reclaim(a, sizeof(struct Atom));
    return 0;
}
// Get a reference to a.
// Does nothing if a == 0.
Atom* ref(Atom* a) {if (a) {a->r++;} return a;}
// Set a's n value.  Adjust references accordingly.
// Adjusts the 'end' flag if needed.
Atom* nset(Atom* a, Atom* n) {
    ref(n);
    if (!a->e) {del(a->n);}
    a->e = !n;
    a->n = n;
    return a;
}
// Set a's t value.  Adjust references accordingly.
Atom* tset(Atom* a, Atom* t) {
    ref(t);
    del(asA(a));
    a->d.a = t;
    return a;
}

// Points d->d to a and appends a to the old d->d
// ends type is used only in tandem with pullr: it signifies the
// empty list, while still holding a parent pointer.
Atom* push(Atom* d, Atom* a) {
    if (isempty(d)) {
        if (!a->e) {del(a->n);}
        a->n = (asA(d)) ? asA(d)->n : d;
        a->e = true;
    }
    else {nset(a, asA(d));}
    return tset(d, a);
}
// Init a new atom pointing to t, and push it to d.
Atom* pushnew(Atom* d, form f, data dat) {
    Atom* n = new(f); n->d = dat;
    if (isA(n)) {ref(dat.a);}
    return push(d, n)->d.a;
}
// Unique push function which empties d.
// Note that t is not ref'd.
Atom* pushend(Atom* d, Atom* t) {
    Atom* e = new(ends);
    e->n = t;
    e->e = true;
    return tset(d, e);
}
// Pops an element from the top of d.  If it's the last element,
// replace it with an ends atom.
void pull(Atom* d) {
    atomfail(d);
    if (isempty(d)) {return;}
    if (isend(asA(d))) {pushend(d, asA(d)->n);}
    else {tset(d, asA(d)->n);}
    return;
}
// Pulls without deleting the resulting atom, and returns it.
Atom* pulla(Atom* d) {
    atomfail(d);
    if (isempty(d)) {return 0;}
    Atom* a = ref(asA(d));
    pull(d);
    return a;
}

Atom* newr(Atom* d, Atom* r) {
    if (!r) {return 0;}
    pushnew(r, atoms, (data) d);
    return pushnew(r, atoms, (data) 0ll);
}
Atom* pullr(Atom* d, Atom* r) {
    if (!r) {pull(d); return 0;}
    Atom* a = asA(d);
    tset(asA(r), tset(new(atoms), a));
    pull(d);
    return a;
}

Atom* makew(Word w) {
    Atom* a = new(words);
    a->d.w = w;
    return a;
}
// Push a number
Atom* pushw(Atom* d, Word w) {
    return push(d, makew(w));
}
// pullr an atom and return the data it was storing
Error pulld(Atom* d, Atom* r) {
    atomfail(d);
    if (isempty(d)) {return fail("d is empty");}
    data dat = asA(d)->d;
    pullr(d, r);
    return pass(dat);
}
// No error handling.  Returns the top word on the stack.
Word pullw(Atom* d, Atom* r) {
    Word w = asA(d)->d.w;
    pullr(d, r);
    return w;
}

// Swap the top two elements.
Error swap(Atom* d) {
    if (isend(asA(d))) {return fail("Not two elements to swap.");}
    Atom* b = asA(d)->n;
    asA(d)->n = asA(d)->n->n;
    b->n = asA(d);
    d->d.a = b;
    // Swap the 'end' flags
    short e = b->e;
    b->e = b->n->e;
    b->n->e = e;
    return passA(d);
}

Atom* duplicate(Atom* a) {
    data d = a->d;
    if(asV(a)) {d = (data) dupvect(asV(a));}
    if(asA(a)) {d = (data) ref(asA(a));}
    Atom* b = new(a->f);
    b->d = d;
    return b;
}

// gets size of the stack inside a
int length(Atom* a) {
    a = asA(a);
    if (!a) {return 0;}
    if (a->f == ends) {return 0;}
    int i = 1;
    while (!a->e) {a = a->n; i++;}
    return i;
}
Error pullx(Atom* d, int i) {
    atomfail(d);
    if (length(d) < i) {return fail("tried to remove too many elements from d");}
    Atom* a = asA(d);
    while (a && !a->e && i) {a = a->n; i--;}
    if (i) {pushend(d, a->n);}
    else {tset(d, a);}
    return passA(a);
}

// Removes the atom after a
Error removeafter(Atom* a) {
    if (a->e) {return fail("No element after a to remove.");}
    bool e = a->n->e;
    if (!e) {nset(a, a->n->n);}
    else {
        Atom* n = a->n;
        a->n = a->n->n;
        del(n);
    }
    a->e = e;
    return passA(a);
}

// Inserts b after a
Error insertafter(Atom* a, Atom* b) {
    nset(b, a->n);
    nset(a, b);
    return passA(a);
}

// Returns the length of the char*
int chlen(char* c) {
    int n = 0;
    while (*c++) {n++;}
    return n;
}

Atom* newvect(int len) {
    int maxlen = (len > 0) ? len : 1;
    Atom* a = new(vects);
    a->f = vects;
    a->d.v = valloclen(maxlen);
    return a;
}

bool contains(char* s, char c) {
    while (*s) {if (*s == c) {return true;} s++;}
    return false;
}
// Returns true if a == b
bool equstr(char* a, char* b) {
    if (!(a && b)) {return false;}
    while (*a == *b && *a && *b) {a++; b++;}
    if (*a || *b) {return false;}
    return true;
}

Vect* vectfromch(char* c, int len) {
    Vect* v = valloclen(len);
    rawpushv(v, c, len);
    return v;
}
// Changes the string object to the new string
Atom* setstr(Atom* s, char* c, int len) {
    s->d.v->len = 0;
    s->d.v = rawpushv(s->d.v, c, len);
    s->d.v = vectpushc(s->d.v, '\0');
    return s;
}
Atom* newstrlen(char* c, int len) {return setstr(newvect(len), c, len);}
Atom* str(char* c) {return newstrlen(c, chlen(c));}
Atom* dupstr(Atom* s) {return str(asV(s)->v);}
Atom* substr(Atom* s, int a, int b) {
    Vect* v = s->d.v;
    if (b == -1) {
        if (!a) {return dupstr(s);}
        b = v->len;
    }
    if (a == b) {return newvect(0);}
    return newstrlen(v->v+a, b-a);
}
Atom* printstr(Atom* s) {
    puts(asV(s)->v);
    fflush(stdout);
    return s;
}
Atom* concat(Atom* a, Atom* b) {
    nset(tail(asA(b)), asA(a));
    tset(a, asA(b));
    del(b);
    return a;
}
void concatvect(Atom* v1, Atom* v2, int len) {
    v1->d.v = rawpushv(asV(v1), asV(v2)->v, len);
}
Atom* addstr(Atom* s1, Atom* s2) {
    if (asV(s1)->len) {asV(s1)->len--;} // remove the null char at the end
    if (!asV(s2)) {return s1;}
    concatvect(s1, s2, asV(s2)->len);
    del(s2);
    return s1;
}
Atom* addstrch(Atom* s, char* ch) {
    if (asV(s)->len) {asV(s)->len--;} // remove the null char at the end
    s->d.v = rawpushv(asV(s), ch, chlen(ch)+1);
    return s;
}
#define WORDCOLOR DARKCYAN
#define VECTCOLOR DARKGREEN
#define FUNCCOLOR GREEN
#define ATOMCOLOR YELLOW
#define DOTSCOLOR DARKRED
Atom* inttostr(int n, int b) {
    bool neg = n < 0;
    if (neg) {n = -n;}

    if (n == 0) {return str("0");}
    Atom* s = newvect(0);
    char c;
    while (n > 0) {
        if (n % b >= 10) {c = 'a' + (n % b) - 10;}
        else {c = '0' + (n % b);}
        s->d.v = vectpushc(asV(s), c);
        n /= b;
    }
    if (neg) {s->d.v = vectpushc(asV(s), '-');}
    reversevect(asV(s));
    s->d.v = vectpushc(asV(s), 0);
    return s;
}

Error scanfunc(Atom* D, Atom* d, Atom* e, Atom* r);
Atom* reversescan(Atom* a, Atom* w);

Atom* scan(Atom* a, Atom* end, char* c);
Atom* scantail(Atom* a, char* c);
Error dot(Atom* D, Atom* d, Atom* e, Atom* r);
bool debugging = false;

Atom* atomstr(Atom* a, int indent, char* spinecolor, bool next) {
    if (!a) {return str(RED "None");}
    Atom* cur = a;
    bool arenewlines = false;
    while (cur && !arenewlines && !isend(cur)) {
        arenewlines = !isempty(cur) || cur->f == links;
        cur = cur->n;
    }
    if (!arenewlines) {arenewlines = !isempty(cur);}
    cur = a;
    Atom* s1 = newvect(0);
    Atom* lines = ref(new(atoms));
    bool newlines = false;
    while (cur) {
        if (newlines || (asA(cur) && cur != a)) {
            addstrch(s1, "\n");
            for (int i = 0; i < indent-1; i++) {addstrch(s1, " ");}
            if (indent) {addstrch(s1, spinecolor); addstrch(s1, "├");}
            while (!isempty(lines)) {
                addstr(s1, ref(asA(lines)));
                addstrch(s1, " ");
                pull(lines);
            }
            newlines = false;
        }
        Atom* s2 = 0;
        if (isA(cur)) {
            if (isempty(cur)) {
                char* c = "\033[4;1;33m@" RESET;
                if (cur->f == execs) {c = "\033[4;1;32m@" RESET;}
                if (cur->f == links) {c = "\033[4;1;33m~" RESET;}
                s2 = str(c);
            }
            else {
                Atom* v = (debugging) ? 0 : scantail(asA(cur), "\"");
                if (v) {
                    Atom* d = ref(new(links));
                    tset(d, cur);
                    push(d, duplicate(v));
                    dot(d, d, 0, 0);
                    s2 = str(RESET);
                    addstr(s2, pulla(d));
                    del(d);
                }
                else {
                    char* c;
                    if (cur->f == atoms) {c = ATOMCOLOR "@ " RESET;}
                    if (cur->f == execs) {c = FUNCCOLOR "@ " RESET;}
                    if (cur->f == links) {c = ATOMCOLOR "~ " RESET;}
                    s2 = str(c);
                    addstr(s2, ref(atomstr(asA(cur), indent + 1, spinecolor, true)));
                }
                arenewlines = true;
                newlines = true;
            }
        }
        else if (cur->f == dots) {s2 = str(DOTSCOLOR ".");}
        else if (cur->f == vects) {
            s2 = str(VECTCOLOR);
            addstrch(s2, cur->d.v->v);
        }
        else if (cur->f == words) {
            s2 = str(WORDCOLOR);
            addstr(s2, ref(inttostr(cur->d.w, 0x10)));
        }
        else if (cur->f == funcs) {
            s2 = str(FUNCCOLOR);
            Atom* rev = reversescan(cur->n, cur);
            if (!rev) {rev = reversescan(Library, cur);}
            if (rev) {addstr(s2, ref(rev));}
            else {addstrch(s2, "func");}
        }

        if (s2) {push(lines, s2);}
        else {return str(RED "None");}
        addstrch(s1, RESET);
        if (isend(cur) || !next) {break;}
        cur = cur->n;
    }
    if (arenewlines) {
        addstrch(s1, "\n");
        for (int i = 0; i < indent-1; i++) {addstrch(s1, " ");}
        if (indent) {
            if (cur->e) {addstrch(s1, spinecolor); addstrch(s1, "╰");}
            else {addstrch(s1, spinecolor); addstrch(s1, "├");}
        }
    }
    while (!isempty(lines)) {
        addstr(s1, ref(asA(lines)));
        addstrch(s1, " ");
        pull(lines);
    }
    del(lines);
    return s1;
}

void printa(Atom* a) {
    Atom* s = atomstr(a, 0, (a == Threads) ? RED : YELLOW, false);
    printstr(s);
    freevect(asV(s));
    reclaim(s, sizeof(struct Atom));
    puts("\n");
}

void println(Atom* a) {
    Atom* s = atomstr(a, 0, (a == Threads) ? RED : YELLOW, true);
    printstr(s);
    freevect(asV(s));
    reclaim(s, sizeof(struct Atom));
    puts("\n");
}

// get the index in the string vect where a member of c first appears
int strindexof(Atom* s, char* c) {
    if (!asV(s)) {return -1;}
    Vect* v = asV(s);
    int i = 0;
    while (i < v->len && !contains(c, v->v[i])) {i++;}
    if (i == v->len) {return -1;}
    return i;
}
int strindexofnot(Atom* s, char* c) {
    if (!asV(s)) {return -1;}
    Vect* v = asV(s);
    int i = 0;
    while (i < v->len && contains(c, v->v[i])) {i++;}
    if (i == v->len) {return -1;}
    return i;
}
bool isstrempty(Atom* s) {return asV(s)->len == 0 || asV(s)->v[0] == 0;}
bool discardn(Atom* s, int n) {
    setstr(s, s->d.v->v+n, s->d.v->len-n);
    return isstrempty(s);
}
// shrinks s1 to length i and returns s2 as the part that was removed
Atom* splitat(Atom* s1, int i) {
    if (i == -1) {i = s1->d.v->len;}
    Atom* s2 = substr(s1, 0, i);
    discardn(s1, i);
    return s2;
}
Atom* splitonchars(Atom* s, char* c) {
    int i = strindexof(s, c);
    return splitat(s, i);
}
Atom* splitonnotchars(Atom* s, char* c) {
    int i = strindexofnot(s, c);
    return splitat(s, i);
}

bool isnum(char c) {return c >= '0' && c <= '9';}
bool ishex(char c) {return isnum(c) || (c >= 'a' && c <= 'f');}

Atom* strtonum(Atom* s) {
    char* str = asV(s)->v;
    if (!str) return 0;
    bool negative = false;
    if (*str == '-') {negative = true; str++;}
    if (!isnum(*str)) {return 0;}

    int b = 10;
    if (str[0] == '0') {
        if (str[1] == 'x')      {b = 0x10; str += 2;}
        else if (str[1] == 'b') {b = 0b10; str += 2;}
        else if (!isnum(*str)) {return 0;}
    }

    Word result = 0;
    while (*str) {
        if (*str == '_') { str++; continue; }
        int digit;
        if      (isnum(*str)) {digit = *str - '0';}
        else if (ishex(*str)) {digit = *str - 'a' + 10;}
        if (digit < 0 || digit > b) {return 0;}
        result = result*b + digit;
        str++;
    }
    Atom* w = new(words);
    w->d.w = negative ? -result : result;
    return w;
}

#define whitespace " \n\t"
bool discardwhitespace(Atom* s) {
    int i = strindexofnot(s, whitespace);
    return discardn(s, i);
}

// Returns the length of the char*, accounting for
// any backslashes, escape characters etc
Atom* charptostr(Atom* a, char addon, char breakon) {
    int i, j, depth = 1;
    char* c = asV(a)->v;
    Atom* s = newvect(0);
    Vect* v = asV(s);
    char ch;
    i = 0; j = 1;
    for (; c[i+j]; i++) {
        if (c[i+j] == breakon) {depth--;}
        if (!depth) {break;}
        if (c[i+j] == addon) {depth++;}
        ch = c[i+j];
        if (c[i+j] == '\\') {
            j++;
            ch = c[i+j];
            if (c[i+j] == 'n') {ch = '\n';}
            if (c[i+j] == 't') {ch = '\t';}
            if (c[i+j] == 'e') {ch = '\e';}
        }
        v = vectpushc(v, ch);
    }
    s->d.v = vectpushc(v, 0);
    discardn(a, i+j+1);
    return s;
}
// Searches straight down.  If a containing list has a parent,
// It will be searched as well.
// This is used for implicit variable search: `val :name name`
// This is related to the :symbol functionality.
// Since one scan call leaves (a reference to) the associated
// variable on the top, :symbol2 can be called again to get
// a variable from the second, interior layer.
Atom* matchvar(Atom* a, char* c) {
    if (!isend(a) && asV(a->n) && equstr(asV(a->n)->v, c)) {
        return a;
    }
    return 0;
}
Atom* reversescan(Atom* a, Atom* w) {
    Atom* prev = 0;
    if (w->d.f == scanfunc) {return str(":");}
    while (a) {
        if (prev && w->d.w == prev->d.w && asV(a)) {break;}
        if (asV(a) && equstr(asV(a)->v, ":")) {
            prev = reversescan(prev->d.a, w);
            if (prev) {return prev;}
        }
        prev = a;
        a = a->n;
    }
    return a;
}

// Scans an atom up to the provided e (end) atom.
Atom* scan(Atom* a, Atom* end, char* c) {
    Atom* v = 0;
    bool totail = (Word) end == -1;
    while (a && a != end) {
        v = matchvar(a, c);
        if (v) {return v;}
        v = matchvar(a, ":");
        if (v) {
            v = scan(asA(v), a, c);
            if (v) {return v;}
        }
        if (totail && isend(a)) {return 0;}
        a = a->n;
    }
    return 0;
}
Atom* scantail(Atom* a, char* c) {
    return scan(a, (Atom*) -1, c);
}
Error varrecscanfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* s = pulla(d);
    vectfail(s);
    if (asV(asA(d))) {
        Error er = varrecscanfunc(D, d, e, r);
        if (er.msg) {return er;}
    }    
    Atom* paa = asA(asA(d));
    Atom* a = scantail(paa, asV(s)->v);
    if (a) {
        ref(a);
        pullr(d, r);
        push(d, duplicate(a));
        del(a);
    }
    else {
        return fail("Could not find variable in scan");
    }
    del(s);
    return passA(d);
}
Error scanfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* s = pulla(d);
    vectfail(s);
    Atom* pa = asA(d);
    Atom* a = scan(pa, 0, asV(s)->v);
    if (a) {push(d, duplicate(a));}
    del(s);
    return passA(d);
}
Error newatomfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    pushnew(d, atoms, (data) (Atom*) 0);
    return passA(d);
}

void growthreadexec(Atom* e, Atom* a) {
    e->f = execs;
    pushnew(e, atoms, (data) a);
    if (!isend(a)) {return growthreadexec(e, a->n);}
}
bool run(Atom* D, Atom* d, Atom* e, Atom* r) {
    if (!e || isempty(e)) {return false;}
    // newr(e, r);
    // Atom* ea = pullr(e, r);
    Atom* ea = pulla(e);
    Atom* eaa = asA(ea);
    if (eaa->f == dots) {dot(D, d, e, r);}
    else if (eaa->f == vects) {push(d, dupstr(eaa));}
    else {push(d, duplicate(eaa));}
    del(ea);
    return true;
}
Error arraydot(Atom* D, Atom* d, Atom* a, Atom* e, Atom* r) {
    Error er = passA(d);
    if (a->f == ends) {return er;}
    if (!isend(a)) {er = arraydot(D, d, a->n, e, r);}
    d = traverselinks(D);
    if (debugging) {
        // printa(D);
    }
    if (er.msg) {return er;}
    if (a->f == dots) {return dot(D, d, e, r);}
    push(d, duplicate(a));
    return passA(d);
}
Error dot(Atom* D, Atom* d, Atom* e, Atom* r) {
    atomfail(D);
    d = traverselinks(D);
    Atom* a = asA(d);
    if (asV(a)) {return varrecscanfunc(D, d, e, r);}
    Func f = asF(a);
    if (f) {newr(d, r); pullr(d, r); return f(D, d, e, r);}
    if (isA(a)) {
        if (isempty(a)) {newr(d, r); pullr(d, r); return passA(d);}
        Atom* temp = pulla(d);
        a = asA(a);
        Error er;
        if (e) {growthreadexec(e, a);}
        else {er = arraydot(D, d, a, e, r);}
        del(temp);
        if (er.msg) {return er;}
    }
    else if (a->f == dots) {newr(d, r); pullr(d, r); return dot(D, d, e, r);}
    return passA(d);
}
Atom* func(Func f);

bool matchandpushvar(Atom* d, Atom* a) {
    Atom* var;
    if (!asA(d)) {var = scan(d, 0, asV(a)->v);}
    else {var = scan(asA(d), 0, asV(a)->v);}
    if (var) {push(d, duplicate(var));}
    else {
        Error er = fail(asV(addstrch(str(asV(a)->v), " not found."))->v);
        return false;
    }
    return true;
}
bool token(Atom* D, Atom* d, Atom* e, Atom* r, Atom* s, Error* er) {
    if (discardwhitespace(s)) {return false;}
    Vect* v = asV(s);
    if (v->v[0] == '"') {push(d, charptostr(s, '"', '"'));}
    else if (v->v[0] == '(') {del(ref(charptostr(s, '(', ')')));}
    else if (v->v[0] == '.') {
        Atom* a = splitonchars(s, whitespace);
        char* ch = asV(a)->v;
        if (ch[1] == '.') {push(d, new(dots));}
        else if (ch[1] != 0) {
            discardn(a, 1);
            matchandpushvar(d, a);
        }
        else {
            *er = dot(D, d, e, r);
            if (er->msg) {return false;}
            d = traverselinks(D);
        }
        del(ref(a));
    }
    else if (v->v[0] == ':') {
        Atom* a = splitonchars(s, whitespace);
        v = asV(a);
        v->len = chlen(v->v)+1;
        bool isref = v->v[v->len-2] == '.';
        if (isref) {v->v[--v->len-1] = 0;}
        if (equstr(asV(a)->v, ":")) {
            push(d, func(scanfunc));
            if (!isref) {dot(D, d, e, r);}
        }
        else {push(d, str(asV(a)->v+1));}
        if (isref) {push(d, new(dots));}
        del(ref(a));
    }
    else {
        Atom* a = splitonchars(s, whitespace "\"(");
        Atom* w = strtonum(a);
        if (w) {del(ref(a)); push(d, w); return true;}
        v = asV(a);
        v->len = chlen(v->v)+1;
        bool isref = v->v[v->len-2] == '.';
        if (isref) {v->v[--v->len-1] = 0;}
        matchandpushvar(d, a);
        if (isref) {push(d, new(dots));}
        else {dot(D, d, e, r);}
        del(ref(a));
    }
    return true;
}

Atom* runall(Atom* D, Atom* e, Atom* r) {
    Atom* d;
    while (run(D, d = traverselinks(D), e, r));
    return d;
}

Error stepfunc(Atom* D, Atom* d, Atom* e, Atom* r);
Error advancethreads() {
    Atom* t = asA(Threads);
    Atom* d = ref(new(atoms));
    Error e;
    while (t) {
        if (isempty(t)) {break;}
        push(t, func(stepfunc));
        e = dot(t, t, 0, 0);
        if (e.msg) {return e;}
        if (isempty(asA(t))) {pull(Threads); break;}
        if (isempty(asA(t->n))) {removeafter(t);}
        if (isend(t)) {break;}
        t = t->n;
    }
    del(d);
    return passA(0);
}

Error tokens(Atom* D, Atom* e, Atom* r, Atom* s) {
    atomfail(D);
    Error er = passA(D);
    Atom* d;
    while (token(D, runall(D, e, r), e, r, s, &er)) {
        advancethreads();
    }
    return er;
}
void addvar(Atom* d, char* k, Atom* v) {
    push(d, str(k));
    push(d, v);
}

Atom* func(Func f) {
    Atom* a = new(funcs);
    a->d.f = f;
    return a;
}

void mathfunc(Word* x, Word* y, Atom* d, Atom* r) {
    wordfail(asA(d));
    wordfail(asA(d)->n);
    if (!r) {
        *x = pulld(d, r).d.w;
        *y = pulld(d, r).d.w;
        return;
    }
    *x = asA(d)->d.w;
    *y = asA(d)->n->d.w;
    pullx(d, 2);
}

#define mathfuncbuild(name, op) \
Error name ## func(Atom* D, Atom* d, Atom* e, Atom* r) { \
    Word x, y; \
    mathfunc(&x, &y, d, r); \
    pushw(d, y op x); \
    return passA(d); \
}
mathfuncbuild(add, +);
mathfuncbuild(sub, -);
mathfuncbuild(mul, *);
mathfuncbuild(div, +);
mathfuncbuild(and, &&);

Error notfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Word w = pulld(d, r).d.w;
    pushw(d, !w);
    return passA(d);
}

Error undofunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    r = asA(asA(d));
    Atom* dr = asA(asA(r)->n);
    tset(dr, asA(asA(asA(r))));
    pullx(r, 2);
    return passA(d);
}

Error enterlink(Atom* d) {
    if (isempty(d)) {return fail("d is empty");}
    Atom* a = asA(d);
    atomfail(a);
    a->f = links;
    return passA(d);
}

Error getlen(Atom* D, Atom* d, Atom* e, Atom* r) {
    pushw(d, length(d));
    return passA(d);
}

Error stepfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    d = asA(d);
    run(asA(d->n), traverselinks(asA(d->n)), d, r);
    return passA(d);
}

Error growexecfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* a = ref(asA(d));
    growthreadexec(a->n, asA(a));
    del(a);
    pull(d);
    return passA(d);
}

Error runfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    r = asA(d)->n->n;
    r = (isA(r)) ? asA(r) : 0;
    runall(asA(asA(d)->n), asA(d), r);
    return passA(d);
}

Error detachfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    pushnew(Threads, atoms, (data) asA(d));
    return passA(d);
}

Error choosefunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* w = get(asA(d), 2);
    wordfail(w);
    if (asW(w)) {swap(d);}
    pullr(d, r);
    return passA(d);
}

bool shapecompare(Atom* a, Atom* b) {
    if (a == 0 && b == 0) {return true;}
    if (a == 0 || b == 0) {return false;}
    if (a->f != b->f) {return false;}
    if (isA(a) && !shapecompare(asA(a), asA(b))) {return false;}
    if (isend(a) && isend(b)) {return true;}
    return shapecompare(a->n, b->n);
}
Error shapecomparefunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* a = asA(d);
    Atom* b = a->n;
    if (a->f != b->f) {pushw(d, 0);}
    else {pushw(d, shapecompare(asA(a), asA(b)));}
    return passA(d);
}

Error assertfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    if (asW(asA(d))) {return fail(asA(d)->n->d.v->v);}
    pullx(d, 2);
    return passA(d);
}

Error duplicatefunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Error er = pulld(d, r);
    if (er.msg) {return er;}
    Word w = er.d.w;
    if (!w) {pullr(d, r); return passA(d);}
    while (--w) {push(d, duplicate(asA(d)));}
    return passA(d);
}

Error pullfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Error er = pulld(d, r);
    if (er.msg) {return er;}
    return pullx(d, er.d.w);
}

Error newlink(Atom* D, Atom* d, Atom* e, Atom* r) {
    if (isempty(d)) {return fail("d is empty");}
    Atom* a = asA(d);
    atomfail(a);
    a->f = links;
    if (a->d.a == 0) {pushend(a, a);}
    return passA(a); // note a not d
}

Error closelink(Atom* D, Atom* d, Atom* e, Atom* r) {
    if (d->f != links) {return fail("d is not a link");}
    d->f = atoms;
    return passA(traverselinks(D));
}

Error linkstep(Atom* D, Atom* d, Atom* e, Atom* r) {
    if (isempty(d)) {return fail("d is empty");}
    // Word w = pulld(d, r).d.w;
    Atom* l = asA(d);
    if (!isA(l)) {return fail("target is not pointable");}
    // if (asA(l)) {tset(l, get(asA(l), w));}
    // else {tset(l, get(l, w));}
    if (asA(l)) {tset(l, asA(l)->n);}
    else {tset(l, l->n);}
    return passA(d);
}

Error linkenter(Atom* D, Atom* d, Atom* e, Atom* r) {
    if (isempty(d)) {return fail("d is empty");}
    Atom* l = asA(d);
    if (!isA(l)) {return fail("target is not pointable");}
    if (asA(l)) {tset(l, asA(asA(l)));}
    else {tset(l, l->n);}
    return passA(d);
}

Error absorbfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    push(d, duplicate(d->n));
    return passA(d);
}

Error throwfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    push(d, duplicate(asA(asA(d))));
    return passA(d);
}

Error printfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* s = asA(d);
    vectfail(s);
    printstr(s);
    pullr(d, r);
    return passA(d);
}

Error fgetfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
     char b[0x200];
    puts(RESET);
    fgets(b, 0x200, stdin);
    b[chlen(b)-1] = 0;
    push(d, str(b));
    return passA(d);
}
Error tokens(Atom* D, Atom* e, Atom* r, Atom* s);
Error parsefunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* s = pulla(d);
    vectfail(s);
    Error er = tokens(D, e, r, s);
    if (er.msg) {return er;}
    del(s);
    return passA(d);
}

Error printnodefunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    println(asA(d));
    return passA(d);
}

Error debugfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    debugging = !debugging;
    return passA(d);
}

unsigned int hash(Word w) {
    unsigned int h = 5381;
    const unsigned char* p = (const unsigned char*) &w;
    for (int i = 0; i < sizeof(w); i++) {
        h = ((h << 5) + h) + p[i];
    }
    return h & 0xf;
}

void mapset(Atom* map[0x10], Word k, Atom* v) {
    unsigned int i = hash(k);
    Atom* m = map[i];
    Atom* a = new(atoms);
    tset(a, v);
    if (m) {nset(a, m);}
    map[i] = nset(makew(k), a);
}

Atom* mapget(Atom* map[0x10], Word k) {
    unsigned int i = hash(k);
    Atom* m = map[i];
    
    while (m) {
        if (m->d.w == k) {return m->n->d.a;}
        m = m->n->n;
    }
    return 0;
}

void* setmem(void* ptr, int value, size_t num) {
    unsigned char* byte_ptr = (unsigned char *)ptr;
    for (size_t i = 0; i < num; ++i) {
        byte_ptr[i] = (unsigned char)value;
    }
    return ptr;
}

void mapinit(Atom* map[0x10]) {
    setmem(map, 0, sizeof(Atom*)*0x10);
}

void printvectkey(Vect* v) {
    printf("\"");
    for (int i = 0; i < v->len; i++) {
        unsigned char c = v->v[i];
        if (c >= 32 && c <= 126)
            printf("%c", c);
        else
            printf("\\x%02X", c);
    }
    printf("\"");
}

void printmap(Atom* map[0x10]) {
    for (int i = 0; i < 0x10; i++) {
        Atom* m = map[i];
        while (m) {
            printf(YELLOW "%p" RESET ":", m->d.a);
            printf(RED "%d" RESET ":", m->r);
            printf(GREEN "%p" RESET, m->n->d.a);
            printf(RED " %d" RESET ": ", m->n->r);
            printa(m->n->d.a);
            m = m->n->n;
        }
    }
}

void storeatomhelper(Atom* lst, Atom* a) {
    pushnew(lst, atoms, (data) a);
    if (isA(a)) {storeatomhelper(lst, asA(a));}
    if (!isend(a)) {storeatomhelper(lst, a->n);}
}

Error storeatomfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* f = asA(d);
    Atom* a = asA(d)->n;
    vectfail(f);
    atomfail(a);

    Vect* filebuf = valloclen(0);
    filebuf = rawpushv(filebuf, &a, sizeof(Atom*));
    Atom acpy;
    setmem(&acpy, 0, sizeof(Atom));
    acpy.d = a->d;
    acpy.e = a->e;
    acpy.f = a->f;
    acpy.n = 0;
    filebuf = rawpushv(filebuf, &acpy, sizeof(Atom));
    
    Atom* lst = ref(new(atoms));
    storeatomhelper(lst, asA(a));
    
    while (!isempty(lst)) {
        a = asA(lst);
        filebuf = rawpushv(filebuf, &a->d.a, sizeof(Word));
        acpy.d = asA(a)->d;
        acpy.e = asA(a)->e;
        acpy.f = asA(a)->f;
        acpy.n = asA(a)->n;
        filebuf = rawpushv(filebuf, &acpy, sizeof(Atom));
        Vect* v = asV(asA(a));
        if (v) {filebuf = rawpushv(filebuf, v, sizeof(Vect)+v->len);}

        pull(lst);
    }
    del(lst);

    FILE* FP = fopen(asV(f)->v, "w");
    if (!FP) {return fail("cannot open file");}
    fwrite(filebuf->v, 1, filebuf->len, FP);
    freevect(filebuf);
    fclose(FP);

    pullr(d, r);
    return passA(d);
}

Error loadatomfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* f = asA(d);
    vectfail(f);
    
    FILE* FP = fopen(asV(f)->v, "r");
    pullr(d, r);
    if (!FP) { return fail("cannot open file"); }

    Atom* newatoms = ref(new(atoms));

    Atom newatom;
    Word id;
    Atom* mapidpts[0x10];
    mapinit(mapidpts);

    fread(&id, sizeof(Word), 1, FP);
    fread(&newatom, sizeof(Atom), 1, FP);

    Atom* root = newraw(&newatom);
    root->r = 0;
    root->n = 0;
    pushnew(newatoms, words, (data) root);
    mapset(mapidpts, id, root);

    while (1) {
        if (fread(&id, sizeof(Word), 1, FP) != 1) {break;}
        if (fread(&newatom, sizeof(Atom), 1, FP) != 1) {break;}

        Atom* a = newraw(&newatom);
        a->r = 0;
        pushnew(newatoms, words, (data) a);
        mapset(mapidpts, id, a);
        if (a->f == vects) {
            Vect vcpy;
            fread(&vcpy, sizeof(Vect), 1, FP);
            Vect* v = valloclen(vcpy.len);
            *v = vcpy;

            if (v->len > 0) {fread(v->v, 1, v->len, FP);}
            a->d.v = v;
        }
    }

    Atom* m, *a;
    while (!isempty(newatoms)) {
        a = asA(newatoms)->d.a;
        m = mapget(mapidpts, (Word) a->n);
        if (m) {
            if (!isend(a)) {a->n = ref(m);}
            else {a->n = m;}
        }
        m = mapget(mapidpts, a->d.w);
        if (m) {
            a->d.a = ref(m);
        }
        pull(newatoms);
    }
    del(newatoms);

    push(d, root);
    for (int i = 0; i < 0x10; i++) {
        del(ref(mapidpts[i]));
    }
    fclose(FP);
    return passA(d);
}

Error storetextfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* f = asA(d);
    Atom* s = asA(d)->n;
    vectfail(f);
    vectfail(s);
    FILE* FP = fopen(asV(f)->v, "w");
    fwrite(asV(s)->v, 1, asV(s)->len-1, FP);
    fclose(FP);
    Error er = pullx(d, 2);
    if (er.msg) {return er;}
    return passA(d);
}

Error appendtextfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* f = asA(d);
    Atom* s = asA(d)->n;
    vectfail(f);
    vectfail(s);
    FILE* FP = fopen(asV(f)->v, "a");
    fwrite(asV(s)->v, 1, asV(s)->len-1, FP);
    fclose(FP);
    Error er = pullx(d, 2);
    if (er.msg) {return er;}
    return passA(d);
}

Error loadtextfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* f = asA(d);
    vectfail(f);
    FILE* FP = fopen(asV(f)->v, "r");
    fseek(FP, 0, SEEK_END);
    int i = ftell(FP);
    rewind(FP);
    Atom* s = newvect(i+1);
    s->d.v->len = i+1;
    fread(asV(s)->v, 1, i, FP);
    asV(s)->v[i] = 0;
    fclose(FP);
    pullr(d, r);
    push(d, s);
    return passA(d);
}

void reversestack(Atom* a) {
    if (isempty(a)) {return;}
    if (isend(asA(a))) {return;}
    Atom* b = ref(new(atoms));
    while (!isempty(a)) {
        Atom* c = ref(asA(a));
        pull(a);
        push(b, c);
        del(c);
    }
    tset(a, asA(b));
    del(b);
}
Error reversefunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    if (isempty(d)) {return fail("d is empty");}
    reversestack(asA(d));
    return passA(d);
}
Error runonbranch(Atom* a, Atom* f) {
    Atom* d = ref(new(links));
    tset(d, a);
    push(d, duplicate(f));
    Error er = dot(d, d, 0, 0);
    if (er.msg) {return er;}
    a = pulla(d);
    del(d);
    return passA(a);
}

Error whilefunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* a = get(asA(d), 0);
    Atom* b = get(asA(d), 1);
    Error er = runonbranch(asA(d), b);
    if (er.msg) {return er;}
    while (asW(er.d.a)) {
        push(d, duplicate(a));
        dot(d, d, 0, 0);
        er = runonbranch(asA(d), b);
    }
    return passA(d);
}

Error isintfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* a = asA(d);
    pushw(d, a->f == words);
    return passA(d);
}

// needs to be changed to not destroy b
Error reducefunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* a = pulla(d);
    Atom* b = asA(d);
    while (!isend(asA(b))) {
        push(b, a);
        dot(b, b, 0, 0);
    }
    return passA(d);
}

Error maphelper(Atom* result, Atom* a, Atom* f) {
    if (!isend(a)) {maphelper(result, a->n, f);}
    Error er = runonbranch(a, f);
    if (er.msg) {return er;}
    push(result, er.d.a);
    del(er.d.a);
    return passA(result);
}
Error mapfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* a = pulla(d);
    Atom* cur = asA(asA(d));
    Atom* result = pushnew(d, atoms, (data) 0ll);
    Error er = maphelper(result, cur, a);
    if (er.msg) {return er;}
    del(a);
    return passA(d);
}

void ziphelper(Atom* d, Atom* a, Atom* b, Word an, Word bn) {
    if (an < bn) {ziphelper(d, a, b->n, an, bn-1); return;}
    if (an > bn) {ziphelper(d, a->n, b, an-1, bn); return;}
    if (!isend(a) && !isend(b)) {ziphelper(d, a->n, b->n, an, bn);}
    push(d, duplicate(b));
    push(d, duplicate(a));
}

Error zipfunc(Atom* D, Atom* d, Atom* e, Atom* r) {
    Atom* a = new(atoms);
    Atom* c = pulla(d);
    Atom* b = pulla(d);
    ziphelper(a, asA(c), asA(b), length(c), length(b));
    push(d, a);
    del(c);
    del(b);
    return passA(d);
}

Error tokench(Atom* d, char* c) {
    Atom* s = ref(str(c));
    Error er = tokens(d, 0, 0, s);
    del(s);
    return er;
}

Atom* newglobal() {
    Atom* g = ref(new(atoms));
    push(g, str(":"));
    push(g, duplicate(Library));
    return g;
}

Atom* newtokench(char* c) {
    Atom* g = newglobal();
    Atom* d = pushnew(g, atoms, (data) 0ll);
    tokench(d, c);
    Atom* a = pulla(g);
    nset(a, 0);
    del(g);
    return a;
}

Atom* envch(Atom* a, char* c) {
    Atom* g = newglobal();
    push(g, str("env"));
    push(g, ref(a));
    del(a);
    Atom* d = pushnew(g, atoms, (data) 0ll);
    tokench(d, c);
    a = pulla(d);
    del(g);
    return a;
}

#define assert(cond) \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } 

#define addfvar(c, f) addvar(Library, c, func(f))
int main(int argc, char** argv) {
    char* fname = "challenge";
    if (argc > 1) {fname = argv[1];}
    FILE* FP = fopen(fname, "r");
    fseek(FP, 0, SEEK_END);
    int i = ftell(FP);
    char program[i+1];
    char* program_ = program;
    rewind(FP);
    while (!feof(FP)) {*program_++ = fgetc(FP);}
    *--program_ = 0;
    fclose(FP);

    Global = ref(new(atoms));
    Threads = ref(new(atoms));
    push(Global, str(":"));
    Library = pushnew(Global, atoms, (data) 0ll);
    addfvar("print",        printfunc);
    addfvar("printnode",    printnodefunc);
    addfvar("debug",        debugfunc);
    addfvar("input",        fgetfunc);
    addfvar("parse",        parsefunc);
    addfvar("store",        storetextfunc);
    addfvar("appendfile",   appendtextfunc);
    addfvar("load",         loadtextfunc);
    addfvar("reverse",      reversefunc);
    addfvar("map",          mapfunc);
    addfvar("length",       getlen);
    addfvar("step",         stepfunc);
    addfvar("growexec",     growexecfunc);
    addfvar("run",          runfunc);
    addfvar("detach",       detachfunc);
    addfvar("storeatom",    storeatomfunc);
    addfvar("loadatom",     loadatomfunc);
    addfvar("assert",       assertfunc);
    addfvar("undo",         undofunc);
    addfvar("zip",          zipfunc);
    addfvar("while",        whilefunc);
    addfvar("int",          isintfunc);
    addfvar("reduce",       reducefunc);
    addfvar("and",          andfunc);
    addfvar("not",          notfunc);
    addfvar("@",            newatomfunc);
    addfvar("#",            shapecomparefunc);
    addfvar("?",            choosefunc);
    addfvar("+",            addfunc);
    addfvar("*",            mulfunc);
    addfvar("-",            subfunc);
    addfvar("/",            divfunc);
    addfvar(";",            duplicatefunc);
    addfvar(",",            pullfunc);
    addfvar("[",            newlink);
    addfvar("]",            closelink);
    addfvar("<~",           linkstep);
    addfvar("~>",           linkenter);
    addfvar("<-",           absorbfunc);
    addfvar("->",           throwfunc);
    Atom* d = pushnew(Global, links, (data) 0ll);
    Error er = tokench(d, program);
    if (er.msg) {
        fprintf(stderr, RED "%s\n" RESET, er.msg);
        del(er.d.a);
    }
    else {println(asA(d));}
    // Atom* d = newtokench("1 2 3 4 + ; env ");
    // println(d);
    // del(d);
    // del(env);
    del(Global);
    del(Threads);
    return 0;
}
