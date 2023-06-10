#pragma once

namespace CM_HlslShaderToy
{
	class UILayout : public UIElement
	{
	public:
		UILayout(HWND parent, HMENU id, RECT rect, bool scrollable = false);
		virtual void Reposition(LPRECT pRect) override;
		virtual void ReLayout() =0;

		// returns a non-owning pointer to the UIElement
		template<typename T>
		T* AddElement(T*&& element)
		{
			m_elements.push_back(std::unique_ptr<T>(std::move(element)));
			T* elem = static_cast<T*>(m_elements.back().get());
			elem->SetParentLayout(this);
			ReLayout();
			return elem;
		}

	protected:
		std::vector<std::unique_ptr<UIElement>> m_elements;
		bool m_scrollable;
		UINT m_padding = 5;
	};

	class UIVerticalLayout : public UILayout
	{
	public:
		UIVerticalLayout(HWND parent, HMENU id, RECT rect) : UILayout(parent, id, rect) {}


		// Inherited via UILayout
		virtual void ReLayout() override;

	};
}