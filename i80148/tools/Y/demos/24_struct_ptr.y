struct Point {
    int x;
    int y;
};

int get_x(struct Point *p) {
    return p->x;
}

int main(void) {
    struct Point p;
    p.x = 10;
    p.y = 20;
    return get_x(&p);
}
