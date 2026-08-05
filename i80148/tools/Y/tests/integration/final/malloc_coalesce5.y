#include <ystdlib.h>
#include <yio.h>

int main(void) {
    char *a;
    char *b;
    char *c;

    a = malloc(16);
    b = malloc(16);
    free(a);
    free(b);
    c = malloc(40);
    printf("a=%p\n", a);
    printf("b=%p\n", b);
    printf("c=%p\n", c);
    return 0;
}
