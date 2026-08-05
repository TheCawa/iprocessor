#include <yio.h>

int main(void) {
    int n;
    char *p;

    n = 255;
    printf("%u", n);
    printf(" %x", n);

    p = 0x00060000;
    printf(" %p", p);

    return 0;
}
