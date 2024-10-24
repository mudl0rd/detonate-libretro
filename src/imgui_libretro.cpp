#include "imgui_libretro.h"
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
struct ImGui_ImplOpenGL3_Data
{
    GLuint          GlVersion;               // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g. 320 for GL 3.2)
    char            GlslVersionString[32];   // Specified by user or detected based on compile time GL settings.
    bool            GlProfileIsES2;
    bool            GlProfileIsES3;
    bool            GlProfileIsCompat;
    GLint           GlProfileMask;
    GLuint          FontTexture;
    GLuint          ShaderHandle;
    GLint           AttribLocationTex;       // Uniforms location
    GLint           AttribLocationProjMtx;
    GLuint          AttribLocationVtxPos;    // Vertex attributes location
    GLuint          AttribLocationVtxUV;
    GLuint          AttribLocationVtxColor;
    unsigned int    VboHandle, ElementsHandle;
    GLsizeiptr      VertexBufferSize;
    GLsizeiptr      IndexBufferSize;
    bool            HasClipOrigin;
    bool            UseBufferSubData;

    ImGui_ImplOpenGL3_Data() { memset((void*)this, 0, sizeof(*this)); }
};

struct ImGui_ImplOpenGL3_VtxAttribState
{
    GLint   Enabled, Size, Type, Normalized, Stride;
    GLvoid* Ptr;

    void GetState(GLint index)
    {
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &Enabled);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &Size);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &Type);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &Normalized);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &Stride);
        glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &Ptr);
    }
    void SetState(GLint index)
    {
        glVertexAttribPointer(index, Size, Type, (GLboolean)Normalized, Stride, Ptr);
        if (Enabled) glEnableVertexAttribArray(index); else glDisableVertexAttribArray(index);
    }
};

static ImGui_ImplOpenGL3_Data* ImGui_ImplOpenGL3_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplOpenGL3_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

bool    ImGui_ImplOpenGL3_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    // Setup backend capabilities flags
    ImGui_ImplOpenGL3_Data* bd = IM_NEW(ImGui_ImplOpenGL3_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_opengl3";
    // Desktop or GLES 3
    GLint major = 3;
    GLint minor = 3;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    // Make an arbitrary GL call (we don't actually need the result)
    // IF YOU GET A CRASH HERE: it probably means the OpenGL function loader didn't do its job. Let us know!
    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_EXTENSIONS
    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (GLint i = 0; i < num_extensions; i++)
    {
        const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
        if (extension != nullptr && strcmp(extension, "GL_ARB_clip_control") == 0)
            bd->HasClipOrigin = true;
    }
#endif

    return true;
}

void    ImGui_ImplOpenGL3_Shutdown()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    IM_DELETE(bd);
}

void    ImGui_ImplOpenGL3_NewFrame()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplOpenGL3_Init()?");

    if (!bd->ShaderHandle)
        ImGui_ImplOpenGL3_CreateDeviceObjects();
}

