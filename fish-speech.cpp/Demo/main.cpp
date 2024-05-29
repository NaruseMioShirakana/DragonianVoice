#include <iostream>
#include <windows.h>
#include "Module.h"

using namespace libtts;

class ConvBlock : public Module
{
public:
	ConvBlock(Module* _Parent, const std::wstring& _Name) :
		Module(_Parent, _Name),
		RegisterLayer(in_conv, { 114,514,8 }, nullptr),
		RegisterLayer(block),
		RegisterLayer(out_conv, { 1919,810,8 }, nullptr)
	{
		block = {
			LayerItem(Conv1D, { 1453,666,8 }, nullptr),
			LayerItem(Linear),
			LayerItem(Conv1D, { 514,0721,8 }, nullptr),
			LayerItem(Linear),
		};
	}
private:
	Conv1D in_conv;
	Sequential block;
	Conv1D out_conv;
};

class MyNet : public Module
{
public:
	MyNet(Module* _Parent, const std::wstring& _Name) :
		Module(_Parent, _Name),
		RegisterLayer(conv_pre, { 114,514,8 }, nullptr),
		RegisterLayer(block_out)
	{
		block_out = {
			LayerItem(ConvBlock),
			LayerItem(Linear),
			LayerItem(ConvBlock),
			LayerItem(Linear),
		};
	}
private:
	Conv1D conv_pre;
	ModuleList block_out;
};

int main()
{
	auto a = MyNet(nullptr, L"");
	std::cout << UnicodeToByte(a.DumpLayerNameInfo());
	system("pause");
	return 0;
}