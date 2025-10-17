#ifndef TABFACTORY_H
#define TABFACTORY_H

#include <QMap>
#include <QString>
#include <functional>
#include "TabAbstract.h"
#include "../../DisposeAbstract/DisposeTXT/TabHandleTXT.h"
#include "../../DisposeAbstract/DisposeCSV/TabHandleCSV.h"
#include "../../DisposeAbstract/DisposeIMG/include/TabHandleIMG.h"
#include "../../DisposeAbstract/DisposeXLSX/TabHandleXLSX.h"
#include "../../DisposeAbstract/DisposeVideo/TabHandleVideo.h"
#include "../../DisposeAbstract/DisposePDF/TabHandlePDF.h"
#include "../../DisposeAbstract/DisposeDB/TabHandleDB.h"

class TabFactory {
public:
    static TabAbstract* create(const QString& fileName);

private:
    static const QMap<QString, std::function<TabAbstract*(const QString&)>> factories;
};

#endif // TABFACTORY_H
