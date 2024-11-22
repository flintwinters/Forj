#include <stdlib.h>
#include <stdio.h>
typedef long long word;
typedef struct Node Node;
enum nodedata {Type, Name};
enum valtype {Nodep, Charp, Literal};
union val {
    Node* n;
    char* charp;
    word v;
};
struct Node {
    // prev, next, bottom, top, data, owner
    Node* p, *n, *b, *t, *d, *o;
    struct Memtrack* mem;
    union val v;
    enum valtype vtype;
    word sp;
};

struct Memtrack {
    struct Memtrack* p, *n;
    void* m;
};
struct Memtrack* M = 0;
void* newmem(void* n) {
    struct Memtrack* m = M;
    M = malloc(sizeof(struct Memtrack));
    M->p = m;
    M->m = n;
    return n;
}
Node* newnode() {
    Node* n = newmem(malloc(sizeof(Node)));
    n->sp = -1;
    n->mem = M;
    return n;
}
void freenode(Node* f) {
    struct Memtrack* m = f->mem;
    if (m->p) {m->p->n = m->n;}
    if (m->n) {m->n->p = m->p;}
    free(m);
    free(f);
}
void freechildren(Node* f) {
    Node* m, *n = f->b;
    while (n) {
        m = n;
        n = n->n;
        free(m);
    }
    free(f);
}
void freeall() {
    struct Memtrack* m = M;
    while (M) {
        m = M;
        M = M->n;
        free(m->m);
        free(m);
    }
}

Node* copynode(Node* n) {
    Node* m = newnode();
    *m = *n;
    return m;
}
void connect(Node* p, Node* n) {
    if (p) {p->n = n;}
    if (n) {n->p = p;}
}
Node* push(Node* s, Node* n) {
    s->sp++;
    connect(s->t, n);
    if (s->sp == 0) {s->b = n;}
    return s->t = n;
}
Node* pushnew(Node* s) {
    Node* n = push(s, newnode());
    n->o = s;
    return n;
}
Node* pull(Node* s) {
    if (s->sp == -1) {return 0;}
    s->sp--;
    Node* n = s->t;
    s->t = s->t->p;
    s->t->n = 0;
    return n;
}
Node* frombot(Node* s, int i) {
    if (!s) {return 0;}
    s = s->b;
    while (i) {
        if (!s) {return 0;}
        s = s->n; i--;
    }
    return s;
}
Node* fromtop(Node* s, int i) {
    if (!s) {return 0;}
    s = s->t;
    while (i) {
        if (s) {return 0;}
        s = s->p; i--;
    }
    return s;
}
Node* get(Node* s, int i) {
    if (i < s->sp/2) {return frombot(s, i);}
    return fromtop(s, i);
}
Node* ripout(Node* n) {
    if (n->p) {n->p->n = n->n;}
    if (n->n) {n->n->p = n->p;}
    return n;
}
Node* replaceat(Node* s, Node* n, int i) {
    Node* m = get(s, i);
    if (m->p) {m->p->n = n;}
    if (m->n) {m->n->p = n;}
    n->p = m->p;
    n->n = m->n;
    return m;
}
Node* getdata(Node* s, enum nodedata i) {return frombot(s->d, i);}
Node* makestr(Node* n) {
    Node* s = newnode();
    char* c = n->v.charp;
    while (*c) {
        pushnew(s)->v.v = *c;
        c++;
    }
    return s;
}
char* str2charp(Node* s) {
    char* c = malloc(sizeof(char)*s->sp);
    Node* n = s->b;
    int i = 0;
    while (n) {
        c[i] = (char) n->v.v;
        n = n->n; i++;
    }
    return c;
}
Node* concat(Node* a, Node* b) {
    Node* s = newnode();
    while (a) {push(s, copynode(a)); a = a->n;}
    while (b) {push(s, copynode(b)); b = b->n;}
    return s;
}
Node* append(Node* s, Node* a) {
    a = a->b;
    while (a) {push(s, a); a = a->n;}
    return s;
}
Node* findidx(Node* a, Node* b) {
    Node* as = a->b;
    Node* bs;
    Node* af;
    while (as) {
        if (as->v.v == b->b->v.v) {
            af = as;
            bs = b->b;
            while (as->v.v == bs->v.v) {
                as = as->n;
                bs = bs->n;
                if (!as) {return 0;}
                if (!bs) {return af;}
            }
        }
        else {as = as->n;}
    }
    return 0;
}
Node* breakopen(Node* s) {return append(s, s->t);}
Node* initnode(Node* type, Node* name) {
    Node* n = newnode();
    n->d = newnode();
    push(n->d, type);
    push(n->d, name);
    return n;
}
void printnode(Node* s) {
    char* str = str2charp(getdata(s, Name));
    printf("%s\n", str);
    free(str);
}
Node* makecharp(char* s) {
    Node* n = newnode();
    n->v.charp = s;
    return makestr(n);
}


int main() {
    Node* type  = initnode(0    , makecharp("type"));
    Node* array = initnode(type , makecharp("array"));
    Node* global= initnode(array, makecharp("global"));
    printnode(global);

    Node* p = newnode();
    Node* n = newnode();
    push(p, n);
    pushnew(p);
    Node* str = pushnew(p);
    Node* str2 = pushnew(p);
    str->v.charp = "hello";
    str2->v.charp = "el";
    push(p, makestr(str));
    push(p, makestr(str2));
    Node* a = findidx(p->t->p, p->t);
    push(p, concat(p->t->b, p->t->p->b));
    char* s = str2charp(p->t);
    printf("%s\n", s);
    free(s);
    freeall();
}