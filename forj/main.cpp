#include "forj.cpp"
#include <cstdio>

template <typename T> string Maybe<T>::str(string s) {
    int idx = 0;
    vector<int> lines;
    while (idx != npos) {
        lines.push_back(idx);
        if (idx > i) {break;}
        idx = s.find("\n", idx)+1;
    }
    if (len) {s.insert(idx, "\033[0m");}
    s.insert(idx-len-1, "\033[31;1;4m!>>>>>");
    return
        "\033[33;1m<" + to_string(lines.size()) + ":" + to_string(idx) + ">:" +
        (*this) + "\033[0m\n" + s.substr(lines[max((int) lines.size()-5, 0)], idx) + "\033[0m";
}

template <typename T> string Stack<T>::str(string (tostr)(T)) {
    string s = " ";
    for (int i = 0; i <= sp; i++) {s += tostr(peek(i)) + " ";}
    return s;
}

string Node::str() {
    return Stack::str([](Node* n)->string{
        if (n->type == tliteral) {return "\033[90;1m" + to_string(n->val) + "\033[0m";}
        if (n->type == texec)    {return "\033[31m" + to_string(n->val) + "\033[0m";}
        if (n->type == tstring)  {return "\033[32m\"" + *(string*) n->val + "\"\033[0m";}
        if (n->isempty()) {return "[]";}
        if (n->type == tarray) {return " ["+n->str()+"] ";}
        return "["+to_string(n->val)+"]";
    });
}

// functions to be executed when bang is called on a type
Maybe<Node*> typefunc(Node* n, Wrap* W) {printf("hi from typefunc\n"); return n;}
Maybe<Node*> nothingfunc(Node* n, Wrap* W) {printf("hi from nothingfunc\n"); return n;}
Maybe<Node*> stringfunc(Node* n, Wrap* W) {printf("hi from stringfunc\n"); return n;}
Maybe<Node*> literalfunc(Node* n, Wrap* W) {return n;}
Maybe<Node*> bangfunc(Node* n, Wrap* W) {
    if (W->peek()->val-- == 1) {
        W->pull();
        W->peek()->type->f(W->peek(), W);
    }
    return n;
}
Maybe<Node*> execfunc(Node* n, Wrap* W) {return n->f(n, W);}
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
Maybe<Node*> addnode(Node* n, Wrap* W) {
    W->pull();
    word w = W->pull()->val+W->pull()->val;
    W->push(Node::New("", tliteral, W->t))->val = w;
    return n;
}
Maybe<Node*> BREAKPOINT(Node* n, Wrap* W) {
    W->pull();
    if (W->peek()->type == tstring) {
        string s = *(string*) W->pull()->val;
        printf("[\033[31mBREAK \033[35;1m%s\033[0m:%s]\n", s.c_str(), W->t->str().c_str());
        return n;
    }
    printf("[\033[31mBREAK \033[0m:%s]\n", W->t->str().c_str());
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
    return n;
}
Maybe<Node*> savefile(Node* n, Wrap* W) {
    W->pull();
    string* str = (string*) W->pull()->val;
    FILE* FP = fopen(str->c_str(), "w");
    delete str;
    str = (string*) W->pull()->val;
    fwrite(str->c_str(), str->size(), 1, FP);
    delete str;
    return n;
}
Maybe<Node*> appendfile(Node* n, Wrap* W) {
    W->pull();
    string* str = (string*) W->pull()->val;
    FILE* FP = fopen(str->c_str(), "a");
    delete str;
    str = (string*) W->pull()->val;
    fwrite(str->c_str(), str->size(), 1, FP);
    delete str;
    return n;
}
Maybe<Node*> splitat(Node* n, Wrap* W) {
    W->pull();
    word w = W->pull()->val;
    string* str = (string*) W->peek()->val;
    W->push(Node::New("", tstring, 0))->val = (word) new string(str->substr(w));
    W->peek(1)->val = (word) new string(str->substr(0, w));
    delete str;
    return n;
}
Maybe<Node*> swapnode(Node* n, Wrap* W) {
    W->pull();
    Node* a = W->pull();
    Node* b = W->pull();
    W->push(a); W->push(b);
    return n;
}
Maybe<Node*> execif(Node* n, Wrap* W) {
    W->pull();
    if (W->peek(1)->val != 0) {
        Node* n = W->pull();
        n->type->f(n, W);
        return W->pull();
    }
    W->pull();
    return n;
}
Maybe<Node*> dupnode(Node* n, Wrap* W) {
    W->pull();
    W->push(W->peek());
    return n;
}
Maybe<Node*> stringconcat(Node* n, Wrap* W) {
    W->pull();
    string* a = (string*) W->pull()->val;
    string* b = (string*) W->peek()->val;
    W->peek()->val = (word) new string(*a+*b);
    delete a;
    delete b;
    return n;
}
Maybe<Node*> includefile(Node* n, Wrap* W) {
    W->pull();
    Node* str = W->pull();
    FILE* FP = fopen(((*(string*) str->val)+".qfj").c_str(), "r");
    string s;
    while (!feof(FP)) {s += fgetc(FP);}
    char c;
    while ((c = fgetc(FP)) != EOF) {s += c;}
    fclose(FP);
    s = s.substr(0, s.size()-1);
    
    Node* nn = W->t->addvar(*(string*) str->val, ttype);
    Wrap* w = W->pushscope(nn);
    Text* T = new Text(s, w);
    Maybe<string> m = T->parse();
    while (m && !T->isempty()) {
        m = T->parse();
        if (!m) {return Fail<Node*>(m);}
    }
    return n;
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
    W->t->addvar("breakpoint",  texec)->f = BREAKPOINT;
    W->t->addvar("loadfile",    texec)->f = loadfile;
    W->t->addvar("savefile",    texec)->f = savefile;
    W->t->addvar("appendfile",  texec)->f = appendfile;
    W->t->addvar("splitat",     texec)->f = splitat;
    W->t->addvar("swap",        texec)->f = swapnode;
    W->t->addvar("concat",      texec)->f = stringconcat;
    W->t->addvar("include",     texec)->f = includefile;
    W->t->addvar("?",           texec)->f = execif;
    W->t->addvar(".",           texec)->f = dupnode;
    W->t->addvar("+",           texec)->f = addnode;
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
    printf("\033[0m%s\n", W->t->str().c_str());
    Node::deleteall();
    delete T;
    delete W;
}