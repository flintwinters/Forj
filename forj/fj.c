#define runrv64
#ifdef runrv64
    #include "../rv64/alloc.c"
#else
    #include <stdlib.h>
    #include <stdio.h>
#endif

typedef struct Node Node;
typedef long long word;
typedef char byte;
typedef byte bool;

struct Vect {word len, maxlen; word v[];};
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
Node* newnode() {
    #ifdef runrv64
        Node* n = alloc(sizeof(Node));
    #else
        Node* n = malloc(sizeof(Node));
    #endif
    n->t = 0;
    n->d = 0;
    n->s = valloclen(0);
    return n;
}
Node* resizeif(Node* n, word need, word newlen) {
    if (need >= n->s->maxlen) {
        n->s = resize(n->s, newlen);
    }
    return n;
}
word rawpush(Node* n, word w) {
    return n->s->v[n->s->len++] = w;
}
word push(Node* n, word w) {
    resizeif(n, n->s->len+1, 1+n->s->maxlen*2);
    return rawpush(n, w);
}
word pushn(Node* n, byte* bytes, word len) {
    resizeif(n, n->s->len+len, n->s->maxlen+len);
    cpymem((byte*) (n->s->v+n->s->len), bytes, len);
    return n->s->v[(n->s->len += len)-1];
}
word pull(Node* n) {
    if (n->s->len-1 < 0) {n->s = 0; return 0;}
    return n->s->v[--n->s->len];
}
word pulln(Node* n, word len) {
    if (n->s->len-len < 0) {n->s = 0; return 0;}
    return n->s->v[n->s->len -= len];
}
word peek(Node* n, int i) {
    if (n->s->len <= i) {n->s = 0; return 0;}
    return n->s->v[n->s->len-i-1];
}
word dump(Node* n, Node* m) {
    push(n, (word) m);
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
void pushstr(Node* n, char* s) {
    word len = (slen(s)+1)/sizeof(word)+1;
    resizeif(n, n->s->len+len, n->s->maxlen+len);
    cpymem((char*) (n->s->v+n->s->len), s, len*sizeof(word));
    n->s->len += len;
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
void findstr(Node* S) {
    char* a = (char*) ((Node*) peek(S, 0))->s->v;
    Node* b = (Node*) peek(S, 1);
    int j, i;
    for (i = 0; *a; i++, a++) {
        for (j = 0; j < b->s->len; j++) {
            if (*a == ((char*) b->s->v)[j]) {
                push(S, ((char*) b->s->v)[j]);
                push(S, i); return;
            }
        }
    }
    push(S, 0);
    push(S, 0);
}
#ifndef runrv64
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
        if (strequ((Node*) map->d->s->v[i], s)) {
            return (Node*) map->d->s->v[i+1];
        }
    }
    return 0;
}
void addmap(Node* map, Node* key, Node* val) {
    if (!map->d) {map->d = newnode();}
    push(map->d, (word) key);
    push(map->d, (word) val);
}

void mainc() {
    char* str =
"[1 2 3 4]"
"";
    Node* T = newnode();
    push(T, (word) newstr(str));
    addmap(T, (Node*) peek(T, 0), T);
    push(T, (word) newstr("56781234"));
    findstr(T);

    reclaimnode(T);
}
int main() {mainc();}
