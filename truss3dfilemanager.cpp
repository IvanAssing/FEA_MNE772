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

#include "truss3dfilemanager.h"

#include <QtWidgets>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QObject>

#include "dxfreader.h"

#define FT3D_ext "ft3d"
#define FTXL_ext "ftxl"
#define DXF_ext "dxf"


Truss3DFileManager::Truss3DFileManager(QTreeWidget *treeWidget)
    : treeWidget(treeWidget)
{
    //QDir::setCurrent("../models");
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


bool Truss3DFileManager::openFile(void)
{
//    currentfilename =
//            QFileDialog::getOpenFileName(this->treeWidget->parentWidget(), QObject::tr("Open Truss3D File"),
//                                         QDir::currentPath(),
//                                         QObject::tr("Truss3D Files (*.ftxl *.ft3d);;DXF File (*.dxf)"));
//    if (currentfilename.isEmpty())
//        return false;

    treeWidget->clear();

    QFileInfo file(currentfilename);
    QString type = file.completeSuffix();


    if(type == DXF_ext)
    {
        DXFReader dxfreader;
        dxfreader.readfile(currentfilename.toStdString().c_str());
        currentfilename.replace(DXF_ext, FT3D_ext);
        dxfreader.writeFT3Dfile(currentfilename.toStdString().c_str());
        this->readFt3dFile(currentfilename);
        currentfilename.replace(FT3D_ext, FTXL_ext);
    }
    else if(type == FT3D_ext)
    {
        this->readFt3dFile(currentfilename);
        currentfilename.replace(FT3D_ext, FTXL_ext);
    }
    else if(type == FTXL_ext)
        this->readFtxlFile(currentfilename);

    treeWidget->setColumnCount(2);
    treeWidget->resizeColumnToContents(1);
    treeWidget->resizeColumnToContents(0);
    treeWidget->setAlternatingRowColors(true);

    return true;
}

bool Truss3DFileManager::saveFile(void)
{
    if (currentfilename.isEmpty())
        return false;


    QFile file(currentfilename);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Truss3D File Manager"),
                             QObject::tr("Cannot write file %1:\n%2.")
                             .arg(currentfilename)
                             .arg(file.errorString()));
        return false;
    }

    writeFile(&file);
    file.close();

    return true;
}



bool Truss3DFileManager::read(QIODevice *device)
{
    rxml.setDevice(device);

    if (rxml.readNextStartElement()) {
        if (rxml.name() == "femtruss3d" && rxml.attributes().value("version") == "1.0")
            readXML();
        else
            rxml.raiseError(QObject::tr("The file is not an FEMTruss3D version 1.0 file."));
    }

    return !rxml.error();
}


QString Truss3DFileManager::errorString() const
{
    return QObject::tr("%1\nLine %2, column %3")
            .arg(rxml.errorString())
            .arg(rxml.lineNumber())
            .arg(rxml.columnNumber());
}


void Truss3DFileManager::readXML()
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "femtruss3d");

    while (rxml.readNextStartElement()) {
        if(rxml.name() == "mesh")
        {
            QTreeWidgetItem *mesh = createChildItem(0);
            //bool colapsed = (rxml.attributes().value("colapsed") != "yes");
            treeWidget->setItemExpanded(mesh, true);

            mesh->setIcon(0, meshIcon);
            mesh->setText(0, QObject::tr("Truss3D"));
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

void Truss3DFileManager::readBoundaryConditions(QTreeWidgetItem *item)
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

void Truss3DFileManager::readLoading(QTreeWidgetItem *item)
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
        else
            rxml.skipCurrentElement();
    }
}

void Truss3DFileManager::readDisplacements(QTreeWidgetItem *item)
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

void Truss3DFileManager::readMaterials(QTreeWidgetItem *item)
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

void Truss3DFileManager::readValue1D(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "value1d");

    QTreeWidgetItem *value1d = createChildItem(item);

    value1d->setFlags(value1d->flags() | Qt::ItemIsEditable);
    value1d->setIcon(0, valueIcon);
    value1d->setText(0, rxml.attributes().value("dim").toString());
    value1d->setText(1, rxml.attributes().value("value").toString());

    rxml.skipCurrentElement();
}

void Truss3DFileManager::readNodes(QTreeWidgetItem *item)
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

void Truss3DFileManager::readElements(QTreeWidgetItem *item)
{
    Q_ASSERT(rxml.isStartElement() && rxml.name() == "elements");

    QTreeWidgetItem *elements = createChildItem(item);
    //elements->setFlags(elements->flags() | Qt::ItemIsEditable);
    elements->setIcon(0, elementIcon);
    elements->setText(0, QObject::tr("Truss3DElement"));
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

QTreeWidgetItem *Truss3DFileManager::createChildItem(QTreeWidgetItem *item)
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

bool Truss3DFileManager::writeFile(QIODevice *device)
{
    wxml.setAutoFormatting(true);

    wxml.setDevice(device);

    wxml.writeStartDocument();
    wxml.writeDTD("<!DOCTYPE femtruss3d>");
    wxml.writeStartElement("femtruss3d");
    wxml.writeAttribute("version", "1.0");
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i)
        writeItem(treeWidget->topLevelItem(i));

    wxml.writeEndDocument();
    return true;
}

