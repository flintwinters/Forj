#ifndef __linux__
#define runrv64
#endif
#ifdef runrv64
#include "../rv64/alloc.c"
#else
#include <stdlib.h>
#include "utils.c"
#endif
typedef struct Vect Vect;
typedef struct Node Node;
typedef Node String;
typedef void (*func)(Node*);
typedef long long word;

struct Vect {short len, maxlen; byte v[];};
struct Node {
    word w;
    Node* p, *name, *d, *t;
    Vect* s;
};
struct Vect* valloclen(int maxlen) {
    int n = sizeof(struct Vect)+maxlen;
    struct Vect* newv = malloc(n);
    newv->maxlen = maxlen;
    newv->len = 0;
    return newv;
}
void cpymem(byte* dest, byte* src, word len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[i];
    }
}
void cpymemrev(byte* dest, byte* src, word len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[len-i-1];
    }
}
struct Vect* resize(struct Vect* v, int newmaxlen) {
    if (!newmaxlen) {
        newmaxlen = (v->maxlen) ? 2*v->maxlen: 1;
    }
    struct Vect* newv = valloclen(newmaxlen);
    newv->len = v->len;
    cpymem(newv->v, v->v, v->len);
    #ifdef runrv64
        reclaim(v, sizeof(struct Vect)+v->maxlen);
    #else
        free(v);
    #endif
    return newv;
}
void reclaimnode(Node* n) {
    #ifdef runrv64
        reclaim(n->s, sizeof(struct Vect)+n->s->maxlen);
        reclaim(n, sizeof(Node));
    #else
        free(n->s);
        free(n);
    #endif
}
void growtofit(Node* n) {
    word newlen = 1;
    if (!n->s->maxlen) {newlen = n->s->maxlen;}
    while (newlen < n->s->len) {
        newlen = newlen << 1;
    }
    n->s = resize(n->s, newlen);
}
void condresize(Node* n, int addlen) {
    n->s->len += addlen;
    if (n->s->len > n->s->maxlen) {
        growtofit(n);
    }
}
void printv(Vect* v) {
    printint(v->len, 8);
    putchar(' ');
    printint(v->maxlen, 8);
    putchar('|');
    putchar(' ');
    for (int i = 0; i < v->len; i++) {
        printint(v->v[i], 1);
        putchar(' ');
    }
    putchar('\t');
    for (int i = 0; i < v->len; i++) {
        if (v->v[i] < ' ') {putchar('.');}
        else {putchar(v->v[i]);}
    }
    putchar('\n');
}

Node* pushn(Node* n, Node* m);
Node* newnode() {return malloc(sizeof(Node));}
Node* initnode(Node* p, Node* t, Node* d, word size) {
    Node* n = newnode();
    n->w = 0;
    n->p = p;
    n->name = 0;
    n->t = t;
    n->d = d;
    if (size) {n->s = valloclen(size);}
    else {n->s = 0;}
    return n;
}
void rawpushv(Node* n, void* dat, word size) {
    condresize(n, size);
    cpymem((char*) n->s->v+n->s->len-size, dat, size);
}
void rawpushvrev(Node* n, void* dat, word size) {
    condresize(n, size);
    cpymemrev((char*) n->s->v+n->s->len-size, dat, size);
}
Node* frombot(Node* n, int i) {return ((Node**) n->s->v)[i];}
Node* fromtop(Node* n, int i) {return ((Node**) (n->s->v))[n->s->len/sizeof(Node*)-i-1];}
Node* top(Node* n) {return fromtop(n, 0);}
void pushc(String* n, char c) {rawpushv(n, &c, sizeof(char));}
void pushw(Node* n, word w) {
    rawpushv(n, &w, sizeof(word));}
Node* pushn(Node* n, Node* m) {rawpushv(n, &m, sizeof(Node*)); return top(n);}
Node* pushi(Node* n, int i) {return pushn(n, fromtop(n, i));}
void* pullb(Node* n, int i) {
    void* p = n->s->v+n->s->len-i;
    n->s->len -= i;
    return p;
}
Node* pullx(Node* n, int x) {
    Node* m = fromtop(n, x-1);
    n->s->len -= sizeof(Node*)*x;
    return m;
}
Node* pulln(Node* n) {return pullx(n, 1);}
Node* throw(Node* dest, Node* src) {return pushn(dest, pulln(src));}
Node* explode(Node* dest, Node* src) {
    rawpushv(dest, &src->s->v, src->s->len);
    return dest;
}

