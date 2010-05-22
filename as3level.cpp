/*
 * FlxExporter for Tiled Map Editor (Qt)
 * Copyright 2010 Jânis Kirðteins <janis@janiskirsteins.org>
 *
 * This file is part of FlxExporter for Tiled Map Editor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <QFile>
#include <QByteArray>
#include <QPixmap>
#include <QPainter>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>
#include <QList>
#include <QApplication>

#include "tilelayer.h"
#include "layer.h"
#include "tile.h"

#include "progressdialog.h"
#include "as3levelplaceholders.h"
#include "as3level.h"

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
    //unsigned int tileCount = 1 + idMap.count() * partsPerTile;

    unsigned int imageWidth = map->tileWidth();     // initial empty tile for flixel
    foreach (Tiled::Tile *tile, idMap.keys())
    {
        if (tile == NULL) continue;

        int partCount = (tile->width() * tile->height()) / (map->tileWidth() * map->tileHeight());
        imageWidth += partCount * map->tileWidth();
    }

    QPixmap pm(imageWidth,
               map->tileHeight());
    pm.fill(Qt::transparent);
    QPainter painter(&pm);

    foreach (Tiled::Tile *tile, idMap.keys())
    {
        if (tile == NULL)
        {
            qDebug() << "Encountered NULL tile while saving tilesheet\n";
            continue;
        }

        unsigned int xRatio = tile->width() / map->tileWidth();
        unsigned int yRatio = tile->height() / map->tileWidth();

        int tileOffset = map->tileWidth();
        int checkedIndex = 1;
        qDebug() << "Current tile: " << idMap[tile] << "\n";
        while (checkedIndex < idMap[tile])
        {
            /**
             * @todo Optimize offset finding (not that easy, since we can't calculate
             * it directly (due to possibly varying tile sizes in one layer)
             */
            foreach (Tiled::Tile *t, idMap.keys())
            {
                if (t == NULL) continue;

                int testIndex = idMap[t];
                if (checkedIndex == testIndex)
                {
                    int tileParts = (t->width() * t->height()) / (map->tileHeight() * map->tileWidth());

                    tileOffset += tileParts * map->tileWidth();
                    checkedIndex += tileParts;


                    qDebug() << "Skipping tiles: " << tileParts << "\n";
                    qDebug() << "Added width: " << tileParts * map->tileWidth() << "\n";

                    break;  // either this, or have a redundant check for (checkedIndex < idMap[tile]) here
                }
            }
        }

        for (unsigned int y = 0; y < yRatio; ++y)
        {
            for (unsigned int x = 0; x < xRatio; ++x, tileOffset += map->tileWidth())
            {
                int srcX = x * map->tileWidth();
                int srcY = y * map->tileHeight();
                QPixmap tilePart = tile->image().copy(srcX, srcY, map->tileWidth(), map->tileHeight());

                painter.drawPixmap(tileOffset, 0, map->tileWidth(), map->tileHeight(), tilePart);
            }
        }
//            tile->image().copy()
    }

    QString imageFile = QString("%1.png").arg(fileName);
    pm.toImage().save(imageFile, 0, 100);
}

/**
 * Function generates and saves the ActionScript file
 */
