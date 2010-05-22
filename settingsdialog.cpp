#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QMessageBox>
#include <QCompleter>
#include <QFileInfo>
#include <QTextStream>
#include <QScrollBar>

#include "tileset.h"
#include "tile.h"
#include "tilelayer.h"

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
 * Function fills the summary textbox about the actions
 * that will take place during export.
 */
const void SettingsDialog::generateSummary(const Tiled::Map *map) const
{
    QString h1 = "<p><span style=\"font-weight:600;\">%1</span></p><br/><br/>";
    QString p = "<p><span>%1</span></p><br/><br/>";
    QString errP = "<p><span style=\"color:#AA0000\">%1</span></p><br/><br/>";
    QString successP = "<p><span style=\"color:#00AA00\">%1</span></p><br/><br/>";
    QString critP = "<p><span style=\"font-weight:600;background-color:#AA0000;color:#FFFFFF\">%1</span></p><br/><br/>";
    QString warnP = "<p><span style=\"background-color:#AAAA00;color:#000000\">%1</span></p><br/><br/>";


    QString exportedLayers;
    QString unsupportedLayers;
    QString skippedLayers;

    bool variableTileSizes = false;
    bool unsupportedTileSizes = false;
    bool invalidMap = false;

    invalidMap = (map->tileWidth() != map->tileHeight());

    foreach (Tiled::Layer *layer, map->layers())
    {
        if (!layer->isVisible())
        {
            QTextStream(&skippedLayers) << layer->name() << "; ";
        }
        else if (layer->asObjectGroup())
        {
            QTextStream(&unsupportedLayers) << layer->name() << "; ";
        }
        else if (layer->asTileLayer())
        {
            QTextStream(&exportedLayers) << layer->name() << "; ";

            for (int i = 0; i < layer->width(); ++i)
            {
                for (int j = 0; j < layer->height(); ++j)
                {
                    Tiled::Tile *tile = layer->asTileLayer()->tileAt(i, j);
                    if (tile == NULL) continue;

                    if (tile->height() != tile->width())
                        unsupportedTileSizes = true;
                    if (tile->height() != map->height())
                    {
                        variableTileSizes = true;

                        if (tile->height() % map->height() != 0)
                            unsupportedTileSizes = true;
                    }
                }
            }

        }
    }

    this->ui->overviewEdit->insertHtml(h1.arg("Export Summary"));

    if (invalidMap)
        this->ui->overviewEdit->insertHtml(critP.arg("Critical error: map tiles are not square!"));

    if (unsupportedTileSizes)
        this->ui->overviewEdit->insertHtml(errP.arg("Invalid tilesheet tile dimensions detected (tiles must be square and their edge length must be a multiples of map tile edge length"));

    if (variableTileSizes)
        this->ui->overviewEdit->insertHtml(warnP.arg("Warning: detected tilesheet tiles with differing dimensions from map tiles. This may cause errors (feature not fully supported yet)."));

    this->ui->overviewEdit->insertHtml(p.arg(QString("<span style=\"font-weight:600;\">Exported class</span>: %1").arg("&lt;class name&gt;")));

    if (exportedLayers.isEmpty()) exportedLayers = "<span style=\"font-weight:600;\">no layers will be exported</span>";
    this->ui->overviewEdit->insertHtml(p.arg(QString("<span style=\"font-weight:600;\">Exported layers</span>: %1").arg(exportedLayers)));

    if (!skippedLayers.isEmpty())
        this->ui->overviewEdit->insertHtml(p.arg(QString("<span style=\"font-weight:600;\">Skipped layers</span>: %1").arg(skippedLayers)));

    if (!unsupportedLayers.isEmpty())
        this->ui->overviewEdit->insertHtml(errP.arg(QString("<span style=\"font-weight:600;\">Unsupported layers</span>: %1").arg(exportedLayers)));

    if (this->ui->genDerived->isEnabled())
        this->ui->overviewEdit->insertHtml(successP.arg("It is possible to auto-generate a derived class in the parent folder."));
    else
        this->ui->overviewEdit->insertHtml(errP.arg(
                "It is not possible to auto-generate a derived class in the parent folder. " +
                QString("The derived class might already exist or the output filename does not have the Base- prefix.")));

    this->ui->overviewEdit->verticalScrollBar()->setValue(-10);
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
