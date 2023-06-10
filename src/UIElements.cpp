#include "pch.h"
#include <Richedit.h>

using namespace CM_HlslShaderToy;


void UIElement::SetDesiredDimension(UIDimension dimension, LayoutFlags flags)
{
	m_desiredDimension = dimension;
	m_layoutFlags = flags;
	if (m_pParentLayout.has_value())
	{
		m_pParentLayout.value()->ReLayout();
	}
}

void UIElement::Reposition(LPRECT pRect)
{
	m_windowRect = *pRect;
	if(!MoveWindow(m_hwnd, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top, true))
	{
		DWORD err = GetLastError();
		if (IDYES == MessageBoxHelper_FormattedBody(MB_ICONERROR | MB_YESNO,
			L"Error", L"Failed to set position of UIElement (id:%d) HWND with error code: %d. DebugBreak here?", m_id, err))
		{
			DebugBreak();
		}
	}
}

UITextArea::UITextArea(HWND parent, HMENU id) : UIElement(parent, id)
{
	UIDimension defaultDim = { 200,50 };
	
	m_hwnd = CreateWindowEx(0, 
		MSFTEDIT_CLASS, NULL, WS_VISIBLE | WS_CHILD | ES_READONLY | ES_MULTILINE | WS_VSCROLL  | WS_BORDER,
		0, 0, defaultDim.width, defaultDim.height, parent, id, NULL, NULL);
	if (m_hwnd == NULL)
	{
		DWORD err = GetLastError();
		MessageBoxHelper_FormattedBody(MB_ICONERROR | MB_OK, L"Error", L"Failed to create TextArea HWND with error code: %d", err);
	}
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void UITextArea::Reposition(LPRECT pRect)
{
	UIElement::Reposition(pRect);
}