int slen(char* s) {
    int n = 0;
    while (*s++) {n++;}
    return n;
}
Node* T;
Node* getvch(Node* n, char* c);
String* newstr(char* c);
#define tmaketype(name) (getvch(T, #name))
#define ttype       tmaketype(type)
#define tliteral    tmaketype(literal)
#define tlink       tmaketype(link)
#define tpath       tmaketype(path)
#define tmap        tmaketype(map)
#define tarray      tmaketype(array)
#define tstring     tmaketype(string)
#define texec       tmaketype(exec)
#define tbang       tmaketype(bang)
bool strequ(String* A, String* B) {
    char* a = (char*) A->s->v;
    char* b = (char*) B->s->v;
    for (int i = 0; i < min(A->s->len, B->s->len); i++) {
        if (a[i] != b[i]) {return 0;}
    }
    return 1;
}
Node* getmap(Node* n, String* k, bool search) {
    if (!n || !n->s || !n->s->len) {return 0;}
    Node** m = (Node**) n->s->v;
    Node* l;
    for (int i = 0; i < n->s->len/sizeof(Node*); i += 2) {
        if (!m[i]) {continue;}
        if (search && m[i]->t == tmap && (l = getmap(m[i], k, true))) {
            return l;
        }
        if (strequ(m[i], k)) {return m[i+1];}
    }
    return 0;
}
Node* searchvar(Node* n, String* k) {
    if (!n) {return 0;}
    Node* m = getmap(n->d, k, true);
    if (m) {return m;}
    m = searchvar(n->t, k);
    if (m) {return m;}
    return searchvar(n->p, k);
}
void concatcharp(Node* n, char* s) {rawpushvrev(n, s, slen(s));}
String* rawnewstr(char* c) {
    String* s = initnode(0, 0, 0, 1);
    // for (int i = 0; i < slen(c); i++) {pushc(s, c[i]);}
    for (int i = 0; i < slen(c); i++) {pushc(s, c[slen(c)-i-1]);}
    return s;
}
Node* searchvch(Node* n, char* c, bool search) {
    String* s = rawnewstr(c);
    Node* m;
    if (search) {m = searchvar(n, s);}
    else {m = getmap(n->d, s, false);}
    reclaimnode(s);
    return m;
}
Node* getvch(Node* n, char* c) {return searchvch(n, c, false);}
Node* newlit(word w) {
    Node* n = initnode(0, tliteral, 0, 0);
    n->w = w;
    return n;
}
Node* addvar(Node* n, String* k, Node* v) {
    if (!n->d) {n->d = initnode(n, tmap, 0, 1);}
    pushn(n->d, k);
    pushn(n->d, v);
    return v;
}
// Node* initnode(Node* p, Node* t, Node* d, word size) {
Node* addnewvar(Node* n, String* name, Node* t, Node* d, word size) {
    Node* m = initnode(n, t, d, size);
    m->name = name;
    addvar(n, name, m);
    return m;
}
String* newstr(char* c) {
    String* s = rawnewstr(c);
    s->t = tstring;
    return s;
}
void printstr(String* s) {
    for (int i = 0; i < s->s->len; i++) {putchar(s->s->v[s->s->len-i-1]);}
}
void printnode(Node* n) {
    if (!n) {return;}
    Node* m;
    if (n->t && (m = getvch(n->t, "printer"))) {
        if (m->w) {((void (*)(Node*)) m->w)(n);}
        else if (n->name) {YELLOW; printstr(n->name); RESET;}
    }
    else if (n->name) {printstr(n->name);}
    else {
        BLACK;
        puts(" 0 ");
        RESET;
    }
}
void printnodeln(Node* n) {
    printnode(n);
    putchar('\n');
}
void ttypeprinter(Node* n) {
    if (n->t) {RED; putchar('('); RESET;}
    if (n->name) {YELLOW; printstr(n->name); RESET;}
    else {puts("''");}
    if (n->t) {
        BLUE; puts(" <: "); RESET;
        printnode(n->t);
        RED; putchar(')'); RESET;
    }
    if (n->d) {printnode(n->d);}
}
void tarrayprinter(Node* n) {
    BLUE;
    if (n->name) {printstr(n->name);}
    else {puts("''");}
    if (!n->s->len) {RESET; return;}
    BLUE; puts(":["); RESET;
    for (int i = 0; i < n->s->len/sizeof(Node*); i++) {
        if (((Node**) n->s->v)[i]) {
            printnode(((Node**) n->s->v)[i]);
        }
        else {BLACK; puts("null"); RESET;}
        if (i < n->s->len/sizeof(Node*)-1) {
            putchar(' ');
        }
    }
    BLUE putchar(']'); RESET;
}
void tmapprinter(Node* n) {
    GREEN;
    if (n->name) {printstr(n->name);}
    else {puts("''");}
    GREEN; puts(":{"); RESET;
    for (int i = 0; i < n->s->len/sizeof(Node*); i++) {
        printnode(((Node**) n->s->v)[i]);
        if (i < n->s->len/sizeof(Node*)-1) {
            if (i % 2) {putchar(' ');}
            else {putchar(':');}
        }
    }
    GREEN; putchar('}'); RESET;
}
void tstringprinter(Node* n) {
    GREEN; putchar('"');
    printstr(n);
    putchar('"'); RESET;
}
void texecprinter(Node* n) {
    PURPLE;
    if (n->name) {printstr(n->name);}
    else {puts("''");}
    RESET;
}
void tliteralprinter(Node* n) {printint(n->w, 8);}
void tbangprinter(Node* n) {
    DARKYELLOW;
    putchar('!');
    for (int i = 0; i < n->w; i++) {
        putchar('!');
    }
    RESET;
}
Node* addfunc(Node* n, char* c, func f) {
    String* s = newstr(c);
    Node* m = addnewvar(n, s, texec, 0, 0);
    m->w = (word) f;
    return m;
}
void bang(Node* P, Node* n) {
    Node* f = searchvch(n, "!", true);
    if (f) {((func) f->w)(P);}
    // else {pushn(top(P), n);}
}
void texecbang(Node* P) {((func) top(top(P))->w)(P);}
void tbangbang(Node* P) {top(top(P))->w--;}
void tarraybang(Node* P) {
    pushn(P, top(top(P)));
    Node* arr = top(P);
    Node** v = (Node**) arr->s->v;
    for (int i = 0; i < arr->s->len/sizeof(Node*); i++) {
        bang(P, v[i]);
    }
}
Node* addtype(char* c) {
    String* s = newstr(c);
    Node* t = addnewvar(T, s, ttype, 0, 0);
    t->p = 0;
    t->name = s;
    return t;
}
void initworld() {
    T = initnode(0, 0, 0, 1);
    T->d = initnode(0, 0, 0, 1);
    Node* str = rawnewstr("type");
    Node* t = addnewvar(T, str, 0, 0, 0);
    t->p = 0;
    t->name = str;
    str = rawnewstr("string");
    Node* n = addnewvar(T, str, ttype, 0, 0);
    n->p = 0;
    n->name = str;
    n->t = t;
    ((Node**) T->d->s->v)[0]->t = tstring;
    ((Node**) T->d->s->v)[2]->t = tstring;
    T->d->t = addtype("map");
    addtype("array");
    addtype("exec");
    addtype("literal");
    addtype("link");
    addtype("path");
    addtype("bang");

    addfunc(ttype,      "printer", 0);
    addfunc(tliteral,   "printer",  tliteralprinter);
    addfunc(tarray,     "printer",  tarrayprinter);
    addfunc(tpath,      "printer",  tarrayprinter);
    addfunc(tmap,       "printer",  tmapprinter);
    addfunc(tstring,    "printer",  tstringprinter);
    addfunc(texec,      "printer",  texecprinter);
    addfunc(tbang,      "printer",  tbangprinter);
    addfunc(texec, "!", texecbang);
    addfunc(tbang, "!", tbangbang);
    addfunc(tarray, "!", tarraybang);
}
int containstr(char* s, char c) {
    for (int i = 0; s[i];) {
        if (s[i] == c) {return i;}
        i++;
    }
    return -1;
}
void strtoint(Node* n) {
    word m = 0;
    int b = 10;
    String* s = top(n);
    char* str = (char*) s->s->v;
    word i = s->s->len-1;
    bool neg = false;
    if (str[i] == '-') {neg = true; i--;}
    if (str[i] == '0') {
        if (str[i-1] == 'x') {b = 0x10; i -= 2;}
        if (str[i-1] == 'b') {b = 2; i -= 2;}
    }
    while (i >= 0) {
        m *= b;
        if (str[i] >= 'a' && str[i] <= 'f') {n += str[i]-'a';}
        else {m += str[i]-'0';}
        i--;
    }
    Node* l = initnode(0, tliteral, 0, 0);
    if (neg) {m *= -1;}
    l->w = m;
    pushn(n, l);
}
void printtest(Node* a) {
    puts("HELLO DOWN THERE!\n");
}
void inttostr(Node* T, word n) {
    int b = 0x10;
    String* s = newstr("");
    while (n) {
        char c[2] = {"0123456789abcdef"[n%0x10], '\0'};
        concatcharp(s, c);
        n /= b;
    }
    concatcharp(s, "x\0");
    String* ss = newstr("");
    rawpushvrev(ss, s->s->v, s->s->len);
    reclaimnode(s);
    pushn(T, ss);
}
void findoneof(Node* n) {
    Node* b = pulln(n);
    String* s = fromtop(n, 0);
    char* a = (char*) s->s->v;
    int len = s->s->len;
    word j, i;
    for (i = len-1; i >= 0; i--) {
        if (-1 != (j = containstr((char*) b->s->v, a[i]))) {
            pushn(n, newlit(len-1-i));
            pushn(n, newlit(((char*) b->s->v)[j]));
            return;
        }
    }
    pushn(n, newlit(len));
    pushn(n, newlit(0));
}
void splitstrat(Node* n, int consume) {
    word  i   = pulln(n)->w;
    String* str = top(n);
    String* newst = pushn(n, newstr(""));
    rawpushv(newst, str->s->v+str->s->len-i, i);
    str->s->len -= i+consume;
}
void splitfind(Node* n, char* c, int consume) {
    pushn(n, newstr(c));
    findoneof(n);
    word w = top(n)->w;
    pushi(n, 2);
    pushi(n, 2);
    if (!w) {splitstrat(n, 0);}
    else {splitstrat(n, consume);}
}
bool isnum(Node* n) {
    String* s = top(n);
    char* c = (char*) s->s->v+s->s->len-1;
    return
        -1 != containstr("0123456789", c[0]) ||
        (s->s->len > 1 && c[0] == '-' &&
        -1 != containstr("0123456789", c[-1]));

}
Node* searchpath(Node* P, Node* k) {
    Node* v;
    for (int i = 0; i < P->s->len/sizeof(Node*); i++) {
        v = searchvar(fromtop(P, i), k);
        if (v) {return v;}
    }
    return 0;
}
void setsearch(Node* P, Node* p) {((Node**) (P->s->v+P->s->len))[-1] = p;}
void addtok(Node* P, Node* S) {
    if (isnum(S)) {strtoint(S); throw(top(P), S);}
    else {
        Node* v = searchpath(P, top(S));
        if (P->w) {pulln(P); P->w = 0;}
        pushn(top(P), v);
    }
}
void cleanleadingspace(String* s) {
    while (-1 != containstr(" \n\t", s->s->v[s->s->len-1])) {
        s->s->len--;
    }
}
void parsetok(Node* P, Node* S) {
    printnodeln(P);
    printnodeln(S);
    struct Vect* s = top(S)->s;
    while (-1 != containstr(" \n\t", s->v[s->len-1])) {
        s->len--;
    }
    splitfind(S, " \n\t:![]()\"`", 1);
    char c = fromtop(S, 2)->w;
    if (!c) {addtok(P, S); pullx(S, 4); return;}
    if (c == ':') {
        if (P->w) {setsearch(P, getmap(top(P)->d, top(S), true));}
        else {P->w = 1; pushn(P, searchpath(P, top(S)));}
        pullx(S, 4);
        return;
    }
    if (top(S)->s->len) {addtok(P, S);}
    pullx(S, 4);
    if (-1 != containstr(" \n\t", c)) {return;}
    if (c == '!') {
        String* s = top(S);
        int n = 0;
        while (s->s->v[s->s->len-n-1] == '!') {n++;}
        s->s->len -= n;
        if (n) {
            pushn(top(P), initnode(top(P), tbang, 0, 0))->w = n;
        }
        else {bang(P, top(top(P)));}
    }
    else if (c == '[') {
        if (P->w) {P->w = 0;}
        else {pushn(P, initnode(top(P), tarray, 0, 1));}
    }
    else if (c == ']') {Node* n = pulln(P); pushn(top(P), n);}
    else if (c == '(') {
        int depth = 1;
        s = top(S)->s;
        while (depth) {
            if ('(' == s->v[s->len-1]) {depth++;}
            else if (')' == s->v[s->len-1]) {depth--;}
            s->len--;
        }
    }
    else if (c == '"' || c == '`') {
        s = top(S)->s;
        String* str = initnode(top(P), tstring, 0, 1);
        while (s->v[s->len-1] != c) {
            if (s->v[s->len-1] == '\\') {s->len--;}
            pushc(str, s->v[s->len-1]);
            s->len--;
        }
        for (int i = 0; i < str->s->len/2; i++) {
            char temp = str->s->v[i];
            str->s->v[i] = str->s->v[str->s->len-i-1];
            str->s->v[str->s->len-i-1] = temp;
        }
        s->len--;
        pushn(top(P), str);
    }
}
void parse(Node* P) {
    Node* S = initnode(top(P), tarray, 0, 1);
    P->name = newstr("P");
    S->name = newstr("S");
    throw(S, top(P));
    while (frombot(S, 0)->s->len) {parsetok(P, S);}
}
void parsech(Node* n, char* c) {
    Node* P = initnode(n, tpath, 0, 1);
    pushn(P, newstr(c));
    parse(P);
}
void mainc() {
    initworld();

    Node* G = initnode(0, 0, 0, 0x100);
    G->name = newstr("G");
    T->name = newstr("T");
    T->t = tarray;
    printnode(T->d); puts("\n");
    pushn(G, T);
    pushn(G, T);
    G->t = tarray;
    pushn(G, newstr("1234"));
    strtoint(G);
    printnodeln(G);
    pushn(G, newstr("hello"));
    printnodeln(G);
    concatcharp(top(G), "hi");
    inttostr(G, 0x123);
    printnodeln(G);
    G->s->len -= sizeof(Node*);
    pushn(G, newlit(2));
    splitstrat(G, 0);
    printnodeln(G);
    printnodeln(getvch(tarray, "printer"));

    addnewvar(G, newstr("hello?"), texec, 0, 0)->w = (word) printtest;
    char* str = "       foo:fee 7 hello? ! arr (arr (arr) arr)"
    " \"hello\" arr:arr2 arr:arr2:arr3 arr:arr2:arr3:[1 2 3 4]123"
    " arr:arr2:arr3:[hello?] !";
    pushn(G, newstr(str));
    addnewvar(G, newstr("arr"), tarray, 0, 1);
    addnewvar(getvch(G, "arr"), newstr("arr2"), tarray, 0, 1);
    addnewvar(getvch(getvch(G, "arr"), "arr2"), newstr("arr3"), tarray, 0, 1);
    addvar(G, newstr("foo"), newstr("bar"));
    addvar(fromtop(G->d, 0), newstr("fee"), newstr("ber"));
    Node* P = initnode(0, tpath, 0, 1);
    pushn(P, G);
    parse(P);
    printnodeln(G);
}
int main() {mainc();}