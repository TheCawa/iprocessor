#include <ystdlib.h>

extern int _heap_start;

int main(void) {
    char *a;
    char *b;
    a = malloc(16);
    b = malloc(16);
    if (b - 4 > _heap_start) {
        printf("greater\n");
    } else {
        printf("le\n");
    }
    free(a);
    free(b);
    return 0;
}
