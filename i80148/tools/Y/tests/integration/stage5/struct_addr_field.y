struct Point {
    int x;
    int y;
};

int main(void) {
    struct Point p;
    int *px;
    p.x = 9;
    p.y = 1;
    px = &p.x;
    *px = 11;
    return p.x;
}
