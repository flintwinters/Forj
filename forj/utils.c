#define BLACK   puts("\033[1;30m");
#define RED     puts("\033[1;31m");
#define DARKRED puts("\033[0;31m");
#define GREEN   puts("\033[1;32m");
#define DARKGREEN puts("\033[2;32m");
#define YELLOW  puts("\033[1;33m");
#define DARKYELLOW puts("\033[2;33m");
#define BLUE    puts("\033[1;34m");
#define PURPLE  puts("\033[1;35m");
#define CYAN    puts("\033[1;36m");
#define RESET   puts("\033[0m");

typedef long long word;
typedef char byte;
typedef byte bool;
#define true 1
#define false 0

int min(int a, int b) {return (a < b) ? a : b;}
int max(int a, int b) {return (a > b) ? a : b;}
extern int getchar();
extern int putchar(int c);
void printint(unsigned long long n, word size) {
    size *= 2;
    char m;
    bool started = false;
    putchar('x');
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
