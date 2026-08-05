#include <yio.h>

int main(void) {
    char *a;
    a = 0x12345678;
    putchar(' ');
    print_ptr(a);
    return 0;
}
