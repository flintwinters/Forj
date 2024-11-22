#include <stdlib.h>
#include <stdio.h>
typedef long long word;
typedef struct Node Node;
typedef Node* (Exec)(Node*, Node*);
enum nodedata {Type, Name};
union val {
    Node* n;
    Exec* e;
    char* charp;
    word v;
};
struct Node {
    // prev, next, bottom, top, data, owner
    Node* p, *n, *b, *t, *d, *o;
    struct Memtrack* mem;
    union val v;
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
    if (m) {m->n = M;}
    M->m = n;
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


Node* tunit;
Node* ttype;
Node* tarray;
Node* tstring;
Node* newnode() {
    Node* n = newmem(malloc(sizeof(Node)));
    n->sp = -1;
    n->mem = M;
    return n;
}
Node* copynode(Node* n) {
    Node* m = newnode();
    *m = *n;
    return m;
}
Node* push(Node* s, Node* n) {
    if (!s) {return 0;}
    s->sp++;
    if (n) {
        n->n = 0;
        n->p = s->t;
    }
    else {n = tunit;}
    if (s->t) {s->t->n = n;}
    else {s->b = n;}
    return s->t = n;
}
Node* pushnew(Node* s) {
    Node* n = push(s, newnode());
    n->o = s;
    return n;
}
Node* pushcopy(Node* s, Node* r) {
    return push(s, (r) ? copynode(r) : 0);
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
    if (i > s->sp) {return 0;}
    s = s->b;
    while (i) {
        if (!s) {return 0;}
        s = s->n; i--;
    }
    return s;
}
Node* fromtop(Node* s, int i) {
    if (!s) {return 0;}
    if (i > s->sp) {return 0;}
    s = s->t;
    while (i) {
        if (s) {return 0;}
        s = s->p; i--;
    }
    return s;
}
Node* ripout(Node* n) {
    if (n->p) {n->p->n = n->n;}
    if (n->n) {n->n->p = n->p;}
    return n;
}
Node* replaceat(Node* s, Node* n, int i) {
    Node* m = frombot(s, i);
    if (m->p) {m->p->n = n;}
    if (m->n) {m->n->p = n;}
    n->p = m->p;
    n->n = m->n;
    return m;
}
Node* getdata(Node* s, enum nodedata i) {return frombot(s->d, i);}
Node* concat(Node* a, Node* b) {
    a = a->b; b = b->b;
    Node* s = newnode();
    while (a) {pushcopy(s, a); a = a->n;}
    while (b) {pushcopy(s, b); b = b->n;}
    return s;
}
Node* append(Node* s, Node* a) {
    a = a->b;
    while (a) {pushcopy(s, a); a = a->n;}
    return s;
}
int equal(Node* a, Node* b) {
    a = a->b; b = b->b;
    while (a && b && a->v.v == b->v.v) {
        a = a->n;
        b = b->n;
    }
    return a == b && b == 0;
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
    if (type) {pushcopy(n->d, type);}
    else {pushcopy(n->d, tunit);}
    pushcopy(n->d, name);
    return n;
}
Node* makestr(Node* n) {
    Node* s = initnode(tstring, tunit);
    char* c = n->v.charp;
    while (*c) {
        pushnew(s)->v.v = *c;
        c++;
    }
    return s;
}
char* str2charp(Node* s) {
    char* c = malloc(sizeof(char)*s->sp+1);
    Node* n = s->b;
    int i = 0;
    while (n) {
        c[i] = (char) n->v.v;
        n = n->n; i++;
    }
    c[i] = 0;
    return c;
}
void printnode(Node* s) {
    if (!s->d) {printf("no data\n"); return;}
    if (!getdata(s, Name)) {printf("no name\n"); return;}
    char* str = str2charp(getdata(s, Name));
    printf("%s\n", str);
    free(str);
}
Node* makecharp(char* s) {
    Node* n = newnode();
    n->v.charp = s;
    return makestr(n);
}
Node* getvar(Node* s, Node* str) {
    Node* a = frombot(s->d, 2);
    while (a) {
        if (equal(getdata(a, Name), str)) {return a;}
        a = a->n;
    }
    return 0;
}

int main() {
    tunit        = initnode(0      , makecharp("nothing"));
    ttype        = initnode(tunit  , makecharp("type"));
    tarray       = initnode(ttype  , makecharp("array"));
    tstring      = initnode(tarray , makecharp("string"));
    Node* global = initnode(tarray , makecharp("global"));
    printnode(global);
    printnode(tarray);

    Node* wow = initnode(tarray, makecharp("wow"));
    pushcopy(ttype->d, wow);
    
    Node* l = pushcopy(global->d, wow);
    printf("0==%d\n", l == getvar(global, makecharp("wow1")));
    printf("1==%d\n", l == getvar(global, makecharp("wow")));

    pushcopy(global, makecharp("hello"));
    pushcopy(global, makecharp("el"));
    Node* a = findidx(global->t->p, global->t);
    pushcopy(global, concat(global->t, global->t->p));
    char* s = str2charp(global->t);
    printf("%s\n", s);
    free(s);
    freeall();
}