bool AS3Level::save(const QString &fileName, const Tiled::Map *map) const
{
    ProgressDialog pd(NULL);
    pd.setMaxProgress(map->layers().count() + 1);
    pd.open();

    QString buffer(this->blueprint);
    QFileInfo targetInfo(fileName);

    buffer = buffer.replace(FlxPlaceholders::PACKAGE_NAME, this->packageName);
    buffer = buffer.replace(FlxPlaceholders::CLASS_NAME, targetInfo.baseName());
    buffer = buffer.replace(FlxPlaceholders::TILEMAP_CLASS, this->tilemapClass);
    buffer = buffer.replace(FlxPlaceholders::GEN_BY, "FlxExporter v0.2");
    buffer = buffer.replace(FlxPlaceholders::GEN_DATE, "@todo Insert real date");

    this->generateTilemapDeclarations(map->layers(), buffer);
    this->generateGfxEmbedStatements(map->layers(), buffer);

    QString tileData;
    QString tilemapInitCode;
    foreach (Tiled::Layer *layer, map->layers())
    {
        pd.updateProgress();
        QApplication::instance()->processEvents();

        if (!layer->isVisible()) continue;

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
        QTextStream(&tilemapInitCode) << this->generateTilemapInitCode(layer);
    }

    buffer = buffer.replace(FlxPlaceholders::LAYER_TILE_DATA, tileData);
    buffer = buffer.replace(FlxPlaceholders::TILEMAP_INITIALIZATION, tilemapInitCode);


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

const QString AS3Level::generateTilemapInitCode(const Tiled::Layer *layer) const
{
    QString result;
    QString tileMapVar = this->generateLayerVarName(layer) + "Tilemap";
    QString tileDataVar = this->generateLayerVarName(layer) + "TileData";
    QString tileGfxVar = this->generateLayerVarName(layer) + "Gfx";

    QTextStream(&result)
            << QString("%1 = new %2();").arg(tileMapVar, this->tilemapClass) << "\n\t\t\t"
            << QString("%1.loadMap(%2, %3);").arg(tileMapVar, tileDataVar, tileGfxVar) << "\n\t\t\t"
            << QString("add(%1);").arg(tileMapVar) << "\n\t\t\t";
    return result;
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

    for (int j = 0; j < tileLayer->height(); ++j)
    {
        for (int i = 0; i < tileLayer->width(); ++i)
        {
            Tiled::Tile *tile = tileLayer->tileAt(i, j);

            int xTileParts = tile != NULL ? tile->width() / layer->map()->tileWidth() : 1;
            int yTileParts = tile != NULL ? tile->height() / layer->map()->tileHeight() : 1;
            unsigned int tileParts = xTileParts * yTileParts;

            if (!idMap.keys().contains(tile))
            {
                idMap[tile] = index;
                index += tileParts;
            }
        }
    }
    idMap.remove(NULL);
}

QString AS3Level::generateLayerVarName(const Tiled::Layer *layer) const
{
    QString useName = layer->name().toAscii();
    useName[0] = useName[0].toLower();
    if (!useName[0].isLetter() && useName[0] != '_')
        useName = "_" + useName;

    useName.remove(QChar(' '), Qt::CaseInsensitive);
    useName.remove(QChar(';'), Qt::CaseInsensitive);
    useName.remove(QChar(' '), Qt::CaseInsensitive);
    useName.replace(QChar('.'), QChar('_'), Qt::CaseInsensitive);
    return useName;
}

/**
 * Function generates tile index string for a given layer
 * that is used by FlxTilemap.loadMap.
 *
 * @todo Fix issue with non-standard size tiles overlapping
 *       borders of map.
 */
const QString AS3Level::generateTileData(Tiled::Layer *layer,
                                const QMap<Tiled::Tile *, int> idMap) const
{
    QString tileDataString;
    QList<int> tileData;

    const int maxLength = layer->height() * layer->width();

    int xTileParts;
    for (int j = 0; j < layer->height(); ++j)
    {
        for (int i = 0; i < layer->width(); i += xTileParts)
        {
            Tiled::Tile* tile = layer->asTileLayer()->tileAt(i, j);

            xTileParts = tile != NULL ? tile->width() / layer->map()->tileWidth() : 1;
            int yTileParts = tile != NULL ? tile->height() / layer->map()->tileHeight() : 1;
            int id = idMap[tile];

            for (int l = yTileParts - 1; l >= 0; --l)
            {
                int negativeOffset = l * layer->width();
                const int indexBase = tileData.length();
                for (int k = 0; k < xTileParts; ++k, ++id)
                {
                    int index = indexBase - negativeOffset + k;

                    if (index < tileData.length())
                    {
                        tileData[index] = id;
                    }
                    else if (index < maxLength)
                    {
                        tileData.append(id);
                    }
                }
            }
            /*QTextStream(&tileData)
                    << idMap[layer->asTileLayer()->tileAt(i, j)]
                    << (i < (layer->width() - 1) ? "," : "");*/
        }
        //if (j != (layer->height() - 1))
        //    QTextStream(&tileData) << "\\n";
    }

    for (int i = 0; i < tileData.length(); ++i)
    {
        bool lastCol = (i > 0) && (((i+1) % layer->width()) == 0);
        QTextStream(&tileDataString)
                << tileData[i]
                << (lastCol ? "\\n" : ",");
    }

    QString result;
    QTextStream(&result)
            << "protected const " << this->generateLayerVarName(layer)
            << QString("TileData: String = \"%1\";\n\t\t").arg(tileDataString);

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
