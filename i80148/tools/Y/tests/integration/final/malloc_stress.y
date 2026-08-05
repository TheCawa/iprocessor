#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;
    char *c;
    char *d;

    /* Allocate two blocks. */
    a = malloc(16);
    if (a == 0) return 1;
    b = malloc(16);
    if (b == 0) return 2;
    if (a == b) return 3;

    /* Free first, reallocate same size -> should reuse a. */
    free(a);
    c = malloc(16);
    if (c != a) return 4;

    /* Free c, then allocate smaller block from the same free chunk. */
    free(c);
    d = malloc(8);
    if (d == 0) return 5;
    if (d != a) return 6; /* first fit should give back the same region */

    free(b);
    free(d);
    return 0;
}
