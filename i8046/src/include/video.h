#ifndef VIDEO_H
#define VIDEO_H
#include "cpu.h"

void video_init();
void video_render(Cpu* cpu);
void video_clear(Cpu* cpu);
void video_shutdown();
bool video_handle_events();

#endif