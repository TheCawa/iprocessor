// Unit tests for the shared disk controller registers (system/disk.c).
//
// Verifies:
//   - DISK_SECTOR_SIZE is 1024 bytes
//   - LBA_SIZE (0x20119) reports 0x0400
//   - DISK_LBA (0x20112) / DISK_LBA_HI (0x20113) read back
//   - DISK_BUFFER (0x20114) / DISK_COUNT (0x20115) read back
//   - byte-wise LBA writes assemble correctly
//   - an end-to-end 1024-byte sector read from a raw image spans a full sector
//
// Compile/link against emulator/src/system/disk.c.

#include "system.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int g_failures = 0;
static int g_tests = 0;

#define CHECK(cond, ...) do { \
    g_tests++; \
    if (!(cond)) { \
        g_failures++; \
        printf("FAIL: " __VA_ARGS__); \
        printf("\n"); \
    } \
} while (0)

static void test_sector_size(void) {
    printf("\n=== Sector size ===\n");
    CHECK(DISK_SECTOR_SIZE == 1024, "DISK_SECTOR_SIZE == 1024 (got %d)", (int)DISK_SECTOR_SIZE);
}

static void test_register_readback(void) {
    printf("\n=== Disk register readback ===\n");
    Cpu cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.disk_current_drive = 0;

    // LBA_SIZE must read as the sector size (0x0400 default).
    CHECK(disk_read_dword(&cpu, 0x00020119) == 0x0400, "LBA_SIZE == 0x0400 (got 0x%X)",
          (unsigned)disk_read_dword(&cpu, 0x00020119));

    // DISK_LBA write/read-back.
    disk_write_dword(&cpu, 0x00020112, 0x00001234);
    CHECK(disk_read_dword(&cpu, 0x00020112) == 0x00001234,
          "DISK_LBA read-back == 0x1234 (got 0x%X)", (unsigned)disk_read_dword(&cpu, 0x00020112));
    CHECK(disk_read_dword(&cpu, 0x00020113) == 0x0000, "DISK_LBA_HI of 0x1234 == 0");

    uint32_t hi = 0xABCD0000;
    disk_write_dword(&cpu, 0x00020112, hi);
    CHECK(disk_read_dword(&cpu, 0x00020113) == 0xABCD,
          "DISK_LBA_HI read-back == 0xABCD (got 0x%X)", (unsigned)disk_read_dword(&cpu, 0x00020113));

    // DISK_BUFFER / DISK_COUNT read-back.
    disk_write_dword(&cpu, 0x00020114, 0x00C0FFEE);
    CHECK(disk_read_dword(&cpu, 0x00020114) == 0x00C0FFEE,
          "DISK_BUFFER read-back == 0xC0FFEE (got 0x%X)", (unsigned)disk_read_dword(&cpu, 0x00020114));
    disk_write_dword(&cpu, 0x00020115, 7);
    CHECK(disk_read_dword(&cpu, 0x00020115) == 7, "DISK_COUNT read-back == 7");

    // Byte-wise LBA writes assemble low/ high bytes.
    memset(&cpu, 0, sizeof(cpu));
    cpu.disk_current_drive = 0;
    disk_write_byte(&cpu, 0x00020112, 0x78);            // low byte (bits 0-7)
    CHECK(disk_read_dword(&cpu, 0x00020112) == 0x78,
          "LBA low byte write (got 0x%X)", (unsigned)disk_read_dword(&cpu, 0x00020112));
    disk_write_byte(&cpu, 0x00020113, 0x34);            // bits 16-23
    CHECK(disk_read_dword(&cpu, 0x00020112) == 0x00340078,
          "LBA high-byte write (got 0x%X)", (unsigned)disk_read_dword(&cpu, 0x00020112));
}

static void test_sector_read_spans_1024(void) {
    printf("\n=== 1024-byte sector read from raw image ===\n");
    const char* path = "disk_regs_test.img";

    // Build a 2-sector raw image; sector 0 = 0x00..0xFF then 1,2,...
    unsigned char img[2 * 1024];
    for (int i = 0; i < 2 * (int)DISK_SECTOR_SIZE; i++) {
        img[i] = (unsigned char)(i % 256);
    }
    FILE* f = fopen(path, "wb");
    if (!f) { printf("FAIL: cannot create temp image file\n"); return; }
    fwrite(img, 1, sizeof(img), f);
    fclose(f);

    Cpu cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.disk_current_drive = 0;
    strncpy(cpu.disk_drives[0].image_path, path, sizeof(cpu.disk_drives[0].image_path) - 1);

    // Read LBA 0.
    disk_write_dword(&cpu, 0x00020112, 0);
    disk_write_dword(&cpu, 0x00020111, 2);  // read single sector
    CHECK(cpu.disk_drives[0].status == 0, "read LBA0 status ok");
    CHECK(cpu.disk_drives[0].sector_buffer[0] == 0x00, "sector buffer[0] == 0x00");
    CHECK(cpu.disk_drives[0].sector_buffer[511] == 0xFF, "sector buffer[511] == 0xFF");
    CHECK(cpu.disk_drives[0].sector_buffer[512] == 0x00, "sector buffer[512] == 0x00 (spans 1024)");
    CHECK(cpu.disk_drives[0].sector_buffer[1023] == 0xFF, "sector buffer[1023] == 0xFF");

    // Read LBA 1 (offset 1024 in the image).
    disk_write_dword(&cpu, 0x00020112, 1);
    disk_write_dword(&cpu, 0x00020111, 2);
    CHECK(cpu.disk_drives[0].sector_buffer[0] == 0x00, "LBA1 buffer[0] == 0x00");
    CHECK(cpu.disk_drives[0].sector_buffer[64] == 0x40, "LBA1 buffer[64] == 0x40");
    CHECK(cpu.disk_drives[0].sector_buffer[1023] == 0xFF, "LBA1 buffer[1023] == 0xFF");

    remove(path);
}

int main(void) {
    test_sector_size();
    test_register_readback();
    test_sector_read_spans_1024();

    printf("\n=== Results: %d tests, %d failures ===\n", g_tests, g_failures);
    return g_failures > 0 ? 1 : 0;
}