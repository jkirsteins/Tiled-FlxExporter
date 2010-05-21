#include "as3level.h"
#include "as3levelplaceholders.h"

#include <QFile>
#include <QByteArray>
#include <QPixmap>
#include <QPainter>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

#include "tilelayer.h"
#include "layer.h"
#include "tile.h"

#include "progressdialog.h"
#include "as3levelplaceholders.h"

using namespace Flx;

AS3Level::AS3Level()
{
    loadBlueprint();
}

/**
 * Function maps every tileset to its first GID
 * * Potentially not required (and not used)
 * @todo Get rid of this function
 */
void AS3Level::initTilesetGIDmap(const Tiled::Map *map)
{
    int firstGid = 1;
    foreach (const Tiled::Tileset *tileset, map->tilesets())
    {
        this->tilesetFirstGidMap[tileset] = firstGid;
        firstGid += tileset->tileCount();
    }
}

void AS3Level::saveLayerTilesheet(const QString &fileName,
                                  const Tiled::Map* map,
                                  const QMap<Tiled::Tile *, int> &idMap
                                  ) const
{
    // index 0 = NULL
    unsigned int xRatio = idMap.keys().at(1)->width() / map->tileWidth();
    unsigned int yRatio = idMap.keys().at(1)->height() / map->tileWidth();
    unsigned int partsPerTile = xRatio * yRatio;

    unsigned int tileCount = 1 + idMap.count() * partsPerTile;

    QPixmap pm(tileCount * map->tileWidth(),
               map->tileHeight());
    //pm.fill(Qt::transparent);
    QPainter painter(&pm);

    foreach (Tiled::Tile *tile, idMap.keys())
    {
        if (tile == NULL)
        {
            qDebug() << "Encountered NULL tile while saving tilesheet\n";
            continue;
        }

        for (unsigned int y = 0; y < yRatio; ++y)
        {
            for (unsigned int x = 0; x < xRatio; ++x)
            {
                int srcX = x * map->tileWidth();
                int srcY = y * map->tileHeight();
                QPixmap tilePart = tile->image().copy(srcX, srcY, map->tileWidth(), map->tileHeight());

                int tileId = idMap[tile] - 1;
                int tileStripStart = (1 + tileId * partsPerTile) * map->tileWidth();
                int tileStripIndex = y*yRatio + x;

                int targetX = tileStripStart + tileStripIndex * map->tileWidth();
                painter.drawPixmap(targetX, 0, map->tileWidth(), map->tileHeight(), tilePart);
            }
        }
//            tile->image().copy()
    }

    QString imageFile = QString("%1.png").arg(fileName);
    qDebug() << "Saving image to " << imageFile << "\n";
    pm.toImage().save(imageFile, 0, 100);
}

/**
 * Function generates and saves the ActionScript file
 */
bool AS3Level::save(const QString &fileName, const Tiled::Map *map) const
{
    ProgressDialog pd(NULL);
    pd.setMaxProgress(10);
    pd.open();

    QString buffer(this->blueprint);
    QFileInfo targetInfo(fileName);

    buffer = buffer.replace(FlxPlaceholders::PACKAGE_NAME, this->packageName);
    buffer = buffer.replace(FlxPlaceholders::CLASS_NAME, targetInfo.baseName());
    buffer = buffer.replace(FlxPlaceholders::TILEMAP_CLASS, this->tilemapClass);

    buffer = buffer.replace(FlxPlaceholders::GEN_BY, "FlxExporter v0.2");
    buffer = buffer.replace(FlxPlaceholders::GEN_DATE, "//! @todo Insert real date");


    this->generateTilemapDeclarations(map->layers(), buffer);
    this->generateGfxEmbedStatements(map->layers(), buffer);

    QString tileData;
    foreach (Tiled::Layer *layer, map->layers())
    {
        if (layer->asObjectGroup())
        {
            qCritical() << "Object layers not supported yet. Ignoring layer '" << layer->name() << "'\n";
            continue;
        }

        QMap<Tiled::Tile *, int> tileIdMap;
        this->generateLayerTileIDMap(layer, tileIdMap);
        this->saveLayerTilesheet(
            this->generateTilesheetPath(fileName, this->generateLayerVarName(layer)),
            map,
            tileIdMap
        );
        QTextStream(&tileData) << this->generateTileData(layer, tileIdMap);
    }

    buffer = buffer.replace(FlxPlaceholders::LAYER_TILE_DATA, tileData);


    //for (unsigned int i = 0; i < (0-1); ++i)
    //{
    //    pd.setProgress(i % 101);
    //}
    pd.close();

    QByteArray contentBytes(buffer.toLatin1());
    QFile output(fileName);
    if (output.open(QIODevice::WriteOnly))
    {
        output.write(contentBytes);
        output.flush();
        output.close();
        return true;
    }
    return false;
}

