#include <stdio.h>
#include "imgui.h"
#include <filesystem>
#include <algorithm>
#include <vector>
#include <string>
#include "imgui_font.h"
#include "imgui_internal.h"
#include "forkawesome.h"
#include "IconsForkAwesome.h"
#include "audiodecode.h"


int window_width,window_height;
struct FileRecord
        {
            bool                  isDir = false;
            std::filesystem::path name;
            std::string           showName;
            std::filesystem::path extension;
        };
std::vector<FileRecord> fileRecords_;
std::filesystem::path pwd_;
std::string selected_fname;
std::string format_string;
int toseekto = 0;

void resizeui(int width, int height)
{
    window_width=width;
    window_height=height;
}



std::string format_duration( std::chrono::milliseconds ms ) {
    using namespace std::chrono;
    auto secs = duration_cast<seconds>(ms);
    ms -= std::chrono::duration_cast<milliseconds>(secs);
    auto mins = std::chrono::duration_cast<minutes>(secs);
    secs -= std::chrono::duration_cast<seconds>(mins);
    auto hour = std::chrono::duration_cast<hours>(mins);
    mins -= std::chrono::duration_cast<minutes>(hour);
    std::string ss;
     if(hour.count() > 0)
    {
      ss+= std::to_string(hour.count());
      ss+="h";
      ss+= " : ";
    }
    if(mins.count() > 0)
    {
    ss+= std::to_string(mins.count());
    ss+= "m";
    ss+= " : ";
    }
    ss+= std::to_string(secs.count());
    ss+="s";
    return ss;
}


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
        if(rcd.name.empty())continue;
        std::string str;
        if(!rcd.isDir)
        {
        rcd.extension = p.path().filename().extension();
        if(rcd.extension.empty())continue;
        std::string str2=rcd.extension.string();
        str2.erase(str2.begin());
        bool ismusicfile=(format_string.find(str2)!= std::string::npos);
        if(!ismusicfile)continue;
        else
        str = ICON_FK_MUSIC " ";
        }
        else
        str =  ICON_FK_FOLDER " ";
        rcd.showName = str + p.path().filename().string();
        fileRecords_.push_back(rcd);
    }
    std::sort(fileRecords_.begin(), fileRecords_.end(),
        [](const FileRecord &L, const FileRecord &R)
    {
        return (L.isDir ^ R.isDir) ? L.isDir : (L.name < R.name);
    });
}

void menus_setdir(const char* dir)
{
std::filesystem::path pah(dir);
pwd_ =  pah.parent_path();
updrecords();
}

void menus_init(float dpi_scaling, int width, int height)
{
  resizeui(width, height);
  
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)width, (float)height);
  #ifdef LIBRETRO
  io.MouseDrawCursor = true;
  #endif
  io.IniFilename = NULL;
  ImFontConfig font_cfg;
  font_cfg.FontDataOwnedByAtlas = false;
  static const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
  io.Fonts->AddFontFromMemoryTTF((unsigned char *)Roboto_Regular, sizeof(Roboto_Regular), dpi_scaling * 12.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
  font_cfg.MergeMode = true;
  font_cfg.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
  io.Fonts->AddFontFromMemoryCompressedTTF((unsigned char*)forkawesome_compressed_data,forkawesome_compressed_size,dpi_scaling * 12.0f,&font_cfg,icons_ranges);             // Merge into first font
  io.Fonts->Build();
  ImGuiStyle *style = &ImGui::GetStyle();
  style->TabRounding = 4;
  style->ScrollbarRounding = 9;
  style->WindowRounding = 7;
  style->GrabRounding = 3;
  style->FrameRounding = 3;
  style->PopupRounding = 4;
  style->ChildRounding = 4;
  style->ScrollbarSize = 10.0f;
  style->ScaleAllSizes(dpi_scaling);
  ImGui::StyleColorsDark();
  pwd_ =  std::filesystem::current_path();
  format_string = auddecode_formats();
  updrecords();
}

void menus_run()
{
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(window_width,window_height));
    ImGui::SetNextWindowPos(ImVec2(0.5f,0.5f));
    ImGui::Begin("test", NULL, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_MenuBar);
ImGuiStyle& style = ImGui::GetStyle();


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
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 5.f);
      ImGui::Button(ICON_FK_PAUSE);
      ImGui::PopStyleVar(1);
      ImGui::SameLine();

      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 5.f);
      ImGui::PushStyleColor(ImGuiCol_Button,style.Colors[ImGuiCol_ButtonHovered]);
      ImGui::Button(ICON_FK_REPEAT);
      ImGui::PopStyleColor(1);
      ImGui::PopStyleVar(1);
        
      if(music_isplaying() && music_getduration())
      {
        int pos = music_getposition();
        ImGui::SameLine();
        std::string posstring = format_duration(std::chrono::milliseconds(pos));
        std::string posstring_dr = format_duration(std::chrono::milliseconds(music_getduration()));
        posstring += " / ";
        posstring += posstring_dr;
        if(ImGui::SliderInt("Song position", &pos, 0,music_getduration(),posstring.c_str()) && ImGui::IsItemEdited())
        toseekto=pos;
       if(ImGui::IsItemDeactivatedAfterEdit())
        music_setposition(toseekto);
      }
     
       


     float reserveHeight = ImGui::GetFrameHeightWithSpacing();
     ImGui::BeginChild("ch", ImVec2(0, -reserveHeight), true,
      ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_HorizontalScrollbar);
    float panelHeight = ImGui::GetContentRegionAvail().y;
    float cellSize = ImGui::GetTextLineHeight();
    int items_sz=fileRecords_.size()*cellSize;
    int columns=(int)(items_sz/(int)panelHeight)+1;
    if(columns < 0)columns=1;
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
    ImGui::EndChild();
    ImGui::End();
    // Rendering
    ImGui::Render();
    music_run();
}