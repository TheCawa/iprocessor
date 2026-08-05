struct Box {
    int w;
    int h;
};

struct Box b;

int main(void) {
    b.w = 5;
    b.h = 6;
    return b.w * b.h;
}
