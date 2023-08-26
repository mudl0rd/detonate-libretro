#include <stdio.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl.h"
#include "glad.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <filesystem>
#include "cmdline.h"
#include "audiodecode.h"
#include "IconsForkAwesome.h"


int window_width,window_height;

void resizeui(int width, int height)
{
    window_width=width;
    window_height=height;
}

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

        rcd.name = p.path().filename().u8string();
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

int main(int argc, char *argv[])
{
  if (argc > 2)
  {
    cmdline::parser a;
    a.add<std::string>("core_name", 'c', "core filename", true, "");
    a.add<std::string>("rom_name", 'r', "rom filename", true, "");
    a.add("pergame", 'g', "per-game configuration");
    a.parse_check(argc, argv);
    std::string rom = a.get<std::string>("rom_name");
    std::string core = a.get<std::string>("core_name");
   // bool pergame = a.exist("pergame");
    //if (!rom.empty() && !core.empty())
    //  return main2(rom.c_str(), core.c_str(), pergame);
   // else
      //printf("\nPress any key to continue....\n");
    //return 0;
  }

  pwd_ =  std::filesystem::current_path();
  updrecords();

  


  int ui();
  return ui();
}

#ifdef _WIN32

#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    // avoid unused argument error while matching template
    ((void)hInstance);
    ((void)hPrevInstance);
    ((void)lpCmdLine);
    ((void)nCmdShow);
    return main(__argc,__argv);
}

#endif


void droppedfile(char* filename)
{

}

void looping(SDL_Window *window)
{
bool done = false;
  bool show_menu = true;
  while (!done)
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
    {

      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
        done = true;

      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1)
      {
        show_menu = !show_menu;
        break;
      }

      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
      {
        int w = event.window.data1;
        int h = event.window.data2;

        glViewport(0, 0, w, h);
        glScissor(0, 0, w, h);
        extern void resizeui(int width, int height);
        resizeui(w,h);
      }
      if (event.type == SDL_DROPFILE)
      {
        char *filez = (char *)event.drop.file;
        extern void droppedfile(char* filename);
        droppedfile(filez);
        SDL_free(filez);
      }
    }

    glClearColor(0., 0., 0., 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    extern void rendermenu(SDL_Window *window, bool show_menu);
    rendermenu(window, show_menu);
  }
}

std::string selected_fname;


void rendermenu(SDL_Window *window, bool show_menu)
{
  std::string window_name;
  if (show_menu)
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
       ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(window_width,window_height));
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
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    music_run();
  }
  SDL_GL_SwapWindow(window);
}