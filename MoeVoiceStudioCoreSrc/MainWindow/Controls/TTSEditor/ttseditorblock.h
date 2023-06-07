#ifndef TTSEDITORBLOCK_H
#define TTSEDITORBLOCK_H

#include <QWidget>

namespace Ui {
class TTSEditorBlock;
}

class TTSEditorBlock : public QWidget
{
    Q_OBJECT

public:
    explicit TTSEditorBlock(QWidget *parent = nullptr);
    ~TTSEditorBlock() override;
    [[nodiscard]] std::wstring GetCurInfo() const;
private:
    Ui::TTSEditorBlock *ui;
};

#endif // TTSEDITORBLOCK_H
