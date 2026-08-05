#include <ystdlib.h>
#include <yio.h>

int main(void) {
    char *a;
    char *x;
    a = 0x12345678;
    x = malloc(16);
    free(x);
    print_ptr(a);
    return 0;
}
