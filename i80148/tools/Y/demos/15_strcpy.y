int main(void) {
    char *src;
    char *dst;
    src = "abc";
    dst = malloc(16);
    strcpy(dst, src);
    return strlen(dst);
}
