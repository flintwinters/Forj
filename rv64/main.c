// https://joe-degs.github.io/systems/2022/06/22/remote-debugging-gdb-qemu.html
int m[10];
void _start() {
    while (1) {
        for (int i = 0; i < 10; i++) {
            m[i] += i;
        }
    }
}

// int main() {
//     printf("test");
// }