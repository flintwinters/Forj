#include "forj.cpp"

// functions to be executed when bang is called on a type
Maybe<Node*> typefunc(Node* n, Wrap* W) {printf("hi from typefunc\n"); return n;}
Maybe<Node*> nothingfunc(Node* n, Wrap* W) {printf("hi from nothingfunc\n"); return n;}
Maybe<Node*> stringfunc(Node* n, Wrap* W) {printf("hi from stringfunc\n"); return n;}
Maybe<Node*> literalfunc(Node* n, Wrap* W) {return n;}
Maybe<Node*> arrayfunc(Node* n, Wrap* W) {
    W->pull();
    for (int i = 0; i <= n->sp; i++) {
        W->push(n->peek(n->sp-i));
        if (W->peek()->type != tarray) {
            W->peek()->type->f(W->peek(), W);
        }
    }
    return n;
}
Maybe<Node*> bangfunc(Node* n, Wrap* W) {
    if (W->peek()->val-- == 1) {
        W->pull();
        W->peek()->type->f(W->peek(), W);
    }
    return W->peek();
}
Maybe<Node*> execfunc(Node* n, Wrap* W) {return n->f(n, W);}

Maybe<Node*> addnode(Node* n, Wrap* W) {
    W->pull();
    word w = W->pull()->val+W->pull()->val;
    W->push(Node::New("", tliteral, W->t))->val = w;
    return n;
}
Maybe<Node*> BREAKPOINT(Node* n, Wrap* W) {
    W->pull();
    string s = *(string*) W->pull()->val;
    printf("[\033[31mBREAK \033[32m%s:\033[0m%s]\n", s.c_str(), W->t->str().c_str());
    return n;
}
Maybe<Node*> loadfile(Node* n, Wrap* W) {
    W->pull();
    Node* str = W->peek();
    FILE* FP = fopen(((string*) str->val)->c_str(), "r");
    string s;
    while (!feof(FP)) {s += fgetc(FP);}
    char c;
    while ((c = fgetc(FP)) != EOF) {s += c;}
    fclose(FP);
    s = s.substr(0, s.size()-1);
    delete (string*) str->val;
    str->val = (word) new string(s);
    return W->peek();
}
Maybe<Node*> savefile(Node* n, Wrap* W) {
    W->pull();
    string* str = (string*) W->pull()->val;
    FILE* FP = fopen(str->c_str(), "w");
    delete str;
    str = (string*) W->pull()->val;
    fwrite(str->c_str(), str->size(), 1, FP);
    delete str;
    return W->peek();
}
int main(int argc, char** argv) {
    if (argc != 2) {printf("Provide a filename\n"); return 1;}
    FILE* FP = fopen(argv[1], "r");
    string s;
    while (!feof(FP)) {s += fgetc(FP);}
    char c;
    while ((c = fgetc(FP)) != EOF) {s += c;}
    fclose(FP);
    s = s.substr(0, s.size()-1);

    (ttype      = Node::New("type",      0,     0))->f = typefunc;
    (tnothing   = Node::New("nothing",   0,     0))->f = nothingfunc;
    (tstring    = Node::New("string",    ttype, 0))->f = stringfunc;
    (tliteral   = Node::New("literal",   ttype, 0))->f = literalfunc;
    (tarray     = Node::New("array",     ttype, 0))->f = arrayfunc;
    (tbang      = Node::New("bang",      ttype, 0))->f = bangfunc;
    (texec      = Node::New("exec",      ttype, 0))->f = execfunc;
    Node* Global = Node::New("Global", tarray, 0);
    Wrap* W = new Wrap(Global, 0);
    W->t->addvar("+", texec)->f = addnode;
    W->t->addvar("breakpoint", texec)->f = BREAKPOINT;
    W->t->addvar("loadfile", texec)->f = loadfile;
    W->t->addvar("savefile", texec)->f = savefile;
    W->pushscope(W->t->addvar("ok", tliteral));
    (*W->t)["ok"]->val = 3;
    (*W->t)["ok"]->addvar("beeb", tliteral)->addvar("sob", tliteral);
    (*(*W->t)["ok"])["beeb"]->val = 2;
    Text* T = new Text(s, W);
    Maybe<string> m = T->parse();
    while (m && !T->isempty()) {
        m = T->parse();
        if (!m) {printf("%s\n", m.str(s).c_str()); return 1;}
    }
    printf("<%s>\n", W->t->str().c_str());
    Node::deleteall();
    delete T;
    delete W;
}