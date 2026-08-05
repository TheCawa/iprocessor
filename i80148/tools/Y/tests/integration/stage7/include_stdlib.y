#include <ystdlib.h>

int main(void) {
    char *p;
    p = malloc(16);
    if (p == 0) {
        return 0;
    }
    free(p);
    return 1;
}
