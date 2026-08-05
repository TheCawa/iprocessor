#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;
    char *c;

    a = malloc(16);
    b = malloc(16);
    printf("a=%p b=%p\n", a, b);
    free(a);
    free(b);
    c = malloc(40);
    printf("c=%p\n", c);
    if (c != a) return 100 + (c - a);
    free(c);
    return 0;
}
