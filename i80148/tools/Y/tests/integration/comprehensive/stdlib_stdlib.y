#include <ystdlib.h>
#include <ystring.h>

int main(void) {
    int x;
    char *p;

    x = atoi("12345");
    if (x != 12345) return 1;

    x = abs(-17);
    if (x != 17) return 2;

    p = malloc(16);
    if (p == 0) return 3;
    strcpy(p, "test");
    if (strlen(p) != 4) return 4;
    free(p);

    return 0;
}
