#include <ystdlib.h>
#include <yio.h>

int main(void) {
    char *a;
    char *b;
    char *c;
    char *k;

    k = 0x12345678;
    a = malloc(16);
    b = malloc(16);
    free(a);
    free(b);
    c = malloc(40);
    print_ptr(k);
    return 0;
}
