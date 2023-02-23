/*
*  MiaoUI 图标按钮扩展实现
*
*  Author: Maple
*  date: 2021-7-31 Create
*  site: winmoes.com
*/

#include "..\Extend\IconButton.h"

namespace Mui {

	namespace Ctrl {

		void IconButton::Register()
		{
			static auto method = [](UIControl* parent) {
				return new IconButton(parent, Property());
			};
			CtrlMgr::RegisterControl(ClassName, method);
		}

		void IconButton::SetAttribute(std::wstring AttribName, std::wstring Attrib, bool draw)
		{
			if (AttribName == L"offset")
			{
				std::vector<int> value;
				M_GetAttribValueInt(Attrib, value, 4);
				textOffset = UIPoint(value[0], value[1]);
				iconOffset = UIPoint(value[2], value[3]);
			}
			else if (AttribName == L"iconSize")
			{
				std::vector<int> value;
				M_GetAttribValueInt(Attrib, value, 2);
				iconSize = UISize(value[0], value[1]);
			}
			else if (AttribName == L"resIcon")
			{
				if (m_icon)
					MSafeRelease(m_icon);
				UIResource* icon = (UIResource*)M_StoLong64(Attrib, M_INFO_DBG);
				if (icon && m_data.Render)
				{
					if (icon->data && icon->size)
						m_icon = m_data.Render->CreateBitmap(*icon);
				}
				else if (!m_data.Render)
					M_OutErrorDbg(M_INFO_DBG, M_ERROR_NORENDER);
			}
			else {
				UIButton::SetAttribute(AttribName, Attrib, draw);
				return;
			}
			if (draw)
				UpdateDisplay();
		}

		void IconButton::OnPaintProc(MRenderCmd* render, MPCRect clipRect, MPCRect destRect, bool cacheCanvas)
		{
			_m_scale scale = { 1.f, 1.f };

			if (m_data.ParentWnd)
			{
				scale = m_data.ParentWnd->GetWindowScale();
			}

			//文字偏移
			UILabel::OffsetDrawRc = destRect->ToRect();
			UILabel::OffsetDrawRc.left += (int)round(scale.cx * (float)textOffset.x);
			UILabel::OffsetDrawRc.left += (int)round(scale.cy * (float)textOffset.y);

			UIButton::OnPaintProc(render, clipRect, destRect, cacheCanvas);

			if (m_icon)
			{
				UIRect dest = *destRect;
				dest.left += (int)round(scale.cx * (double)iconOffset.x);
				dest.top += (int)round(scale.cy * (double)iconOffset.y);

				dest.right = dest.left + (int)round(scale.cx * (float)iconSize.width);
				dest.bottom = dest.top + (int)round(scale.cy * (float)iconSize.height);

				render->DrawBitmap(m_icon, m_data.AlphaDst, dest);
			}
		}

		void IconButton::SetIcon(UIResource res, bool draw)
		{
			if (m_icon)
				MSafeRelease(m_icon);
			m_icon = m_data.Render->CreateBitmap(res);
			if (draw)
				UpdateDisplay();
		}

		void IconButton::SetOffsetAndSize(UIPoint text, UIPoint icon, UISize iconSize, bool draw)
		{
			textOffset = text;
			iconOffset = icon;
			this->iconSize = iconSize;
			if (draw)
				UpdateDisplay();
		}
	}
}