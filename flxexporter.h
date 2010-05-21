#ifndef FLXEXPORTER_H
#define FLXEXPORTER_H

#include <QtCore/qglobal.h>

#if defined(FLX_LIBRARY)
#  define FLXSHARED_EXPORT Q_DECL_EXPORT
#else
#  define FLXSHARED_EXPORT Q_DECL_IMPORT
#endif

#include "mapwriterinterface.h"

#include <QObject>
#include <QDir>

namespace Flx
{

    class FLXSHARED_EXPORT FlxExporter : public QObject,
                        public Tiled::MapWriterInterface
    {
        Q_OBJECT
        Q_INTERFACES(Tiled::MapWriterInterface)

        public:
            FlxExporter();

            bool write(const Tiled::Map *map, const QString &fileName);
            QString nameFilter() const;
            QString errorString() const;

        protected:
            void extractPackageNamesFromFiles(const QDir &targetDir, QStringList &packageNames) const;
            void extractPackageNameFromPath(QDir targetDir, QStringList &packageNames) const;

            void generatePackageNameSuggestions(QDir targetDir, QStringList &packageNames) const;

        private:
            QString mError;
    };

} // namespace Flx

#endif // FLXEXPORTER_H
