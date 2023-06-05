#include "ttseditor.h"
#include "ui_ttseditor.h"
#include <QWheelEvent>

TTSEditor::TTSEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TTSEditor)
{
    ui->setupUi(this);
}

TTSEditor::~TTSEditor()
{
    delete ui;
}

void TTSEditor::wheelEvent(QWheelEvent* event)
{
    const QRectF editorRect = ui->TTSEditorScrollArea->rect();
    if(editorRect.contains(event->position()))
    {
    	const auto item = ui->TTSEditorScrollAreaWidgetContents;
	    if (event->modifiers() == Qt::ControlModifier) {
	    	const int delta = event->angleDelta().y();
			if (delta > 0)
			{
				if (int(item->size().width() * 1.1) < 10000)
					item->setFixedWidth(int(item->size().width() * 1.1));
				else
					item->setFixedWidth(10000);
			}
			else if (delta < 0)
			{
				if (int(item->size().width() * 0.9) > ui->TTSEditorScrollArea->size().width())
					item->setFixedWidth(int(item->size().width() * 0.9));
				else
					item->setFixedWidth(ui->TTSEditorScrollArea->size().width());
			}
	    }
		ui->TTSEditorScrollAreaWidgetContents->update();
    }
}

void TTSEditor::paintEvent(QPaintEvent* event)
{
	ui->TTSEditorScrollArea->update();
	ui->TTSEditorScrollAreaWidgetContents->update();
}
