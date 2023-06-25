IMGUI_API bool ImGui_ImplRW_Init(void);
IMGUI_API void ImGui_ImplRW_Shutdown(void);
IMGUI_API void ImGui_ImplRW_NewFrame(float timeDelta);
sk::EventStatus ImGuiEventHandler(sk::Event e, void *param);
void ImGui_ImplRW_RenderDrawLists(ImDrawData* draw_data);

// Conversión de color entre rw:RBGA e ImVec4
ImVec4 rwRGBAToImVec4(const rw::RGBA& color);
rw::RGBA ImVec4TorwRGBA(const ImVec4& imColor);