#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;
    int *p;
    a = malloc(16);
    if (a == 0) return 1;
    p = a - 4;
    printf("a=%p header=%d\n", a, *p);
    free(a);
    b = malloc(16);
    if (b == 0) return 2;
    printf("b=%p\n", b);
    if (b != a) return 3;
    free(b);
    return 0;
}
