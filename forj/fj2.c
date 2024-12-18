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
typedef Node Type;
typedef void (*func)(Node*);
typedef long long word;

// Dynamic array
struct Vect {short len, maxlen; byte v[];};
// The homoiconic data type
struct Node {
    word w;
    Node* p, *name, *d;
    Node* t; // <- array node
    Vect* s;
};
// Pre-allocate a dynamic array of `maxlen` bytes
struct Vect* valloclen(int maxlen) {
    int n = sizeof(struct Vect)+maxlen;
    struct Vect* newv = malloc(n);
    newv->maxlen = maxlen;
    newv->len = 0;
    return newv;
}
// Copy `len` bytes from `src` to `dest`
void cpymem(byte* dest, byte* src, word len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[i];
    }
}
// Copy `len` bytes from `src` to `dest`, mirroring the data
void cpymemrev(byte* dest, byte* src, word len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[len-i-1];
    }
}
// Resize a dynamic array's allocation
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
// Free a node and its associated dynamic array
// Works on both linux and bare riscv, with alloc.c
void reclaimnode(Node* n) {
    #ifdef runrv64
        reclaim(n->s, sizeof(struct Vect)+n->s->maxlen);
        reclaim(n, sizeof(Node));
    #else
        free(n->s);
        free(n);
    #endif
}
// Reclaims only the node, not the array
void reclaimhead(Node* n) {
    #ifdef runrv64
        reclaim(n, sizeof(Node));
    #else
        free(n);
    #endif
}

// Grow the dynamic array to fit `n->s->len`
// Grows by powers of two.
void growtofit(Node* n) {
    word newlen = 1;
    if (!n->s->maxlen) {newlen = n->s->maxlen;}
    while (newlen < n->s->len) {
        newlen = newlen << 1;
    }
    n->s = resize(n->s, newlen);
}
// Resize if the new len is bigger.
void condresize(Node* n, int addlen) {
    n->s->len += addlen;
    if (n->s->len > n->s->maxlen) {
        growtofit(n);
    }
}
// Print the raw data of a dynamic array
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

// Push m onto n's stack
Node* pushn(Node* n, Node* m);
// Allocate a new node
Node* newnode() {return malloc(sizeof(Node));}
// Push raw `dat` to n's stack
// `size` is in bytes
void rawpushv(Node* n, void* dat, word size) {
    condresize(n, size);
    cpymem((char*) n->s->v+n->s->len-size, dat, size);
}
// Push raw, mirrored `dat` to n's stack
// `size` is in bytes
void rawpushvrev(Node* n, void* dat, word size) {
    condresize(n, size);
    cpymemrev((char*) n->s->v+n->s->len-size, dat, size);
}
// Return the ith node from the bottom of n's stack
Node* frombot(Node* n, int i) {return ((Node**) n->s->v)[i];}
// Return the ith node from the top of n's stack
Node* fromtop(Node* n, int i) {return ((Node**) (n->s->v))[n->s->len/sizeof(Node*)-i-1];}
// Return the top of n's stack
Node* top(Node* n) {return fromtop(n, 0);}
// Push one char
void pushc(String* n, char c) {rawpushv(n, &c, sizeof(char));}
// Push raw word
void pushw(Node* n, word w)   {rawpushv(n, &w, sizeof(word));}
// Push m onto n's stack, returns m
Node* pushn(Node* n, Node* m) {rawpushv(n, &m, sizeof(Node*)); return top(n);}
// Bubble the ith element from the top, copying it to the top, returning it.
Node* pushi(Node* n, int i) {return pushn(n, fromtop(n, i));}
// Pull x (Node*)s from n
// returns the last element that was removed
Node* pullx(Node* n, int x) {
    Node* m = fromtop(n, x-1);
    n->s->len -= sizeof(Node*)*x;
    return m;
}
// Pull one Node* from n and return it.
Node* pulln(Node* n) {return pullx(n, 1);}
// Transfer the top element of `src` to `dest`
// returns the element
Node* throw(Node* dest, Node* src) {return pushn(dest, pulln(src));}
// Spill all of `src` onto `dest`
// returns dest
Node* explode(Node* dest, Node* src) {
    rawpushv(dest, &src->s->v, src->s->len);
    return dest;
}
// Returns true if a contains b, false otherwise.
bool contains(Node* a, Node* b) {
    if (!a || !b) {return false;}
    Node** m = (Node**) a->s->v;
    for (int i = 0; i < a->s->len/sizeof(Node*); i++) {
        if (m[i] == b) {return true;} 
    }
    return false;
}

