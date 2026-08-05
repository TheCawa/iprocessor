struct Point {
    int x;
    int y;
};

int sum(struct Point *p) {
    return p->x + p->y;
}

int main(void) {
    struct Point p;
    p.x = 2;
    p.y = 3;
    return sum(&p);
}
