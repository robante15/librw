#include <rw.h>
#include <skeleton.h>
#include <assert.h>

#include "imgui/imgui.h"
#include "imgui_impl_rw.h"

using namespace rw::RWDEVICE;

static rw::Texture *g_FontTexture;
static Im2DVertex *g_vertbuf;
static int g_vertbufSize;

void
ImGui_ImplRW_RenderDrawLists(ImDrawData* draw_data)
{
	ImGuiIO &io = ImGui::GetIO();

	// minimized
	if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
		return;

	if(g_vertbuf == nil || g_vertbufSize < draw_data->TotalVtxCount){
		if(g_vertbuf){
			rwFree(g_vertbuf);
			g_vertbuf = nil;
		}
		g_vertbufSize = draw_data->TotalVtxCount + 5000;
		g_vertbuf = rwNewT(Im2DVertex, g_vertbufSize, 0);
	}

	float xoff = 0.0f;
	float yoff = 0.0f;
#ifdef RWHALFPIXEL
	xoff = -0.5;
	yoff = 0.5;
#endif

	rw::Camera *cam = (rw::Camera*)rw::engine->currentCamera;
	Im2DVertex *vtx_dst = g_vertbuf;
	float recipZ = 1.0f/cam->nearPlane;
	for(int n = 0; n < draw_data->CmdListsCount; n++){
		const ImDrawList *cmd_list = draw_data->CmdLists[n];
		const ImDrawVert *vtx_src = cmd_list->VtxBuffer.Data;
		for(int i = 0; i < cmd_list->VtxBuffer.Size; i++){
			vtx_dst[i].setScreenX(vtx_src[i].pos.x + xoff);
			vtx_dst[i].setScreenY(vtx_src[i].pos.y + yoff);
			vtx_dst[i].setScreenZ(rw::im2d::GetNearZ());
			vtx_dst[i].setCameraZ(cam->nearPlane);
			vtx_dst[i].setRecipCameraZ(recipZ);
			vtx_dst[i].setColor(vtx_src[i].col&0xFF, vtx_src[i].col>>8 & 0xFF, vtx_src[i].col>>16 & 0xFF, vtx_src[i].col>>24 & 0xFF);
			vtx_dst[i].setU(vtx_src[i].uv.x, recipZ);
			vtx_dst[i].setV(vtx_src[i].uv.y, recipZ);
		}
		vtx_dst += cmd_list->VtxBuffer.Size;
	}

	int vertexAlpha = rw::GetRenderState(rw::VERTEXALPHA);
	int srcBlend = rw::GetRenderState(rw::SRCBLEND);
	int dstBlend = rw::GetRenderState(rw::DESTBLEND);
	int ztest = rw::GetRenderState(rw::ZTESTENABLE);
	void *tex = rw::GetRenderStatePtr(rw::TEXTURERASTER);
	int addrU = rw::GetRenderState(rw::TEXTUREADDRESSU);
	int addrV = rw::GetRenderState(rw::TEXTUREADDRESSV);
	int filter = rw::GetRenderState(rw::TEXTUREFILTER);
	int cullmode = rw::GetRenderState(rw::CULLMODE);

	rw::SetRenderState(rw::VERTEXALPHA, 1);
	rw::SetRenderState(rw::SRCBLEND, rw::BLENDSRCALPHA);
	rw::SetRenderState(rw::DESTBLEND, rw::BLENDINVSRCALPHA);
	rw::SetRenderState(rw::ZTESTENABLE, 0);
	rw::SetRenderState(rw::CULLMODE, rw::CULLNONE);

	int vtx_offset = 0;
	for(int n = 0; n < draw_data->CmdListsCount; n++){
		const ImDrawList *cmd_list = draw_data->CmdLists[n];
		int idx_offset = 0;
		for(int i = 0; i < cmd_list->CmdBuffer.Size; i++){
			const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[i];
			if(pcmd->UserCallback)
				pcmd->UserCallback(cmd_list, pcmd);
			else{
				rw::Texture *tex = (rw::Texture*)pcmd->TextureId;
				if(tex && tex->raster){
					rw::SetRenderStatePtr(rw::TEXTURERASTER, tex->raster);
					rw::SetRenderState(rw::TEXTUREADDRESSU, tex->getAddressU());
					rw::SetRenderState(rw::TEXTUREADDRESSV, tex->getAddressV());
					rw::SetRenderState(rw::TEXTUREFILTER, tex->getFilter());
				}else
					rw::SetRenderStatePtr(rw::TEXTURERASTER, nil);
				rw::im2d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST,
					g_vertbuf+vtx_offset, cmd_list->VtxBuffer.Size,
					cmd_list->IdxBuffer.Data+idx_offset, pcmd->ElemCount);
			}
			idx_offset += pcmd->ElemCount;
		}
		vtx_offset += cmd_list->VtxBuffer.Size;
	}

	rw::SetRenderState(rw::VERTEXALPHA,vertexAlpha);
	rw::SetRenderState(rw::SRCBLEND, srcBlend);
	rw::SetRenderState(rw::DESTBLEND, dstBlend);
	rw::SetRenderState(rw::ZTESTENABLE, ztest);
	rw::SetRenderStatePtr(rw::TEXTURERASTER, tex);
	rw::SetRenderState(rw::TEXTUREADDRESSU, addrU);
	rw::SetRenderState(rw::TEXTUREADDRESSV, addrV);
	rw::SetRenderState(rw::TEXTUREFILTER, filter);
	rw::SetRenderState(rw::CULLMODE, cullmode);
}

