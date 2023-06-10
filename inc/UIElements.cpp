#include "pch.h"

using namespace CM_HlslShaderToy;

UITextArea::UITextArea(HWND parent, HMENU id) : UIElement(parent, id)
{
	UIDimension defaultDim = { 200,50 };
	HWND hwnd = CreateWindowEx(0, L"RICHEDIT", 0, 0, 0, 0, defaultDim.width, defaultDim.height, parent, id, NULL, NULL);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
	InitHwnd(hwnd);
}