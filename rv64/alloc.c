#include "../forj/utils.c"

struct Block {
    // next, prev
    struct Block* n;
    word size;
};
#define FREES ((struct Block*) (0x87fff000)+1)

extern void initheap() {
    struct Block* frees = FREES;
    frees->n = frees-1;
    frees->n->size = 0x10000;
    frees->size = 0;
    frees->n->n = 0;
}
void setmem(char* bytes, int i, char n) {
    for (int j = 0; j < i; j++) {
        bytes[j] = n;
    }
}
word roundblock(word a) {
    word b = sizeof(struct Block);
    b = (a/b)*b;
    if (b == a) {return a;}
    return b+sizeof(struct Block);
}
struct Block* splitblock(struct Block* b, int a) {
    struct Block* new = ((struct Block*) ((char*) b-a));
    new->n = b->n;
    new->size = b->size-a;
    return new;
}
extern void* malloc(unsigned long a) {
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
    return (void*) roundblock((word) ((char*) (f+1)-a));
}
extern void reclaim(void* b_, word a) {
    struct Block* frees = FREES;
    struct Block* f = frees->n;
    a = roundblock(a);
    struct Block* b = (struct Block*) ((char*) b_+a)-1;
    if (f >= b) {
        while (f->n >= b) {f = f->n;}
        if (((char*) f)-f->size == ((char*) b)) {
            f->size += a;
            while (((char*) f)-f->size <= (char*) f->n) {
                f->size += f->n->size;
                f->n = f->n->n;
            }
            return;
        }
        b->n = f->n;
        f->n = b;
    }
    else {
        if (b == frees-1) {b->n = f;}
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

char printnumchar(int n) {
    char m = 0xf & n;
    if (m == 0) {putchar('.');}
    else if (m < 10) {putchar('0'+m);}
    else {putchar('a'+m-10);}
    return m;
}
void printblock(struct Block* b) {
    putchar(' ');
    for (int j = 15; j >= 0; j--) {
        printnumchar(((char*) b)[j]>>4);
        printnumchar(((char*) b)[j]);
        if (j%4 == 0) {putchar(' ');}
    }
    putchar('\n');
}

void printheap() {
    struct Block* b = FREES;
    BLACK;
    printint((word) (b-1), 8);
    printblock(b);
    RESET;
    struct Block* d = FREES-1;
    bool free = false;
    while (b->n) {
        while (d != b->n) {
            YELLOW;
            printint((word) (d), 8);
            RESET;
            printblock(d);
            d--;
        }
        BLACK;
        if (d->n) {
            for (int i = sizeof(struct Block); i < b->n->size; i += sizeof(struct Block)) {
                printint((word) (d), 8);
                printblock(d);
                d--;
            }
        }
        printint((word) (d), 8);
        printblock(d);
        RESET;
        d--;
        b = b->n;
    }
}