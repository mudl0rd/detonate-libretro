#include "libretro.h"
#include "imgui.h"
#include "glsym/glsym.h"
#include <time.h>
#include <stdint.h>

void ImGui_ImpLibretro_ProcessKeys(bool down, unsigned keycode,
      uint32_t character, uint16_t key_modifiers);
void ImGui_ImplLibretro_ProcessMouse(int mouse_button, bool pressed, int x, int y);
void ImGui_ImplLibretro_ProcessMW(float mousewh);
bool ImGui_ImplLibretro_Init();
void ImGui_ImplLibretro_Shutdown();
void ImGui_ImplLibretro_NewFrame();

IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_Init();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplOpenGL3_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplOpenGL3_DestroyDeviceObjects();
