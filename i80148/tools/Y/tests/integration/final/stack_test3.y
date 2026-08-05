#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;
    char *c;

    a = malloc(16);
    b = malloc(16);
    c = malloc(16);
    free(a);
    free(b);
    free(c);
    if (a == 0 || b == 0 || c == 0) return 1;
    return 0;
}
