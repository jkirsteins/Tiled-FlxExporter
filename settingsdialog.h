#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QStringList>

#include "layer.h"
#include "map.h"

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    bool exportCollisionData(const QString &name) const;
    void setMap(const Tiled::Map *map);
    void setPackageHints(const QStringList & list);

    const QString getPackageName() const;
    const QString getTilemapClass() const;
    const void enableDerivedClassOption(const QString &name);

    const QString getDerivedFileName() const;

protected slots:

protected:
    QString derivedFileName;

    void changeEvent(QEvent *e);

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
