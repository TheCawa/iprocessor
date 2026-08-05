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
    return c - a;
}
