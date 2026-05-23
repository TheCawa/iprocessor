#include "emulator.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <cstring>

// Нативное разрешение текстового экрана (80x25 символов * шрифт 8x8)
#define TEXT_COLS 80
#define TEXT_ROWS 25
#define FONT_SIZE 8

#define SCREEN_TEX_W (TEXT_COLS * FONT_SIZE) // 640
#define SCREEN_TEX_H (TEXT_ROWS * FONT_SIZE) // 200

#define DISPLAY_W 640
#define DISPLAY_H 400

static SDL_Window* g_window = nullptr;
static SDL_Texture* g_screen_texture = nullptr;

// 16-цветная палитра VGA
static const uint32_t vga_palette[16] = {
    0xFF000000, 0xFF0000AA, 0xFF00AA00, 0xFF00AAAA,
    0xFFAA0000, 0xFFAA00AA, 0xFFAA5500, 0xFFAAAAAA,
    0xFF555555, 0xFF5555FF, 0xFF55FF55, 0xFF55FFFF,
    0xFFFF5555, 0xFFFF55FF, 0xFFFFFF55, 0xFFFFFFFF
};

static const uint64_t vga_font8x8[] = {
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0038003838383838, 0x00000000006C6C6C, 0x006C6CFE6CFE6C6C,
    0x0010FC167CD07E10, 0x008646201008C4C2, 0x0076889C64504830, 0x0000000000303030,
    0x0008103030301008, 0x0020101818181020, 0x0000000000543854, 0x000018187E181800,
    0x1030300000000000, 0x000000007E000000, 0x0030300000000000, 0x0040202010080804,
    0x00384464544C4438, 0x007C101010503010, 0x007C444038044438, 0x0038440418044438,
    0x0004047C4424140C, 0x003844040478407C, 0x0038444478404438, 0x001010080804047C,
    0x0038444438444438, 0x003844043C444438, 0x0030300000303000, 0x1030300000303000,
    0x000C18306030180C, 0x0000007C007C0000, 0x006030180C183060, 0x001800180C06663C,
    0x0038405C5C5C4438, 0x004444447C444438, 0x0078242438242478, 0x0038444040404438,
    0x0078242424242478, 0x007C24203820247C, 0x007020203820247C, 0x003844445C404438,
    0x004444447C444444, 0x007C10101010107C, 0x003844040404041C, 0x0064242830282464,
    0x007C242020202070, 0x0044444444546C44, 0x004444444C546444, 0x0038444444444438,
    0x0070202038242478, 0x0006384444444438, 0x0064242438242478, 0x0038440438404438,
    0x003810101010547C, 0x0038444444444444, 0x0010284444444444, 0x0028545444444444,
    0x0044442810284444, 0x0010101010284444, 0x007C44201008447C, 0x0038202020202038,
    0x0004080810202040, 0x0038080808080838, 0x00000000C66C3810, 0x007C000000000000,
    0x0000000000183060, 0x003A443C04380000, 0x0058242424382070, 0x003C4040403C0000,
    0x0034484848380818, 0x003C407C44380000, 0x0020202078202418, 0x78043C44443A0000,
    0x0064242424382060, 0x007C101010700010, 0x38440404041C0004, 0x0064283028242060,
    0x007C101010101070, 0x0054545454680000, 0x0024242424580000, 0x0038444444380000,
    0x6020382424780000, 0x0808384848340000, 0x0070202024580000, 0x00780438403C0000,
    0x0018242020702020, 0x003A444444440000, 0x0010282844440000, 0x0028545444440000,
    0x0044281028440000, 0x78043C4444440000, 0x007C4038047C0000, 0x000C10102010100C,
    0x0010101010101010, 0x0030080804080830, 0x0000000000005028, 0x0000000000000000,
    0xFFFFFFFFFFFFFFFF
};

static uint32_t get_vga_color(uint8_t attr, bool is_fg) {
    uint8_t color_idx = is_fg ? (attr & 0x0F) : ((attr >> 4) & 0x0F);
    return vga_palette[color_idx];
}

bool emulator_init(SDL_Window** out_window, SDL_Renderer** out_renderer) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) return false;

    g_window = SDL_CreateWindow("i8046 - Emulator",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                1150, 680, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!g_window) return false;

    SDL_Renderer* renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return false;

    g_screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING, SCREEN_TEX_W, SCREEN_TEX_H);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();
    
    ImGui_ImplSDL2_InitForSDLRenderer(g_window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    *out_window = g_window;
    *out_renderer = renderer;
    return true;
}