QString AS3Level::generateTilesheetPath(const QString &levelFileName,
                                     const QString &sheetFileName) const
{
    QFileInfo fileInfo(levelFileName);
    QDir targetDir(fileInfo.absolutePath());
    targetDir.mkdir("gfx");
    targetDir.cd("gfx");
    return targetDir.filePath(sheetFileName);
}

void AS3Level::generateLayerTileIDMap(Tiled::Layer *layer, QMap<Tiled::Tile *, int> &idMap) const
{
    unsigned int index = 0;

    idMap.clear();
    idMap[NULL] = index++;

    Tiled::TileLayer *tileLayer = layer->asTileLayer();
    if (!tileLayer)
    {
        qFatal("generateLayerTileIDMap: received invalid (non-tile) layer\n");
    }

    for (int i = 0; i < tileLayer->width(); ++i)
    {
        for (int j = 0; j < tileLayer->height(); ++j)
        {
            Tiled::Tile *tile = tileLayer->tileAt(i, j);
            if (!idMap.contains(tile))
            {
                idMap[tile] = index++;
            }
        }
    }
    idMap.remove(NULL);
}

QString AS3Level::generateLayerVarName(const Tiled::Layer *layer) const
{
    QString useName = layer->name().toAscii();
    useName[0] = useName[0].toLower();
    useName.remove(QChar(' '), Qt::CaseInsensitive);
    useName.remove(QChar(';'), Qt::CaseInsensitive);
    useName.remove(QChar(' '), Qt::CaseInsensitive);
    useName.replace(QChar('.'), QChar('_'), Qt::CaseInsensitive);
    return useName;
}

const QString AS3Level::generateTileData(Tiled::Layer *layer,
                                const QMap<Tiled::Tile *, int> idMap) const
{
    QString tileData;
    for (int j = 0; j < layer->height(); ++j)
    {
        for (int i = 0; i < layer->width(); ++i)
        {
            QTextStream(&tileData)
                    << idMap[layer->asTileLayer()->tileAt(i, j)]
                    << (i < (layer->width() - 1) ? "," : "");
        }
        if (j != (layer->height() - 1))
            QTextStream(&tileData) << "\\n";
    }

    QString result;
    QTextStream(&result)
            << "protected const " << this->generateLayerVarName(layer)
            << QString("TileData: String = \"%1\";\n\t\t").arg(tileData);

    return result;
}

void AS3Level::generateGfxEmbedStatements(const QList<Tiled::Layer *> &layers, QString &buffer) const
{
    QString embedStatements;
    foreach (Tiled::Layer *layer, layers)
    {
        QTextStream(&embedStatements)
                << QString("[Embed(source=\"gfx/%1.png\")]\n\t\t").arg(this->generateLayerVarName(layer))
                << "protected static const " << this->generateLayerVarName(layer) << "Gfx: Class;\n\t\t";
    }
    buffer = buffer.replace(FlxPlaceholders::GFX_EMBED_STATEMENTS, embedStatements);
}

void AS3Level::generateTilemapDeclarations(const QList<Tiled::Layer *> &layers, QString &buffer) const
{
    QString tilemapDeclarations = "";

    foreach (Tiled::Layer * layer, layers)
    {
        if (!layer->isVisible()) continue;
        if (!layer->asTileLayer()) continue;

        QString varName = this->generateLayerVarName(layer);
        QTextStream(&tilemapDeclarations)
                << QString("protected var %1Tilemap: %2;\n\t\t").arg(varName, this->tilemapClass);

    }

    buffer = buffer.replace(FlxPlaceholders::TILEMAP_DECLARATIONS, tilemapDeclarations);
}

void AS3Level::setPackageName(const QString &packageName)
{
    this->packageName = packageName;
}

void AS3Level::setTilemapClass(const QString &className)
{
    this->tilemapClass = className;
}

/**
 * Function loads the template file from a hardcoded resource
 * to a string.
 */
void AS3Level::loadBlueprint()
{
    QFile tmp(":/baseLevelTemplate.as");
    if (!tmp.open(QIODevice::ReadOnly | QIODevice::Text))
         return; //false;

    blueprint = QString(tmp.readAll());
}