static void ImGui_ImplOpenGL3_SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height, GLuint vertex_array_object)
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
  //  float tmp = T; T = B; B = tmp; // Swap top and bottom if origin is upper left
    const float ortho_projection[4][4] =
    {
        { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
        { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
        { 0.0f,         0.0f,        -1.0f,   0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
    };
    glUseProgram(bd->ShaderHandle);
    glUniform1i(bd->AttribLocationTex, 0);
    glUniformMatrix4fv(bd->AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(vertex_array_object);

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    glBindBuffer(GL_ARRAY_BUFFER, bd->VboHandle);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ElementsHandle);
    glEnableVertexAttribArray(bd->AttribLocationVtxPos);
    glEnableVertexAttribArray(bd->AttribLocationVtxUV);
    glEnableVertexAttribArray(bd->AttribLocationVtxColor);
   glVertexAttribPointer(bd->AttribLocationVtxPos,   2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(bd->AttribLocationVtxUV,    2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(bd->AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
}

void    ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLuint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&last_program);
    GLuint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&last_texture);
    GLuint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&last_array_buffer);
    // This is part of VAO on OpenGL 3.0+ and OpenGL ES 3.0+.
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_pos; last_vtx_attrib_state_pos.GetState(bd->AttribLocationVtxPos);
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_uv; last_vtx_attrib_state_uv.GetState(bd->AttribLocationVtxUV);
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_color; last_vtx_attrib_state_color.GetState(bd->AttribLocationVtxColor);
    GLuint last_vertex_array_object; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&last_vertex_array_object);
    GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
    GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
    GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
    GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
    GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
    GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to. VAO are not shared among GL contexts)
    // The renderer would actually work without any VAO bound, but then our VertexAttrib calls would overwrite the default one currently bound.
    GLuint vertex_array_object = 0;
   glGenVertexArrays(1, &vertex_array_object);
    ImGui_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        // - OpenGL drivers are in a very sorry state nowadays....
        //   During 2021 we attempted to switch from glBufferData() to orphaning+glBufferSubData() following reports
        //   of leaks on Intel GPU when using multi-viewports on Windows.
        // - After this we kept hearing of various display corruptions issues. We started disabling on non-Intel GPU, but issues still got reported on Intel.
        // - We are now back to using exclusively glBufferData(). So bd->UseBufferSubData IS ALWAYS FALSE in this code.
        //   We are keeping the old code path for a while in case people finding new issues may want to test the bd->UseBufferSubData path.
        // - See https://github.com/ocornut/imgui/issues/4468 and please report any corruption issues.
        const GLsizeiptr vtx_buffer_size = (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
        const GLsizeiptr idx_buffer_size = (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);

        glBufferData(GL_ARRAY_BUFFER, vtx_buffer_size, (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_buffer_size, (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);
      

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
               glScissor((int)clip_min.x, (int)((float)fb_height - clip_max.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y));
                // Bind texture, Draw
               glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
               glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
            }
        }
    }

    // Destroy the temporary VAO
   glDeleteVertexArrays(1, &vertex_array_object);

    // Restore modified GL state
    // This "glIsProgram()" check is required because if the program is "pending deletion" at the time of binding backup, it will have been deleted by now and will cause an OpenGL error. See #6220.
    if (last_program == 0 || glIsProgram(last_program)) glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glActiveTexture(last_active_texture);
    glBindVertexArray(last_vertex_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    last_vtx_attrib_state_pos.SetState(bd->AttribLocationVtxPos);
    last_vtx_attrib_state_uv.SetState(bd->AttribLocationVtxUV);
    last_vtx_attrib_state_color.SetState(bd->AttribLocationVtxColor);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_stencil_test) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

    // Desktop OpenGL 3.0 and OpenGL 3.1 had separate polygon draw modes for front-facing and back-facing faces of polygons
    glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);

    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    (void)bd; // Not all compilation paths use this
}

bool ImGui_ImplOpenGL3_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Build texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &bd->FontTexture);
    glBindTexture(GL_TEXTURE_2D, bd->FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);
    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    return true;
}

void ImGui_ImplOpenGL3_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->FontTexture)
    {
        glDeleteTextures(1, &bd->FontTexture);
        io.Fonts->SetTexID(0);
        bd->FontTexture = 0;
    }
}

// If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
static bool CheckShader(GLuint handle, const char* desc)
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    return (GLboolean)status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
static bool CheckProgram(GLuint handle, const char* desc)
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    return (GLboolean)status == GL_TRUE;
}

