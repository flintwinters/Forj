#include "forj.cpp"

template <typename T> string Maybe<T>::str(string s) {
    int idx = 0;
    vector<int> lines;
    lines.push_back(idx);
    while (idx <= i && s.find("\n", idx) != npos) {
        lines.push_back(idx);
        idx = s.find("\n", idx)+1;
    }
    int escapelen = string("\033[31;1;4m!>>>>>").length();
    if (len) {s.insert(i+len, "\033[0m"); escapelen += string("\033[0m").length();}
    s.insert(max(0, i), "\033[31;1;4m!>>>>>");
    return
        "\033[33;1m<" + to_string(lines.size()) + ":" + to_string(i) + ">:" +
        (*this) + "\033[0m\n" + s.substr(lines[max((int) lines.size()-5, 0)], i+len+escapelen) + "\033[0m";
}

template <typename T> string Stack<T>::str(string (tostr)(T)) {
    string s = " ";
    for (int i = 0; i <= sp; i++) {s += tostr(peek(i)) + " ";}
    return s;
}

string Node::str() {
    return Stack::str([](Node* n)->string {
        if (n->gettype() == tliteral) {return "\033[90;1m" + to_string(n->val) + "\033[0m";}
        if (n->gettype() == texec)    {return "\033[31m" + to_string(n->val) + "\033[0m";}
        if (n->gettype() == tstring)  {return "\033[32m\"" + *(string*) n->val + "\"\033[0m";}
        if (n->isempty()) {return "[]";}
        if (n->gettype() == tarray) {return " ["+n->str()+"] ";}
        return "["+n->str()+"]";
    });
}

// functions to be executed when bang is called on a type
Maybe<Node*> typefunc(Node* n, Wrap* W) {printf("hi from typefunc\n"); return n;}
Maybe<Node*> nothingfunc(Node* n, Wrap* W) {printf("hi from nothingfunc\n"); return n;}
Maybe<Node*> stringfunc(Node* n, Wrap* W) {
    string s = *(string*) W->pull()->val;
    printf("%s\n", s.c_str());
    return n;
}
Maybe<Node*> literalfunc(Node* n, Wrap* W) {return n;}
Maybe<Node*> bangfunc(Node* n, Wrap* W) {
    if (W->peek()->val-- == 1) {
        W->pull();
        W->peek()->gettype()->f(W->peek(), W);
    }
    return n;
}
Maybe<Node*> execfunc(Node* n, Wrap* W) {return n->f(n, W);}
Maybe<Node*> arrayfunc(Node* n, Wrap* W) {
    W->pull();
    Node* m = W->peek();
    for (int i = 0; i <= n->sp; i++) {
        W->push(n->peek(n->sp-i));
        if (W->peek()->gettype() != tarray) {
            W->peek()->gettype()->f(W->peek(), W);
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
    if (W->peek()->gettype() == tstring) {
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
    word a = W->pull()->val;
    W->t->swapi(a);
    return n;
}
Maybe<Node*> execif(Node* n, Wrap* W) {
    W->pull();
    if (W->peek(1)->val != 0) {
        n = W->pull();
        n->gettype()->f(n, W);
    }
    else {W->pull(); W->pull();}
    return n;
}
Maybe<Node*> assign(Node* n, Wrap* W) {
    W->pull();
    string* s = (string*) W->pull()->val;
    W->peek(0)->name = *s;
    (*W->t)[*s] = W->pull();
    return n;
}
Maybe<Node*> link(Node* n, Wrap* W) {
    W->pull();
    W->peek(1)->val = (word) W->peek(0);
    W->pull(); W->pull();
    return n;
}
Maybe<Node*> lunk(Node* n, Wrap* W) {
    W->pull();
    Node* u = W->pull();
    W->push((Node*) u->val);
    return n;
}
Maybe<Node*> setval(Node* n, Wrap* W) {
    W->pull();
    W->peek(1)->val = W->peek(0)->val;
    W->pull();
    return n;
}
Maybe<Node*> clone(Node* n, Wrap* W) {
    W->pull();
    W->push(new Node(*W->pull()));
    return n;
}
Maybe<Node*> stringconcat(Node* n, Wrap* W) {
    W->pull();
    string* a = (string*) W->pull()->val;
    string* b = (string*) W->peek()->val;
    W->peek()->val = (word) new string(*b+*a);
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
Maybe<Node*> runsystem(Node* n, Wrap* W) {
    W->pull();
    string* str = (string*) W->pull()->val;
    system(str->c_str());
    return n;
}
Maybe<Node*> fjpull(Node* n, Wrap* W) {
    W->pull();
    W->pull();
    return n;
}
Maybe<Node*> fjpush(Node* n, Wrap* W) {
    W->pull();
    W->push(W->peek(W->pull()->val));
    return n;
}
Maybe<Node*> entype(Node* n, Wrap* W) {
    W->pull();
    W->peek(1)->implicit[0] = W->peek(0);
    W->pull(); W->pull();
    return n;
}
int errorexit(Maybe<string> m, string s) {
    printf("%s\n", m.str(s).c_str());
    return 1;
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

    Node* Global = Node::New("Global", tarray, 0);
    Wrap* W = new Wrap(Global, 0);

    (ttype      = W->t->addvar("type",      0))->f = typefunc;
    (tnothing   = W->t->addvar("nothing",   ttype))->f = nothingfunc;
    (tstring    = W->t->addvar("string",    ttype))->f = stringfunc;
    (tliteral   = W->t->addvar("literal",   ttype))->f = literalfunc;
    (tarray     = W->t->addvar("array",     ttype))->f = arrayfunc;
    (tbang      = W->t->addvar("bang",      ttype))->f = bangfunc;
    (texec      = W->t->addvar("exec",      ttype))->f = execfunc;
    W->t->addvar("breakpoint",  texec)->f = BREAKPOINT;
    W->t->addvar("loadfile",    texec)->f = loadfile;
    W->t->addvar("savefile",    texec)->f = savefile;
    W->t->addvar("appendfile",  texec)->f = appendfile;
    W->t->addvar("splitat",     texec)->f = splitat;
    W->t->addvar("swap",        texec)->f = swapnode;
    W->t->addvar("concat",      texec)->f = stringconcat;
    W->t->addvar("include",     texec)->f = includefile;
    W->t->addvar("entype",      texec)->f = entype;
    W->t->addvar("?",           texec)->f = execif;
    W->t->addvar("assign",      texec)->f = assign;
    W->t->addvar("<link",       texec)->f = link;
    W->t->addvar(">lunk",       texec)->f = lunk;
    W->t->addvar("set",         texec)->f = setval;
    W->t->addvar("clone",       texec)->f = clone;
    W->t->addvar("+",           texec)->f = addnode;
    tliteral->addvar("+", texec)->f = addnode;
    W->t->addvar("system",      texec)->f = runsystem;
    W->t->addvar("pull",        texec)->f = fjpull;
    W->t->addvar("push",        texec)->f = fjpush;

    Text* T = new Text(s, W);
    Maybe<string> m = T->parse();
    while (m && !T->isempty()) {
        m = T->parse();
        if (!m) {
            return errorexit(m, s);
        }
    }
    printf("\033[0m%s\n", W->t->str().c_str());
    Node::deleteall();
    delete T;
    Wrap* w = W;
    while (W) {
        w = w->prev;
        delete w;
        W = w;
    }
}