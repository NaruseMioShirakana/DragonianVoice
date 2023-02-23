/*
* file: UISliderLayer.h
* info: 实现UISlider层
*
* Author: Maplespe(mapleshr@icloud.com) & NaruseMioYumemiShirakana
*
* date: 2023-1-2 Create
*/
#include "UISliderLayer.h"
#include "MainWindow.h"

namespace Mui 
{
	UISliderLayer::UISliderLayer(MHelper::MuiXML* mxml, UIControl* parent)
	{
		m_xml = mxml;
		m_parent = parent;
		m_anicls = new MAnimation(parent->GetParentWin());

		const int height = ttsUI::m_wndSize.height - 30;

		const std::wstring xml = LR"(
		<root>
		<UIControl frame="0,30,)" + std::to_wstring(ttsUI::m_wndSize.width) + L"," + std::to_wstring(height) + LR"(" bgColor="255,255,255,160"
		 name="uisliderpage" visible="false">
			<UIControl bgColor="255,255,255,255" frameColor="150,150,150,255" frameWidth="1" frameRound="10" align="4"
			size="1000,600">
				<UIButton frame="r_150,b_70,80,30" skin="skin_button" fontStyle="style" autoSize="false" textAlign="5" fontSize="14"
				animate="true" name="uislider_ret" text="完成" />
				<UIEditBox frame="r+15,=,100,20" skin="skin_editbox" name="edit" />
				
				<UILabel fontStyle="style" fontSize="16" name="uislidertitle" pos="15,15" text="页面标题" />
				<UIPanel name="uislider_panel" skin="skin_panel" frameColor="150,150,150,255" frameWidth="1"
				 frame="15,50,970,485" skin1="skin_scroll" skin2="skin_scroll" barWidth="10" autoSize="false" />
			</UIControl>
		</UIControl>
		</root>
		)";

		if (!mxml->CreateUIFromXML(parent, xml))
			throw std::runtime_error(R"(界面创建失败 XML代码有误)");