bool    ImGui_ImplOpenGL3_CreateDeviceObjects()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLint last_texture, last_array_buffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    const GLchar* vertex_shader_glsl_130[] ={
        "#version 140\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n"};

    const GLchar* fragment_shader_glsl_130[] ={
        "#version 140\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n"};

    // Create shaders
    GLuint vert_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_handle, ARRAY_SIZE(vertex_shader_glsl_130), vertex_shader_glsl_130, nullptr);
    glCompileShader(vert_handle);
    CheckShader(vert_handle, "vertex shader");
    GLuint frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_handle, ARRAY_SIZE(fragment_shader_glsl_130), fragment_shader_glsl_130, nullptr);
    glCompileShader(frag_handle);
    CheckShader(frag_handle, "fragment shader");

    // Link
    bd->ShaderHandle = glCreateProgram();
    glAttachShader(bd->ShaderHandle, vert_handle);
    glAttachShader(bd->ShaderHandle, frag_handle);
    glLinkProgram(bd->ShaderHandle);
    CheckProgram(bd->ShaderHandle, "shader program");
    glDeleteShader(vert_handle);
    glDeleteShader(frag_handle);
    glUseProgram(bd->ShaderHandle);
    bd->AttribLocationTex = glGetUniformLocation(bd->ShaderHandle, "Texture");
    bd->AttribLocationProjMtx = glGetUniformLocation(bd->ShaderHandle, "ProjMtx");
    bd->AttribLocationVtxPos = (GLuint)glGetAttribLocation(bd->ShaderHandle, "Position");
    bd->AttribLocationVtxUV = (GLuint)glGetAttribLocation(bd->ShaderHandle, "UV");
    bd->AttribLocationVtxColor = (GLuint)glGetAttribLocation(bd->ShaderHandle, "Color");

    // Create buffers
    glGenBuffers(1, &bd->VboHandle);
    glGenBuffers(1, &bd->ElementsHandle);

    ImGui_ImplOpenGL3_CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);

    return true;
}

void    ImGui_ImplOpenGL3_DestroyDeviceObjects()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->VboHandle)      { glDeleteBuffers(1, &bd->VboHandle); bd->VboHandle = 0; }
    if (bd->ElementsHandle) { glDeleteBuffers(1, &bd->ElementsHandle); bd->ElementsHandle = 0; }
    if (bd->ShaderHandle)   { glDeleteProgram(bd->ShaderHandle); bd->ShaderHandle = 0; }
    ImGui_ImplOpenGL3_DestroyFontsTexture();
}




static int g_Time = 0;

