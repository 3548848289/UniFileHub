#ifndef TABFACTORY_H
#define TABFACTORY_H

#include <QMap>
#include <QString>
#include <QtGlobal>
#include <functional>
#include "TabAbstract.h"
#include "../../DisposeAbstract/DisposeTXT/TabHandleTXT.h"
#include "../../DisposeAbstract/DisposeCSV/TabHandleCSV.h"
#include "../../DisposeAbstract/DisposeIMG/include/TabHandleIMG.h"
#if QT_VERSION_MAJOR >= 6
#include "../../DisposeAbstract/DisposeXLSX/TabHandleXLSX.h"
#endif
#include "../../DisposeAbstract/DisposeVideo/TabHandleVideo.h"
#if QT_VERSION_MAJOR >= 6
#include "../../DisposeAbstract/DisposePDF/TabHandlePDF.h"
#endif
#include "../../DisposeAbstract/DisposeDB/TabHandleDB.h"
#include "../../DisposeAbstract/DisposePSD/include/TabHandlePSD.h"

class TabFactory {
public:
    static TabAbstract* create(const QString& fileName);

private:
    static const QMap<QString, std::function<TabAbstract*(const QString&)>> factories;
};

#endif // TABFACTORY_H
