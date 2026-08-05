#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;
    char *c;

    a = malloc(16);
    b = malloc(16);
    free(a);
    free(b);
    c = malloc(40);
    if (c == 0) return 1;
    if (a == 0) return 5;
    if (b == 0) return 6;
    return 0;
}
