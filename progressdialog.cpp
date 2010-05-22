#include "ProgressDialog.h"
#include "ui_ProgressDialog.h"

ProgressDialog::ProgressDialog(QWidget *parent) :
    QProgressDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::updateProgress(const unsigned char step) const
{
    ui->progressBar->setValue(ui->progressBar->value() + step);
}

void ProgressDialog::setProgress(unsigned char pctg) const
{
    ui->progressBar->setValue(pctg);
}

void ProgressDialog::setMaxProgress(unsigned char pctg) const
{
    ui->progressBar->setMaximum(pctg);
}

void ProgressDialog::changeEvent(QEvent *e)
{
    QProgressDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
