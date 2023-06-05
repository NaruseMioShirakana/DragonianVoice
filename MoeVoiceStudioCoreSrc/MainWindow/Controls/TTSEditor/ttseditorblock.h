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
    [[nodiscard]] std::pair<std::wstring, int64_t> GetCurInfo() const;
    void changeTone(const int64_t tone_)
    {
        tone = tone_;
    }
private:
    Ui::TTSEditorBlock *ui;
    int64_t tone = 0;
};

#endif // TTSEDITORBLOCK_H
