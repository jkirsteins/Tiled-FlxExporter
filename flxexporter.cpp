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

#include "flxexporter.h"

#include "map.h"
#include "tilelayer.h"
#include "layer.h"
#include "objectgroup.h"
#include "mapobject.h"
#include "tileset.h"
#include "tile.h"

#include "settingsdialog.h"
#include "as3level.h"

#include <QStringList>
#include <QQueue>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QBuffer>
#include <QPainter>
#include <QPixmap>
#include <QMap>
#include <QBitmap>
#include <QFile>
#include <QFileInfo>
#include <QRgb>
#include <QRegExp>
#include <QTextStream>
#include <QMessageBox>

using namespace Flx;

FlxExporter::FlxExporter()
{
}

/**
 * Function looks for existing package names in all *.as files in a given directory
 * and adds to the given list
 */
void FlxExporter::extractPackageNamesFromFiles(const QDir &targetDir, QStringList &packageNames) const
{
    QStringList existingFiles = targetDir.entryList(QStringList("*.as"),
                                    QDir::Files | QDir::NoSymLinks);

    QRegExp reg("package\\s+([^\\s]+)");
    qDebug() << "Found " << existingFiles.count() << " existing .as files\n";

    foreach (QString fileName, existingFiles)
    {
        fileName = targetDir.filePath(fileName);
        qDebug() << "Searching for package names in: " << fileName << "\n";

        QFile file(fileName);

        if (file.open(QIODevice::ReadOnly))
        {
            QTextStream in(&file);
            QString content = in.readAll();
            int offset = reg.indexIn(content);
            if (offset > -1)
            {
                qDebug()
                        << "Detected existing package declaration: "
                        << reg.cap(1)
                        << "\n";
                packageNames.append(reg.cap(1));
            }
            else
            {
                qDebug() << "Did not find a 'package' statement in " << fileName << "\n";
            }
        }
        else
        {
            qWarning() << "Could not open " << fileName << " for reading\n";
        }
    }
}

/**
 * Function generates the package name based on file path (travels up
 * through the dir hierarchy until finds a dir that might be
 * the base project source folder (i.e. 'src' or 'source')
 */
void FlxExporter::extractPackageNameFromPath(QDir targetDir, QStringList &packageNames) const
{
    QStringList projectBaseFolders; // case-insensitive
        projectBaseFolders.append("src");
        projectBaseFolders.append("source");

    QString packageName = targetDir.dirName();
    while (targetDir.cdUp())
    {
        if (projectBaseFolders.contains(
            targetDir.dirName(), Qt::CaseInsensitive))
        {
            qDebug()
                << "Determined package name based on path: "
                << packageName
                << "\n";

            packageNames.append(packageName);
            break;
        }
        packageName = QString("%1.%2").arg(targetDir.dirName(), packageName);
    }

    qDebug() << "Could not determine package name based on dir hierarchy\n";
}

/**
 * Function finds suggestions for package names, based on existing *.as files
 * and folder hierarchy
 */
void FlxExporter::generatePackageNameSuggestions(QDir targetDir, QStringList &packageNames) const
{
    this->extractPackageNamesFromFiles(targetDir, packageNames);
    this->extractPackageNameFromPath(targetDir, packageNames);
}

