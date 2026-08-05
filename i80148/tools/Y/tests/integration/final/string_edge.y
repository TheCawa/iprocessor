#include <ystring.h>
#include <yio.h>

int main(void) {
    char buf[16];
    char a[4];
    char b[4];

    if (strlen("") != 0) return 1;

    strcpy(buf, "");
    if (strlen(buf) != 0) return 2;

    strcpy(buf, "abc");
    if (strlen(buf) != 3) return 3;

    strncpy(a, "abcd", 3);
    if (a[0] != 'a' || a[1] != 'b' || a[2] != 'c') return 4;

    memset(b, 0, 4);
    b[0] = 'x';
    if (memcmp(a, b, 4) == 0) return 5;
    if (memcmp(a, a, 4) != 0) return 6;

    memcpy(b, a, 3);
    if (memcmp(a, b, 3) != 0) return 7;

    return 0;
}
