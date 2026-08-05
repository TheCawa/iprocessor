#include <ystdlib.h>

char *ga;
char *gb;
char *gc;

int main(void) {
    ga = malloc(16);
    gb = malloc(16);
    free(ga);
    free(gb);
    gc = malloc(40);
    if (gc == 0) return 1;
    if (gc == ga) return 0;
    return 2;
}
