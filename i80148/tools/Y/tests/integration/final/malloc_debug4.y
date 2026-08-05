#include <ystdlib.h>
#include <yio.h>

int main(void) {
    char *a;
    char *b;
    a = malloc(8);
    b = malloc(8);
    print_ptr(a);
    putchar(' ');
    print_ptr(b);
    return 0;
}
