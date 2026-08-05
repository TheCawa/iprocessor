int main(void) {
    char *p;
    p = malloc(32);
    strcpy(p, "test");
    return strlen(p);
}
