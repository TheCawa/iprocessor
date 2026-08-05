extern int shared;
extern int get_shared(void);

static int local_fn(void) {
    return 3;
}

int main(void) {
    if (local_fn() != 3) return 1;
    if (get_shared() != 77) return 2;
    shared = 5;
    if (shared != 5) return 3;
    return 0;
}