// Returns length of string s
int slen(char* s) {
    int n = 0;
    while (*s++) {n++;}
    return n;
}
// The base type.
// All values inherit from this on some level.
// This inherits from no type.
Node* T = 0;

Node* srch(Node* n, char* c);
#define tmaketype(name) (srch(T, #name))
#define tliteral    tmaketype(literal)
#define tpath       tmaketype(path)
#define tmap        tmaketype(map)
#define tarray      tmaketype(array)
#define tstring     tmaketype(string)
#define texec       tmaketype(exec)
#define tbang       tmaketype(bang)
String* newstr(char* c);
Node* addtype(Node* n, Node* t);
// Create a new node and initialize some of its values.
// Also pre-allocates `size` on its stack.
// If `size` is 0, no stack will be allocated.
Node* initnode(Node* p, Node* t, word size) {
    Node* n = newnode();
    n->w = 0;
    n->p = p;
    n->name = 0;
    n->d = 0;
    if (size) {n->s = valloclen(size);}
    else {n->s = 0;}
    n->t = 0;
    if (t) {addtype(n, t);}
    return n;
}
// Add a type t to n's typelist
Node* addtype(Node* n, Node* t) {
    if (!n->t) {
        n->t = initnode(n, 0, sizeof(Node*));
        n->t->t = initnode(n->t, 0, sizeof(Node*));
        pushn(n->t->t, T);
    }
    return pushn(n->t, t);
}
// Returns true if String* A === char* b
bool strequcharp(String* A, char* b) {
    char* a = (char*) A->s->v;
    word blen = slen(b);
    for (int i = 0; i < min(A->s->len, blen); i++) {
        if (a[i] != b[i]) {return false;}
    }
    return true;
}
// Returns true if String* A === String* b
bool strequ(String* A, String* B) {
    char* a = (char*) A->s->v;
    char* b = (char*) B->s->v;
    for (int i = 0; i < min(A->s->len, B->s->len); i++) {
        if (a[i] != b[i]) {return 0;}
    }
    return 1;
}
// Concatenates char* s onto the String node.
void concatcharp(String* n, char* s) {rawpushvrev(n, s, slen(s));}
// Create a new String* object, with its type unset.
String* rawnewstr(char* c) {
    String* s = initnode(0, 0, 1);
    for (int i = 0; i < slen(c); i++) {pushc(s, c[slen(c)-i-1]);}
    return s;
}

