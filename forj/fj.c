#define runrv64
#ifdef runrv64
    #include "../rv64/alloc.c"
#else
    #include <stdlib.h>
    #include <stdio.h>
#endif

typedef struct Node Node;
typedef void (*func)(Node*);
typedef long long word;
typedef char byte;
typedef byte bool;

typedef union Ntyp Ntyp;
union Ntyp {struct Node* n; func f; word w; char* s;};
#define Zero ((Ntyp) (Node*) 0)
struct Vect {word len, maxlen; Ntyp v[];};
struct Vect* valloclen(int maxlen) {
    int n = sizeof(struct Vect)+sizeof(word)*maxlen;
    #ifdef runrv64
        struct Vect* newv = alloc(n);
    #else
        struct Vect* newv = malloc(n);
    #endif
    newv->maxlen = maxlen;
    newv->len = 0;
    return newv;
}
void cpymem(byte* dest, byte* src, word len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[i];
    }
}
struct Vect* resize(struct Vect* v, int newmaxlen) {
    struct Vect* newv = valloclen(newmaxlen);
    newv->len = v->len;
    cpymem((byte*) newv->v, (byte*) v->v, sizeof(word)*v->len);
    #ifdef runrv64
        reclaim(v, sizeof(struct Vect)+sizeof(word)*v->maxlen);
    #else
        free(v);
    #endif
    return newv;
}
struct Node {
    Node* t; // type
    Node* d; // vars
    struct Vect* s; // stack
};
Node* allocnode() {
    #ifdef runrv64
        Node* n = alloc(sizeof(Node));
    #else
        Node* n = malloc(sizeof(Node));
    #endif
    n->t = 0;
    n->d = 0;
    return n;
}
Node* newnode() {
    Node* n = allocnode();
    n->s = valloclen(0);
    return n;
}
Node* newnodeallocn(int nn) {
    Node* n = allocnode();
    n->s = valloclen(nn);
    return n;
}
Node* resizeif(Node* n, word need, word newlen) {
    if (need >= n->s->maxlen) {
        n->s = resize(n->s, newlen);
    }
    return n;
}
Ntyp rawpush(Node* n, Ntyp w) {
    return n->s->v[n->s->len++] = w;
}
Ntyp push(Node* n, Ntyp w) {
    resizeif(n, n->s->len+1, 1+n->s->maxlen*2);
    return rawpush(n, w);
}
Ntyp pushn(Node* n, byte* bytes, word len) {
    resizeif(n, n->s->len+len, n->s->maxlen+len);
    cpymem((byte*) (n->s->v+n->s->len), bytes, len);
    return n->s->v[(n->s->len += len)-1];
}
Ntyp pull(Node* n) {
    if (n->s->len-1 < 0) {n->s = 0; return Zero;}
    return n->s->v[--n->s->len];
}
Ntyp pulln(Node* n, word len) {
    if (n->s->len-len < 0) {n->s = 0; return Zero;}
    return n->s->v[n->s->len -= len];
}
Ntyp peek(Node* n, word i) {
    if (n->s->len <= i) {n->s = 0; return Zero;}
    return n->s->v[n->s->len-i-1];
}
Ntyp pushi(Node* n, word i) {
    resizeif(n, n->s->len+1, 1+n->s->maxlen*2);
    return push(n, peek(n, i));
}
Ntyp dump(Node* n, Node* m) {
    push(n, (Ntyp) m);
    return pushn(n, (byte*) m->s->v, sizeof(word)*m->s->len);
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

int slen(char* s) {
    int n = 0;
    while (*s++) {n++;}
    return n;
}
void cpymemrev(byte* dest, byte* src, word len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[len-i-1];
    }
}
void pushstr(Node* n, char* s) {
    word len = (slen(s)+1)/sizeof(word)+1;
    resizeif(n, n->s->len+len, n->s->maxlen+len);
    // cpymem((char*) (n->s->v+n->s->len), s, len*sizeof(word));
    word lens = slen(s);
    for (int i = 0; i < lens; i++) {
        ((char*) (n->s->v+n->s->len))[i] = s[lens-i-2];
    }
    n->s->len += len;
}
void concatstr(Node* n, char* s) {
    word nlen = slen((char*) n->s->v);
    word strlen = slen(s);
    resizeif(n, (nlen+strlen)/sizeof(word)+1, n->s->maxlen+strlen);
    cpymem(((char*) n->s->v)+nlen, s, slen(s));
    n->s->len = (nlen+strlen)/sizeof(word)+1;
}
Node* newstr(char* s) {
    Node* new = newnode();
    pushstr(new, s);
    return new;
}
bool strequ(Node* A, Node* B) {
    char* a = (char*) A->s->v;
    char* b = (char*) B->s->v;
    while (*a == *b) {
        a++; b++;
        if (!*a && !*b) {return 1;}
    }
    return 0;
}
int strmatch(Node* A, Node* B) {
    char* a = (char*) A->s->v;
    char* b = (char*) B->s->v;
    int i = 0;
    while (*a && *b && *a == *b) {
        a++; b++; i++;
    }
    return i;
}
void strtoint(Node* S) {
    word n = 0;
    int b = 10;
    Node* s = peek(S, 0).n;
    char* str = (char*) s->s->v;
    word i = slen(str)-1;
    if (str[i] == '0') {
        if (str[i-1] == 'x') {b = 0x10; i -= 2;}
        if (str[i-1] == 'b') {b = 2; i -= 2;}
    }
    while (i >= 0) {
        n *= b;
        if (str[i] >= 'a' && str[i] <= 'f') {n += str[i]-'a';}
        else {n += str[i]-'0';}
        i--;
    }
    push(S, (Ntyp) n);
}
void inttostr(Node* S, word n) {
    int b = 0x10;
    Node* s = push(S, (Ntyp) newstr("")).n;
    while (n) {
        char c[2] = {"0123456789abcdef"[n%0x10], '\0'};
        concatstr(s, c);
        n /= b;
    }
    concatstr(s, "\0");
}
#ifdef runrv64
void puts(char* s) {
    while (*s) {
        const char c = *s;
        putchar(c);
        s++;
    }
}
void putsrev(char* s) {
    int i = slen(s);
    while (i) {
        putchar(s[i-1]);
        i--;
    }
}
void printarr(Node* n) {
    if (!n) {puts("print 0\n"); return;}
    for (int i = 0; i < n->s->len; i++) {
        puts("x");
        inttostr(n, n->s->v[i].w);
        putsrev((char*) (pull(n).n->s->v));
        puts(" ");
    }
    puts("|\n");
}
#else
void printarr(Node* n) {
    if (!n) {printf("print 0\n"); return;}
    for (int i = 0; i < n->s->len; i++) {
        printf("x%llx ", n->s->v[i]);
    }
    printf("|\n");
}
#endif
Node* getmap(Node* map, Node* s) {
    if (!map->d->s->len) {return 0;}
    for (int i = 0; i < map->d->s->len; i++) {
        if (strequ(map->d->s->v[i].n, s)) {
            return map->d->s->v[i+1].n;
        }
    }
    return 0;
}
void addmap(Node* map, Node* key, Node* val) {
    if (!map->d) {map->d = newnode();}
    push(map->d, (Ntyp) key);
    push(map->d, (Ntyp) val);
}
int containstr(char* s, char c) {
    int i = -1;
    while (*s) {i++; if (*s == c) {break;} s++;}
    return i;
}
void findstr(Node* S) {
    Node* b = peek(S, 0).n;
    char* a = (char*) (peek(S, 1).n)->s->v;
    word j, i;
    for (i = 0; a[i]; i++) {
        if (-1 != (j = containstr((char*) b->s->v, a[i]))) {
            push(S, (Ntyp) i);
            push(S, (Ntyp) (word) ((char*) b->s->v)[j]);
            return;
        }
    }
    push(S, Zero);
    push(S, Zero);
}
void execif(Node* T) {
    func f = pull(T).f;
    if (pull(T).w) {f(T);}
}
void splitstrat(Node* T) {
    word  i   = peek(T, 0).w;
    Node* str = peek(T, 1).n;
    Node* newst = push(T, (Ntyp) newnodeallocn(i)).n;
    pushn(newst, (byte*) str->s->v, i+1);
}

void mainc() {
    puts("abcd");
    char* str = " [1 2 3 4]";
    Node* T = newnode();
    Node* E = newnode();
    push(T, (Ntyp) newstr(str));
    // for (int i = 0; s->s->len; i++) {
    //     if (-1 != containstr(" \n\t", ((char*) s->s->v)[i])) {
    //         push(T, (Ntyp) (word) i);
    //         splitstrat(T);
    //         break;
    //     }
    // }
    // push(T, (Ntyp) newstr(" \n\t"));
    // push(T, (Ntyp) newstr(":![]()/\"`"));
    // printarr(T);
    // findstr(T);
    // pushi(T, 3);
    // pushi(T, 2);
    // splitstrat(T);
    push(T, (Ntyp) newstr("0x123456"));
    strtoint(T);

    reclaimnode(T);
}
int main() {mainc();}
