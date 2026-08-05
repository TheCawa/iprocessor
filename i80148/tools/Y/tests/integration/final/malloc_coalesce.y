#include <ystdlib.h>

extern int _free_list;

int main(void) {
    char *a;
    char *b;
    char *c;
    int *h;

    a = malloc(16);
    if (a == 0) return 1;
    h = a - 4;
    printf("a=%p a.h=%d\n", a, *h);
    b = malloc(16);
    if (b == 0) return 2;
    free(a);
    printf("after free a: list=%p a.h=%d\n", _free_list, *h);
    free(b);
    printf("after free b: list=%p a.h=%d\n", _free_list, *h);
    c = malloc(40);
    if (c == 0) return 3;
    printf("c=%p\n", c);
    if (c != a) return 100 + (c - a);
    free(c);
    return 0;
}
