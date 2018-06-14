#pragma once
struct ImGuiContext;
#include <Windows.h>

class ModInterface {
public:
	virtual int Init(ImGuiContext* context) = 0;
	virtual int UnInit(ImGuiContext* context) = 0;
	virtual int Render(ImGuiContext* context) = 0;
	virtual int TextureBegin() = 0;
	virtual void TextureEnd() = 0;
	virtual void TextureData(int index, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel) = 0;
	virtual bool GetTextureDirtyRect(int index, int dindex, RECT* rect) = 0;
	virtual void SetTexture(int index, void* texture) = 0;
	virtual bool UpdateFont(ImGuiContext* context) = 0;
	virtual bool Menu(bool* show) = 0;
};
