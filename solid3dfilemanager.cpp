/****************************************************************************
** Copyright (C) 2017 Ivan Assing da Silva
** Contact: ivanassing@gmail.com
**
** This file is part of the FEA_MNE772 project.
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

#include "solid3dfilemanager.h"

#include <QtWidgets>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QObject>

#include <iostream>

#include "msglog.h"

#define FSXL_ext "fsxl"
#define CDB_ext "cdb"


Solid3DFileManager::Solid3DFileManager(QTreeWidget *treeWidget)
    : treeWidget(treeWidget)
{

    //QStyle *style = treeWidget->style();

    meshIcon.addPixmap(QPixmap(":/icons/mesh.png"));
    boundaryIcon.addPixmap(QPixmap(":/icons/boundary.png"));
    supportIcon.addPixmap(QPixmap(":/icons/suport.png"));
    forceIcon.addPixmap(QPixmap(":/icons/force.png"));
    //loadingIcon.addPixmap(QPixmap(":/icons/load.png"));
    displacementIcon.addPixmap(QPixmap(":/icons/displacement.png"));
    valueIcon.addPixmap(QPixmap(":/icons/number.png"));
    materialIcon.addPixmap(QPixmap(":/icons/material.png"));
    nodeIcon.addPixmap(QPixmap(":/icons/node.png"));
    elementIcon.addPixmap(QPixmap(":/icons/element.png"));

    currentfilename = "";

}


bool Solid3DFileManager::openFile(void)
{
//    currentfilename =
//            QFileDialog::getOpenFileName(this->treeWidget->parentWidget(), QObject::tr("Open Solid3D File"),
//                                         QDir::currentPath(),
//                                         QObject::tr("Solid3D Files (*.fsxl);;Ansys CDB File (*.cdb)"));
//    if (currentfilename.isEmpty())
//        return false;

    treeWidget->clear();

    QFileInfo file(currentfilename);
    QString type = file.completeSuffix();

    MsgLog::information(QString("Reading file ")+currentfilename);

    if(type == CDB_ext)
    {
        this->readCdbFile(currentfilename);
        currentfilename.replace(CDB_ext, FSXL_ext);
    }
    else if(type == FSXL_ext)
        this->readFsxlFile(currentfilename);

    treeWidget->setColumnCount(2);
    treeWidget->resizeColumnToContents(1);
    treeWidget->resizeColumnToContents(0);
    treeWidget->setAlternatingRowColors(true);

    return true;
}

bool Solid3DFileManager::saveFile(void)
{
    if (currentfilename.isEmpty())
        return false;

    MsgLog::information(QString("Writing file ")+currentfilename);

    QFile file(currentfilename);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Solid3D File Manager"),
                             QObject::tr("Cannot write file %1:\n%2.")
                             .arg(currentfilename)
                             .arg(file.errorString()));
        return false;
    }

    writeFile(&file);
    file.close();

    return true;
}



bool Solid3DFileManager::read(QIODevice *device)
{
    rxml.setDevice(device);

    if (rxml.readNextStartElement()) {
        if (rxml.name() == "femsolid3d" && rxml.attributes().value("version") == "1.0")
            readXML();
        else
            rxml.raiseError(QObject::tr("The file is not an FEMSolid3D version 1.0 file."));
    }

    return !rxml.error();
}


QString Solid3DFileManager::errorString() const
{
    return QObject::tr("%1\nLine %2, column %3")
            .arg(rxml.errorString())
            .arg(rxml.lineNumber())
            .arg(rxml.columnNumber());
}


void Solid3DFileManager::readXML()
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "femsolid3d");

    while (rxml.readNextStartElement()) {
        if(rxml.name() == "mesh")
        {
            QTreeWidgetItem *mesh = createChildItem(0);
            //bool colapsed = (rxml.attributes().value("colapsed") != "yes");
            treeWidget->setItemExpanded(mesh, true);

            mesh->setIcon(0, meshIcon);
            mesh->setText(0, QObject::tr("Solid3D"));
            mesh->setText(1, QObject::tr(""));

            while (rxml.readNextStartElement())
            {
                if (rxml.name() == "boundaryconditions")
                    readBoundaryConditions(mesh);

                else if (rxml.name() == "loading")
                    readLoading(mesh);

                else if (rxml.name() == "displacements")
                    readDisplacements(mesh);

                else if (rxml.name() == "materials")
                    readMaterials(mesh);

                else if (rxml.name() == "nodes")
                    readNodes(mesh);

                else if (rxml.name() == "elements")
                    readElements(mesh);

                else
                    rxml.skipCurrentElement();
            }
        }
    }
}

void Solid3DFileManager::readBoundaryConditions(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "boundaryconditions");

    QTreeWidgetItem *boundaryConditions = createChildItem(item);
    //boundaryConditions->setFlags(boundaryConditions->flags() | Qt::ItemIsEditable);
    boundaryConditions->setIcon(0, boundaryIcon);
    boundaryConditions->setText(0, QObject::tr("Boundary Conditions"));
    boundaryConditions->setText(1, rxml.attributes().value("count").toString());

    while (rxml.readNextStartElement()) {
        if (rxml.name() == "support")
        {
            QTreeWidgetItem *support= createChildItem(boundaryConditions);
            //support->setFlags(support->flags() | Qt::ItemIsEditable);
            support->setIcon(0, supportIcon);
            support->setText(0, QString("support %1").arg(rxml.attributes().value("index").toString()));
            //support->setText(1, rxml.attributes().value("index").toString());
            while (rxml.readNextStartElement()) {
                if (rxml.name() == "value1d")
                    readValue1D(support);
                else
                    rxml.skipCurrentElement();
            }
        }
        else
            rxml.skipCurrentElement();
    }
}

void Solid3DFileManager::readLoading(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "loading");

    QTreeWidgetItem *loading = createChildItem(item);
    //loading->setFlags(loading->flags() | Qt::ItemIsEditable);
    loading->setIcon(0, forceIcon);
    loading->setText(0, QObject::tr("Loading"));
    loading->setText(1, rxml.attributes().value("count").toString());

    while (rxml.readNextStartElement()) {
        if (rxml.name() == "force")
        {
            QTreeWidgetItem *force= createChildItem(loading);
            //force->setFlags(force->flags() | Qt::ItemIsEditable);
            force->setIcon(0, forceIcon);
            force->setText(0, QString("force %1").arg(rxml.attributes().value("index").toString()));
            //force->setText(1, rxml.attributes().value("index").toString());
            while (rxml.readNextStartElement()) {
                if (rxml.name() == "value1d")
                    readValue1D(force);
                else
                    rxml.skipCurrentElement();
            }
        }
        else if (rxml.name() == "pressure")
        {
            QTreeWidgetItem *pressure= createChildItem(loading);
            //force->setFlags(force->flags() | Qt::ItemIsEditable);
            pressure->setIcon(0, forceIcon);
            pressure->setText(0, QString("pressure %1").arg(rxml.attributes().value("index").toString()));
            //force->setText(1, rxml.attributes().value("index").toString());
            while (rxml.readNextStartElement()) {
                if (rxml.name() == "value1d")
                    readValue1D(pressure);
                else
                    rxml.skipCurrentElement();
            }
        }
        else
            rxml.skipCurrentElement();
    }
}

void Solid3DFileManager::readDisplacements(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "displacements");

    QTreeWidgetItem *displacements = createChildItem(item);
    //displacements->setFlags(displacements->flags() | Qt::ItemIsEditable);
    displacements->setIcon(0, displacementIcon);
    displacements->setText(0, QObject::tr("Displacements"));
    displacements->setText(1, rxml.attributes().value("count").toString());

    while (rxml.readNextStartElement()) {
        if (rxml.name() == "displacement")
        {
            QTreeWidgetItem *displacement= createChildItem(displacements);
            //displacement->setFlags(displacement->flags() | Qt::ItemIsEditable);
            displacement->setIcon(0, displacementIcon);
            displacement->setText(0, QString("displacement %1").arg(rxml.attributes().value("index").toString()));
            //displacement->setText(1, rxml.attributes().value("index").toString());
            while (rxml.readNextStartElement()) {
                if (rxml.name() == "value1d")
                    readValue1D(displacement);
                else
                    rxml.skipCurrentElement();
            }
        }
        else
            rxml.skipCurrentElement();
    }
}

void Solid3DFileManager::readMaterials(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "materials");

    QTreeWidgetItem *materials = createChildItem(item);
    //materials->setFlags(materials->flags() | Qt::ItemIsEditable);
    materials->setIcon(0, materialIcon);
    materials->setText(0, QObject::tr("Materials"));
    materials->setText(1, rxml.attributes().value("count").toString());

    while (rxml.readNextStartElement()) {
        if (rxml.name() == "material")
        {
            QTreeWidgetItem *material= createChildItem(materials);
            //material->setFlags(material->flags() | Qt::ItemIsEditable);
            material->setIcon(0, materialIcon);
            material->setText(0, QString("material %1").arg(rxml.attributes().value("index").toString()));
            //material->setText(1, rxml.attributes().value("index").toString());
            while (rxml.readNextStartElement()) {
                if (rxml.name() == "value1d")
                    readValue1D(material);
                else
                    rxml.skipCurrentElement();
            }
        }
        else
            rxml.skipCurrentElement();
    }
}

void Solid3DFileManager::readValue1D(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "value1d");

    QTreeWidgetItem *value1d = createChildItem(item);

    value1d->setFlags(value1d->flags() | Qt::ItemIsEditable);
    value1d->setIcon(0, valueIcon);
    value1d->setText(0, rxml.attributes().value("dim").toString());
    value1d->setText(1, rxml.attributes().value("value").toString());

    rxml.skipCurrentElement();
}

void Solid3DFileManager::readNodes(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "nodes");

    QTreeWidgetItem *nodes = createChildItem(item);
    //nodes->setFlags(nodes->flags() | Qt::ItemIsEditable);
    nodes->setIcon(0, nodeIcon);
    nodes->setText(0, QObject::tr("Node3D"));
    nodes->setText(1, rxml.attributes().value("count").toString());

    while (rxml.readNextStartElement()) {
        if (rxml.name() == "node")
        {
            QTreeWidgetItem *node= createChildItem(nodes);
            //node->setFlags(node->flags() | Qt::ItemIsEditable);
            node->setIcon(0, nodeIcon);
            node->setText(0, QString("node %1").arg(rxml.attributes().value("index").toString()));
            //node->setText(1, rxml.attributes().value("index").toString());
            while (rxml.readNextStartElement()) {
                if (rxml.name() == "value1d")
                    readValue1D(node);
                else
                    rxml.skipCurrentElement();
            }
        }
        else
            rxml.skipCurrentElement();
    }
}

void Solid3DFileManager::readElements(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "elements");

    QTreeWidgetItem *elements = createChildItem(item);
    //elements->setFlags(elements->flags() | Qt::ItemIsEditable);
    elements->setIcon(0, elementIcon);
    elements->setText(0, QObject::tr("Solid3DElement"));
    elements->setText(1, rxml.attributes().value("count").toString());

    while (rxml.readNextStartElement()) {
        if (rxml.name() == "element")
        {
            QTreeWidgetItem *element= createChildItem(elements);
            //element->setFlags(element->flags() | Qt::ItemIsEditable);
            element->setIcon(0, elementIcon);
            element->setText(0, QString("element %1").arg(rxml.attributes().value("index").toString()));
            //element->setText(1, rxml.attributes().value("index").toString());
            while (rxml.readNextStartElement()) {
                if (rxml.name() == "value1d")
                    readValue1D(element);
                else
                    rxml.skipCurrentElement();
            }
        }
        else
            rxml.skipCurrentElement();
    }
}

QTreeWidgetItem *Solid3DFileManager::createChildItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *childItem;
    if (item) {
        childItem = new QTreeWidgetItem(item);
    } else {
        childItem = new QTreeWidgetItem(treeWidget);
    }
    childItem->setData(0, Qt::UserRole, rxml.name().toString());
    return childItem;
}

bool Solid3DFileManager::writeFile(QIODevice *device)
{
    wxml.setAutoFormatting(true);

    wxml.setDevice(device);

    wxml.writeStartDocument();
    wxml.writeDTD("<!DOCTYPE femsolid3d>");
    wxml.writeStartElement("femsolid3d");
    wxml.writeAttribute("version", "1.0");
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i)
        writeItem(treeWidget->topLevelItem(i));

    wxml.writeEndDocument();
    return true;
}

void Solid3DFileManager::writeItem(QTreeWidgetItem *item)
{
    QString tagName = item->data(0, Qt::UserRole).toString();
    if (tagName == "mesh") {
        //bool colapsed = !treeWidget->isItemExpanded(item);
        wxml.writeStartElement(tagName);
        wxml.writeAttribute("colapsed", "yes");
        for (int i = 0; i < item->childCount(); ++i)
            writeItem(item->child(i));
        wxml.writeEndElement();
    } else if (tagName == "boundaryconditions" || tagName == "loading" || tagName == "displacements"
               || tagName == "materials" || tagName == "nodes" || tagName == "elements") {
        wxml.writeStartElement(tagName);
        if (!item->text(1).isEmpty())
            wxml.writeAttribute("count", item->text(1));
        for (int i = 0; i < item->childCount(); ++i)
            writeItem(item->child(i));
        wxml.writeEndElement();
    } else if (tagName == "support" || tagName == "force" || tagName == "pressure" || tagName == "displacement"
               || tagName == "node" || tagName == "material" || tagName == "element") {
        wxml.writeStartElement(tagName);
        wxml.writeAttribute("index", item->text(0).replace(QString("%1 ").arg(tagName),""));
        for (int i = 0; i < item->childCount(); ++i)
            writeItem(item->child(i));
        wxml.writeEndElement();
    } else if (tagName == "value1d") {
        wxml.writeStartElement(tagName);
        if (!item->text(0).isEmpty())
            wxml.writeAttribute("dim", item->text(0));
        if (!item->text(1).isEmpty())
            wxml.writeAttribute("value", item->text(1));
        wxml.writeEndElement();
    }
}

bool Solid3DFileManager::readCdbFile(QString filename)
{
    QFile cdbfile(filename);
    if (!cdbfile.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Solid3D File Manager"),
                             QObject::tr("Cannot open file %1:\n%2.")
                             .arg(filename)
                             .arg(cdbfile.errorString()));
        return false;
    }

    QString wxmlfilename = filename.replace(CDB_ext, FSXL_ext);

    QFile wxmlfile(wxmlfilename);
    if (!wxmlfile.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Soli3D File Manager"),
                             QObject::tr("Cannot write file %1:\n%2.")
                             .arg(wxmlfilename )
                             .arg(wxmlfile.errorString()));
        return false;
    }

    wxml.setAutoFormatting(true);

    wxml.setDevice(&wxmlfile);

    wxml.writeStartDocument();
    wxml.writeDTD("<!DOCTYPE femtruss3d>");
    wxml.writeStartElement("femsolid3d");
    wxml.writeAttribute("version", "1.0");

    // ###########################  READING CDB FILE

    QTextStream text(&cdbfile);

    QString line;
    QStringList str;

    QStringList *strNodes;
    int nstrNodes;

    QStringList *strElements;
    int nstrElements;

    QStringList *strMaterials;
    int nstrMaterials;

    QStringList *strLoadings;
    int cstrLoadings=0;

    QStringList *strRestrictions;
    int cstrRestrictions=0;

    QStringList *strDisplacements;
    int cstrDisplacements=0;


    const int buffer = 1000;

    strLoadings = new QStringList[buffer];
    strRestrictions = new QStringList[buffer];
    strDisplacements = new QStringList[buffer];


    // default bondary condition
    strRestrictions[0] << QString("0");
    strRestrictions[0] << QString("0"); // false
    strRestrictions[0] << QString("0"); // false
    strRestrictions[0] << QString("0"); // false
    cstrRestrictions++;

    // default displacements
    strDisplacements[0] << QString("0");
    strDisplacements[0] << QString("0");
    strDisplacements[0] << QString("0");
    strDisplacements[0] << QString("0");
    cstrDisplacements++;

    // default loading
    strLoadings[0] << QString("0");
    strLoadings[0] << QString("0");
    strLoadings[0] << QString("0");
    strLoadings[0] << QString("0");
    cstrLoadings++;

    QString pressure;


    do {
        line = text.readLine();
        if(line.isNull())
            break;
        str = line.split(",", QString::SkipEmptyParts);

        if(str[0]=="NUMOFF")
        {
            if(str[1]=="NODE")
            {
                nstrNodes = str[2].toInt();
                strNodes = new QStringList[nstrNodes];
            }
            else if(str[1]=="ELEM")
            {
                nstrElements = str[2].toInt();
                strElements = new QStringList[nstrElements];

            }
            else if(str[1]=="MAT ")
            {
                nstrMaterials= str[2].toInt();
                strMaterials = new QStringList[nstrMaterials];

                for(int i=0; i<nstrMaterials; i++)
                {
                    strMaterials[i] << QString("%1").arg(i);
                    strMaterials[i] << QString("material %1").arg(i);
                }
            }
        }

        if(str[0]=="MP")
        {
            do {
                int index = str[2].toInt()-1;

                if(str[1]=="DENS")
                    strMaterials[index] << str[3];
                else if(str[1]=="EX")
                    strMaterials[index] << str[3];
                else if(str[1]=="PRXY")
                    strMaterials[index] << str[3];

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);
            }while(str[0]=="MP");
        }

        if(str[0]=="N")
        {
            for(int i=0; i<nstrNodes+1; i++)
            {
                if(str[0] != "N")
                    break;
                if(str[3].toInt()-1 != i) std::cerr<<"error in nodes indexing";

                strNodes[i] << QString("%1").arg(i); // index
                strNodes[i] << str[4]; // x
                strNodes[i] << str[5]; // y
                strNodes[i] << str[6]; // z
                strNodes[i] << QString("0"); // restriction
                strNodes[i] << QString("0"); // loading
                strNodes[i] << QString("0"); // displacement

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);
            }
        }

        if(str[0]=="EN")
        {
            int ima;
            for(int i=0; i<nstrElements+1; i++)
            {
                str = line.split(",", QString::SkipEmptyParts);
                if(str[0] != "EN")
                    break;

                ima = str[4].toInt()-1;
                if(str[7].toInt()-1 != i) std::cerr<<"error in elements indexing";

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);

                strElements[i] << QString("%1").arg(i); // index
                strElements[i] << QString("%1").arg(str[3].toInt()-1); // node0
                strElements[i] << QString("%1").arg(str[4].toInt()-1); // node1
                strElements[i] << QString("%1").arg(str[5].toInt()-1); // node2
                strElements[i] << QString("%1").arg(str[6].toInt()-1); // node3
                strElements[i] << QString("%1").arg(ima); // material
                strElements[i] << QString("-1"); // pface
                strElements[i] << QString("0"); // pressure

                line = text.readLine();
                //str = line.split(",", QString::SkipEmptyParts);
            }
        }

        if(str[0]=="SFE")
        {
            int iel;
            do {
                iel = str[1].toInt()-1;
                //qDebug()<<iel;

                strElements[iel][6] = QString("%1").arg(str[2].toInt()-1); // pface

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);

                // TODO implement for multiple inputs
                pressure = str[0];
                strElements[iel][7] = QString("1"); // pressure

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);
            }while(str[0]=="SFE");
        }

        if(str[0]=="D")
        {
            // TODO implement for multiple inputs
            strRestrictions[1] << QString("1");
            strRestrictions[1] << QString("1"); // false
            strRestrictions[1] << QString("1"); // false
            strRestrictions[1] << QString("1"); // false
            cstrRestrictions=2;

            strDisplacements[1] << QString("1");
            strDisplacements[1] << str[3];
            strDisplacements[1] << QString("0");
            strDisplacements[1] << QString("0");
            cstrDisplacements=2;

            int inode;
            do {
                inode = str[1].toInt()-1;
                strNodes[inode][4] = QString("1");
                strNodes[inode][6] = QString("1");

                line = text.readLine();
                if(line.isNull())
                    break;

                str = line.split(",", QString::SkipEmptyParts);
            }while(str[0]=="D");
        }
    } while (!line.isNull());

    // ###########################  END READING CDB FILE



    QStringList dim;
    dim <<"x"<<"y"<<"z";

    // >> mesh
    QString tagName = "mesh";
    wxml.writeStartElement(tagName);
    wxml.writeAttribute("colapsed", "yes");
    // >> boundaryconditions
    tagName = "boundaryconditions";
    wxml.writeStartElement(tagName);

    wxml.writeAttribute("count", QString("%1").arg(cstrRestrictions));

    // >> support
    for(int i=0; i<cstrRestrictions; i++)
    {
        tagName = "support";
        wxml.writeStartElement(tagName);

        wxml.writeAttribute("index", strRestrictions[i][0]);
        tagName = "value1d";
        for(int j=0;j<3;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", strRestrictions[i][j+1]);
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }
    // << support
    wxml.writeEndElement();
    // << boundaryconditions

    // >> loading
    tagName = "loading";
    wxml.writeStartElement(tagName);


    wxml.writeAttribute("count", QString("%1").arg(cstrLoadings+1));

    // >> force
    for(int i=0; i<cstrLoadings; i++)
    {
        tagName = "force";
        wxml.writeStartElement(tagName);

        wxml.writeAttribute("index", strLoadings[i][0]);
        tagName = "value1d";
        for(int j=0;j<3;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", strLoadings[i][j+1]);
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }
    // pressure
    tagName = "pressure";
    wxml.writeStartElement(tagName);

    wxml.writeAttribute("index", "0");
    tagName = "value1d";
    for(int j=0;j<3;j++)
    {
        wxml.writeStartElement(tagName);
        wxml.writeAttribute("dim", dim.at(j));
        wxml.writeAttribute("value", pressure);
        wxml.writeEndElement();
    }
    wxml.writeEndElement();

    // << force
    wxml.writeEndElement();
    // << loadig


    // >> displacements
    tagName = "displacements";
    wxml.writeStartElement(tagName);

    wxml.writeAttribute("count", QString("%1").arg(cstrDisplacements));

    // >> displacement
    for(int i=0; i<cstrDisplacements; i++)
    {
        tagName = "displacement";
        wxml.writeStartElement(tagName);

        wxml.writeAttribute("index", strDisplacements[i][0]);
        tagName = "value1d";
        for(int j=0;j<3;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", strDisplacements[i][j+1]);
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }
    // << displacement
    wxml.writeEndElement();
    // << displacements

    dim <<"support"<<"force"<<"displacement";
    // >> nodes
    tagName = "nodes";
    wxml.writeStartElement(tagName);

    wxml.writeAttribute("count", QString("%1").arg(nstrNodes));

    // >> node
    for(int i=0; i<nstrNodes; i++)
    {
        tagName = "node";
        wxml.writeStartElement(tagName);

        wxml.writeAttribute("index", strNodes[i][0]);
        tagName = "value1d";
        for(int j=0;j<6;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", strNodes[i][j+1]);
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }
    // << node
    wxml.writeEndElement();
    // << nodes

    dim.clear();
    dim <<"name"<<"density"<<"E"<<"poison";
    // >> materials
    tagName = "materials";
    wxml.writeStartElement(tagName);

    wxml.writeAttribute("count", QString("%1").arg(nstrMaterials));

    // >> material
    for(int i=0; i<nstrMaterials; i++)
    {
        tagName = "material";
        wxml.writeStartElement(tagName);

        wxml.writeAttribute("index", strMaterials[i][0]);
        tagName = "value1d";
        for(int j=0;j<4;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", strMaterials[i][j+1]);
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }


    wxml.writeEndElement();
    // << materials

    dim.clear();
    dim <<"node0"<<"node1"<<"node2"<<"node3"<<"material"<<"pface"<<"pressure";
    // >> elements
    tagName = "elements";
    wxml.writeStartElement(tagName);

    wxml.writeAttribute("count", QString("%1").arg(nstrElements));

    // >> element
    for(int i=0; i<nstrElements; i++)
    {
        tagName = "element";
        wxml.writeStartElement(tagName);

        wxml.writeAttribute("index", strElements[i][0]);
        tagName = "value1d";
        for(int j=0;j<7;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", strElements[i][j+1]);
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }
    // << element
    wxml.writeEndElement();
    // << elements

    wxml.writeEndElement();
    // << mesh
    wxml.writeEndDocument();


    delete [] strDisplacements;
    delete [] strElements;
    delete [] strLoadings;
    delete [] strMaterials;
    delete [] strNodes;
    delete [] strRestrictions;


    cdbfile.close();
    wxmlfile.close();

    //    QFile wxmlfile(wxmlfilename);
    if (!wxmlfile.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Solid3D File Manager"),
                             QObject::tr("Cannot read file %1:\n%2.")
                             .arg(wxmlfilename )
                             .arg(wxmlfile.errorString()));
        return false;
    }

    if (!read(&wxmlfile)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Solid3D File Manager"),
                             QObject::tr("Parse error in file %1:\n\n%2")
                             .arg(wxmlfilename)
                             .arg(this->errorString()));
        return false;
    }

    wxmlfile.close();
    return true;
}

bool Solid3DFileManager::readFsxlFile(QString filename)
{
    QFile ftxlfile(filename);
    if (!ftxlfile.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Truss3D File Manager"),
                             QObject::tr("Cannot open file %1:\n%2.")
                             .arg(filename)
                             .arg(ftxlfile.errorString()));
        return false;
    }


    if (!read(&ftxlfile)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Truss3D File Manager"),
                             QObject::tr("Parse error in file %1:\n\n%2")
                             .arg(filename)
                             .arg(this->errorString()));
        return false;
    }

    ftxlfile.close();
    return true;
}