void Truss3DFileManager::writeItem(QTreeWidgetItem *item)
{
    QString tagName = item->data(0, Qt::UserRole).toString();
    if (tagName == "mesh") {
        //bool colapsed = !treeWidget->isItemExpanded(item);
        wxml.writeStartElement(tagName);
        wxml.writeAttribute("colapsed", "no");
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
    } else if (tagName == "support" || tagName == "force" || tagName == "displacement"
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

bool Truss3DFileManager::readFt3dFile(QString filename)
{
    QFile ft3dfile(filename);
    if (!ft3dfile.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Truss3D File Manager"),
                             QObject::tr("Cannot open file %1:\n%2.")
                             .arg(filename)
                             .arg(ft3dfile.errorString()));
        return false;
    }

    QString wxmlfilename = filename.replace(FT3D_ext, FTXL_ext);

    QFile wxmlfile(wxmlfilename);
    if (!wxmlfile.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Truss3D File Manager"),
                             QObject::tr("Cannot write file %1:\n%2.")
                             .arg(wxmlfilename )
                             .arg(wxmlfile.errorString()));
        return false;
    }

    wxml.setAutoFormatting(true);

    wxml.setDevice(&wxmlfile);

    wxml.writeStartDocument();
    wxml.writeDTD("<!DOCTYPE femtruss3d>");
    wxml.writeStartElement("femtruss3d");
    wxml.writeAttribute("version", "1.0");

    QTextStream file(&ft3dfile);
    QStringList str;
    QStringList dim;
    dim <<"x"<<"y"<<"z";

    // >> mesh
    QString tagName = "mesh";
    wxml.writeStartElement(tagName);
    wxml.writeAttribute("colapsed", "yes");
    // >> boundaryconditions
    tagName = "boundaryconditions";
    wxml.writeStartElement(tagName);

    str.clear();
    str<<file.readLine().split(" ");
    wxml.writeAttribute("count", str.at(0));

    // >> support
    int count = str.at(0).toInt();
    for(int i=0; i<count; i++)
    {
        tagName = "support";
        wxml.writeStartElement(tagName);
        str.clear();
        str<<file.readLine().split(" ");
        wxml.writeAttribute("index", str.at(0));
        tagName = "value1d";
        for(int j=0;j<3;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", str.at(j+1));
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

    str.clear();
    str<<file.readLine().split(" ");
    wxml.writeAttribute("count", str.at(0));

    // >> force
    count = str.at(0).toInt();
    for(int i=0; i<count; i++)
    {
        tagName = "force";
        wxml.writeStartElement(tagName);
        str.clear();
        str<<file.readLine().split(" ");
        wxml.writeAttribute("index", str.at(0));
        tagName = "value1d";
        for(int j=0;j<3;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", str.at(j+1));
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }
    // << force
    wxml.writeEndElement();
    // << loadig


    // >> displacements
    tagName = "displacements";
    wxml.writeStartElement(tagName);

    str.clear();
    str<<file.readLine().split(" ");
    wxml.writeAttribute("count", str.at(0));

    // >> displacement
    count = str.at(0).toInt();
    for(int i=0; i<count; i++)
    {
        tagName = "displacement";
        wxml.writeStartElement(tagName);
        str.clear();
        str<<file.readLine().split(" ");
        wxml.writeAttribute("index", str.at(0));
        tagName = "value1d";
        for(int j=0;j<3;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", str.at(j+1));
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

    str.clear();
    str<<file.readLine().split(" ");
    wxml.writeAttribute("count", str.at(0));

    // >> node
    count = str.at(0).toInt();
    for(int i=0; i<count; i++)
    {
        tagName = "node";
        wxml.writeStartElement(tagName);
        str.clear();
        str<<file.readLine().split(" ");
        wxml.writeAttribute("index", str.at(0));
        tagName = "value1d";
        for(int j=0;j<6;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", str.at(j+1));
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }
    // << node
    wxml.writeEndElement();
    // << nodes

    dim.clear();
    dim <<"name"<<"E"<<"A";
    // >> materials
    tagName = "materials";
    wxml.writeStartElement(tagName);

    str.clear();
    str<<file.readLine().split(" ");
    wxml.writeAttribute("count", str.at(0));

    // >> material
    count = str.at(0).toInt();
    for(int i=0; i<count; i++)
    {
        tagName = "material";
        wxml.writeStartElement(tagName);
        str.clear();
        str<<file.readLine().split(" ");
        wxml.writeAttribute("index", str.at(0));
        tagName = "value1d";
        for(int j=0;j<3;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", str.at(j+1));
            wxml.writeEndElement();
        }
        wxml.writeEndElement();
    }


    // << material
    wxml.writeEndElement();
    // << materials

    dim.clear();
    dim <<"node1"<<"node2"<<"material";
    // >> elements
    tagName = "elements";
    wxml.writeStartElement(tagName);

    str.clear();
    str<<file.readLine().split(" ");
    wxml.writeAttribute("count", str.at(0));

    // >> element
    count = str.at(0).toInt();
    for(int i=0; i<count; i++)
    {
        tagName = "element";
        wxml.writeStartElement(tagName);
        str.clear();
        str<<file.readLine().split(" ");
        wxml.writeAttribute("index", str.at(0));
        tagName = "value1d";
        for(int j=0;j<3;j++)
        {
            wxml.writeStartElement(tagName);
            wxml.writeAttribute("dim", dim.at(j));
            wxml.writeAttribute("value", str.at(j+1));
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
    //return true;

    ft3dfile.close();
    wxmlfile.close();

    //    QFile wxmlfile(wxmlfilename);
    if (!wxmlfile.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Truss3D File Manager"),
                             QObject::tr("Cannot read file %1:\n%2.")
                             .arg(wxmlfilename )
                             .arg(wxmlfile.errorString()));
        return false;
    }

    if (!read(&wxmlfile)) {
        QMessageBox::warning(this->treeWidget->parentWidget(), QObject::tr("Truss3D File Manager"),
                             QObject::tr("Parse error in file %1:\n\n%2")
                             .arg(wxmlfilename)
                             .arg(this->errorString()));
        return false;
    }

    wxmlfile.close();
    return true;
}

bool Truss3DFileManager::readFtxlFile(QString filename)
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



