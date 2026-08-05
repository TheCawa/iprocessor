#include <ystdlib.h>
#include <yio.h>

int main(void) {
    char *a;
    char *b;
    char *c;

    a = malloc(16);
    b = malloc(16);
    free(a);
    free(b);
    c = malloc(40);
    print_ptr(a);
    putchar(' ');
    print_ptr(c);
    return 0;
}
