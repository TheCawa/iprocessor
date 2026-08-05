int g;

int *get_g_ptr(void) {
    return &g;
}

int main(void) {
    *get_g_ptr() = 5;
    return g;
}
