#ifndef ProgressDialog_H
#define ProgressDialog_H

#include <QProgressDialog>

namespace Ui {
    class ProgressDialog;
}

class ProgressDialog : public QProgressDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = 0);
    ~ProgressDialog();

    void setProgress(const unsigned char pctg) const;
    void updateProgress(const unsigned char step = 1) const;
    void setMaxProgress(const unsigned char val) const;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ProgressDialog *ui;
};

#endif // ProgressDialog_H
