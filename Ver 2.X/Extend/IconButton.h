/*
*  MiaoUI Í¼±ê°´Å¥À©Õ¹
*
*  Author: Maple
*  date: 2021-7-31 Create
*  site: winmoes.com
*/
#pragma once
#include "..\framework.h"

namespace Mui {

	namespace Ctrl {

		class IconButton : public UIButton
		{
		public:
			IconButton(UIControl* parent, UIButton::Property config) : UIButton(parent, config)
			{
				UILabel::OffsetDraw = true;
			}
			~IconButton() { MSafeRelease(m_icon); }

			M_CLASSNAME(L"IconButton");

			virtual void SetAttribute(std::wstring AttribName, std::wstring Attrib, bool draw = true) override;

			void SetIcon(UIResource res, bool draw = true);

			void SetOffsetAndSize(UIPoint text, UIPoint icon, UISize iconSize, bool draw = true);

		protected:
			virtual void OnPaintProc(MRenderCmd* render, MPCRect clipRect, MPCRect destRect, bool cacheCanvas) override;

		private:
			MBitmap* m_icon = nullptr;
			UIPoint textOffset;
			UIPoint iconOffset;
			UISize iconSize;
		};
	}
}