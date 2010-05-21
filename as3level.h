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
