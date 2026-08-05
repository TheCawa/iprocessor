#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;

    a = malloc(16);
    b = malloc(16);
    free(a);
    free(b);
    if (a == 0) return 1;
    if (b == 0) return 2;
    return 0;
}
