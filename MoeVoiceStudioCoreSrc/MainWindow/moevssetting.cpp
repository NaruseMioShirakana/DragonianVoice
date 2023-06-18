#include "moevssetting.h"
#include "ui_moevssetting.h"
#include "ModelBase.hpp"
#ifdef MOEVSDMLPROVIDER
#include <DXGI.h>
#endif

MoeVSSetting::MoeVSSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MoeVSSetting)
{
    ui->setupUi(this);
    ui->SettingNumThreadComboBox->setEnabled(false);
    ui->SettingDeviceIDComboBox->setEnabled(false);
    const auto num_threads = std::thread::hardware_concurrency();
    for (unsigned i = 1; i <= num_threads; ++i)
        ui->SettingNumThreadComboBox->addItem(std::to_string(i).c_str());
    ui->SettingProvicerComboBox->addItem("CPU");
    ui->SettingProvicerComboBox->addItem("CUDA");
#ifdef MOEVSDMLPROVIDER
    ui->SettingProvicerComboBox->addItem("DML");
#endif
    FILE* _CONFIGFILE = nullptr;
    fopen_s(&_CONFIGFILE, "Config.mcfg", "rb");
    if (_CONFIGFILE)
    {
        //fread(,1,sizeof,file)
        fread(&InferClass::__MOESS_DEVICE, 1, sizeof(InferClass::OnnxModule::Device), _CONFIGFILE);
        fread(&InferClass::__MoeVSNumThreads, 1, sizeof(unsigned int), _CONFIGFILE);
        fread(&InferClass::__MoeVSGPUID, 1, sizeof(uint64_t), _CONFIGFILE);
        ui->SettingProvicerComboBox->setCurrentIndex(int(InferClass::__MOESS_DEVICE));
        fclose(_CONFIGFILE);
    }
}

#ifdef MOEVSDMLPROVIDER
void MoeVSSetting::DmlAddDevice() const
{
    InferClass::__MoeVSGPUID = 0;
    ui->SettingNumThreadComboBox->setEnabled(false);
    ui->SettingDeviceIDComboBox->clear();
    ui->SettingDeviceIDComboBox->setEnabled(true);
    IDXGIFactory* pFactory;
    IDXGIAdapter* pAdapter;
    int iAdapterNum = 0;
    const HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
    if (FAILED(hr))
        return;
    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC adapterDesc;
        pAdapter->GetDesc(&adapterDesc);
        ui->SettingDeviceIDComboBox->addItem(to_byte_string(adapterDesc.Description).c_str());
        ++iAdapterNum;
    }
}
#endif

void MoeVSSetting::CudaAddDevice() const
{
    InferClass::__MoeVSGPUID = 0;
    ui->SettingDeviceIDComboBox->setEnabled(true);
    ui->SettingNumThreadComboBox->setEnabled(false);
    ui->SettingDeviceIDComboBox->clear();
#ifdef WIN32
    IDXGIFactory* pFactory;
    IDXGIAdapter* pAdapter;
    int iAdapterNum = 0;
    const HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
    if (FAILED(hr))
        return;
    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC adapterDesc;
        pAdapter->GetDesc(&adapterDesc);
        const auto desc_txt = to_byte_string(adapterDesc.Description);
        if (desc_txt.find("Nvidia") != std::string::npos || desc_txt.find("NVIDIA") != std::string::npos)
            ui->SettingDeviceIDComboBox->addItem(desc_txt.c_str());
        ++iAdapterNum;
    }
#endif
}

MoeVSSetting::~MoeVSSetting()
{
    delete ui;
}

void MoeVSSetting::on_SettingProvicerComboBox_currentIndexChanged(int value) const
{
	InferClass::__MOESS_DEVICE = InferClass::OnnxModule::Device(value);

    if (InferClass::__MOESS_DEVICE == InferClass::OnnxModule::Device::CUDA)
        CudaAddDevice();
#ifdef MOEVSDMLPROVIDER
    else if (InferClass::__MOESS_DEVICE == InferClass::OnnxModule::Device::DML)
        DmlAddDevice();
#endif
    else
    {
        ui->SettingDeviceIDComboBox->clear();
        ui->SettingDeviceIDComboBox->setEnabled(false);
        ui->SettingNumThreadComboBox->setCurrentIndex(int(InferClass::__MoeVSNumThreads - 1));
        ui->SettingNumThreadComboBox->setEnabled(true);
    }
    if (InferClass::__MOESS_DEVICE != InferClass::OnnxModule::Device::CPU)
        ui->SettingDeviceIDComboBox->setCurrentIndex(int(InferClass::__MoeVSGPUID));
}

void MoeVSSetting::on_SettingNumThreadComboBox_currentIndexChanged(int value)
{
    if(value>=0)
        InferClass::__MoeVSNumThreads = value + 1;
}

void MoeVSSetting::on_SettingDeviceIDComboBox_currentIndexChanged(int value)
{
    if (value >= 0)
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