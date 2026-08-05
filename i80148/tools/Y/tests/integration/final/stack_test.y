#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;

    a = malloc(16);
    b = a;
    free(a);
    if (b != a) return 1;
    return 0;
}