static ImGuiKey ImGui_ImplLibretro_KeyToImGuiKey(int key)
{
    switch (key)
    {
        case RETROK_TAB:                      return ImGuiKey_Tab;
        case RETROK_LEFT:       return ImGuiKey_LeftArrow;
        case RETROK_RIGHT:      return ImGuiKey_RightArrow;
        case RETROK_UP:         return ImGuiKey_UpArrow;
        case RETROK_DOWN:       return ImGuiKey_DownArrow;
        case RETROK_PAGEUP:    return ImGuiKey_PageUp;
        case RETROK_PAGEDOWN:  return ImGuiKey_PageDown;
        case RETROK_HOME:       return ImGuiKey_Home;
        case RETROK_END:        return ImGuiKey_End;
        case RETROK_INSERT:     return ImGuiKey_Insert;
        case RETROK_DELETE:                       return ImGuiKey_Delete;
        case RETROK_BACKSPACE:                         return ImGuiKey_Backspace;
        case RETROK_SPACE:                       return ImGuiKey_Space;
        case RETROK_RETURN:                        return ImGuiKey_Enter;
        case RETROK_ESCAPE:                        return ImGuiKey_Escape;
        //case RETROK_A:                        return ImGuiKey_Apostrophe;
        case RETROK_COMMA:                        return ImGuiKey_Comma;
        case RETROK_MINUS:                        return ImGuiKey_Minus;
        case RETROK_PERIOD:                        return ImGuiKey_Period;
        case RETROK_SLASH:                        return ImGuiKey_Slash;
        case RETROK_SEMICOLON:                        return ImGuiKey_Semicolon;
        case RETROK_EQUALS:                        return ImGuiKey_Equal;
        case RETROK_LEFTBRACKET:                        return ImGuiKey_LeftBracket;
        case RETROK_BACKSLASH:                        return ImGuiKey_Backslash;
        case RETROK_RIGHTBRACKET:                        return ImGuiKey_RightBracket;
       // case RETROK_A:                        return ImGuiKey_GraveAccent;
        case RETROK_CAPSLOCK:                         return ImGuiKey_CapsLock;
        case RETROK_SCROLLOCK:                         return ImGuiKey_ScrollLock;
        case RETROK_NUMLOCK:              return ImGuiKey_NumLock;
        case RETROK_PRINT:                         return ImGuiKey_PrintScreen;
        case RETROK_PAUSE:                         return ImGuiKey_Pause;
        case RETROK_KP0:                       return ImGuiKey_Keypad0;
        case  RETROK_KP1:                       return ImGuiKey_Keypad1;
        case  RETROK_KP2:                       return ImGuiKey_Keypad2;
        case  RETROK_KP3:                       return ImGuiKey_Keypad3;
        case  RETROK_KP4:                       return ImGuiKey_Keypad4;
        case  RETROK_KP5:                       return ImGuiKey_Keypad5;
        case  RETROK_KP6:                       return ImGuiKey_Keypad6;
        case  RETROK_KP7:                       return ImGuiKey_Keypad7;
        case  RETROK_KP8:                       return ImGuiKey_Keypad8;
        case  RETROK_KP9:                       return ImGuiKey_Keypad9;
        case  RETROK_KP_PERIOD:                        return ImGuiKey_KeypadDecimal;
        case  RETROK_KP_DIVIDE:                        return ImGuiKey_KeypadDivide;
        case RETROK_KP_MULTIPLY:                        return ImGuiKey_KeypadMultiply;
        case RETROK_KP_MINUS:                        return ImGuiKey_KeypadSubtract;
        case RETROK_KP_PLUS:                        return ImGuiKey_KeypadAdd;
        case RETROK_KP_ENTER:                        return ImGuiKey_KeypadEnter;
        case RETROK_KP_EQUALS:                         return ImGuiKey_KeypadEqual;
        case RETROK_LCTRL:              return ImGuiKey_LeftCtrl;
        case RETROK_LSHIFT:              return ImGuiKey_LeftShift;
        case RETROK_LALT:              return ImGuiKey_LeftAlt;
        //case 0:                         return ImGuiKey_LeftSuper;
        case RETROK_RCTRL:            return ImGuiKey_RightCtrl;
        case RETROK_RSHIFT:              return ImGuiKey_RightShift;
        case RETROK_RALT:              return ImGuiKey_RightAlt;
        //case 0:                         return ImGuiKey_RightSuper;
        case RETROK_MENU:                         return ImGuiKey_Menu;
        case RETROK_0:                     return ImGuiKey_0;
        case RETROK_1:                       return ImGuiKey_1;
        case RETROK_2:                       return ImGuiKey_2;
        case RETROK_3:                       return ImGuiKey_3;
        case RETROK_4:                       return ImGuiKey_4;
        case RETROK_5:                       return ImGuiKey_5;
        case RETROK_6:                       return ImGuiKey_6;
        case RETROK_7:                       return ImGuiKey_7;
        case RETROK_8:                       return ImGuiKey_8;
        case RETROK_9:                       return ImGuiKey_9;
        case RETROK_a:             return ImGuiKey_A;
        case RETROK_b:             return ImGuiKey_B;
        case RETROK_c:             return ImGuiKey_C;
        case RETROK_d:             return ImGuiKey_D;
        case RETROK_e:             return ImGuiKey_E;
        case RETROK_f:             return ImGuiKey_F;
        case RETROK_g:             return ImGuiKey_G;
        case RETROK_h:             return ImGuiKey_H;
        case RETROK_i:             return ImGuiKey_I;
        case RETROK_j:             return ImGuiKey_J;
        case RETROK_k:             return ImGuiKey_K;
        case RETROK_l:             return ImGuiKey_L;
        case RETROK_m:             return ImGuiKey_M;
        case RETROK_n:             return ImGuiKey_N;
        case RETROK_o:             return ImGuiKey_O;
        case RETROK_p:             return ImGuiKey_P;
        case RETROK_q:             return ImGuiKey_Q;
        case RETROK_r:             return ImGuiKey_R;
        case RETROK_s:             return ImGuiKey_S;
        case RETROK_t:             return ImGuiKey_T;
        case RETROK_u:             return ImGuiKey_U;
        case RETROK_v:             return ImGuiKey_V;
        case RETROK_w:             return ImGuiKey_W;
        case RETROK_x:             return ImGuiKey_X;
        case RETROK_y:             return ImGuiKey_Y;
        case RETROK_z:             return ImGuiKey_Z;
        case RETROK_F1:         return ImGuiKey_F1;
        case RETROK_F2:         return ImGuiKey_F2;
        case RETROK_F3:         return ImGuiKey_F3;
        case RETROK_F4:         return ImGuiKey_F4;
        case RETROK_F5:         return ImGuiKey_F5;
        case RETROK_F6:         return ImGuiKey_F6;
        case RETROK_F7:         return ImGuiKey_F7;
        case RETROK_F8:         return ImGuiKey_F8;
        case RETROK_F9:         return ImGuiKey_F9;
        case RETROK_F10:        return ImGuiKey_F10;
        case RETROK_F11:        return ImGuiKey_F11;
        case RETROK_F12:        return ImGuiKey_F12;
        case RETROK_UNKNOWN:    return ImGuiKey_None;
        default:                        return ImGuiKey_None;
    }
}

