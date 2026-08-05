#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;
    a = malloc(16);
    if (a == 0) return 1;
    free(a);
    b = malloc(16);
    if (b == 0) return 2;
    if (b != a) return 100 + (b - a);
    free(b);
    return 0;
}
