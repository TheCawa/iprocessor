#include <ystring.h>

char buf[32];

int main(void) {
    char *s;
    int n;

    s = "hello";
    if (strlen(s) != 5) return 1;

    strcpy(buf, s);
    if (memcmp(buf, s, 6) != 0) return 2;

    strncpy(buf, "world", 5);
    buf[5] = 0;
    if (strlen(buf) != 5) return 3;

    memcpy(buf, "abcd", 5);
    if (buf[0] != 'a') return 4;
    if (buf[4] != 0) return 5;

    memset(buf, 'x', 10);
    if (buf[0] != 'x') return 6;
    if (buf[9] != 'x') return 7;

    n = memcmp("abc", "abc", 3);
    if (n != 0) return 8;

    return 0;
}
