#include <ystdlib.h>
#include <yio.h>

int main(void) {
    char *a;
    a = 0x12345678;
    malloc(16);
    print_ptr(a);
    return 0;
}