// Gets the variable with name `k` in `n`.
// n must already be a map.  ie: myNode->d.
// It's not a real hashmap, merely a list of ordered pairs,
// (String, Node).  Therefore access is worse at O(n).
Node* getmap(Node* n, String* k) {
    if (!n || !n->s || !n->s->len) {return 0;}
    // Hardcode run-time access to a value's type, by doing myNode:#
    if (strequcharp(k, "#")) {return n->t;}
    Node** m = (Node**) n->s->v;
    for (int i = 0; i < n->s->len/sizeof(Node*); i += 2) {
        if (!m[i]) {continue;}
        if (strequ(m[i], k)) {return m[i+1];}
    }
    return 0;
}
// Recurses getmap over n's containers, and types.
Node* searchvar(Node* n, String* k) {
    if (!n) {return 0;}
    if (strequcharp(k, "#")) {return n->t;}
    Node* m = getmap(n->d, k);
    if (m) {return m;}
    if (n->t) {
        Node** l = (Node**) n->t->s->v;
        for (int i = 0; i < n->t->s->len/sizeof(Node*); i++) {
            if ((m = searchvar(l[i], k))) {return m;} 
        }
    }
    return searchvar(n->p, k);
}
// Applies searchvar but with a char* key
Node* srch(Node* n, char* c) {
    String* s = rawnewstr(c);
    Node* m;
    m = searchvar(n, s);
    reclaimnode(s);
    return m;
}
// Returns a new literal Node
Node* newlit(word w) {
    Node* n = initnode(0, tliteral, 0);
    n->w = w;
    return n;
}
// Adds a variable to a node.  Returns the variable.
Node* addvar(Node* n, String* k, Node* v) {
    if (!n->d) {n->d = initnode(n, tmap, 1);}
    pushn(n->d, k);
    pushn(n->d, v);
    return v;
}
// Initializes a new Node and adds it to n as a variable
Node* addnewvar(Node* n, String* name, Node* t, word size) {
    Node* m = initnode(n, t, size);
    m->name = name;
    addvar(n, name, m);
    return m;
}
// Creates a new string, this time properly initializing the type.
String* newstr(char* c) {
    String* s = rawnewstr(c);
    addtype(s, tstring);
    return s;
}
// Prints a String*
void printstr(String* s) {
    for (int i = 0; i < s->s->len; i++) {
        putchar(s->s->v[s->s->len-i-1]);
    }
}
void printarraylike(Node* n);
// Print a node, according to printing functions provided by its types.
void printnode(Node* n) {
    if (!n) {return;}
    Node* m;
    if (n->t) {
        if ((m = srch(*(Node**) n->t->s->v, "printer"))) {
            if (m->w) {((void (*)(Node*)) m->w)(n);}
            else if (n->name) {YELLOW; printstr(n->name); RESET;}
        }
        else {BLACK; puts(" noprinter "); RESET;}
    }
    else if (n->name) {printstr(n->name);}
    else {YELLOW; puts("#"); RESET; printarraylike(n);}
}
void printnodeln(Node* n) {printnode(n); putchar('\n');}
// Calls printnode on elements in a Node's stack.
// Used by some other printing methods.
void printarraylike(Node* n) {
    if (!n->s) {return;}
    for (int i = 0; i < n->s->len/sizeof(Node*); i++) {
        if (((Node**) n->s->v)[i]) {
            printnode(((Node**) n->s->v)[i]);
        }
        else {BLACK; puts("null"); RESET;}
        if (i < n->s->len/sizeof(Node*)-1) {
            putchar(' ');
        }
    }
}
void ttypeprinter(Node* n) {
    if (!n) {return;}
    if (n->t && n->s) {RED; putchar('('); RESET;}
    YELLOW;
    if (n->name) {printstr(n->name);}
    else {puts("''");}
    RESET;
    
    if (n->t && n->s) {
        BLUE; puts(" <: "); RESET;
        RED; putchar('('); RESET;
        printarraylike(n);
        RED; putchar(')'); RESET;
        RED; putchar(')'); RESET;
    }
}
void tarrayprinter(Node* n) {
    BLUE;
    if (n->name) {printstr(n->name);}
    else {puts("''");}
    if (!n->s->len) {RESET; return;}
    BLUE; puts(":["); RESET;
    printarraylike(n);
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
// Adds function f as a variable to n
Node* addfunc(Node* n, char* c, func f) {
    String* s = newstr(c);
    Node* m = addnewvar(n, s, texec, 0);
    m->w = (word) f;
    return m;
}
// The `!` operator
// P is the execution path, n is the Node being operated on.
void bang(Node* P, Node* n) {
    Node* f;
    if ((f = srch(n, "!"))) {
        ((func) f->w)(P);
    }
}

void texecbang(Node* P) {((func) pulln(top(P))->w)(P);}
void tbangbang(Node* P) {
    top(top(P))->w--;
    if (!pulln(top(P))->w) {
        bang(P, top(top(P)));
    }
}
void tarraybang(Node* P) {
    Node* arr = pulln(top(P));
    for (int i = 0; i < arr->s->len/sizeof(Node*); i++) {
        pushn(top(P), ((Node**) arr->s->v)[i]);
        bang(P, ((Node**) arr->s->v)[i]);
    }
}

// Declare a new type and add it to T
Node* maketype(char* c) {
    String* s = newstr(c);
    Node* t = addnewvar(T, s, T, 0);
    t->p = 0;
    t->name = s;
    pushn(T, t);
    return t;
}
// Bootstrap type tree
void initworld() {
    T = initnode(0, 0, 1);
    T->name = newstr("T");
    T->d = initnode(T, 0, 1);
    T->p = 0;
    Node* str = rawnewstr("string");
    pushn(T->d, str);
    pushn(T, pushn(T->d, initnode(0, T, 0)))->name = str;
    addtype(T->d, maketype("map"));
    addtype(str, tstring);
    addtype(T->name, tstring);
    
    maketype("exec");
    maketype("literal");
    maketype("array");
    maketype("path");
    maketype("bang");

    addfunc(T,   "printer",  ttypeprinter);
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
    Node* l = initnode(0, tliteral, 0);
    if (neg) {m *= -1;}
    l->w = m;
    pushn(n, l);
}
void inttostr(Node* p, word n) {
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
    pushn(p, ss);
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
            reclaimnode(b);
            return;
        }
    }
    pushn(n, newlit(len));
    pushn(n, newlit(0));
    reclaimnode(b);
}
void splitstrat(Node* n, int consume) {
    Node* f = pulln(n);
    word  i = f->w;
    String* str = top(n);
    String* newst = pushn(n, newstr(""));
    rawpushv(newst, str->s->v+str->s->len-i, i);
    str->s->len -= i+consume;
    reclaimhead(f);
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
        if (!v) {puts("Not Found: "); printstr(top(S)); putchar('\n');}
        if (P->w) {pulln(P); P->w = false;}
        pushn(top(P), v);
    }
}
void cleanleadingspace(String* s) {
    while (-1 != containstr(" \n\t", s->s->v[s->s->len-1])) {
        s->s->len--;
    }
}
void parsetok(Node* P, Node* S) {
    // printnodeln(P);
    // printnodeln(S);
    struct Vect* s = top(S)->s;
    while (-1 != containstr(" \n\t", s->v[s->len-1])) {
        s->len--;
    }
    splitfind(S, " \n\t:![]()\"`", 1);
    char c = fromtop(S, 2)->w;
    if (!c) {addtok(P, S); pullx(S, 4); return;}
    if (c == ':') {
        if (P->w) {setsearch(P, getmap(top(P)->d, top(S)));}
        else {P->w = true; pushn(P, searchpath(P, top(S)));}
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
            pushn(top(P), initnode(top(P), tbang, 0))->w = n;
        }
        else {bang(P, top(top(P)));}
    }
    else if (c == '[') {
        if (P->w) {P->w = false;}
        else {pushn(P, initnode(top(P), tarray, 1));}
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
        String* str = initnode(top(P), tstring, 1);
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
    Node* S = initnode(top(P), tarray, 1);
    S->name = newstr("S");
    throw(S, top(P));
    while (frombot(S, 0)->s->len) {parsetok(P, S);}
}
void parsech(Node* n, char* c) {
    Node* P = initnode(n, tpath, 1);
    pushn(P, newstr(c));
    parse(P);
}