bool FlxExporter::write(const Tiled::Map *map, const QString &fileName)
{
    QFileInfo targetFileInfo(fileName);

    QStringList packageNames;
    QDir targetDir(targetFileInfo.absolutePath());
    this->generatePackageNameSuggestions(targetDir, packageNames);

    qDebug() << "Detected path: " << targetFileInfo.absolutePath() << "\n";

    SettingsDialog sd(NULL);
    sd.setPackageHints(packageNames);

    QString outputBaseName = targetFileInfo.fileName();

    if (outputBaseName.indexOf("Base") == 0)
    {
        QDir parentDir(targetDir);
        parentDir.cdUp();
        QString derivedClassName = parentDir.filePath(outputBaseName.right(outputBaseName.length()-4));
        //if (packageNames.at(0).indexOf("my.base") ==
        //    packageNames.at(0).length() - 4)
        //{
        sd.enableDerivedClassOption(derivedClassName);
        //}
    }

    sd.generateSummary(map);
    if (sd.exec() == QDialog::Rejected)
    {
        mError = tr("User cancelled export dialog");
        return false;
    }

    AS3Level output;
    output.setPackageName(sd.getPackageName());
    qDebug() << "tilemap class: " << sd.getTilemapClass() << "\n";
    output.setTilemapClass(sd.getTilemapClass());


    if (output.save(fileName, map))
    {
        QString derivedFileName = sd.getDerivedFileName();
        if (!derivedFileName.isEmpty())
        {
            /* QFile tmp(":/derivedLevelTemplate.as");
            if (!tmp.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                mError = tr("Could not generate derived file");
                return false;
            }

            QString derivedTemplate = QString(tmp.readAll()); */
            qDebug() << "Saving derived file to: " << derivedFileName << "\n";
        }
        else
            qDebug() << "Not saving derived file\n";

        return true;
    }
    else
    {
        mError = tr("Could not export map (unknown error)");
        return false;
    }

#ifdef NONO
    int iterUntilLast = map->layers().count();

    foreach (Tiled::Layer *layer, map->layers())
    {
        if (!layer->isVisible()) continue;

        if (layer->asObjectGroup())
        {
            Tiled::ObjectGroup *group = layer->asObjectGroup();
            foreach (Tiled::MapObject *item, group->objects())
            {
                QTextStream (&globalObjectDeclarations)
                    << "mapObjects[\"" << item->name() << "\"] = "
                    << "new " << item->type() << "();\n\t\t\t";

                QTextStream (&globalObjectDeclarations)
                    << "mapObjects[\"" << item->name() << "\"].x = " << (item->x() * map->tileWidth()) << ";\n\t\t\t"
                    << "mapObjects[\"" << item->name() << "\"].y = " << (item->y() * map->tileWidth()) << ";\n\t\t\t"
                    << "mapObjects[\"" << item->name() << "\"].width = " << (item->width() * map->tileWidth()) << ";\n\t\t\t"
                    << "mapObjects[\"" << item->name() << "\"].height = " << (item->height() * map->tileWidth()) << ";\n\t\t\t";

                /*QMap<QString, QString> *properties = item->properties();
                foreach (QString key, properties->keys())
                {
                    QTextStream (&globalObjectDeclarations)
                            << key << ": \"" << properties->value(key) << "\",\n\t\t\t";
                }*/

                //QTextStream (&globalObjectDeclarations) << "\r\r\r\r\r\n\t\t};\n\t\t";
            }

            continue;
        }

        --iterUntilLast;
        bool isLastLayer = (iterUntilLast == 0);
        bool exportCollisionData = sd.exportCollisionData(layer->name());

        QString useName = layer->name().toAscii();
        useName.remove(QChar(' '), Qt::CaseInsensitive);
        useName.remove(QChar(';'), Qt::CaseInsensitive);
        useName.remove(QChar(' '), Qt::CaseInsensitive);
        useName.replace(QChar('.'), QChar('_'), Qt::CaseInsensitive);

        QTextStream(&globalTilemapDeclarations) <<
                "protected var " << useName << "Tilemap: FlxTilemap;\n\t\t";

        QTextStream(&globalCollisionDictDeclarations) <<
                "protected var " << useName << "CollisionData: Dictionary;\n\t\t";


        QMap<const Tiled::Tile*, int> newTileIDs;
        newTileIDs.insert(NULL, 0);

        QList<int> layerTileData;

        int mw, mh;

        bool done = false;
        for (int i = 0; i < layer->height() && !done; ++i)
        {
            for (int j = 0; j < layer->width() && !done; j++)
            {
                Tiled::Tile *t = layer->asTileLayer()->tileAt(i, j);

                if (t != NULL)
                {
                    mw = t->width();
                    mh = mw;
                    done = true;
                }
            }
        }


        int xRatio = mw / map->tileWidth();
        int yRatio = mh / map->tileHeight();

        QMap< int, QList<bool> > alphaData; // tile id => tile col. bools

        bool skipOne = false;

        if (exportCollisionData)
            QTextStream(&globalCollisionInitCode) <<
                    useName << "CollisionData = new Dictionary(true);\n\t\t\t";

        for (int i = 0; i < layer->height(); ++i)
        {
            for (int j = 0; j < layer->width(); j++)
            {
                if (skipOne)
                {
                    skipOne = false;
                    continue;
                }
                Tiled::Tile * tile = layer->asTileLayer()->tileAt(j, i);


                /* get tile info only if it's not an empty tile */
                if (tile != NULL)
                {

                    mw = tile->width();//mw > tile->width() ? mw : tile->width();
                    mh = tile->height();//mh > tile->height() ? mh : tile->height();

                    if (!newTileIDs.keys().contains(tile))
                    {
                        int newIndex = newTileIDs.count(); // * xRatio * yRatio;
                        newTileIDs[tile] = newIndex;
                    }
                }


                if (mw == 32 && tile != NULL) /* do fancy stuff only if it's not empty AND wrong sized */
                {
                   int blListIndex = layerTileData.length(); // not in list
                   //int brListIndex = blListIndex + 1;        // not in list
                   int tlListIndex = blListIndex - layer->width(); // possibly in list
                   int trListIndex = tlListIndex + 1;              // possibly in list

                   int blTileId = 1 + (newTileIDs[tile]-1)*xRatio*yRatio + 2;
                   int brTileId = 1 + (newTileIDs[tile]-1)*xRatio*yRatio + 3;
                   int tlTileId = 1 + (newTileIDs[tile]-1)*xRatio*yRatio;
                   int trTileId = 1 + (newTileIDs[tile]-1)*xRatio*yRatio + 1;

                   if (tlListIndex >= 0) layerTileData[tlListIndex] = tlTileId;
                   if (trListIndex >= 0) layerTileData[trListIndex] = trTileId;

                   layerTileData.append(blTileId);

                   if (j + 1 < layer->width())
                   {
                       skipOne = true;
                       layerTileData.append(brTileId);
                   }
                }
                else
                {
                    layerTileData.append(newTileIDs[tile]);
                }
                //QTextStream(&csv) << newTileIDs[tile] << ",";
            }
            //QTextStream(&csv) << "\\n";
        }

        QString csv;
        QTextStream ts(&csv);


        for (int i = 0; i < layerTileData.length(); ++i)
        {
            int tileId = layerTileData[i];
            ts << tileId;
            if (i < layerTileData.length()-1) ts << ",";

            if (i > 0 && ((i+1) % layer->width()) == 0)
                ts << "\\n";        
        }

        QTextStream(&globalTileData) << "'" << useName << "': \"" << csv << "\"";
        if (!isLastLayer)
            QTextStream(&globalTileData) << ",\n\t\t\t";

        QTextStream(&globalTileGraphics) << "[Embed(source=\"" << useName << ".png\")]\n\t\t"
                << "protected var " << useName << "Tilesheet: Class;\n\t\t";

        QTextStream(&globalInitCode) << useName << "Tilemap = new FlxTilemap();\n\t\t\t"
                << useName << "Tilemap.loadMap(layerTileIndices[\"" << useName << "\"], " << useName << "Tilesheet, " << map->tileWidth() << "," << map->tileHeight() << ");\n\t\t\t"
                << "this.add("<<useName<<"Tilemap);\n\t\t\t";


        // @todo take into account the possibility of tilesets with different sized tiles
        newTileIDs.remove(NULL);
        
        QPixmap pm(map->tileWidth() + newTileIDs.count() * map->tileWidth() * xRatio * yRatio,
                   map->tileHeight());
        pm.fill(Qt::transparent);
        QPainter painter(&pm);
        //painter->

        QMapIterator<const Tiled::Tile*, int> i(newTileIDs);
        while (i.hasNext()) {
            i.next();
            const Tiled::Tile *tile = i.key();
            for (int y = 0; y < yRatio; ++y)
            {
                for (int x = 0; x < xRatio; ++x)
                {
                    int srcX = x * map->tileWidth();
                    int srcY = y * map->tileHeight();
                    QPixmap tilePart = tile->image().copy(srcX, srcY, map->tileWidth(), map->tileHeight());

                    int tileId = i.value() - 1;
                    int tileStripStart = map->tileWidth() + tileId*yRatio*xRatio * map->tileWidth();
                    int tileStripIndex = y*yRatio + x;

                    int targetX = tileStripStart + tileStripIndex * map->tileWidth();
                    painter.drawPixmap(targetX, 0, map->tileWidth(), map->tileHeight(), tilePart);
                }
            }
//            tile->image().copy()
        }

        QString imageFile = targetFileInfo.absoluteDir().filePath(QString("%1.png").arg(useName));
        pm.toImage().save(imageFile, 0, 100);

        /* Output tilemap collision data */
        // optimization ideas:
        //  * export fully opaque blocks as single entry that's checked before the rest
        if (exportCollisionData)
        {
            QImage img = pm.toImage();

            for (int cx = 0; cx < img.width(); ++cx)
            {
                for (int cy = 0; cy < img.height(); ++cy)
                {
                    if (img.pixel(cx, cy) & 0xFF000000)
                    {
                        QTextStream(&globalCollisionInitCode) <<
                            useName << "CollisionData[" << (cy * img.width() + cx) <<
                            "] = 1;\n\t\t\t";
                    }
                }
            }
        }
    }
    QString content;
    content = content.arg(globalTileGraphics);
    content = content.arg(globalTilemapDeclarations);
    content = content.arg(globalCollisionDictDeclarations);
    content = content.arg(globalTileData);
    content = content.arg(globalCollisionInitCode);
    content = content.arg(globalInitCode);
    content = content.arg(globalObjectDeclarations);

    QByteArray contentBytes(content.toLatin1());
    //QFile output(fileName);
    //output.open(QIODevice::WriteOnly);
    //output.write(contentBytes);
    //output.flush();
    //output.close();
    return true;
#endif
    return true;
}

QString FlxExporter::nameFilter() const
{
    return tr("ActionScript files for Flixel (v2.0) (*.as)");
}

QString FlxExporter::errorString() const
{
    return mError;
}

Q_EXPORT_PLUGIN2(Flx, FlxExporter)
