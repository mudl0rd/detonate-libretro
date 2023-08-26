#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "libretro.h"
#include "imgui_libretro.h"
#include "glsym/glsym.h"
#include "audiodecode.h"
#include "IconsForkAwesome.h"
#include "imgui_font.h"
#include "forkawesome.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static struct retro_hw_render_callback hw_render;
#define RARCH_GL_FRAMEBUFFER GL_FRAMEBUFFER
#define RARCH_GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE
#define RARCH_GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0

retro_environment_t environ_cb = NULL;
retro_video_refresh_t video_cb = NULL;
retro_audio_sample_t audio_cb = NULL;
retro_audio_sample_batch_t audio_batch_cb = NULL;
retro_input_poll_t poller_cb = NULL;
retro_input_state_t input_state_cb = NULL;
retro_keyboard_callback kb_cb;

struct FileRecord
        {
            bool                  isDir = false;
            std::filesystem::path name;
            std::string           showName;
            std::filesystem::path extension;
        };
std::vector<FileRecord> fileRecords_;
std::filesystem::path pwd_;

void updrecords()
{
   fileRecords_.clear();
   fileRecords_ = { FileRecord{ true, "..", ICON_FK_FOLDER" ..", "" } };

    for(auto &p : std::filesystem::directory_iterator(pwd_))
    {
        FileRecord rcd;

        if(p.is_regular_file())
        {
            rcd.isDir = false;
        }
        else if(p.is_directory())
        {
            rcd.isDir = true;
        }
        else
        {
            continue;
        }

        rcd.name = p.path().filename();
        if(rcd.name.empty())
        {
            continue;
        }

        rcd.extension = p.path().filename().extension();
        std::string str2 = p.path().filename().extension().string();
        const char *fext=str2.c_str();
        bool ismusicfile=(strstr(filetypes,fext)!=NULL) && !rcd.isDir;
        if(!ismusicfile && !rcd.isDir)continue;
       // const int N = sizeof( filetypes ) / sizeof( *filetypes );
        std::string str =  (rcd.isDir ? ICON_FK_FOLDER " " : ICON_FK_MUSIC " ");
        rcd.showName = str + p.path().filename().string();
        fileRecords_.push_back(rcd);
    }
    std::sort(fileRecords_.begin(), fileRecords_.end(),
        [](const FileRecord &L, const FileRecord &R)
    {
        return (L.isDir ^ R.isDir) ? L.isDir : (L.name < R.name);
    });
} 

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
#endif

#ifndef EXPORT
#if defined(CPPCLI)
#define EXPORT EXTERNC
#elif defined(_WIN32)
#define EXPORT EXTERNC __declspec(dllexport)
#else
#define EXPORT EXTERNC __attribute__((visibility("default")))
#endif
#endif
std::string selected_fname;
#define BASE_WIDTH 1280
#define BASE_HEIGHT 720
#define MAX_WIDTH 2048
#define MAX_HEIGHT 2048
static unsigned width  = BASE_WIDTH;
static unsigned height = BASE_HEIGHT;

EXPORT void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

EXPORT void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

EXPORT void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

EXPORT void retro_set_input_poll(retro_input_poll_t cb)
{
   poller_cb = cb;
}

EXPORT void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}