bool
ImGui_ImplRW_Init(void)
{
	using namespace sk;

	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	io.KeyMap[ImGuiKey_Tab] = sk::KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = sk::KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = sk::KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = sk::KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = sk::KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = sk::KEY_PGUP;
	io.KeyMap[ImGuiKey_PageDown] = sk::KEY_PGDN;
	io.KeyMap[ImGuiKey_Home] = sk::KEY_HOME;
	io.KeyMap[ImGuiKey_End] = sk::KEY_END;
	io.KeyMap[ImGuiKey_Delete] = sk::KEY_DEL;
	io.KeyMap[ImGuiKey_Backspace] = sk::KEY_BACKSP;
	io.KeyMap[ImGuiKey_Enter] = sk::KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = sk::KEY_ESC;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';

	return true;
}

void
ImGui_ImplRW_Shutdown(void)
{
}

static bool
ImGui_ImplRW_CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO &io = ImGui::GetIO();
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, nil);

	rw::Image *image;
	image = rw::Image::create(width, height, 32);
	image->allocate();
	for(int y = 0; y < height; y++)
		memcpy(image->pixels + image->stride*y, pixels + width*4* y, width*4);
	g_FontTexture = rw::Texture::create(rw::Raster::createFromImage(image));
	g_FontTexture->setFilter(rw::Texture::LINEAR);
	image->destroy();
	
	// Store our identifier
	io.Fonts->TexID = (void*)g_FontTexture;

	return true;
}

bool
ImGui_ImplRW_CreateDeviceObjects()
{
//	if(!g_pd3dDevice)
//		return false;
	if(!ImGui_ImplRW_CreateFontsTexture())
		return false;
	return true;
}

void
ImGui_ImplRW_NewFrame(float timeDelta)
{
	if(!g_FontTexture)
		ImGui_ImplRW_CreateDeviceObjects();

	ImGuiIO &io = ImGui::GetIO();

	io.DisplaySize = ImVec2(sk::globals.width, sk::globals.height);
	io.DeltaTime = timeDelta;

	io.KeyCtrl = io.KeysDown[sk::KEY_LCTRL] || io.KeysDown[sk::KEY_RCTRL];
	io.KeyShift = io.KeysDown[sk::KEY_LSHIFT] || io.KeysDown[sk::KEY_RSHIFT];
	io.KeyAlt = io.KeysDown[sk::KEY_LALT] || io.KeysDown[sk::KEY_RALT];
	io.KeySuper = false;

	if(io.WantSetMousePos)
		sk::SetMousePosition(io.MousePos.x, io.MousePos.y);

	ImGui::NewFrame();
}

sk::EventStatus
ImGuiEventHandler(sk::Event e, void *param)
{
	using namespace sk;

	ImGuiIO &io = ImGui::GetIO();
	MouseState *ms;
	rw::uint c;

	switch(e){
	case KEYDOWN:
		c = *(int*)param;
		if(c < 256)
			io.KeysDown[c] = 1;
		return EVENTPROCESSED;
	case KEYUP:
		c = *(int*)param;
		if(c < 256)
			io.KeysDown[c] = 0;
		return EVENTPROCESSED;
	case CHARINPUT:
		c = (rw::uint)(uintptr)param;
		io.AddInputCharacter((unsigned short)c);
		return EVENTPROCESSED;
	case MOUSEMOVE:
		ms = (MouseState*)param;
		io.MousePos.x = ms->posx;
		io.MousePos.y = ms->posy;
		return EVENTPROCESSED;
	case MOUSEBTN:
		ms = (MouseState*)param;
		io.MouseDown[0] = !!(ms->buttons & 1);
		io.MouseDown[2] = !!(ms->buttons & 2);
		io.MouseDown[1] = !!(ms->buttons & 4);
		return EVENTPROCESSED;
	}
	return EVENTPROCESSED;
}

/**
 * @brief Convierte una estructura rw::RGBA a ImVec4.
 *
 * Esta función convierte una estructura rw::RGBA a la representación equivalente en ImVec4 utilizada por ImGui.
 *
 * @param color La estructura rw::RGBA que se va a convertir.
 * @return El color convertido en formato ImVec4.
 */
ImVec4 
rwRGBAToImVec4(const rw::RGBA& color)
{
	return ImVec4(color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, color.alpha / 255.0f);
}

/**
 * @brief Convierte una estructura ImVec4 a rw::RGBA.
 *
 * Esta función convierte una estructura ImVec4 utilizada por imGui al formato rw:RGBA
 *
 * @param imColor La estructura ImVec4 que se va a convertir.
 * @return El color convertido en formato rw::RGBA.
 */
rw::RGBA
ImVec4TorwRGBA(const ImVec4& imColor)
{
	rw::RGBA color;
	color.red = static_cast<uint8_t>(imColor.x * 255.0f);
	color.green = static_cast<uint8_t>(imColor.y * 255.0f);
	color.blue = static_cast<uint8_t>(imColor.z * 255.0f);
	color.alpha = static_cast<uint8_t>(imColor.w * 255.0f);
	return color;
}
