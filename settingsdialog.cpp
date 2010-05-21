#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QMessageBox>
#include <QCompleter>
#include <QFileInfo>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

/**
 * Function returns the selected package name
 */
const QString SettingsDialog::getPackageName() const
{
    return this->ui->packageBox->currentText();
}

const QString SettingsDialog::getDerivedFileName() const
{
    if (this->ui->genDerived->isChecked())
        return derivedFileName;

    return "";
}

const void SettingsDialog::enableDerivedClassOption(const QString &derivedFileName)
{
    QFileInfo fileInfo(derivedFileName);
    this->ui->genDerived->setText(
        QString("Generate derived class file (%1) in the parent directory")
            .arg(fileInfo.fileName())
    );

    if (!fileInfo.exists())
    {
        this->derivedFileName = derivedFileName;
        this->ui->genDerived->setEnabled(true);
    }
}

/**
 * @return Tilemap class name (FlxTilemap by default)
 */
const QString SettingsDialog::getTilemapClass() const
{
    return this->ui->tilemapClassEdit->text().isEmpty()
            ? this->ui->tilemapClassEdit->placeholderText()
            : this->ui->tilemapClassEdit->text();
}

bool SettingsDialog::exportCollisionData(const QString &name) const
{
    /*for (int i = 0; i < this->ui->listWidget->count(); ++i)
        if (this->ui->listWidget->item(i)->checkState() == Qt::Checked &&
            this->ui->listWidget->item(i)->text() == name) return true;*/
    return false;
}


void SettingsDialog::setPackageHints(const QStringList &list)
{
    if (list.isEmpty()) return;

    this->ui->packageBox->addItems(list);
    this->ui->packageBox->setAutoCompletion(true);

    /*this->ui->packageEdit->setText(list.first());
    this->ui->packageEdit->setCompleter(
            new QCompleter(list, this));*/
}

void SettingsDialog::setMap(const Tiled::Map *map)
{
    /*foreach (Tiled::Layer *layer, map->layers())
    {

        ui->listWidget->insertItem(0, layer->name());

        QListWidgetItem * item = ui->listWidget->item(0);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }*/
}

void SettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
