char* colorcodes[] = {"\033[1;30m",
                      "\033[1;31m",
                      "\033[0;31m",
                      "\033[1;32m",
                      "\033[0;32m",
                      "\033[0;33m",
                      "\033[2;33m",
                      "\033[1;34m",
                      "\033[2;34m",
                      "\033[1;35m",
                      "\033[1;36m",
                      "\033[0;36m",
                      "\033[0m"};

#define BLACK       "\033[1;30m"
#define RED         "\033[1;31m"
#define DARKRED     "\033[0;31m"
#define GREEN       "\033[1;32m"
#define DARKGREEN   "\033[0;32m"
#define YELLOW      "\033[1;33m"
#define DARKYELLOW  "\033[2;33m"
#define BLUE        "\033[1;34m"
#define DARKBLUE    "\033[2;34m"
#define PURPLE      "\033[1;35m"
#define CYAN        "\033[1;36m"
#define DARKCYAN    "\033[0;36m"
#define RESET       "\033[0m"

typedef long long Word;
typedef char byte;
typedef byte bool;
#define true 1
#define false 0

int min(int a, int b) {return (a < b) ? a : b;}
int max(int a, int b) {return (a > b) ? a : b;}
extern int getchar();
extern int putchar(int c);
void printint(unsigned long long n, Word size) {
    if (!n) {putchar('0');}
    size *= 2;
    char m;
    bool started = false;
    for (int i = size-1; i >= 0; i--) {
        m = 0xf & n>>(4*i);
        if (m == 0 && !started) {}
        else {
            started = true;
            if (m < 10) {putchar('0'+m);}
            else {putchar('a'+m-10);}
        }
    }
}
int puts(const char* s) {
    while (*s) {
        const char c = *s;
        putchar(c);
        s++;
    }
    return 0;
}
