#include <ydisk.h>

int main(void) {
    disk_read(0, 0, 0);
    return DISK_SECTOR_SIZE;
}
