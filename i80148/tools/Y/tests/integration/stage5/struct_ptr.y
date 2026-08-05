struct Point {
    int x;
    int y;
};

int main(void) {
    struct Point p;
    struct Point *q;
    p.x = 7;
    p.y = 8;
    q = &p;
    return q->x + q->y;
}