		m_page = m_parent->Child(L"uisliderpage");
	}

	UISliderLayer::~UISliderLayer()
	{
		delete m_anicls;
	}

	void UISliderLayer::ShowLayer(const std::wstring& title, std::vector<itemParam>&& dstValue, callback&& hideCallback)
	{
		m_dstList = dstValue;
		m_callback = hideCallback;
		opened = true;
		//设置标题
		m_page->Child<UILabel>(L"uislidertitle")->SetProperty(UILabel::Label_Text, title);
		CreateItemList();

		m_page->SetAlpha(0);
		m_page->SetVisible(true);
		auto anifun = [this](const MAnimation::MathsCalc* calc, float percent)
		{
			const auto alpha = calc->calc(MAnimation::Default, 0, 255);
			m_page->SetAlpha((_m_byte)M_MIN(alpha, 255));
			return true;
		};
		m_anicls->CreateTask(anifun, 150);
	}

	void UISliderLayer::HideLayer()
	{
		auto anifun = [this](const MAnimation::MathsCalc* calc, float percent)
		{
			const auto alpha = calc->calc(MAnimation::Default, 255, 0);
			m_page->SetAlpha((_m_byte)M_MAX(alpha, 0));
			if (percent == 100.f)
			{
				ClearItemList();
				m_page->SetVisible(false);
				if (m_callback)
				{
					std::vector<float> list;
					list.reserve(m_dstList.size());
					for (auto& dst : m_dstList)
						list.push_back(dst.value);
					m_callback(list);
				}
				m_dstList.clear();
			}
			return true;
		};
		m_anicls->CreateTask(anifun, 150);
	}

	std::wstring UISliderLayer::cast_title(float value) const
	{
		const std::wstring str = std::to_wstring(value);
		return str.substr(0, str.find(L'.') + 2);
	}

	bool UISliderLayer::EventProc(UINotifyEvent event, UIControl* control, _m_param param)
	{
		if (MUIEVENT(Event_Mouse_LClick, L"uislider_ret"))
		{
			HideLayer();
			return true;
		}
		else if(event == Event_Slider_Change)
		{
			const auto iter = m_itemList.find((UISlider*)control);
			if (iter == m_itemList.end())
				return false;

			float value = 0.f;
			if (param != 0)
				value = float(param) / 10.f;

			m_dstList[iter->second.index].value = value;
			iter->second.value->SetProperty(UILabel::Label_Text, cast_title(value));
			return true;
		}

		return false;
	}

	void UISliderLayer::CreateItemList()
	{
		auto panel = m_page->Child<UIPanel>(L"uislider_panel");
		auto skin = m_xml->GetSkinStyle(L"skin_slider1");
		auto skin_btn = m_xml->GetSkinStyle(L"skin_sliderbtn");
		auto style = m_xml->GetFontStyle(L"style");

		int last = 0;
		int ix = 0;
		for (auto& dst : m_dstList)
		{
			UISlider::Property prop;
			prop.BottomShow = true;
			prop.Vertical = true;
			prop.MaxValue = 100;
			prop.MinValue = 0;
			prop.TrackInset = { 0, 10, 0, 0 };
			prop.m_skin = skin;
			prop.m_btnSkin = skin_btn;
			prop.Value = _m_uint(dst.value * 10.f);
			auto slider = new UISlider(panel, prop);
			slider->SetPos(last + 10, 30, true, true);
			slider->SetSize(18, 405, true, true);

			style.Text = dst.title;
			auto label = new UILabel(panel, style);
			label->SetPos(last + 12, 10, true, true);

			style.Text = cast_title(dst.value);
			auto value = new UILabel(panel, style);
			value->SetPos(last + 12, 440, true, true);

			item _item;
			_item.index = ix;
			_item.title = label;
			_item.value = value;

			m_itemList.insert(std::make_pair(slider, _item));

			ix++;
			last += 28;
		}
		int width = ix * 28;
		auto scale = m_page->GetParentWin()->GetWindowScale();

		width = int(float(width + 10) * scale.cx);

		auto prop = panel->GetProperty();
		prop.ScrollSize = UISize(width, panel->GetFrame().GetHeight() - 30);
		prop.ViewPos = UIPoint(0);
		panel->SetProperty(prop);
	}

	void UISliderLayer::ClearItemList()
	{
		const auto panel = m_page->Child<UIPanel>(L"uislider_panel");
		for (const auto& item : m_itemList)
		{
			panel->RemoveChildren(item.first);
			panel->RemoveChildren(item.second.title);
			panel->RemoveChildren(item.second.value);
			delete item.first;
			delete item.second.title;
			delete item.second.value;
		}
		m_itemList.clear();
		panel->SetAttribute(L"scrollSize", L"0,0");
	}

	VcConfigLayer::VcConfigLayer(MHelper::MuiXML* mxml, UIControl* parent)
	{
		m_xml = mxml;
		m_parent = parent;
		m_anicls = new MAnimation(parent->GetParentWin());

		const int height = ttsUI::m_wndSize.height - 30;

		const std::wstring xml = LR"(
		<root>
		<UIControl frame="0,30,)" + std::to_wstring(ttsUI::m_wndSize.width) + L"," + std::to_wstring(height) + LR"(" bgColor="255,255,255,160"
		 name="vcconfpage" visible="false">
			<UIControl bgColor="255,255,255,255" frameColor="150,150,150,255" frameWidth="1" frameRound="10" align="4"
			size="300,200">
				<UIButton frame="25,163,100,24" skin="skin_button" fontStyle="style" autoSize="false" textAlign="5" fontSize="14"
				animate="true" name="vcslider_ret" text="完成" />
				<UIButton frame="175,163,100,24" skin="skin_button" fontStyle="style" autoSize="false" textAlign="5" fontSize="14"
				animate="true" name="vcslider_retC" text="取消" />
				<UIEditBox frame="100,35,40,20" skin="skin_editbox" name="vcedit1" limitText="3" text="0" />
				<UIEditBox frame="100,60,40,20" skin="skin_editbox" name="vcedit2" number="true" limitText="4" text="30" />
				<UIEditBox frame="100,85,40,20" skin="skin_editbox" name="vcedit3" number="true" limitText="2" text="4" />
				<UIEditBox frame="100,110,40,20" skin="skin_editbox" name="vcedit4" number="true" limitText="5" text="4096" />
				<UIEditBox frame="100,135,40,20" skin="skin_editbox" name="vcedit5" number="true" limitText="4" text="512" />
				<UIEditBox frame="225,47,40,20" skin="skin_editbox" name="vcedit6" number="true" limitText="4" text="1000" />
				<UIEditBox frame="225,122,40,20" skin="skin_editbox" name="vcedit7" number="true" limitText="3" text="100" />
				<UILabel fontStyle="style" fontSize="12" pos="20,38" text="升降调：" />
				<UILabel fontStyle="style" fontSize="12" pos="20,63" text="切片阈值：" />
				<UILabel fontStyle="style" fontSize="12" pos="20,88" text="切片最短长度：" />
				<UILabel fontStyle="style" fontSize="12" pos="20,113" text="切片帧长度：" />
				<UILabel fontStyle="style" fontSize="12" pos="20,138" text="切片帧偏移：" />
				<UILabel fontStyle="style" fontSize="12" pos="175,50" text="Step：" />
				<UILabel fontStyle="style" fontSize="12" pos="175,125" text="Pndm：" />
				<UILabel fontStyle="style" fontSize="16" name="vcslidertitle" pos="15,8" text="页面标题" />
			</UIControl>
		</UIControl>
		</root>
		)";

		if (!mxml->CreateUIFromXML(parent, xml))
			throw std::runtime_error(R"(界面创建失败 XML代码有误)");

		m_page = m_parent->Child(L"vcconfpage");
	}

	VcConfigLayer::~VcConfigLayer()
	{
		delete m_anicls;
	}

	void VcConfigLayer::ShowLayer(const std::wstring& title, bool V2, callback&& hideCallback)
	{
		m_callback = hideCallback;
		opened = true;
		//设置标题
		m_page->Child<UILabel>(L"vcslidertitle")->SetProperty(UILabel::Label_Text, title);
		m_page->Child<UIEditBox>(L"vcedit6")->SetEnabled(V2);
		m_page->Child<UIEditBox>(L"vcedit7")->SetEnabled(V2);
		m_page->SetAlpha(0);
		m_page->SetVisible(true);
		auto anifun = [this](const MAnimation::MathsCalc* calc, float percent)
		{
			const auto alpha = calc->calc(MAnimation::Default, 0, 255);
			m_page->SetAlpha((_m_byte)M_MIN(alpha, 255));
			return true;
		};
		m_anicls->CreateTask(anifun, 150);
	}

	bool VcConfigLayer::EventProc(UINotifyEvent event, const UIControl* control, _m_param param)
	{
		if (MUIEVENT(Event_Mouse_LClick, L"vcslider_ret"))
		{
			HideLayer();
			return true;
		}
		if (MUIEVENT(Event_Mouse_LClick, L"vcslider_retC"))
		{
			HideLayer();
			canceled = true;
			return true;
		}
		return false;
	}

	void VcConfigLayer::HideLayer() const
	{
		auto anifun = [this](const MAnimation::MathsCalc* calc, float percent)
		{
			const auto alpha = calc->calc(MAnimation::Default, 255, 0);
			m_page->SetAlpha((_m_byte)M_MAX(alpha, 0));
			if (percent == 100.f)
			{
				CInfo cInfo;
				cInfo.keys = _wtoi(m_page->Child<UIEditBox>(L"vcedit1")->GetCurText().c_str());
				cInfo.threshold = _wtoi(m_page->Child<UIEditBox>(L"vcedit2")->GetCurText().c_str());
				cInfo.minLen = _wtoi(m_page->Child<UIEditBox>(L"vcedit3")->GetCurText().c_str());
				cInfo.frame_len = _wtoi(m_page->Child<UIEditBox>(L"vcedit4")->GetCurText().c_str());
				cInfo.frame_shift = _wtoi(m_page->Child<UIEditBox>(L"vcedit5")->GetCurText().c_str());
				cInfo.step = _wtoi64(m_page->Child<UIEditBox>(L"vcedit6")->GetCurText().c_str());
				cInfo.pndm = _wtoi64(m_page->Child<UIEditBox>(L"vcedit7")->GetCurText().c_str());
				m_page->SetVisible(false);
				if (m_callback)
					m_callback(cInfo);
			}
			return true;
		};
		m_anicls->CreateTask(anifun, 150);
	}
}