/****************************************************************************
** Copyright (C) 2017 Ivan Assing da Silva
** Contact: ivanassing@gmail.com
**
** This file is part of the FEA_MNE715 project.
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef SOLID3DFILEMANAGER_H
#define SOLID3DFILEMANAGER_H


#include <QIcon>
#include <QMainWindow>
#include <QXmlStreamReader>

//QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
//QT_END_NAMESPACE

class Solid3DReader;

///
/// \brief The Solid3DFileManager class
///
class Solid3DFileManager
{
public:
    Solid3DFileManager(QTreeWidget *treeWidget);

    bool openFile(void);
    bool saveFile(void);
    bool readFsxlFile(QString filename);
    bool readCdbFile(QString filename);

    bool writeFile(QIODevice *device);
    bool read(QIODevice *device);
    QString errorString() const;

    QString currentfilename;

    friend class Solid3DReader;

private:
    void readXML();
    void readBoundaryConditions(QTreeWidgetItem *item);
    void readLoading(QTreeWidgetItem *item);
    void readDisplacements(QTreeWidgetItem *item);
    void readValue1D(QTreeWidgetItem *item);
    void readMaterials(QTreeWidgetItem *item);
    void readNodes(QTreeWidgetItem *item);
    void readElements(QTreeWidgetItem *item);
    void writeItem(QTreeWidgetItem *item);

    QXmlStreamWriter wxml;
    QXmlStreamReader rxml;

    QTreeWidgetItem *createChildItem(QTreeWidgetItem *item);


    QTreeWidget *treeWidget;

    QIcon meshIcon;
    QIcon boundaryIcon;
    QIcon supportIcon;
    QIcon forceIcon;
    QIcon loadingIcon;
    QIcon displacementIcon;
    QIcon valueIcon;
    QIcon materialIcon;
    QIcon nodeIcon;
    QIcon elementIcon;
};

#endif // SOLID3DFILEMANAGER_H
