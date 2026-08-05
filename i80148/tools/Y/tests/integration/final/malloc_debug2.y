#include <ystdlib.h>
#include <yio.h>

int main(void) {
    char *a;
    char *b;
    int *pa;
    int *pb;

    a = malloc(16);
    b = malloc(16);
    print_ptr(a);
    putchar(' ');
    print_ptr(b);
    putchar(' ');
    pa = a;
    pb = b;
    pa = pa - 1;
    pb = pb - 1;
    print_hex(*pa);
    putchar(' ');
    print_hex(*pb);
    return 0;
}