struct retro_core_option_v2_category option_cats_us[] = {
   {
      "hacks",
      "Hacks",
      "Configure bullshit which you REALLy shouldn't care about."
   },
   {
      "shit_hacks",
      "Other Hacks",
      "Configure other bits of bullshit."
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_category none[] = {
{ NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {
  {
      "region",
      "Console Region (Restart)",
      NULL,
      "Specify which region the system is from. 'PAL' is 50hz, 'NTSC' is 60hz.",
      NULL,
      "hacks",
      {
         { "auto", "Auto" },
         { "NTSC", NULL },
         { "PAL",  NULL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "frameskip",
      "Frameskip",
      NULL,
      "Skip frames to avoid audio buffer under-run (crackling). Improves performance at the expense of visual smoothness.",
      NULL,
      "shit_hacks",
      {
         { "disabled", NULL },
         { "auto",     "Auto" },
         { "manual",   "Manual" },
         { NULL, NULL },
      },
      "disabled"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};


struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};


void event(bool down, unsigned keycode,
      uint32_t character, uint16_t key_modifiers)
{
   ImGui_ImpLibretro_ProcessKeys(down,keycode,character,key_modifiers);
}


EXPORT void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;
  environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
   kb_cb.callback = event;
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK,&kb_cb);
}

EXPORT void retro_deinit(void) {}

EXPORT unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}




EXPORT void retro_init(void)
{
}

EXPORT void retro_get_system_info(struct retro_system_info *info)
{
   const struct retro_system_info myinfo = {"Detonate", "v1", "wav|s3m|mod|xm|flac|ogg|mp3", true, false};
   memcpy(info, &myinfo, sizeof(myinfo));
}

EXPORT void retro_get_system_av_info(struct retro_system_av_info *info)
{
    info->timing = (struct retro_system_timing) {
      .fps = 60.0,
      .sample_rate = 44100,
   };

   info->geometry = (struct retro_game_geometry) {
      .base_width   = BASE_WIDTH,
      .base_height  = BASE_HEIGHT,
      .max_width    = MAX_WIDTH,
      .max_height   = MAX_HEIGHT,
      .aspect_ratio = 16.0 / 9.0,
   };
}

EXPORT void retro_reset(void)
{
}

struct  mouseloop {
   int  bind;
   int retropad;
 }  mouse1[] ={
    {0, RETRO_DEVICE_ID_MOUSE_LEFT},
    {1,RETRO_DEVICE_ID_MOUSE_RIGHT},
    {2,RETRO_DEVICE_ID_MOUSE_MIDDLE}
 };

EXPORT void retro_run(void)
{
   poller_cb();

   ImGuiIO &io = ImGui::GetIO();
   const float X_FACTOR = ((float)width / 65536.0f);
   const float Y_FACTOR = ((float)height / 65536.0f);
   float mouse_x = (input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X) + 32768.0f) * X_FACTOR;
   float mouse_y = (input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y) + 32768.0f) * Y_FACTOR;
   int mouseup = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP);
   int mousedown = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN);
   for(int i=0;i<ARRAY_SIZE(mouse1);i++)
   ImGui_ImplLibretro_ProcessMouse(mouse1[i].bind, input_state_cb(0, RETRO_DEVICE_MOUSE, 0, 
   mouse1[i].retropad), mouse_x,mouse_y);
   if(mouseup)
   ImGui_ImplLibretro_ProcessMW(1.0);
   else if(mousedown)
   ImGui_ImplLibretro_ProcessMW(-1.0);
   else
   ImGui_ImplLibretro_ProcessMW(0.0);
   ImGui_ImplOpenGL3_NewFrame();
   glBindFramebuffer(RARCH_GL_FRAMEBUFFER, hw_render.get_current_framebuffer());
   ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(width,height));
    ImGui::SetNextWindowPos(ImVec2(0.5f,0.5f));
    ImGui::Begin("test", NULL, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_MenuBar);

