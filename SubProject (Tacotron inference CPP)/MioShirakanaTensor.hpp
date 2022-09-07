#pragma once
#ifndef SHIRAKANAMIOTENSOR
#define SHIRAKANAMIOTENSOR
#include <onnxruntime_cxx_api.h>
#include <cmath>
typedef int64_t int64;
typedef Ort::Value MTensor;
namespace STensor {
    /// <summary>
    /// 获取一个初始值为0的指定shape的张量
    /// </summary>
    /// <typeparam name="__ty">张量内部数据类型</typeparam>
    /// <param name="shape">张量的shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>初始值为0，指定shape的张量</returns>
    template <typename __ty>
    MTensor getZero(std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 获取一个初始值为0的指定shape的张量
    /// </summary>
    /// <typeparam name="__ty">张量内部数据类型</typeparam>
    /// <param name="shape">张量的shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>初始值为0，指定shape的张量</returns>
    template <typename __ty>
    MTensor getZero(std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 获取一个指定shape，初始值为num的张量
    /// </summary>
    /// <typeparam name="__ty">张量内部数据类型</typeparam>
    /// <param name="num">张量的初始值</param>
    /// <param name="shape">张量的shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>指定shape，初始值为num的张量</returns>
    template <typename __ty>
    MTensor getNumTensor(__ty num, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 获取一个指定shape，初始值为num的张量
    /// </summary>
    /// <typeparam name="__ty">张量内部数据类型</typeparam>
    /// <param name="num">张量的初始值</param>
    /// <param name="shape">张量的shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>指定shape，初始值为num的张量</returns>
    template <typename __ty>
    MTensor getNumTensor(__ty num, std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 将维数中的1去除，并拷贝构造一个新的张量
    /// </summary>
    /// <typeparam name="__ty">张量的类型</typeparam>
    /// <param name="inputTensor">需要处理的张量</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>维数中的1去除后的一个新的张量</returns>
    template <typename __ty>
    MTensor squeezeCopy(MTensor& inputTensor, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 将维数中的1去除，并拷贝构造一个新的张量
    /// </summary>
    /// <typeparam name="__ty">张量的类型</typeparam>
    /// <param name="inputTensor">需要处理的张量</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>维数中的1去除后的一个新的张量</returns>
    template <typename __ty>
    MTensor squeezeCopy(MTensor&& inputTensor, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 将维数中的1去除，不拷贝构造
    /// </summary>
    /// <typeparam name="__ty">张量的类型</typeparam>
    /// <param name="inputTensor">需要处理的张量</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>维数中的1去除后的一个张量</returns>
    template <typename __ty>
    MTensor squeezeNoCopy(MTensor& inputTensor, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 将维数中的1去除，不拷贝构造
    /// </summary>
    /// <typeparam name="__ty">张量的类型</typeparam>
    /// <param name="inputTensor">需要处理的张量</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>维数中的1去除后的一个张量</returns>
    template <typename __ty>
    MTensor squeezeNoCopy(MTensor&& inputTensor, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 判断张量中数据是否小于等于Other，若小于等于则返回张量对应位置为True
    /// </summary>
    /// <typeparam name="__ty">张量数据类型</typeparam>
    /// <param name="inputTensor">输入张量</param>
    /// <param name="Other">比较数据</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>一个布尔张量</returns>
    template <typename __ty>
    MTensor leCpp(MTensor& inputTensor, __ty Other, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 判断张量中数据是否小于等于Other，若小于等于则返回张量对应位置为True
    /// </summary>
    /// <typeparam name="__ty">张量数据类型</typeparam>
    /// <param name="inputTensor">输入张量</param>
    /// <param name="Other">比较数据</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>一个布尔张量</returns>
    template <typename __ty>
    MTensor leCpp(MTensor&& inputTensor, __ty Other, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 判断张量中数据是否小于等于Other中对应位置的张量，若小于等于则返回张量对应位置为True
    /// </summary>
    /// <typeparam name="__ty">张量数据类型</typeparam>
    /// <param name="inputTensor">输入张量</param>
    /// <param name="Other">比较数据</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>一个布尔张量</returns>
    template <typename __ty>
    MTensor leCpp(MTensor& inputTensor, MTensor& Other, Ort::MemoryInfo& memoryInfo);
    /// <summary>
/// 判断张量中数据是否小于等于Other中对应位置的张量，若小于等于则返回张量对应位置为True
/// </summary>
/// <typeparam name="__ty">张量数据类型</typeparam>
/// <param name="inputTensor">输入张量</param>
/// <param name="Other">比较数据</param>
/// <param name="memoryInfo">设备</param>
/// <returns>一个布尔张量</returns>
    template <typename __ty>
    MTensor leCpp(MTensor&& inputTensor, MTensor& Other, Ort::MemoryInfo& memoryInfo);
    /// <summary>
/// 判断张量中数据是否小于等于Other中对应位置的张量，若小于等于则返回张量对应位置为True
/// </summary>
/// <typeparam name="__ty">张量数据类型</typeparam>
/// <param name="inputTensor">输入张量</param>
/// <param name="Other">比较数据</param>
/// <param name="memoryInfo">设备</param>
/// <returns>一个布尔张量</returns>
    template <typename __ty>
    MTensor leCpp(MTensor& inputTensor, MTensor&& Other, Ort::MemoryInfo& memoryInfo);
    /// <summary>
/// 判断张量中数据是否小于等于Other中对应位置的张量，若小于等于则返回张量对应位置为True
/// </summary>
/// <typeparam name="__ty">张量数据类型</typeparam>
/// <param name="inputTensor">输入张量</param>
/// <param name="Other">比较数据</param>
/// <param name="memoryInfo">设备</param>
/// <returns>一个布尔张量</returns>
    template <typename __ty>
    MTensor leCpp(MTensor&& inputTensor, MTensor&& Other, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 使用数组构造指定shape的张量
    /// </summary>
    /// <typeparam name="__ty">数据类型</typeparam>
    /// <param name="inputArray">输入数组</param>
    /// <param name="shape">张量shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>元素为数组中元素，指定shape的张量</returns>
    template <typename __ty>
    MTensor fromArray(__ty* inputArray, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 使用数组构造指定shape的张量
    /// </summary>
    /// <typeparam name="__ty">数据类型</typeparam>
    /// <param name="inputArray">输入数组</param>
    /// <param name="shape">张量shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>元素为数组中元素，指定shape的张量</returns>
    template <typename __ty>
    MTensor fromArray(const __ty* inputArray, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 使用数组构造指定shape的张量
    /// </summary>
    /// <typeparam name="__ty">数据类型</typeparam>
    /// <param name="inputArray">输入数组</param>
    /// <param name="shape">张量shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>元素为数组中元素，指定shape的张量</returns>
    template <typename __ty>
    MTensor fromArray(__ty* inputArray, std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 使用数组构造指定shape的张量
    /// </summary>
    /// <typeparam name="__ty">数据类型</typeparam>
    /// <param name="inputArray">输入数组</param>
    /// <param name="shape">张量shape</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>元素为数组中元素，指定shape的张量</returns>
    template <typename __ty>
    MTensor fromArray(const __ty* inputArray, std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// Torch Sigmoid
    /// </summary>
    /// <typeparam name="__ty"></typeparam>
    /// <param name="input"></param>
    /// <param name="memoryInfo"></param>
    /// <returns></returns>
    template <typename __ty>
    MTensor sigmoid(MTensor& input, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// Torch Sigmoid
    /// </summary>
    /// <typeparam name="__ty"></typeparam>
    /// <param name="input"></param>
    /// <param name="memoryInfo"></param>
    /// <returns></returns>
    template <typename __ty>
    MTensor sigmoid(MTensor&& input, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 合并两个张量
    /// </summary>
    /// <typeparam name="__ty">张量数据类型</typeparam>
    /// <param name="tensorA">A</param>
    /// <param name="tensorB">B</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>合并后的张量（A的数据链接在B后）</returns>
    template <typename __ty>
    MTensor cat(MTensor& tensorA, MTensor& tensorB, int64 D, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 合并两个张量
    /// </summary>
    /// <typeparam name="__ty">张量数据类型</typeparam>
    /// <param name="tensorA">A</param>
    /// <param name="tensorB">B</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>合并后的张量（A的数据链接在B后）</returns>
    template <typename __ty>
    MTensor cat(MTensor& tensorA, MTensor&& tensorB, int64 D, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 合并两个张量
    /// </summary>
    /// <typeparam name="__ty">张量数据类型</typeparam>
    /// <param name="tensorA">A</param>
    /// <param name="tensorB">B</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>合并后的张量（A的数据链接在B后）</returns>
    template <typename __ty>
    MTensor cat(MTensor&& tensorA, MTensor& tensorB, int64 D, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 合并两个张量
    /// </summary>
    /// <typeparam name="__ty">张量数据类型</typeparam>
    /// <param name="tensorA">A</param>
    /// <param name="tensorB">B</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>合并后的张量（A的数据链接在B后）</returns>
    template <typename __ty>
    MTensor cat(MTensor&& tensorA, MTensor&& tensorB, int64 D, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 升维
    /// </summary>
    /// <typeparam name="__ty">类型</typeparam>
    /// <param name="tensor">升维的张量</param>
    /// <param name="D">升维的下标</param>
    /// <param name="C">升维的数量</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>拷贝构造张量</returns>
    template <typename __ty>
    MTensor unsqueezeCopy(MTensor& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 升维
    /// </summary>
    /// <typeparam name="__ty">类型</typeparam>
    /// <param name="tensor">升维的张量</param>
    /// <param name="D">升维的下标</param>
    /// <param name="C">升维的数量</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>拷贝构造张量</returns>
    template <typename __ty>
    MTensor unsqueezeCopy(MTensor&& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 升维
    /// </summary>
    /// <typeparam name="__ty">类型</typeparam>
    /// <param name="tensor">升维的张量</param>
    /// <param name="D">升维的下标</param>
    /// <param name="C">升维的数量</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>不拷贝构造张量</returns>
    template <typename __ty>
    MTensor unsqueezeNoCopy(MTensor& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo);
    /// <summary>
    /// 升维
    /// </summary>
    /// <typeparam name="__ty">类型</typeparam>
    /// <param name="tensor">升维的张量</param>
    /// <param name="D">升维的下标</param>
    /// <param name="C">升维的数量</param>
    /// <param name="memoryInfo">设备</param>
    /// <returns>不拷贝构造张量</returns>
    template <typename __ty>
    MTensor unsqueezeNoCopy(MTensor&& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo);
    MTensor cat1(MTensor& tensorA, MTensor&& tensorB, Ort::MemoryInfo& memoryInfo);
}


template <typename __ty>
MTensor STensor::getZero(std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    memset(data, static_cast<__ty>(0), sizeof(__ty) * tensorSize);
    return MTensor::CreateTensor(memoryInfo, data, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::getZero(std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    memset(data, static_cast<__ty>(0), sizeof(__ty) * tensorSize);
    return MTensor::CreateTensor(memoryInfo, data, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::getNumTensor(__ty num, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(data + i) = num;
    return MTensor::CreateTensor(memoryInfo, data, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::getNumTensor(__ty num, std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(data + i) = num;
    return MTensor::CreateTensor(memoryInfo, data, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::squeezeCopy(MTensor& inputTensor, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shape;
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    for (int64 i : inputTensor.GetTensorTypeAndShapeInfo().GetShape())
        if (i - 1)shape.push_back(i);
    __ty* newTensorData = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (newTensorData == NULL) throw std::exception("内存不足");
    memcpy(newTensorData, inputTensor.GetTensorData<__ty>(), sizeof(__ty) * tensorSize);
    return MTensor::CreateTensor<__ty>(
        memoryInfo, newTensorData, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::squeezeCopy(MTensor&& inputTensor, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shape;
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    for (int64 i : inputTensor.GetTensorTypeAndShapeInfo().GetShape())
        if (i - 1)shape.push_back(i);
    __ty* newTensorData = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (newTensorData == NULL) throw std::exception("内存不足");
    memcpy(newTensorData, inputTensor.GetTensorData<__ty>(), sizeof(__ty) * tensorSize);
    return MTensor::CreateTensor<__ty>(
        memoryInfo, newTensorData, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::squeezeNoCopy(MTensor& inputTensor, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shape;
    for (int64 i : inputTensor.GetTensorTypeAndShapeInfo().GetShape())
        if (i - 1)shape.push_back(i);
    return MTensor::CreateTensor<__ty>(
        memoryInfo, inputTensor.GetTensorMutableData<__ty>(), inputTensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::squeezeNoCopy(MTensor&& inputTensor, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shape;
    for (int64 i : inputTensor.GetTensorTypeAndShapeInfo().GetShape())
        if (i - 1)shape.push_back(i);
    return MTensor::CreateTensor<__ty>(
        memoryInfo, inputTensor.GetTensorMutableData<__ty>(), inputTensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::leCpp(MTensor& inputTensor, __ty Other, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    bool* boolTensorData = (bool*)malloc(sizeof(bool) * tensorSize);
    if (boolTensorData == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(boolTensorData + i) = inputTensor.GetTensorData<__ty>()[i] <= Other;
    return MTensor::CreateTensor<bool>(
        memoryInfo, boolTensorData, tensorSize, inputTensor.GetTensorTypeAndShapeInfo().GetShape().data(), inputTensor.GetTensorTypeAndShapeInfo().GetShape().size());
}
template <typename __ty>
MTensor STensor::leCpp(MTensor&& inputTensor, __ty Other, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    bool* boolTensorData = (bool*)malloc(sizeof(bool) * tensorSize);
    if (boolTensorData == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(boolTensorData + i) = inputTensor.GetTensorData<__ty>()[i] <= Other;
    return MTensor::CreateTensor<bool>(
        memoryInfo, boolTensorData, tensorSize, inputTensor.GetTensorTypeAndShapeInfo().GetShape().data(), inputTensor.GetTensorTypeAndShapeInfo().GetShape().size());
}
template <typename __ty>
MTensor STensor::leCpp(MTensor& inputTensor, MTensor& Other, Ort::MemoryInfo& memoryInfo) {
    if (inputTensor.GetTensorTypeAndShapeInfo().GetShape().size() != Other.GetTensorTypeAndShapeInfo().GetShape().size()) throw std::exception("shape不同");
    for (size_t i = 0; i < inputTensor.GetTensorTypeAndShapeInfo().GetShape().size(); i++)
        if (inputTensor.GetTensorTypeAndShapeInfo().GetShape()[i] != Other.GetTensorTypeAndShapeInfo().GetShape()[i]) throw std::exception("shape不同");
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    bool* boolTensorData = (bool*)malloc(sizeof(bool) * tensorSize);
    if (boolTensorData == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(boolTensorData + i) = inputTensor.GetTensorData<__ty>()[i] <= Other.GetTensorData<__ty>()[i];
    return MTensor::CreateTensor<bool>(
        memoryInfo, boolTensorData, tensorSize, inputTensor.GetTensorTypeAndShapeInfo().GetShape().data(), inputTensor.GetTensorTypeAndShapeInfo().GetShape().size());
}
template <typename __ty>
MTensor STensor::leCpp(MTensor&& inputTensor, MTensor& Other, Ort::MemoryInfo& memoryInfo) {
    if (inputTensor.GetTensorTypeAndShapeInfo().GetShape().size() != Other.GetTensorTypeAndShapeInfo().GetShape().size()) throw std::exception("shape不同");
    for (size_t i = 0; i < inputTensor.GetTensorTypeAndShapeInfo().GetShape().size(); i++)
        if (inputTensor.GetTensorTypeAndShapeInfo().GetShape()[i] != Other.GetTensorTypeAndShapeInfo().GetShape()[i]) throw std::exception("shape不同");
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    bool* boolTensorData = (bool*)malloc(sizeof(bool) * tensorSize);
    if (boolTensorData == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(boolTensorData + i) = inputTensor.GetTensorData<__ty>()[i] <= Other.GetTensorData<__ty>()[i];
    return MTensor::CreateTensor<bool>(
        memoryInfo, boolTensorData, tensorSize, inputTensor.GetTensorTypeAndShapeInfo().GetShape().data(), inputTensor.GetTensorTypeAndShapeInfo().GetShape().size());
}
template <typename __ty>
MTensor STensor::leCpp(MTensor& inputTensor, MTensor&& Other, Ort::MemoryInfo& memoryInfo) {
    if (inputTensor.GetTensorTypeAndShapeInfo().GetShape().size() != Other.GetTensorTypeAndShapeInfo().GetShape().size()) throw std::exception("shape不同");
    for (size_t i = 0; i < inputTensor.GetTensorTypeAndShapeInfo().GetShape().size(); i++)
        if (inputTensor.GetTensorTypeAndShapeInfo().GetShape()[i] != Other.GetTensorTypeAndShapeInfo().GetShape()[i]) throw std::exception("shape不同");
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    bool* boolTensorData = (bool*)malloc(sizeof(bool) * tensorSize);
    if (boolTensorData == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(boolTensorData + i) = inputTensor.GetTensorData<__ty>()[i] <= Other.GetTensorData<__ty>()[i];
    return MTensor::CreateTensor<bool>(
        memoryInfo, boolTensorData, tensorSize, inputTensor.GetTensorTypeAndShapeInfo().GetShape().data(), inputTensor.GetTensorTypeAndShapeInfo().GetShape().size());
}
template <typename __ty>
MTensor STensor::leCpp(MTensor&& inputTensor, MTensor&& Other, Ort::MemoryInfo& memoryInfo) {
    if (inputTensor.GetTensorTypeAndShapeInfo().GetShape().size() != Other.GetTensorTypeAndShapeInfo().GetShape().size()) throw std::exception("shape不同");
    for (size_t i = 0; i < inputTensor.GetTensorTypeAndShapeInfo().GetShape().size(); i++)
        if (inputTensor.GetTensorTypeAndShapeInfo().GetShape()[i] != Other.GetTensorTypeAndShapeInfo().GetShape()[i]) throw std::exception("shape不同");
    int64 tensorSize = inputTensor.GetTensorTypeAndShapeInfo().GetElementCount();
    bool* boolTensorData = (bool*)malloc(sizeof(bool) * tensorSize);
    if (boolTensorData == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(boolTensorData + i) = inputTensor.GetTensorData<__ty>()[i] <= Other.GetTensorData<__ty>()[i];
    return MTensor::CreateTensor<bool>(
        memoryInfo, boolTensorData, tensorSize, inputTensor.GetTensorTypeAndShapeInfo().GetShape().data(), inputTensor.GetTensorTypeAndShapeInfo().GetShape().size());
}
template <typename __ty>
MTensor STensor::fromArray(__ty* inputArray, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    return MTensor::CreateTensor<__ty>(memoryInfo, inputArray, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::fromArray(const __ty* inputArray, std::vector<int64>& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    return MTensor::CreateTensor<__ty>(memoryInfo, inputArray, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::fromArray(__ty* inputArray, std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    return MTensor::CreateTensor<__ty>(memoryInfo, inputArray, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::fromArray(const __ty* inputArray, std::vector<int64>&& shape, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = 1;
    for (int64 i : shape)
        tensorSize *= i;
    return MTensor::CreateTensor<__ty>(memoryInfo, inputArray, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::sigmoid(MTensor& input, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = input.GetTensorTypeAndShapeInfo().GetElementCount();
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(data+i)= (__ty)(1.0F / (1.0F + exp(0.0F - input.GetTensorData<__ty>()[i])));
    return MTensor::CreateTensor<__ty>(memoryInfo, data, tensorSize, input.GetTensorTypeAndShapeInfo().GetShape().data(), input.GetTensorTypeAndShapeInfo().GetShape().size());
}
template <typename __ty>
MTensor STensor::sigmoid(MTensor&& input, Ort::MemoryInfo& memoryInfo) {
    int64 tensorSize = input.GetTensorTypeAndShapeInfo().GetElementCount();
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL)throw std::exception("内存不足");
    for (int64 i = 0; i < tensorSize; i++)
        *(data + i) = (__ty)(1.0F / (1.0F + exp(0.0F - input.GetTensorData<__ty>()[i])));
    return MTensor::CreateTensor<__ty>(memoryInfo, data, tensorSize, input.GetTensorTypeAndShapeInfo().GetShape().data(), input.GetTensorTypeAndShapeInfo().GetShape().size());
}
template <typename __ty>
MTensor STensor::cat(MTensor& tensorA, MTensor& tensorB, int64 D, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shapeA = tensorA.GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> shapeB = tensorB.GetTensorTypeAndShapeInfo().GetShape();
    if (shapeA.size() != shapeB.size() || D > shapeA.size() - 1) throw std::exception("shape不同");
    for (int64 i = 0; i < shapeA.size(); i++) {
        if (i == D) continue;
        if (shapeA[i] != shapeA[i]) throw std::exception("shape不同");
    }
    std::vector<int64> shape;
    int64 offsetA = 1, offsetB = 1, cpyCount = 1, tensorSize = 1, cpyCountA = 1, cpuCountB = 1;
    for (int64 i = 0; i < shapeA.size(); i++) {
        shape.push_back(shapeA[i] + shapeB[i]);
        tensorSize *= shape[i];
        offsetA *= i && (i > D - 1) ? shapeA[i] : 1;
        offsetB *= i && (i > D - 1) ? shapeB[i] : 1;
        cpyCountA *= (i > D - 1) ? shapeA[i] : 1;
        cpuCountB *= (i > D - 1) ? shapeB[i] : 1;
        cpyCount *= i < D ? shapeA[i] : 1;
    }
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < cpyCount; i++) {
        memcpy((data + i * (offsetA + offsetB)), (tensorA.GetTensorData<__ty>() + i * offsetA), sizeof(__ty) * cpyCountA);
        memcpy((data + i * (offsetA + offsetB))+ cpyCountA, (tensorB.GetTensorData<__ty>() + i * offsetB), sizeof(__ty) * cpuCountB);
    }
    free(tensorA.GetTensorMutableData<float>());
    return MTensor::CreateTensor<__ty>(memoryInfo, data, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::cat(MTensor& tensorA, MTensor&& tensorB, int64 D, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shapeA = tensorA.GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> shapeB = tensorB.GetTensorTypeAndShapeInfo().GetShape();
    if (shapeA.size() != shapeB.size() || D > shapeA.size() - 1) throw std::exception("shape不同");
    for (int64 i = 0; i < shapeA.size(); i++) {
        if (i == D) continue;
        if (shapeA[i] != shapeA[i]) throw std::exception("shape不同");
    }
    std::vector<int64> shape;
    int64 offsetA = 1, offsetB = 1, cpyCount = 1, tensorSize = 1, cpyCountA = 1, cpuCountB = 1;
    for (int64 i = 0; i < shapeA.size(); i++) {
        shape.push_back(shapeA[i] + shapeB[i]);
        tensorSize *= shape[i];
        offsetA *= i && (i > D - 1) ? shapeA[i] : 1;
        offsetB *= i && (i > D - 1) ? shapeB[i] : 1;
        cpyCountA *= (i > D - 1) ? shapeA[i] : 1;
        cpuCountB *= (i > D - 1) ? shapeB[i] : 1;
        cpyCount *= i < D ? shapeA[i] : 1;
    }
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < cpyCount; i++) {
        memcpy((data + i * (offsetA + offsetB)), (tensorA.GetTensorData<__ty>() + i * offsetA), sizeof(__ty) * cpyCountA);
        memcpy((data + i * (offsetA + offsetB)) + cpyCountA, (tensorB.GetTensorData<__ty>() + i * offsetB), sizeof(__ty) * cpuCountB);
    }
    free(tensorA.GetTensorMutableData<float>());
    return MTensor::CreateTensor<__ty>(memoryInfo, data, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::cat(MTensor&& tensorA, MTensor& tensorB, int64 D, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shapeA = tensorA.GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> shapeB = tensorB.GetTensorTypeAndShapeInfo().GetShape();
    if (shapeA.size() != shapeB.size() || D > shapeA.size() - 1) throw std::exception("shape不同");
    for (int64 i = 0; i < shapeA.size(); i++) {
        if (i == D) continue;
        if (shapeA[i] != shapeA[i]) throw std::exception("shape不同");
    }
    std::vector<int64> shape;
    int64 offsetA = 1, offsetB = 1, cpyCount = 1, tensorSize = 1, cpyCountA = 1, cpuCountB = 1;
    for (int64 i = 0; i < shapeA.size(); i++) {
        shape.push_back(shapeA[i] + shapeB[i]);
        tensorSize *= shape[i];
        offsetA *= i && (i > D - 1) ? shapeA[i] : 1;
        offsetB *= i && (i > D - 1) ? shapeB[i] : 1;
        cpyCountA *= (i > D - 1) ? shapeA[i] : 1;
        cpuCountB *= (i > D - 1) ? shapeB[i] : 1;
        cpyCount *= i < D ? shapeA[i] : 1;
    }
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < cpyCount; i++) {
        memcpy((data + i * (offsetA + offsetB)), (tensorA.GetTensorData<__ty>() + i * offsetA), sizeof(__ty) * cpyCountA);
        memcpy((data + i * (offsetA + offsetB)) + cpyCountA, (tensorB.GetTensorData<__ty>() + i * offsetB), sizeof(__ty) * cpuCountB);
    }
    free(tensorA.GetTensorMutableData<float>());
    return MTensor::CreateTensor<__ty>(memoryInfo, data, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::cat(MTensor&& tensorA, MTensor&& tensorB, int64 D, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> shapeA = tensorA.GetTensorTypeAndShapeInfo().GetShape();
    std::vector<int64> shapeB = tensorB.GetTensorTypeAndShapeInfo().GetShape();
    if (shapeA.size() != shapeB.size() || D > shapeA.size() - 1) throw std::exception("shape不同");
    for (int64 i = 0; i < shapeA.size(); i++) {
        if (i == D) continue;
        if (shapeA[i] != shapeA[i]) throw std::exception("shape不同");
    }
    std::vector<int64> shape;
    int64 offsetA = 1, offsetB = 1, cpyCount = 1, tensorSize = 1, cpyCountA = 1, cpuCountB = 1;
    for (int64 i = 0; i < shapeA.size(); i++) {
        shape.push_back(shapeA[i] + shapeB[i]);
        tensorSize *= shape[i];
        offsetA *= i && (i > D - 1) ? shapeA[i] : 1;
        offsetB *= i && (i > D - 1) ? shapeB[i] : 1;
        cpyCountA *= (i > D - 1) ? shapeA[i] : 1;
        cpuCountB *= (i > D - 1) ? shapeB[i] : 1;
        cpyCount *= i < D ? shapeA[i] : 1;
    }
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensorSize);
    if (data == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < cpyCount; i++) {
        memcpy((data + i * (offsetA + offsetB)), (tensorA.GetTensorData<__ty>() + i * offsetA), sizeof(__ty) * cpyCountA);
        memcpy((data + i * (offsetA + offsetB)) + cpyCountA, (tensorB.GetTensorData<__ty>() + i * offsetB), sizeof(__ty) * cpuCountB);
    }
    free(tensorA.GetTensorMutableData<float>());
    return MTensor::CreateTensor<__ty>(memoryInfo, data, tensorSize, shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::unsqueezeCopy(MTensor& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo) {
    std::vector shape = tensor.GetTensorTypeAndShapeInfo().GetShape();
    if (D > shape.size()) throw std::exception("下标越界");
    for (int i = 0; i < C; i++)
        shape.insert(shape.begin() + D, 1);
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensor.GetTensorTypeAndShapeInfo().GetElementCount());
    if (data == NULL) throw std::exception("内存不足");
    memcpy(data, tensor.GetTensorData<__ty>(), sizeof(__ty) * tensor.GetTensorTypeAndShapeInfo().GetElementCount());
    return MTensor::CreateTensor(memoryInfo, data, tensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::unsqueezeCopy(MTensor&& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo) {
    std::vector shape = tensor.GetTensorTypeAndShapeInfo().GetShape();
    if (D > shape.size()) throw std::exception("下标越界");
    for (int i = 0; i < C; i++)
        shape.insert(shape.begin() + D, 1);
    __ty* data = (__ty*)malloc(sizeof(__ty) * tensor.GetTensorTypeAndShapeInfo().GetElementCount());
    if (data == NULL) throw std::exception("内存不足");
    memcpy(data, tensor.GetTensorData<__ty>(), sizeof(__ty) * tensor.GetTensorTypeAndShapeInfo().GetElementCount());
    return MTensor::CreateTensor(memoryInfo, data, tensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::unsqueezeNoCopy(MTensor&& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo) {
    std::vector shape = tensor.GetTensorTypeAndShapeInfo().GetShape();
    if (D > shape.size()) throw std::exception("下标越界");
    for (int i = 0; i < C; i++)
        shape.insert(shape.begin() + D, 1);
    return MTensor::CreateTensor(memoryInfo, tensor.GetTensorMutableData<__ty>(), tensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
}
template <typename __ty>
MTensor STensor::unsqueezeNoCopy(MTensor& tensor, int64 D, int64 C, Ort::MemoryInfo& memoryInfo) {
    std::vector shape = tensor.GetTensorTypeAndShapeInfo().GetShape();
    if (D > shape.size()) throw std::exception("下标越界");
    for (int i = 0; i < C; i++)
        shape.insert(shape.begin() + D, 1);
    return MTensor::CreateTensor(memoryInfo, tensor.GetTensorMutableData<__ty>(), tensor.GetTensorTypeAndShapeInfo().GetElementCount(), shape.data(), shape.size());
}
MTensor STensor::cat1(MTensor& tensorA, MTensor&& tensorB, Ort::MemoryInfo& memoryInfo) {
    std::vector<int64> Shape = tensorA.GetTensorTypeAndShapeInfo().GetShape();
    Shape[2]++;
    int64 tensorSize = Shape[0] * Shape[1] * Shape[2];
    float* data = (float*)malloc(sizeof(float) * tensorSize);
    if (data == NULL) throw std::exception("内存不足");
    for (int64 i = 0; i < Shape[1]; i++) {
        memcpy(data + (i * Shape[2]), tensorA.GetTensorData<float>() + (i * (Shape[2] - 1)), (Shape[2] - 1) * sizeof(float));
        *(data + ((i * Shape[2]) + Shape[2] - 1)) = tensorB.GetTensorData<float>()[i];
    }
    return Ort::Value::CreateTensor<float>(memoryInfo, data, tensorSize, Shape.data(), Shape.size());
}

#endif