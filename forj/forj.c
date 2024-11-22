#include <stdlib.h>
#include <stdio.h>
typedef long long word;
typedef struct Node Node;
typedef Node* (Exec)(Node*);
enum nodedata {Type, Name};
enum valtype {vnull, vnode, vexec, vcharp, vallocated, vword};
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
    enum valtype vty;
    word sp;
};

struct Memtrack {
    struct Memtrack* p, *n;
    void* m;
};
struct Memtrack* M = 0;
int count = 0;
void* newmem(void* n) {
    struct Memtrack* m = M;
    M = malloc(sizeof(struct Memtrack));
    M->n = m;
    M->p = 0;
    if (m) {m->p = M;}
    M->m = n;
    count += 1;
    return n;
}
struct Memtrack* yankmem(struct Memtrack* m) {
    if (m->p) {m->p->n = m->n;}
    if (m->n) {m->n->p = m->p;}
    return m;
}
void freenode(Node* f) {
    if (f->vty == vallocated) {free(f->v.charp);}

}
void freeall() {
    struct Memtrack* m = M;
    while (M) {
        m = M;
        M = M->n;
        freenode(m->m);
        free(m->m);
        free(m);
    }
}


Node* tunit;
Node* ttype;
Node* tliteral;
Node* texec;
Node* tarray;
Node* tstring;
Node* newnode() {
    Node* n = newmem(malloc(sizeof(Node)));
    n->sp = -1;
    n->mem = M;
    n->t = 0;
    n->b = 0;
    n->d = 0;
    n->v.n = 0;
    n->vty = vnull;
    return n;
}
Node* copynode(Node* n) {
    Node* m = newnode();
    *m = *n;
    return m;
}
Node* pushbot(Node* s, Node* n) {
    if (!s) {return 0;}
    s->sp++;
    if (n) {
        n->p = 0;
        n->n = s->b;
    }
    else {n = tunit;}
    if (s->b) {s->b->p = n;}
    else {s->t = n;}
    return s->b = n;
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
Node* pushref(Node* s, Node* r) {
    s = pushnew(s);
    s->v.n = r;
    s->vty = vnode;
    return r;
}
Node* pushcopy(Node* s, Node* r) {return push(s, (r) ? copynode(r) : 0);}
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
    return n;
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
    if (!a) {return s;}
    a = a->b;
    while (a) {
        pushcopy(s, a);
        a = a->n;
    }
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

Node* initnode(Node* type, Node* name) {
    Node* n = newnode();
    n->d = newnode();
    n->d->o = n;
    if (type) {pushref(n->d, type);}
    else {pushref(n->d, tunit);}
    pushcopy(n->d, name);
    return n;
}
char* node2charp(Node* s) {
    if (!s->b) {return "";}
    int size = 0;
    Node* n = s->b;
    while (n) {
        if (n->vty == vcharp) {size += n->sp+1;}
        else if (n->vty == vword) {size++;}
        n = n->n;
    }
    char* c = malloc(sizeof(char)*(size+1));
    Node* cmem = newnode();
    cmem->v.charp = c;
    cmem->vty = vallocated;
    n = s->b;
    int j;
    int i = 0;
    while (n) {
        if (n->vty == vcharp) {
            if (n->v.charp) {
                for (j = 0; n->v.charp[j]; j++) {
                    c[i] = n->v.charp[j];
                    i++;
                }   
            }
        }
        else if (n->vty == vword) {
            c[i] = (char) n->v.v;
            i++;
        }
        n = n->n;
    }
    c[i] = 0;
    return c;
}
Node* makestr(Node* n) {
    Node* s = initnode(tstring, tunit);
    s->v.charp = 0;
    s->vty = vcharp;
    char* c = (n->v.charp) ? n->v.charp : n->b->v.charp;
    while (*c) {
        n = pushnew(s);
        n->v.v = *c;
        n->vty = vword;
        c++;
    }
    return s;
}
Node* makecharp(char* s) {
    Node* m = initnode(tstring, 0);
    Node* n = initnode(tstring, 0);
    push(m, n);
    m->v.charp = 0;
    m->vty = vcharp;
    int l = -1;
    while (s[l+1]) {l++;}
    n->v.charp = s;
    n->vty = vcharp;
    n->sp = l;
    return m;
}
Node* strcompress(Node* s) {
    Node* c = makecharp(node2charp(s));
    c->vty = vallocated;
    Node* m, *n = s->b;
    while (n) {
        m = n;
        n = n->n;
    }
    return c;
}
#define RESET  "\e[0m"
#define GRAY   "\e[90m"
#define RED    "\e[31m"
#define GREEN  "\e[32m"
#define YELLOW "\e[33m"
#define BLUE   "\e[34m"
#define PURPLE "\e[35m"
#define CYAN   "\e[36m"

Node* inttostr(Node* n) {
    int v = n->v.v;
    if (!v) {return makecharp("0");}
    Node* l, *m = initnode(tstring, 0);
    char c;
    while (v) {
        l = pushbot(m, initnode(tliteral, 0));
        l->v.v = '0' + (v%0x10 > 9)*('a'-'0'-10) + v%0x10;
        l->vty = vword;
        v /= 0x10;
    }
    pushbot(m, makecharp("0x")->b);
    return strcompress(m);
}
Node* prettynode(Node* s, int depth) {
    if (!s->d) {return makecharp(GRAY"|"RESET);}
    if (!getdata(s, Name)) {return makecharp("no name");}
    Node* ty = getdata(s, Type)->v.n;
    if (!ty) {return makecharp(YELLOW"unit"RESET);}
    Node* str = makecharp("");
    if (depth) {
        append(str, makecharp("\n"));
        for (int i = 0; i < depth; i++) {
            append(str, makecharp("\t"));
        }
    }
    if (ty == tstring) {
        append(str, makecharp(CYAN"\""));
        append(str, s);
        append(str, makecharp("\" "RESET));
        return strcompress(str);
    }
    append(str, makecharp(BLUE"("RESET));
    if (ty == ttype || ty == tunit) {append(str, makecharp(YELLOW));}
    append(str, getdata(s, Name));
    if (ty == ttype || ty == tunit) {append(str, makecharp(RESET));}
    if (!depth) {
        append(str, makecharp(GREEN" <: "RESET));
        append(str, prettynode(ty, 0));
    }
    append(str, makecharp(BLUE")"RESET));
    if (ty == tliteral) {
        append(str, makecharp(":"PURPLE));
        append(str, inttostr(s));
        append(str, makecharp(RESET));
        return strcompress(str);
    }
    if (ty == tarray) {
        append(str, makecharp(":"YELLOW"{"RESET));
        Node* d = s->d->b->n->n;
        while (d) {
            append(str, prettynode(d, depth+1));
            d = d->n;
        }
        if (!depth) {append(str, makecharp("\n"));}
        append(str, makecharp(YELLOW"}"RESET));
        append(str, makecharp(RED"["RESET));
        d = s->b;
        while (d) {
            append(str, prettynode(d, depth+1));
            d = d->n;
        }
        if (s->b) {
            append(str, makecharp("\n"));
            if (depth) {
                for (int i = 0; i < depth; i++) {
                    append(str, makecharp("\t"));
                }
            }
        }
        append(str, makecharp(RED"]"RESET));
    }
    return strcompress(str);
}
void printname(Node* n) {
    char* str = node2charp(getdata(n, Name));
    printf("%s\n", str);
}
void printnode(Node* n) {
    struct Memtrack* m = M;
    n = prettynode(n, 0);
    char* c = node2charp(n);
    printf("%s\n", c);
}
Node* getvar(Node* s, Node* str) {
    Node* a = frombot(s->d, 2);
    while (a) {
        if (equal(getdata(a, Name), str)) {return a;}
        a = a->n;
    }
    return 0;
}
Node* parse(Node* str) {
    return str;
}

int main() {
    FILE* FP = fopen("challenge", "r");
    fseek(FP, 0, SEEK_END);
    int size = ftell(FP);
    char ftxt[size+1];
    rewind(FP);
    char* ftxtp = ftxt;
    while (!feof(FP)) {*ftxtp++ = fgetc(FP);}
    *ftxtp = 0;
    fclose(FP);

    tunit        = initnode(0      , makecharp("nothing"));
    ttype        = initnode(tunit  , makecharp("type"));
    tliteral     = initnode(ttype  , makecharp("literal"));
    texec        = initnode(ttype  , makecharp("exec"));
    tarray       = initnode(ttype  , makecharp("array"));
    tstring      = initnode(tarray , makecharp("string"));
    Node* global = initnode(tarray , makecharp("global"));
    Node* prog = push(global, makecharp(ftxt));

    Node* str = initnode(tarray, makecharp("ok"));
    printnode(str);
    printnode(str);

    Node* wow = initnode(tliteral, makestr(makecharp("wow")));
    wow->v.v = 4;
    printnode(wow);
    
    Node* l = pushcopy(global->d, wow);
    pushcopy(global->d, wow);
    pushcopy(global->d, wow);
    wow->v.v = 0x12345;
    pushcopy(global->d, wow);
    pushcopy(getvar(global, makecharp("wow")), wow);
    pushcopy(global->d, wow);
    push(global, newnode());
    push(global, newnode());
    push(global, newnode());
    printnode(global);

    pushcopy(global, makecharp("hello"));
    pushcopy(global, makecharp("el"));
    Node* a = findidx(global->t->p, global->t);
    pushcopy(global, concat(global->t, global->t->p));
    char* s = node2charp(global->t);
    printf("%s\n", s);
    printf("%d\n", count);
    freeall();
}