#pragma once

namespace CM_HlslShaderToy
{
	enum LayoutFlags
	{
		NONE = 0x0,
		FLEX_WIDTH = 0x1,	// When supported, fits to layout width
		FLEX_HEIGHT = 0x2,	// when supported, fits to layout height

		// bitmasks
		FLEX_WH = FLEX_WIDTH | FLEX_HEIGHT
	};

	struct UIDimension
	{
		UINT width;
		UINT height;
	};

	class UIElement
	{
		friend class UILayout;
	public:
		UIElement(HWND parent, HMENU id) : m_parentHwnd(parent), m_id(id) {}
		
		const HWND GetParentHwnd() const { return m_parentHwnd; }
		const bool TryGetParentLayout(__out UILayout*& ppLayout) const
		{
			if (m_pParentLayout.has_value()) { ppLayout = m_pParentLayout.value(); }
			return m_pParentLayout.has_value();
		}
		
		const HMENU GetId() const { return m_id; }
		const HWND GetHwnd() const { return m_hwnd; }
		const RECT& GetRect() const { return m_windowRect; }
		const UIDimension& GetDesiredDimension() const { return m_desiredDimension; }
		const LayoutFlags GetLayoutFlags() const { return m_layoutFlags; }
		
		void SetDesiredDimension(UIDimension dimension, LayoutFlags flags);
		virtual void Reposition(LPRECT pRect);

	private:
		void SetParentLayout(UILayout* layout) { m_pParentLayout.emplace(layout); }

	protected:
		RECT m_windowRect = {0,0,20,20};
		HWND m_hwnd;
		
	private:
		HWND m_parentHwnd;
		HMENU m_id;

		std::optional<UILayout*> m_pParentLayout;
		UIDimension m_desiredDimension = {20,20}; //desfaults to 20x20 so something will be seen if user doesn't set 
		LayoutFlags m_layoutFlags = LayoutFlags::NONE;
	};

	class UITextArea : public UIElement
	{
	public:
		UITextArea(HWND parent, HMENU id);
		void SetText(std::wstring text) { m_text = text; SetWindowText(m_hwnd, m_text.c_str()); UpdateWindow(GetParentHwnd()); }
		std::wstring GetText() const { return m_text; }

		// Inherited via UIElement
		virtual void Reposition(LPRECT pRect) override;

	private:
		std::wstring m_text;

		
	};
}