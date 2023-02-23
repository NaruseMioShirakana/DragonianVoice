/*
* file: UISliderLayer.h
* info: 实现UISlider层
*
* Author: Maplespe(mapleshr@icloud.com) & NaruseMioYumemiShirakana
*
* date: 2023-1-2 Create
*/
#pragma once
#include "../framework.h"
#include <Mui_Animation.h>
#include <unordered_map>

namespace Mui 
{

	class UISliderLayer
	{
	public:
		UISliderLayer(MHelper::MuiXML* mxml, UIControl* parent);
		~UISliderLayer();

		using callback = std::function<void(const std::vector<float>&)>;

		struct itemParam
		{
			std::wstring title;
			float value = 0.f;
		public:
			itemParam(std::wstring&& t, float v)
			{
				this->title = t;
				this->value = v;
			}
			itemParam(const std::wstring& t, float v)
			{
				this->title = t;
				this->value = v;
			}
		};

		/*显示Layer
		* @param title - 页面标题
		* @param dstValue - 数组的size决定Slider的数量
		* @param hideCallback - 页面被关闭时调用回调
		*/
		void ShowLayer(const std::wstring& title, std::vector<itemParam>&& dstValue, callback&& hideCallback = nullptr);

		//事件处理 返回false代表事件没有被UISliderLayer处理
		bool EventProc(UINotifyEvent event, UIControl* control, _m_param param);
		bool opened = false;
	private:
		void CreateItemList();
		void ClearItemList();

		void HideLayer();

		std::wstring cast_title(float value) const;

		MHelper::MuiXML* m_xml = nullptr;
		UIControl* m_parent = nullptr;
		UIControl* m_page = nullptr;
		MAnimation* m_anicls = nullptr;

		std::vector<itemParam> m_dstList;
		callback m_callback = nullptr;
		
		struct item
		{
			UILabel* title = nullptr;
			UILabel* value = nullptr;
			int index = 0;
		};
		std::unordered_map<UISlider*, item> m_itemList;
	};

	class VcConfigLayer
	{
	public:
		VcConfigLayer(MHelper::MuiXML* mxml, UIControl* parent);
		~VcConfigLayer();

		struct CInfo
		{
			long keys = 0;
			long threshold = 30;
			long minLen = 5;
			long frame_len = 4 * 1024;
			long frame_shift = 512;
			int64_t pndm = 100;
			int64_t step = 1000;
		};

		using callback = std::function<void(const CInfo&)>;

		void ShowLayer(const std::wstring& title, bool V2, callback&& hideCallback = nullptr);

		bool EventProc(UINotifyEvent event, const UIControl* control, _m_param param);

		bool isCanceled()
		{
			if (canceled)
			{
				canceled = false;
				return true;
			}
			return false;
		}

		bool opened = false;
	private:
		bool canceled = false;
		void HideLayer() const;
		MHelper::MuiXML* m_xml = nullptr;
		UIControl* m_parent = nullptr;
		UIControl* m_page = nullptr;
		MAnimation* m_anicls = nullptr;
		callback m_callback = nullptr;
	};
}