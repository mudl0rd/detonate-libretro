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

int window_width, window_height;
struct FileRecord
{
    bool isDir = false;
    std::filesystem::path name;
    std::string showName;
    std::filesystem::path extension;
};
std::vector<FileRecord> fileRecords_;
std::filesystem::path pwd_;
std::string selected_fname;
std::string format_string;
int toseekto = 0;

void resizeui(int width, int height)
{
    window_width = width;
    window_height = height;
}

static void ToolTip(const char *desc)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

#ifdef _WIN32
#include <windows.h>
inline std::uint32_t GetDrivesBitMask()
{
    const DWORD mask = GetLogicalDrives();
    std::uint32_t ret = 0;
    for (int i = 0; i < 26; ++i)
    {
        if (!(mask & (1 << i)))
        {
            continue;
        }
        const char rootName[4] = {static_cast<char>('A' + i), ':', '\\', '\0'};
        const UINT type = GetDriveTypeA(rootName);
        if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED || type == DRIVE_REMOTE)
        {
            ret |= (1 << i);
        }
    }
    return ret;
}
static uint32_t drives_ = GetDrivesBitMask();
#endif

template <class Functor>
struct ScopeGuard
{
    ScopeGuard(Functor &&t) : func(std::move(t)) {}

    ~ScopeGuard() { func(); }

private:
    Functor func;
};

bool HyperLink(const char *label, bool underlineWhenHoveredOnly = false)
{
    ImGuiStyle &style = ImGui::GetStyle();
    const ImU32 linkColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);
    const ImU32 linkHoverColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    const ImU32 linkFocusColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

    const ImGuiID id = ImGui::GetID(label);

    ImGuiWindow *const window = ImGui::GetCurrentWindow();
    ImDrawList *const draw = ImGui::GetWindowDrawList();

    const ImVec2 pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
    const ImVec2 size = ImGui::CalcTextSize(label);
    ImRect bb(pos, {pos.x + size.x, pos.y + size.y});

    ImGui::ItemSize(bb, 0.0f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool isHovered = false;
    const bool isClicked = ImGui::ButtonBehavior(bb, id, &isHovered, nullptr);
    const bool isFocused = ImGui::IsItemFocused();

    const ImU32 color = isHovered ? linkHoverColor : isFocused ? linkFocusColor
                                                               : linkColor;

    draw->AddText(bb.Min, color, label);

    if (isFocused)
        draw->AddRect(bb.Min, bb.Max, color);
    else if (!underlineWhenHoveredOnly || isHovered)
        draw->AddLine({bb.Min.x, bb.Max.y}, bb.Max, color);

    return isClicked;
}

std::string format_duration(std::chrono::milliseconds ms)
{
    using namespace std::chrono;
    auto secs = duration_cast<seconds>(ms);
    ms -= std::chrono::duration_cast<milliseconds>(secs);
    auto mins = std::chrono::duration_cast<minutes>(secs);
    secs -= std::chrono::duration_cast<seconds>(mins);
    auto hour = std::chrono::duration_cast<hours>(mins);
    mins -= std::chrono::duration_cast<minutes>(hour);
    std::string ss;
    if (hour.count() > 0)
    {
        ss += std::to_string(hour.count());
        ss += "h";
        ss += " : ";
    }
    if (mins.count() > 0)
    {
        ss += std::to_string(mins.count());
        ss += "m";
        ss += " : ";
    }
    ss += std::to_string(secs.count());
    ss += "s";
    return ss;
}