void printtest(Node* a) {
    puts("HELLO DOWN THERE!\n");
}
void mainc(char* str) {
    initworld();

    Node* G = initnode(0, tarray, 0x100);
    G->name = newstr("G");

    addnewvar(G, newstr("hello?"), texec, 0)->w = (word) printtest;
    pushn(G, newstr(str));
    addnewvar(G, newstr("arr"), tarray, 1);
    addnewvar(srch(G, "arr"), newstr("arr2"), tarray, 1);
    addnewvar(srch(srch(G, "arr"), "arr2"), newstr("arr3"), tarray, 1);
    addvar(G, newstr("foo"), newstr("bar"));
    addtype(srch(G, "foo"), tarray);
    addvar(fromtop(G->d, 0), newstr("fee"), newstr("ber"));
    Node* P = initnode(0, tpath, 1);
    P->name = newstr("P");
    pushn(P, G);
    parse(P);
    printnodeln(G);
    // printnodeln(T);

    // char c = 0;
    // char buff[0x40];
    // int i = 0;

    // puts("\n<- ");
    // while (1) {
    //     i = 0;
    //     while (c != '\r') {
    //         while (!(c = getchar()));
    //         buff[i++] = putchar(c);
    //     }
    //     if (c) {
    //         puts("\n-> ");
    //         puts(buff);
    //         puts("\n<- ");
    //     }
    //     c = getchar();
    // }
}
int main() {
#ifdef runrv64
    char* str = "       foo:# foo:fee 7 hello? !! ! arr (arr (arr) arr)"
        " \"hello\" arr:arr2 arr:arr2:arr3 arr:arr2:arr3:[1 2 3 4]123"
        " arr:arr2:arr3:[hello?] ! arr:arr2:arr3 !! !";
#else
    #include <stdio.h>
    FILE* fp = fopen("challenge", "r");
    fseek(fp, 0, SEEK_END);
    int i = ftell(fp);
    rewind(fp);
    char str[i];
    i = 0;
    while (!feof(fp)) {str[i] = fgetc(fp); i++;}
    fclose(fp);
#endif
    mainc(str);
}