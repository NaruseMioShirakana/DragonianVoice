#include <iostream>
#include <windows.h>
#include "../include/Base.h"

using namespace libtts;

class Conv1D : public Module
{
public:
	Conv1D(Module* _Parent, const std::wstring& _Name) :
		Module(_Parent, _Name),
		RegisterLayer(weight),
		RegisterLayer(bias)
	{}
private:
	Parameter weight;
	Parameter bias;
};

class Linear : public Module
{
public:
	Linear(Module* _Parent, const std::wstring& _Name) :
		Module(_Parent, _Name),
		RegisterLayer(weight),
		RegisterLayer(bias)
	{}
private:
	Parameter weight;
	Parameter bias;
};

class ConvBlock : public Module
{
public:
	ConvBlock(Module* _Parent, const std::wstring& _Name) :
		Module(_Parent, _Name),
		RegisterLayer(in_conv),
		RegisterLayer(block),
		RegisterLayer(out_conv)
	{
		block = {
			LayerItem(Conv1D),
			LayerItem(Linear),
			LayerItem(Conv1D),
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
		RegisterLayer(conv_pre),
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
	Sequential block_out;
};

int main()
{
	auto a = MyNet(nullptr, L"");
	std::cout << UnicodeToByte(a.DumpLayerNameInfo());
	system("pause");
	return 0;
}