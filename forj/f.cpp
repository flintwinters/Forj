#include <stdio.h>
#include <map>
#include <string>
#include <vector>
using namespace std;
typedef long long word;

template <typename T> class Maybe : public string {
public:
    T val;
    int i = -1;
    bool fail;
    Maybe(T val): val(val), fail(false) {}
    Maybe(string s, int i): string(s), i(i), fail(true) {}
    T operator->() {return val;}
    operator bool() {return fail;}
    string str() {return "<" + to_string(i) + ">: " + (*this);}
    string str(string s) {
        s.insert(i-1, "\033[31;1;4m!>>>>>");
        return "\033[33;1;1m<" + to_string(i) + ">: " + (*this) + "\033[0m\n" + s + "\033[0m";
    }
};
template <typename T> Maybe<T> Fail(string msg, int i = 0) {return Maybe<T>(msg, i);}
template <typename T> Maybe<T> Success(T val) {return Maybe<T>(val);}


template <typename T> class Stack {
private:
    vector<T> s;
public:
    int sp = -1;
    Stack() {}
    T pull() {return s[sp--];}
    T push(T t) {
        sp++;
        if (sp >= s.size()) {s.push_back(t);}
        else {s[sp] = t;}
        return t;
    }
    T offset(int i) {return s[sp-i];}
    bool isempty() {return sp<=-1;}
};

class Text : public Stack<Text*>, public string {
public:
    Text(string s): string(s) {}
    Text* split(int i, int n) {
        push(new Text(substr(i+n)));
        push(new Text(substr(0, i)));
        return offset(0);
    }
    Maybe<int> find(string match) {
        if (!size()) {return Fail<int>("Couldn't find in empty string");}
        int i = 0;
        for (; match.find((*this)[i]) != string::npos; i++) {
            if (i >= size()-1) {
                return Fail<int>("Couldn't find any of '" + match + "' in: '" + *this + "'");
            }
            i++;
        }
        return Success(i);
    }
};

class Node : public map<string, Node*>, public Stack<Node*> {
public:
    Node* p;
    word v;
    Node() {}
    bool in(string s) {return find(s) == end();}
    Node* global(string s) {
        if (in(s)) {return (*this)[s];}
        return p->global(s);
    }
};

int main() {
    printf("Test\n");
    Node G;
    G["hi"] = new Node();
    Text t("hello");
    printf("%s\n", t.c_str());
    t.split(1, 0);
    printf("%s\n", t.c_str());
    printf("%s\n", t.offset(0)->c_str());
    printf("%s\n", t.offset(1)->c_str());
}