// #include "../rv64/alloc.c"
#include <stdlib.h>

typedef long long word;
typedef char byte;

struct Vect {int len, maxlen; word v[];};
word getmemsize(struct Vect* v) {
    return sizeof(struct Vect)+v->maxlen;
}
struct Vect* valloclen(int maxlen) {
    // struct Vect* newv = alloc(sizeof(struct Vect)+sizeof(word)*maxlen);
    struct Vect* newv = malloc(sizeof(struct Vect)+sizeof(word)*maxlen);
    newv->maxlen = maxlen;
    newv->len = 0;
    return newv;
}
struct Vect* resize(struct Vect* v, int newmaxlen) {
    struct Vect* newv = valloclen(newmaxlen);
    newv->len = v->len;
    for (int i = 0; i < v->len; i++) {
        newv->v[i] = v->v[i];
    }
    // reclaim(v, sizeof(struct Vect)+v->maxlen);
    free(v);
    return newv;
}
typedef struct Node Node;
struct Node {
    Node* d; // metadata
    struct Vect* s; // stack
};
word push(Node* n, word w) {
    if (n->s->len+1 > n->s->maxlen) {
        n->s = resize(n->s, n->s->maxlen*2);
    }
    return n->s->v[n->s->len++] = w;
}
word pull(Node* n, word w) {
    if (n->s->len-1 < 0) {n->s = 0; return 0;}
    return n->s->v[--n->s->len];
}
word peek(Node* n) {
    if (n->s->len == 0) {n->s = 0; return 0;}
    return n->s->v[n->s->len-1];
}

void mainc() {
    char* str =
""
"";
    Node n;
    n.s = valloclen(0x2);
    for (int i = 0; i < 0x20; i++) {push(&n, i);}
    free(n.s);
}
int main() {mainc();}