void update_screen_texture(Cpu* cpu) {
    if (!cpu->screen_dirty) return;

    uint32_t* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(g_screen_texture, nullptr, (void**)&pixels, &pitch) == 0) {
        
        uint32_t base = VBUFFER_BASE;
        const size_t font_chars_count = sizeof(vga_font8x8) / sizeof(vga_font8x8[0]);

        for (int y = 0; y < TEXT_ROWS; y++) {
            for (int x = 0; x < TEXT_COLS; x++) {
                uint32_t addr = base + (y * TEXT_COLS + x) * 2;
                uint8_t ch = cpu->mem[addr];
                uint8_t attr = cpu->mem[addr + 1];
                
                uint32_t fg_color = get_vga_color(attr, true);
                uint32_t bg_color = get_vga_color(attr, false);
                
                uint64_t glyph = 0;
                if (ch < font_chars_count) {
                    glyph = vga_font8x8[ch];
                }

                for (int bit_y = 0; bit_y < FONT_SIZE; bit_y++) {
                    uint8_t row_byte = (glyph >> (bit_y * 8)) & 0xFF;
                    int pixel_y = y * FONT_SIZE + bit_y;

                    for (int bit_x = 0; bit_x < FONT_SIZE; bit_x++) {
                        bool is_pixel_active = (row_byte >> (7 - bit_x)) & 1;
                        int pixel_x = x * FONT_SIZE + bit_x;

                        // Пишем прямо в буфер текстуры 640x200
                        pixels[pixel_y * SCREEN_TEX_W + pixel_x] = is_pixel_active ? fg_color : bg_color;
                    }
                }
            }
        }
        SDL_UnlockTexture(g_screen_texture);
        cpu->screen_dirty = false;
    }
}

void emulator_render(Cpu* cpu, SDL_Renderer* renderer) {
    update_screen_texture(cpu);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("FILE")) {
            if (ImGui::MenuItem("Open ROM...")) {}
            if (ImGui::MenuItem("Reset CPU"))   { cpu_reset(cpu); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("EDIT"))    { ImGui::EndMenu(); }
        if (ImGui::BeginMenu("VIEW"))    { ImGui::EndMenu(); }
        if (ImGui::BeginMenu("INPUT"))   { ImGui::EndMenu(); }
        if (ImGui::BeginMenu("OUTPUT"))  { ImGui::EndMenu(); }
        if (ImGui::BeginMenu("ACTIONS")) { ImGui::EndMenu(); }
        if (ImGui::BeginMenu("OTHER"))   { ImGui::EndMenu(); }

        float stats_width = 300.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - stats_width);
        ImGui::Text("MEM: 16384K ALLOC, 4096K USED");

        ImGui::EndMainMenuBar();
    }

    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight()));
    
    ImGui::Begin("MasterWindow", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // --- ЛЕВАЯ ПАНЕЛЬ ---
    ImGui::BeginChild("ScreenPanel", ImVec2(670, 0), true);
    ImGui::Text("SCREEN (80x25 Text Mode -> scaled to 640x400)");
    ImGui::Separator();
    ImGui::Image((void*)(intptr_t)g_screen_texture, ImVec2(DISPLAY_W, DISPLAY_H));
    ImGui::EndChild();
    ImGui::SameLine();
    // --- ПРАВАЯ ПАНЕЛЬ: СВОЙСТВА И ВКЛАДКИ ---
    ImGui::BeginChild("ControlPanel", ImVec2(0, 0), true);
    
    if (ImGui::BeginTabBar("TabsContainer")) {
        if (ImGui::BeginTabItem("CPU STATEMENT")) {
            ImGui::Spacing();
            
            if (ImGui::BeginTable("RegistersTable", 2, ImGuiTableFlags_NoBordersInBody)) {
                ImGui::TableNextColumn();
                ImGui::Text("X1 = %04X", (unsigned int)cpu_get_reg(cpu, 1));
                ImGui::Text("X2 = %04X", (unsigned int)cpu_get_reg(cpu, 4));
                ImGui::Text("X3 = %04X", (unsigned int)cpu_get_reg(cpu, 7));
                ImGui::Text("X4 = %04X", (unsigned int)cpu_get_reg(cpu, 10));
                ImGui::Text("X5 = %04X", (unsigned int)cpu_get_reg(cpu, 13));
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::Text("CS = %06X", 0x020000); 
                ImGui::Text("DS = 025555"); 
                ImGui::Text("SS = 02AAAA");
                ImGui::Text("ES = 02FFFF");
                ImGui::TableNextColumn();
                ImGui::Text("IC = %06X", (unsigned int)cpu->regs[REG_IC]);
                ImGui::Text("FL = %08X", (unsigned int)cpu->regs[REG_FL]);
                ImGui::Text("SP = %06X", (unsigned int)cpu->regs[REG_SP]);
                ImGui::Text("BP = 000800");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::Text("SCS = 000800");
                ImGui::Text("SDS = 000800");
                ImGui::Text("SSS = 000FFF");
                ImGui::Text("SES = 000400");
                ImGui::EndTable();
            }
            
            ImGui::Separator();
            ImGui::Text("IX = 000048    A0 = 000064");
            ImGui::Text("IY = 010001    A1 = 010801");
            ImGui::Spacing();
            ImGui::Text("IE  = %d", cpu->irq_enabled ? 1 : 0);
            ImGui::Text("RES = 80x25 Text, 16 colors");
            
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("MEMORY DUMP")) {
            ImGui::Text("example (RAM)...");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    ImGui::EndChild();
    ImGui::End();

    ImGui::Render();
    SDL_SetRenderDrawColor(renderer, 200, 255, 0, 255);
    SDL_RenderClear(renderer);
    
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

bool emulator_handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        
        if (e.type == SDL_QUIT) return false;
    }
    return true;
}

void emulator_shutdown() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (g_screen_texture) SDL_DestroyTexture(g_screen_texture);
    SDL_Quit();
}