if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help "))
			{
				ImGui::MenuItem("Dummy");
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
      ImGui::Button(ICON_FK_PAUSE);
       ImGui::SameLine();
      ImGui::Button(ICON_FK_STOP);
     float reserveHeight = ImGui::GetFrameHeightWithSpacing();
     ImGui::BeginChild("ch", ImVec2(0, -reserveHeight), true,
      ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_HorizontalScrollbar);
    float panelHeight = ImGui::GetContentRegionAvail().y;
    float cellSize = ImGui::GetTextLineHeight();
    int items_sz=fileRecords_.size()*cellSize;
    int columns=(int)(items_sz/(int)panelHeight)+1;
    float items=0;
	ImGui::Columns(columns, 0, false);
     for(auto &rsc : fileRecords_)
        {

            if(!rsc.name.empty() && rsc.name.c_str()[0] == '$')
            {
                continue;
            }

            bool selected = rsc.showName == selected_fname;

            ImGui::Selectable(rsc.showName.c_str(), selected,
                          ImGuiSelectableFlags_DontClosePopups);

            if(ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
            {
                if(rsc.isDir)
                {
                 pwd_ = (rsc.name != "..") ? (pwd_ / rsc.name) :pwd_.parent_path();
                 updrecords();
                }
                else
                {
                  std::string path2 = std::filesystem::path
                  (std::filesystem::canonical(pwd_) / rsc.name).string();
                   if(music_isplaying())
                   music_stop();
                    selected_fname = rsc.showName ;
                    music_play((const char*)path2.c_str());
                }
            }

            items += cellSize;
            if(items<=panelHeight)
            {
              items=0;
              ImGui::NextColumn();
            }
        }
    ImGui::Columns(1);
    ImGui::EndChild();
    ImGui::End();
    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   video_cb(RETRO_HW_FRAME_BUFFER_VALID, width, height, 0);

   music_run();

}

static void context_reset(void)
{
   fprintf(stderr, "Context reset!\n");
   rglgen_resolve_symbols(hw_render.get_proc_address);
}

static void context_destroy(void)
{

}

static bool retro_init_hw_context(void)
{
   hw_render.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
   hw_render.version_major = 3;
   hw_render.version_minor = 3;
   hw_render.context_reset = context_reset;
   hw_render.context_destroy = context_destroy;
   hw_render.depth = true;
   hw_render.stencil = true;
   hw_render.bottom_left_origin = true;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render))
      return false;
   return true;
}


EXPORT size_t retro_serialize_size(void)
{
   return 0;
}

EXPORT bool retro_serialize(void *data, size_t size)
{

   return true;
}

EXPORT bool retro_unserialize(const void *data, size_t size)
{

   return true;
}

EXPORT bool retro_load_game(const struct retro_game_info *game)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.MouseDrawCursor = true;
    // Setup Dear ImGui style
    io.IniFilename = NULL;
  ImFontConfig font_cfg;
  font_cfg.FontDataOwnedByAtlas = false;
  static const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
  io.Fonts->AddFontFromMemoryTTF((unsigned char *)Roboto_Regular, sizeof(Roboto_Regular),12.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
  font_cfg.MergeMode = true;
  font_cfg.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
  io.Fonts->AddFontFromMemoryCompressedTTF((unsigned char*)forkawesome_compressed_data,forkawesome_compressed_size,12.0f,&font_cfg,icons_ranges);             // Merge into first font
  io.Fonts->Build();


   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      fprintf(stderr, "XRGB8888 is not supported.\n");
      return false;
   }

   if (!retro_init_hw_context())
   {
      fprintf(stderr, "HW Context could not be initialized, exiting...\n");
      return false;
   }

   ImGui_ImplLibretro_Init();
   ImGui_ImplOpenGL3_Init();

   pwd_ =  std::filesystem::path(game->path).parent_path();
   updrecords();
   return music_play(game->path);
}

EXPORT bool retro_load_game_special(unsigned game_type,
                                    const struct retro_game_info *info, size_t num_info)
{
   return false;
}

EXPORT void retro_unload_game(void)
{
   ImGui_ImplOpenGL3_Shutdown();
}

EXPORT unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

EXPORT void *retro_get_memory_data(unsigned id) { return NULL; }
EXPORT size_t retro_get_memory_size(unsigned id) { return 0; }
EXPORT void retro_cheat_reset(void) {}
EXPORT void retro_cheat_set(unsigned index, bool enabled, const char *code) {}
EXPORT void retro_set_controller_port_device(unsigned port, unsigned device) {}