typedef long long word;
struct Block {
    // next, prev
    struct Block* n;
    int size;
};
#define FREES ((struct Block*) (0x87fff000)+1)
extern void getchar();
extern void putchar(int c);
extern void initheap() {
    struct Block* frees = FREES;
    frees->n = frees-1;
    frees->n->size = 0x8000;
    frees->size = 0;
    frees->n->n = 0;
}
// assuming b->size > a
int min(int a, int b) {return (a < b) ? a : b;}
int max(int a, int b) {return (a > b) ? a : b;}
int roundblock(int a) {
    int b = sizeof(struct Block);
    b = a/b*b;
    if (b == a) {return a;}
    return b+sizeof(struct Block);
}
struct Block* splitblock(struct Block* b, int a) {
    struct Block* new = ((struct Block*) ((char*) b-a));
    new->n = b->n;
    new->size = b->size-a;
    return new;
}
extern void* alloc(int a) {
    struct Block* frees = FREES;
    struct Block* f = frees->n;
    struct Block* prev = frees;
    a = roundblock(a);
    while (f && a > f->size) {
        prev = f;
        f = f->n;
    }
    if (!f) {return 0;}
    if (f->size != a) {
        prev->n = splitblock(f, a);
    }
    else {prev->n = f->n;}
    f->n = 0;
    return (char*) (f+1)-a;
}
extern void reclaim(void* b_, int a) {
    struct Block* frees = FREES;
    struct Block* b = (struct Block*) ((char*) b_+a)-1;
    struct Block* f = frees->n;
    a = roundblock(a);
    if (f >= b) {
        while (f->n >= b) {f = f->n;}
        if (((char*) f)-f->size == ((char*) b)) {
            f->size += a;
            while (((char*) f)-f->size == (char*) f->n) {
                f->size += f->n->size;
                f->n = f->n->n;
            }
            return;
        }
        b->n = f->n;
        f->n = b;
    }
    else {
        if (b == frees-1) {b->n = f->n;}
        else {b->n = frees->n;}
        frees->n = b;
    }
    b->size = a;
    while (((char*) b)-b->size == ((char*) f)) {
        b->size += f->size;
        b->n = f->n;
        f = f->n;
    }
}