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

#ifndef AS3LEVEL_H
#define AS3LEVEL_H

#include <QString>

#include "map.h"
#include "tile.h"
#include "layer.h"
#include "tileset.h"

namespace Flx
{
    /**
     * Class encapsulates the logic for generating an ActionScript output file
     */
    class AS3Level
    {
    protected:
        /**
         * @brief ActionScript file template (with placeholders, e.g., %collisionData%)
         *
         * @todo Document the possible placeholders
         */
        QString blueprint;

        QMap<const Tiled::Tileset *, int> tilesetFirstGidMap;

        /**
         * Package name
         */
        QString packageName;

        /**
         * Tilemap class name
         */
        QString tilemapClass;

        /**
         * Function loads the ActionScript code template from a bundled resource into blueprint.
         *
         * @see blueprint
         */
        void loadBlueprint();

        void generateTilemapDeclarations(const QList<Tiled::Layer*> &layers, QString &buffer) const;
        void generateGfxEmbedStatements(const QList<Tiled::Layer*> &layers, QString &buffer) const;

        const QString generateTileData(Tiled::Layer* layer,
                              const QMap<Tiled::Tile *, int> idMap) const;
        const QString generateTilemapInitCode(const Tiled::Layer *layer) const;

        void generateLayerTileIDMap(Tiled::Layer *layer, QMap<Tiled::Tile *, int> &idMap) const;

        QString generateLayerVarName(const Tiled::Layer *layer) const;

        void saveLayerTilesheet(const QString &fileName,
                                const Tiled::Map *map,
                                const QMap<Tiled::Tile*, int> &idMap) const;

        QString generateTilesheetPath(const QString &levelFileName, const QString &sheetFileName) const;

    public:
        AS3Level();

        bool save(const QString &fileName, const Tiled::Map *map) const;

        void setTilemapClass(const QString &className);
        void setPackageName(const QString &packageName);

        void initTilesetGIDmap(const Tiled::Map *map);
    };
}
#endif // AS3LEVEL_H
