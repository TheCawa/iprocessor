// ------------------------------------------------------------------------------
//          cpu_i80148_gui.cpp - i80148 ImGui register panel
//
//  Copyright (C) 2026  TheCawa <vos80584@gmail.com>
// ------------------------------------------------------------------------------
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://gnu.org>.
// ------------------------------------------------------------------------------

#include "cpu_i80148.h"
#include "cpu_api.h"

#ifdef NO_IMGUI

extern "C" void i80148_render_state(Cpu* cpu) {
    (void)cpu;
}

#else

#include "imgui.h"
#include <cstdio>

static void i80148_render_reg_pair(const char* name1, uint32_t val1,
                                   const char* name2, uint32_t val2) {
    ImGui::TableNextColumn();
    ImGui::Text("%s = %08X", name1, val1);
    ImGui::TableNextColumn();
    ImGui::Text("%s = %08X", name2, val2);
}

extern "C" void i80148_render_state(Cpu* cpu) {
    if (!cpu || !cpu->backend) return;

    if (ImGui::BeginTable("i80148_Registers", 2, ImGuiTableFlags_NoBordersInBody)) {
        i80148_render_reg_pair("R0",  (uint32_t)cpu_get_reg(cpu, REG_R0),
                               "A0",  (uint32_t)cpu_get_reg(cpu, REG_A0));
        for (int i = 0; i < 7; i++) {
            char name1[8], name2[8];
            snprintf(name1, sizeof(name1), "EX%d", i + 1);
            snprintf(name2, sizeof(name2), "A%d",  i + 1);
            i80148_render_reg_pair(name1, (uint32_t)cpu_get_reg(cpu, REG_EX1 + i),
                                   name2, (uint32_t)cpu_get_reg(cpu, REG_A1 + i));
        }
        i80148_render_reg_pair("IY",  (uint32_t)cpu_get_reg(cpu, REG_IY),
                               "SP",  (uint32_t)cpu_get_reg(cpu, REG_SP));
        i80148_render_reg_pair("IX",  (uint32_t)cpu_get_reg(cpu, REG_IX),
                               "BP",  (uint32_t)cpu_get_reg(cpu, REG_BP));
        i80148_render_reg_pair("IC",  (uint32_t)cpu_get_reg(cpu, REG_IC),
                               "IDTR", (uint32_t)cpu_get_reg(cpu, REG_IDTR));
        i80148_render_reg_pair("FL",  (uint32_t)cpu_get_reg(cpu, REG_FL),
                               "",    0);

        ImGui::EndTable();
    }
}

#endif
