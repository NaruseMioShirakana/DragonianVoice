#include "ttseditorblock.h"
#include "ui_ttseditorblock.h"

TTSEditorBlock::TTSEditorBlock(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TTSEditorBlock)
{
    ui->setupUi(this);
}

TTSEditorBlock::~TTSEditorBlock()
{
    delete ui;
}

std::wstring TTSEditorBlock::GetCurInfo() const
{
	return ui->TTSEditorBlockLayoutPh->text().toStdWString();
}