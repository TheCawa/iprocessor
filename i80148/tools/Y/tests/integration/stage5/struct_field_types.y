struct Data {
    char a;
    short b;
    int c;
};

int main(void) {
    struct Data d;
    d.a = 1;
    d.b = 2;
    d.c = 3;
    return d.a + d.b + d.c;
}
