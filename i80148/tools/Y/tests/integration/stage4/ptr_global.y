int g;

int main(void) {
    int *p;
    p = &g;
    *p = 42;
    return g;
}
