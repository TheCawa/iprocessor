#define DEBUG

int main(void) {
    int x;
#ifdef DEBUG
    x = 5;
#else
    x = 9;
#endif
    return x;
}
