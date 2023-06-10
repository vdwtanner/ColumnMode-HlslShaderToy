#include "pch.h"

using namespace CM_HlslShaderToy;

UILayout::UILayout(HWND parent, HMENU id, RECT rect, bool scrollable) : UIElement(parent, id),  m_scrollable(scrollable)
{
	m_windowRect = rect;
	m_hwnd = CreateWindow(L"CM_HlslShaderToy", NULL,
		WS_CHILD | WS_VISIBLE ,
		rect.left, rect.top,       //x,y
		rect.right - rect.left, rect.bottom - rect.top,   //width, height
		parent,
		id,
		NULL,
		NULL);

	if (m_hwnd == NULL)
	{
		DWORD err = GetLastError();
		if (MessageBoxHelper_FormattedBody(MB_ICONERROR | MB_YESNO, L"Error", L"Failed to create UILayout (id: %d) HWND with error code: %d.\nDebugBreak() here?", id, err) == IDYES)
		{
			DebugBreak();
		}
	}
}

void UILayout::Reposition(LPRECT pRect)
{
	UIElement::Reposition(pRect);
	ReLayout();
}

void UIVerticalLayout::ReLayout()
{
	UINT currentYOffset = 0;
	UINT width = m_windowRect.right - m_windowRect.left;
	for (auto& elem : m_elements)
	{
		UIDimension dim = elem->GetDesiredDimension();
		LayoutFlags flags = elem->GetLayoutFlags();
		RECT rect;
		rect.left = 0;
		rect.top = currentYOffset;
		rect.bottom = rect.top + dim.height;
		rect.right = rect.left + (flags & LayoutFlags::FLEX_WIDTH ? width : dim.width);
		elem->Reposition(&rect);

		currentYOffset += rect.bottom + m_padding;
	}
}
