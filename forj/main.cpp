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
    for (int i = sp; i >= 0; i--) {
        s += tostr(peek(i)) + " ";
    }
    return s;
}

string Node::str() {
    return Stack::str([](Node* n)->string {
        if (n->type == tcontext) {return "{"+n->str()+"}";}
        if (n->type == tliteral) {return "\033[90;1m" + to_string(n->val) + "\033[0m";}
        if (n->type == texec)    {return "\033[31m" + n->name + "\033[0m";}
        if (n->type == tstring)  {
            if (!n->val) {return "\033[32m0\033[0m";}
            return "\033[32m\"" + *(string*) n->val + "\"\033[0m";
        }
        if (n->type == tbang)    {
            string s = "!";
            for (int i = 0; i < n->val; i++) {
                s += '!';
            }
            return "\033[33;1m" + s + "\033[0m";}
        if (n->isempty()) {return "\033[34m" + n->name + ":" + "[]\033[0m";}
        if (n->type == tarray) {return " ["+n->str()+"] ";}
        return "["+n->str()+"]";
    });
}

// functions to be executed when bang is called on a type
int typefunc(Wrap* W) {
    W->pull();
    W->peek()->type = W->pull();
    return 1;
}
int contextfunc(Wrap* W) {
    W->pull();
    if (!W->push((Node*) W->pull()->val)->exec(W)) {return 0;}
    return 1;
}
int nothingfunc(Wrap* W) {W->pull(1); return 1;}
int stringfunc(Wrap* W) {
    W->pull(); 
    return 1;
}
int literalfunc(Wrap* W) {W->pull(); return 1;}
int bangfunc(Wrap* W) {
    W->pull();
    Node* m = W->peek();
    if (m->val == 1) {
        W->pull();
        if (!W->peek()->exec(W)) {return 0;};
    }
    else {
        W->push(new Node("", tbang))->val = W->pull()->val-1;;
    }
    return 1;
}
int execfunc(Wrap* W) {return W->peek()->f(W);}
int arrayfunc(Wrap* W) {
    Node* n = W->pull(); 
    W->pull();
    for (int i = 0; i <= n->sp; i++) {
        W->push(n->peek(n->sp-i));
        if (W->peek()->type == tbang) {
            if (!W->peek()->exec(W)) {return 1;}
        }
    }
    return 1;
}
int fjmap(Wrap* W) {
    W->pull(2);
    Node* f = W->pull();
    Node* n = W->pull();
    W = W->pushscope(new Node("", tarray));
    for (int i = 0; i <= n->sp; i++) {
        W->push(n->peek(n->sp-i));
        if (!W->push(f)->exec(W)) {return 0;}
    }
    n = W->t;
    (W = W->pullscope())->push(n);
    return 1;
}
int fjapply(Wrap* W) {
    W->pull(2);
    Node* b = W->pull();
    Node* a = W->pull();
    // for (int i = b->sp; i >= 0; i--) {
    for (int i = 0; i <= b->sp; i++) {
        W->push(a->peek(a->sp-i));
        if (!W->push(b->peek(b->sp-i))->exec(W)) {return 0;};
    }
    return 1;
}
int fjrun(Wrap* W) {
    W->pull(2);
    Node* n = W->pull(); 
    for (int i = 0; i <= n->sp; i++) {
        W->push(n->peek(n->sp-i));
        if (W->peek()->type != tarray) {
            if (!W->peek()->exec(W)) {return 0;}
        }
    }
    return 1;
}
int fjin(Wrap* W) {
    W->pull(2);
    Node* f = W->pull();
    W = W->pushscope(W->pull());
    if (!W->push(f)->exec(W)) {return 0;}
    return 1;
}
int fjlength(Wrap* W) {
    W->pull(2);
    word len = W->pull()->sp+1;
    W->push(new Node("", tliteral))->val = len;
    return 1;
}
int fjadd(Wrap* W) {
    W->pull(2);
    word w = W->pull()->val+W->pull()->val;
    W->push(new Node("", tliteral))->val = w;
    return 1;
}
int fjsub(Wrap* W) {
    W->pull(2);
    word w = W->peek(1)->val-W->peek(0)->val;
    W->pull(2);
    W->push(new Node("", tliteral))->val = w;
    return 1;
}
int fjmul(Wrap* W) {
    W->pull(2);
    word w = W->pull()->val*W->pull()->val;
    W->push(new Node("", tliteral))->val = w;
    return 1;
}
int fjdiv(Wrap* W) {
    W->pull(2);
    word w = W->peek(1)->val/W->peek(0)->val;
    W->pull(2);
    W->push(new Node("", tliteral))->val = w;
    return 1;
}
int fjnot(Wrap* W) {
    W->pull(2);
    word w = W->pull()->val;
    W->push(new Node("", tliteral))->val = !w;
    return 1;
}
int fjand(Wrap* W) {
    W->pull(2);
    word w = W->peek(1)->val&&W->peek(0)->val;
    W->pull(2);
    W->push(new Node("", tliteral))->val = w;
    return 1;
}
int fjor(Wrap* W) {
    W->pull(2);
    word w = W->peek(1)->val||W->peek(0)->val;
    W->pull(2);
    W->push(new Node("", tliteral))->val = w;
    return 1;
}
int fjequal(Wrap* W) {
    W->pull(2);
    word w = W->peek(1)->val==W->peek(0)->val;
    W->pull(2);
    W->push(new Node("", tliteral))->val = w;
    return 1;
}
int BREAKPOINT(Wrap* W) {
    W->pull(2);
    if (W->t->isempty()) {
        printf("[\033[31mBREAK\033[35;1m\033[0m]\n");
        return 1;
    }
    if (W->peek()->type == tstring) {
        string s = *(string*) W->pull()->val;
        printf("[\033[31mBREAK \033[35;1m%s\033[0m:%s]\n", s.c_str(), W->t->str().c_str());
        return 1;
    }
    printf("[\033[31mBREAK \033[0m:%s]\n", W->t->str().c_str());
    return 1;
}
int fjexit(Wrap* W) {
    BREAKPOINT(W);
    return 0;
}
int DEBUG(Wrap* W) {
    W->pull(2);
    W->debugging = !W->debugging;
    return 1;
}
int loadfile(Wrap* W) {
    W->pull(2);
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
    return 1;
}
int savefile(Wrap* W) {
    W->pull(2);
    string* str = (string*) W->pull()->val;
    FILE* FP = fopen(str->c_str(), "w");
    delete str;
    str = (string*) W->pull()->val;
    fwrite(str->c_str(), str->size(), 1, FP);
    delete str;
    return 1;
}
int appendfile(Wrap* W) {
    W->pull(2); 
    string* str = (string*) W->pull()->val;
    FILE* FP = fopen(str->c_str(), "a");
    delete str;
    str = (string*) W->pull()->val;
    fwrite(str->c_str(), str->size(), 1, FP);
    delete str;
    return 1;
}
int splitat(Wrap* W) {
    W->pull(2); 
    word w = W->pull()->val;
    string* str = (string*) W->peek()->val;
    W->push(new Node("", tstring))->val = (word) new string(str->substr(w));
    W->peek(1)->val = (word) new string(str->substr(0, w));
    delete str;
    return 1;
}
int swapnode(Wrap* W) {
    W->pull(2); 
    word a = W->pull()->val;
    W->t->swapi(a);
    return 1;
}
int execif(Wrap* W) {
    W->pull(2);
    if (W->peek(1)->val != 0) {
        if (!W->pull()->exec(W)) {return 0;}
    }
    else {W->pull(2);}
    return 1;
}
int equals(Wrap* W) {
    W->pull(2);
    W->push(new Node("", tliteral))->val = W->pull()->val == W->pull()->val;
    return 1;
}
int isempty(Wrap* W) {
    W->pull(2);
    Node* n = new Node("", tliteral);
    n->val = W->pull()->isempty();
    W->push(n);
    return 1;
}
int assign(Wrap* W) {
    W->pull(2);
    string* s = (string*) W->pull()->val;
    W->peek(0)->name = *s;
    (*W->t)[*s] = W->pull();
    return 1;
}
int declare(Wrap* W) {
    W->pull(2);
    string* s = (string*) W->pull()->val;
    W->push(W->t->addvar(*s, tnothing));
    return 1;
}
int printfunc(Wrap* W) {
    W->pull(2);
    string s = *(string*) W->pull()->val;
    printf("%s\n", s.c_str());
    return 1;
}
int link(Wrap* W) {
    W->pull(2);
    W->peek(1)->val = (word) W->peek(0);
    W->pull(2);
    return 1;
}
int lunk(Wrap* W) {
    W->pull(2); 
    Node* u = W->pull();
    W->push((Node*) u->val);
    return 1;
}
int setval(Wrap* W) {
    W->pull(2);
    W->peek(1)->val = W->peek(0)->val;
    W->pull();
    return 1;
}
int replicate(Wrap* W) {
    W->pull(2);
    word w = W->pull()->val;
    Node* n = W->pull();
    (W = W->pushscope(new Node("", tarray)));
    for (int i = 0; i < w; i++) {
        W->push(new Node(*n));
    }
    n = W->t;
    (W = W->pullscope())->push(n);
    return 1;
}
int stringconcat(Wrap* W) {
    W->pull(2); 
    string* a = (string*) W->pull()->val;
    string* b = (string*) W->peek()->val;
    W->peek()->val = (word) new string(*b+*a);
    delete a;
    delete b;
    return 1;
}
int includefile(Wrap* W) {
    W->pull(2); 
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
    return 1;
}
int runsystem(Wrap* W) {
    W->pull(2);
    string* str = (string*) W->pull()->val;
    system(str->c_str());
    return 1;
}
int fjpull(Wrap* W) {
    W->pull(3); 
    return 1;
}
int fjpush(Wrap* W) {
    W->pull(2); 
    W->push(W->peek(W->pull()->val));
    return 1;
}
int detype(Wrap* W) {
    W->pull(2);
    W->push(W->pull()->type);
    return 1;
}
int errorexit(Maybe<string> m, string s) {
    printf("%s\n", m.str(s).c_str());
    return 1;
}
#define STRING(f) #f
func addvar(Wrap* W, func f, string s = "", Node* t = texec) {
    if (s == "") {s = STRING(f);}
    W->t->addvar(s, t)->f = f;
    return f;
}
int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Provide a filename\n");
        return 1;
    }
    FILE* FP = fopen(argv[1], "r");
    string s;
    while (!feof(FP)) {s += fgetc(FP);}
    char c;
    while ((c = fgetc(FP)) != EOF) {s += c;}
    fclose(FP);
    s = s.substr(0, s.size()-1);

    Node* Global = new Node("Global", tarray);
    Wrap* W = new Wrap(Global, 0);

    (ttype      = W->t->addvar("type",      0))->f = typefunc;
    (tcontext   = W->t->addvar("context",     0))->f = arrayfunc;
    (tnothing   = W->t->addvar("nothing",   ttype))->f = nothingfunc;
    (tstring    = W->t->addvar("string",    ttype))->f = stringfunc;
    (tliteral   = W->t->addvar("literal",   ttype))->f = literalfunc;
    (tarray     = W->t->addvar("array",     ttype))->f = arrayfunc;
    (tbang      = W->t->addvar("bang",      ttype))->f = bangfunc;
    (texec      = W->t->addvar("exec",      ttype))->f = execfunc;

    addvar(W, BREAKPOINT,   "breakpoint");
    addvar(W, DEBUG,        "debug");
    addvar(W, loadfile);        
    addvar(W, savefile);        
    addvar(W, appendfile);      
    addvar(W, splitat);     
    addvar(W, swapnode,     "swap");
    addvar(W, stringconcat, "concat");
    addvar(W, includefile,  "include");
    addvar(W, detype);      
    addvar(W, execif,       "?");
    addvar(W, equals,       "==");
    addvar(W, fjnot,        "not");
    addvar(W, isempty);     
    addvar(W, declare);     
    addvar(W, assign);      
    addvar(W, fjin,         "in");
    addvar(W, fjlength,     "length");
    addvar(W, printfunc,    "print");
    addvar(W, link,         "<-");
    addvar(W, lunk,         "->");
    addvar(W, setval,       "=");
    addvar(W, replicate);       
    addvar(W, runsystem,    "system");
    addvar(W, fjpull,       "pull");
    addvar(W, fjpush,       "push");
    addvar(W, fjmap,        "map");
    addvar(W, fjapply,      "apply");
    addvar(W, fjrun,        "run");
    addvar(W, fjexit,       "exit");
    tliteral->addvar("+", texec)->f = addvar(W, fjadd, "+");
    tliteral->addvar("-", texec)->f = addvar(W, fjsub, "-");
    tliteral->addvar("*", texec)->f = addvar(W, fjmul, "*");
    tliteral->addvar("/", texec)->f = addvar(W, fjdiv, "/");

    tliteral->addvar("==", texec)->f  = addvar(W, fjequal, "==");
    tliteral->addvar("not", texec)->f = addvar(W, fjnot, "not");
    tliteral->addvar("and", texec)->f = addvar(W, fjand, "and");
    tliteral->addvar("or", texec)->f  = addvar(W, fjor, "or");

    Text* T = new Text(s, W);
    Maybe<string> m = T->parse();
    while (m && !T->isempty()) {
        m = T->parse();
        if (!m) {return errorexit(m, s);}
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