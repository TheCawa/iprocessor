int main(void) {
    int x;
    int *p;
    x = 4;
    p = &x;
    (*p)++;
    return x;
}
