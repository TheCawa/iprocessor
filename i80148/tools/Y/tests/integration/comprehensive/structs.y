struct Point {
    int x;
    int y;
};

struct Rect {
    struct Point tl;
    struct Point br;
};

int main(void) {
    struct Point p;
    struct Rect r;
    struct Point *pp;

    p.x = 3;
    p.y = 4;
    if (p.x + p.y != 7) return 1;

    pp = &p;
    pp->x = 10;
    if (p.x != 10) return 2;

    r.tl.x = 1;
    r.tl.y = 2;
    r.br.x = 3;
    r.br.y = 4;
    if (r.br.y - r.tl.x != 3) return 3;

    return 0;
}
