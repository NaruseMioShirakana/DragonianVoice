#include <iostream>
#include <windows.h>
#include "llama.h"

using namespace libtts;

int main()
{
	auto a = BaseTransformer(nullptr, L"", BaseModelArgs());
	std::cout << UnicodeToByte(a.DumpLayerNameInfo());
	system("pause");
	return 0;
}