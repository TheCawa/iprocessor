#include <yio.h>

int main(void) {
    int n;
    unsigned int u;
    char *p;

    n = -1;
    u = 0xFFFFFFFF;
    p = 0;

    printf("neg=%d", n);
    printf(" uint=%u", u);
    printf(" hex=%x", 0);
    printf(" ptr=%p", p);

    return 0;
}
