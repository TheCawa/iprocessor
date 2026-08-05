#include <ystdlib.h>

extern int _heap_start;

int main(void) {
    char *a;
    a = malloc(16);
    if (a == 0) return 1;
    printf("heap_start=%p a=%p\n", &_heap_start, a);
    free(a);
    return 0;
}
