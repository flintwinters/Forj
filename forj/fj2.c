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

struct Vect {word len, maxlen; byte v[];};
struct Node {
    word w;
    Node* name, *d, *t;
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

void* top(Node* n) {
    return n->s->v+n->s->len;
}
Node* pushn(Node* n, Node* m);
Node* newnode() {return malloc(sizeof(Node));}
Node* initnode(Node* t, Node* d, word size) {
    Node* n = newnode();
    n->w = 0;
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
void pushc(String* n, char c) {rawpushv(n, &c, sizeof(char));}
void pushw(Node* n, word w) {rawpushv(n, &w, sizeof(word));}
Node* pushn(Node* n, Node* m) {pushw(n, (word) m); return fromtop(n, 0);}
Node* pushi(Node* n, int i) {return pushn(n, fromtop(n, i));}
Node* pullx(Node* n, int x) {
    Node* m = fromtop(n, x-1);
    n->s->len -= sizeof(Node*)*x;
    return m;
}
Node* pulln(Node* n) {return pullx(n, 1);}

int slen(char* s) {
    int n = 0;
    while (*s++) {n++;}
    return n;
}
Node* T;
String* newstr(char* c);
#define tmaketype(name) (getmch(T, #name))
#define ttype       tmaketype(type)
#define tliteral    tmaketype(literal)
#define tmap        tmaketype(map)
#define tarray      tmaketype(array)
#define tstring     tmaketype(string)
#define texec       tmaketype(exec)
bool strequ(String* A, String* B) {
    char* a = (char*) A->s->v;
    char* b = (char*) B->s->v;
    for (int i = 0; i < min(A->s->len, B->s->len); i++) {
        if (a[i] != b[i]) {return 0;}
    }
    return 1;
}
Node* getmap(Node* n, String* k) {
    if (!n->d || !n->d->s || !n->d->s->len) {return 0;}
    for (int i = 0; i < n->d->s->len/sizeof(Node*); i += 2) {
        if (strequ(((Node**) n->d->s->v)[i], k)) {
            return ((Node**) n->d->s->v)[i+1];
        }
    }
    return 0;
}
void concatcharp(Node* n, char* s) {rawpushvrev(n, s, slen(s));}
String* rawnewstr(char* c) {
    String* s = initnode(0, 0, 1);
    // for (int i = 0; i < slen(c); i++) {pushc(s, c[i]);}
    for (int i = 0; i < slen(c); i++) {pushc(s, c[slen(c)-i-1]);}
    return s;
}
Node* getmch(Node* n, char* c) {
    String* s = rawnewstr(c);
    Node* m = getmap(n, s);
    reclaimnode(s);
    return m;
}
Node* newlit(word w) {
    Node* n = initnode(tliteral, 0, 0);
    n->w = w;
    return n;
}
Node* addmap(Node* n, String* k, Node* v) {
    if (!n->d) {n->d = initnode(tarray, 0, 1);}
    pushn(n->d, k);
    pushn(n->d, v);
    return v;
}
Node* addtype(char* c) {
    String* s = newstr(c);
    Node* t = addmap(T, s, initnode(ttype, 0, 0));
    t->name = s;
    return t;
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
    if (n->t && (m = getmch(n->t, "printer"))) {
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
        printnode(((Node**) n->s->v)[i]);
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
Node* addprinter(Node* n, func f) {
    String* s = newstr("printer");
    Node* m = addmap(n, s, initnode(texec, 0, 0));
    m->w = (word) f;
    m->name = s;
    return m;
}
void inittypeprinters() {
    addprinter(ttype, 0);
    addprinter(tliteral, tliteralprinter);
    addprinter(tarray, tarrayprinter);
    addprinter(tmap, tmapprinter);
    addprinter(tstring, tstringprinter);
    addprinter(texec, texecprinter);
}
void initworld() {
    T = initnode(0, 0, 1);
    T->d = initnode(0, 0, 1);
    Node* str = rawnewstr("type");
    Node* t = addmap(T, str, initnode(0, 0, 0));
    t->name = str;
    str = rawnewstr("string");
    Node* n = addmap(T, str, initnode(ttype, 0, 0));
    n->name = str;
    n->t = t;
    ((Node**) T->d->s->v)[0]->t = tstring;
    ((Node**) T->d->s->v)[2]->t = tstring;
    T->d->t = addtype("map");
    addtype("array");
    addtype("exec");
    addtype("literal");

    inittypeprinters();
}
void strtoint(Node* n) {
    word m = 0;
    int b = 10;
    String* s = fromtop(n, 0);
    char* str = (char*) s->s->v;
    word i = s->s->len-1;
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
    Node* l = initnode(tliteral, 0, 0);
    l->w = m;
    pushn(n, l);
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
int containstr(char* s, char c) {
    for (int i = 0; s[i]; i++) {
        if (s[i] == c) {return i;}
    }
    return -1;
}
void findstr(Node* T) {
    Node* b = fromtop(T, 0);
    String* s = fromtop(T, 1);
    char* a = (char*) s->s->v;
    int len = s->s->len;
    word j, i;
    for (i = len-1; i >= 0; i--) {
        if (-1 != (j = containstr((char*) b->s->v, a[i]))) {
            pushn(T, newlit(len-1-i));
            pushn(T, newlit(((char*) b->s->v)[j]));
            return;
        }
    }
    pushn(T, newlit(0));
    pushn(T, newlit(0));
}
void splitstrat(Node* n) {
    word  i   = pulln(n)->w;
    Node* str = fromtop(n, 0);
    String* newst = pushn(n, newstr(""));
    rawpushv(newst, str->s->v+str->s->len-i, i);
}
bool iftop(Node* n) {return fromtop(n, 0)->w;}
void splitfind(Node* n, char* c) {
    pushn(n, newstr(c));
    findstr(n);
    if (iftop(n)) {
        pushi(n, 3);
        pushi(n, 2);
        splitstrat(n);
    }
}
void parse(Node* n) {
    splitfind(n, " \n\t");
    splitfind(n, ":![]()/\"`");
    pushi(n, 2);
    if (!iftop(n)) {
        pulln(n);
    }
}
void parsech(Node* n, char* c) {
    pushn(n, newstr(c));
    parse(n);
}
void mainc() {
    initworld();

    Node* G = initnode(0, 0, 0x100);
    G->name = newstr("G");
    Node* n = initnode(0, 0, 1);
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
    concatcharp(fromtop(G, 0), "hi");
    inttostr(G, 0x123);
    printnodeln(G); puts("\n");
    G->s->len -= sizeof(Node*);
    pushn(G, newlit(2));
    splitstrat(G);
    printnodeln(G);
    printnode(getmch(tarray, "printer"));

    char* str = "a[1 2 3 4]123";
    pushn(G, newstr(str));
    parse(G);
    printnode(G);
}
int main() {mainc();}