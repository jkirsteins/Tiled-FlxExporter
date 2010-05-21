#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
    class ProgressDialog;
}

class ProgressDialog : public QDialog {
    Q_OBJECT
public:
    ProgressDialog(QWidget *parent = 0);
    ~ProgressDialog();

    void setProgress(const unsigned char pctg) const;
    void setMaxProgress(const unsigned char val) const;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ProgressDialog *ui;
};

#endif // PROGRESSDIALOG_H