void menu_setdir(const char *dir)
{
    std::filesystem::path pah(dir);
    pwd_ = pah.parent_path();
    auto rombrowse_update = [=]()
    {
        fileRecords_ = {FileRecord{true, "..", ICON_FK_FOLDER " ..", ""}};

        for (auto &p : std::filesystem::directory_iterator(pwd_))
        {
            FileRecord rcd = {FileRecord{false, "", "", ""}};

            if (p.is_regular_file())
            {
                rcd.isDir = false;
            }
            else if (p.is_directory())
            {
                rcd.isDir = true;
            }
            else
            {
                continue;
            }

            rcd.name = p.path().filename();
            if (rcd.name.empty())
                continue;
            std::string str;
            if (!rcd.isDir)
            {
                rcd.extension = p.path().filename().extension();
                if (rcd.extension.empty())
                    continue;
                std::string str2 = rcd.extension.string();
                str2.erase(str2.begin());
                bool ismusicfile = (format_string.find(str2) != std::string::npos);
                if (!ismusicfile)
                    continue;
                else
                    str = ICON_FK_MUSIC " ";
            }
            else
                str = ICON_FK_FOLDER " ";
            rcd.showName = str + p.path().filename().string();
            fileRecords_.push_back(rcd);
        }
        std::sort(fileRecords_.begin(), fileRecords_.end(),
                  [](const FileRecord &L, const FileRecord &R)
                  {
                      return (L.isDir ^ R.isDir) ? L.isDir : (L.name < R.name);
                  });
    };
    rombrowse_update();
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
    static const ImWchar icons_ranges[] = {ICON_MIN_FK, ICON_MAX_FK, 0};
    io.Fonts->AddFontFromMemoryTTF((unsigned char *)Roboto_Regular, sizeof(Roboto_Regular), dpi_scaling * 12.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
    font_cfg.MergeMode = true;
    font_cfg.GlyphMinAdvanceX = 13.0f;                                                                                                                                 // Use if you want to make the icon monospaced
    io.Fonts->AddFontFromMemoryCompressedTTF((unsigned char *)forkawesome_compressed_data, forkawesome_compressed_size, dpi_scaling * 12.0f, &font_cfg, icons_ranges); // Merge into first font
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
    pwd_ = std::filesystem::current_path();
    format_string = auddecode_formats();
    menu_setdir(pwd_.string().c_str());
}

void menus_run()
{
    ImVec2 winsize{window_width, window_height};
    float mainmenu_y = 0.0;
    ImGui::NewFrame();
    ImGuiIO &io = ImGui::GetIO();
    int ypos = 0;

    ImGui::SetNextWindowSize(winsize);
    ImGui::SetNextWindowPos(ImVec2(0.5f, 0.5f));
    ImGui::Begin("test", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar);

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
        mainmenu_y = ImGui::GetFrameHeight();
        ImGui::EndMenuBar();
        winsize.y -= mainmenu_y;
    }
    ypos = mainmenu_y;
    ImGuiStyle &style = ImGui::GetStyle();
    bool updrecs = false;
    int height_toolbar = ImGui::GetFrameHeight() * 3.1;
    ImGui::SetNextWindowSize(ImVec2(window_width, height_toolbar));
    ImGui::SetNextWindowPos(ImVec2(0.5f, ypos));
    ImGui::BeginChild("toolbar", ImVec2(window_width, height_toolbar), true, NULL);

    const char currentDrive = static_cast<char>(pwd_.c_str()[0]);
    const char driveStr[] = {currentDrive, ':', '\0'};

#ifdef _WIN32
    ImGui::PushItemWidth(4 * ImGui::GetFontSize());
    if (ImGui::BeginCombo("##select_drive", driveStr))
    {
        ScopeGuard guard([&]
                         { ImGui::EndCombo(); });

        for (int i = 0; i < 26; ++i)
        {
            if (!(drives_ & (1 << i)))
            {
                continue;
            }

            const char driveCh = static_cast<char>('A' + i);
            const char selectableStr[] = {driveCh, ':', '\0'};
            const bool selected = currentDrive == driveCh;

            if (ImGui::Selectable(selectableStr, selected) && !selected)
            {
                char newPwd[] = {driveCh, ':', '\\', '\0'};
                std::filesystem::path pah(newPwd);
                menu_setdir(pah.string().c_str());
            }
        }
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
#endif

    int secIdx = 0, newDirLastSecIdx = -1;
    for (const auto &sec : pwd_)
    {
#ifdef _WIN32
        if (secIdx == 1)
        {
            ++secIdx;
            continue;
        }
#endif

        ImGui::PushID(secIdx);
        if (secIdx > 0)
        {
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
        }
        if (HyperLink(sec.string().c_str()))
        {
            newDirLastSecIdx = secIdx;
        }
        ImGui::PopID();

        ++secIdx;
    }

    if (newDirLastSecIdx >= 0)
    {
        int i = 0;
        std::filesystem::path dstDir;
        for (const auto &sec : pwd_)
        {
            if (i++ > newDirLastSecIdx)
            {
                break;
            }
            dstDir /= sec;
        }

#ifdef _WIN32
        if (newDirLastSecIdx == 0)
        {
            dstDir /= "\\";
        }
#endif
        pwd_ = dstDir;
        menu_setdir(pwd_.string().c_str());
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 5.f);
    ImGui::Button(ICON_FK_PAUSE);
    ImGui::PopStyleVar(1);
    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 5.f);
    ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonHovered]);
    ImGui::Button(ICON_FK_REPEAT);
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(1);

    if (music_isplaying() && music_getduration())
    {
        int pos = music_getposition();
        ImGui::SameLine();
        std::string posstring = format_duration(std::chrono::milliseconds(pos));
        std::string posstring_dr = format_duration(std::chrono::milliseconds(music_getduration()));
        posstring += " / ";
        posstring += posstring_dr;
        static int tooseek = 0;
        ImGui::PushItemWidth(10 * ImGui::GetFontSize());
        if (ImGui::SliderInt("Song position", &pos, 0, music_getduration(), posstring.c_str()) && ImGui::IsItemEdited())
        {
            std::string tt = format_duration(std::chrono::milliseconds(pos));
            tooseek = pos;
            ImGui::SetTooltip(tt.c_str());
        }
        ImGui::PopItemWidth();

        if (ImGui::IsItemDeactivatedAfterEdit())
            music_setposition(tooseek);
    }

    ImGui::EndChild();

    ypos += height_toolbar;
    winsize.y -= ypos;

    ImGui::SetNextWindowSize(ImVec2(winsize));
    ImGui::SetNextWindowPos(ImVec2(0.5f, ypos));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::BeginChild("rombrowser", ImVec2(winsize), false,
                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar);

    float panelHeight = ImGui::GetContentRegionAvail().y;
    float cellSize = ImGui::CalcTextSize("TEST").y;
    int items_sz = fileRecords_.size() * cellSize;
    int columns = (int)(items_sz / (int)panelHeight) + 1;
    if (columns <= 0)
        columns = 1;
    float items = 0;
    ImGui::Columns(columns, 0, false);
    panelHeight = ImGui::GetContentRegionAvail().y;
    for (auto &rsc : fileRecords_)
    {

        if (!rsc.name.empty() && rsc.name.c_str()[0] == '$')
            continue;
        bool selected = rsc.showName == selected_fname;
        ImGui::Selectable(rsc.showName.c_str(), selected,
                          ImGuiSelectableFlags_DontClosePopups);

        if (ImGui::IsItemHovered())
        {
            int w = ImGui::GetColumnWidth();
            ImVec2 textsz = ImGui::CalcTextSize(rsc.showName.c_str());
            if (textsz.x - 1 > w)
                ToolTip(rsc.showName.c_str());
        }

        if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
        {
            if (rsc.isDir)
            {
                pwd_ = (rsc.name != "..") ? (pwd_ / rsc.name) : pwd_.parent_path();
                updrecs = true;
            }
            else
            {
                std::string path2 = std::filesystem::path(std::filesystem::canonical(pwd_) / rsc.name).string();
                if (music_isplaying())
                    music_stop();
                selected_fname = rsc.showName;
                music_play((const char *)path2.c_str());
            }
        }
        items += cellSize;
        if (items <= panelHeight)
        {
            items = 0;
            ImGui::NextColumn();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::End();
    // Rendering
    ImGui::Render();
    music_run();
    if (updrecs)
    {
        menu_setdir(pwd_.string().c_str());
        updrecs = false;
    }
}