void ImGui_ImpLibretro_ProcessKeys(bool down, unsigned keycode,
      uint32_t character, uint16_t key_modifiers)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (key_modifiers &  RETROKMOD_CTRL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (key_modifiers &  RETROKMOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (key_modifiers &  RETROKMOD_ALT) != 0);
    io.AddKeyEvent(ImGui_ImplLibretro_KeyToImGuiKey(keycode),down);
}


void ImGui_ImplLibretro_ProcessMouse(int mouse_button, bool pressed, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent((float)x, (float)y);
    io.AddMouseButtonEvent(mouse_button, pressed);

}

void ImGui_ImplLibretro_ProcessMW(float mousewh)
{
    ImGuiIO& io = ImGui::GetIO();
    if (mousewh != 0)
    io.AddMouseWheelEvent(0.0f, mousewh > 0 ? 1.0f : -1.0f);
}

bool ImGui_ImplLibretro_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl_libretro";
    io.MouseDrawCursor = true;
    return true;
}

void ImGui_ImplLibretro_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
}


void ImGui_ImplLibretro_NewFrame()
{
    // Setup time step
    ImGuiIO& io = ImGui::GetIO();
    int current_time = time(0);
    int delta_time_ms = (current_time - g_Time);
    if (delta_time_ms <= 0)
        delta_time_ms = 1;
    io.DeltaTime = delta_time_ms / 1000.0f;
    g_Time = current_time;
}
/*
static void ImGui_ImplLibretro_UpdateKeyModifiers()
{
    ImGuiIO& io = ImGui::GetIO();
    int key_mods_ctrl = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LCTRL) ||
    input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RCTRL);
    int key_mods_shift= input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LSHIFT)||
    input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RSHIFT);
    int key_mods_alt= input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LALT)||
    input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RALT);
    io.AddKeyEvent(ImGuiMod_Ctrl, key_mods_ctrl != 0);
    io.AddKeyEvent(ImGuiMod_Shift, key_mods_shift != 0);
    io.AddKeyEvent(ImGuiMod_Alt, key_mods_alt != 0);
}*/