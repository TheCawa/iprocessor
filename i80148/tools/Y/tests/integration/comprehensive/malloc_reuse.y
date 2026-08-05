#include <ystdlib.h>

int main(void) {
    char *a;
    char *b;

    a = malloc(16);
    if (a == 0) return 1;

    free(a);

    b = malloc(16);
    if (b == 0) return 2;

    /* The allocator should reuse the freed block, so both pointers match. */
    if (a != b) return 3;

    return 0;
}
