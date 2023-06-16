#include "moevssetting.h"
#include "ui_moevssetting.h"
#include "ModelBase.hpp"
#include <DXGI.h>

MoeVSSetting::MoeVSSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MoeVSSetting)
{
    ui->setupUi(this);
    const auto num_threads = std::thread::hardware_concurrency();
    for (unsigned i = 1; i <= num_threads; ++i)
        ui->SettingNumThreadComboBox->addItem(std::to_string(i).c_str());
    ui->SettingProvicerComboBox->addItem("CPU");
#ifdef MOEVSCUADPROVIDER
    ui->SettingProvicerComboBox->addItem("CUDA");
#endif // MOEVSCUADPROVIDER
#ifdef MOEVSDMLPROVIDER
    ui->SettingProvicerComboBox->addItem("DML");
#endif
    IDXGIFactory* pFactory;
    IDXGIAdapter* pAdapter;
    int iAdapterNum = 0;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
    if (FAILED(hr))
        return;
    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC adapterDesc;
        pAdapter->GetDesc(&adapterDesc);
        ui->SettingDeviceIDComboBox->addItem(to_byte_string(adapterDesc.Description).c_str());
        ++iAdapterNum;
    }
    FILE* _CONFIGFILE = nullptr;
    fopen_s(&_CONFIGFILE, "Config.mcfg", "rb");
    if(_CONFIGFILE)
    {
        //fread(,1,sizeof,file)
        fread(&InferClass::__MOESS_DEVICE, 1, sizeof(InferClass::OnnxModule::Device), _CONFIGFILE);
        fread(&InferClass::__MoeVSNumThreads, 1, sizeof(unsigned int), _CONFIGFILE);
        fread(&InferClass::__MoeVSGPUID, 1, sizeof(uint64_t), _CONFIGFILE);
        ui->SettingNumThreadComboBox->setCurrentIndex(InferClass::__MoeVSNumThreads - 1);
        ui->SettingDeviceIDComboBox->setCurrentIndex(InferClass::__MoeVSGPUID);
        ui->SettingProvicerComboBox->setCurrentIndex(int(InferClass::__MOESS_DEVICE));
        fclose(_CONFIGFILE);
    }
}

MoeVSSetting::~MoeVSSetting()
{
    delete ui;
}

void MoeVSSetting::on_SettingProvicerComboBox_currentIndexChanged(int value)
{
    if (value)
#ifdef MOEVSCUADPROVIDER
        InferClass::__MOESS_DEVICE = InferClass::OnnxModule::Device::CUDA;
#endif // MOEVSCUADPROVIDER
#ifdef MOEVSDMLPROVIDER
        InferClass::__MOESS_DEVICE = InferClass::OnnxModule::Device::DML;
#endif
    else
        InferClass::__MOESS_DEVICE = InferClass::OnnxModule::Device::CPU;
}

void MoeVSSetting::on_SettingNumThreadComboBox_currentIndexChanged(int value)
{
    InferClass::__MoeVSNumThreads = value + 1;
}

void MoeVSSetting::on_SettingDeviceIDComboBox_currentIndexChanged(int value)
{
    InferClass::__MoeVSGPUID = value;
}

void MoeVSSetting::closeEvent(QCloseEvent* event)
{
    FILE* _CONFIGFILE = nullptr;
    fopen_s(&_CONFIGFILE, "Config.mcfg", "wb");
    if (_CONFIGFILE)
    {
        fwrite(&InferClass::__MOESS_DEVICE, 1, sizeof(InferClass::OnnxModule::Device), _CONFIGFILE);
        fwrite(&InferClass::__MoeVSNumThreads, 1, sizeof(unsigned int), _CONFIGFILE);
        fwrite(&InferClass::__MoeVSGPUID, 1, sizeof(uint64_t), _CONFIGFILE);
        fclose(_CONFIGFILE);
    }
}