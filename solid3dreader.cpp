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

#include <QtWidgets>
#include "solid3dreader.h"


Solid3DReader::Solid3DReader(Solid3D *parent)
{
    mesh = parent;

}


Solid3D* Solid3DReader::read(Solid3DFileManager *solid3dfile)
{
    QFile file(solid3dfile->currentfilename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(solid3dfile->treeWidget->parentWidget(), QObject::tr("Solid3D File Manager"),
                             QObject::tr("Cannot open file %1:\n%2.")
                             .arg(solid3dfile->currentfilename)
                             .arg(file.errorString()));
        return nullptr;
    }

    xml.setDevice(&file);

    if (xml.readNextStartElement()) {
        if (xml.name() == "femsolid3d" && xml.attributes().value("version") == "1.0")
            readXML();
        else
            xml.raiseError(QObject::tr("The file is not an FEMSolid3D version 1.0 file."));
    }

    return this->mesh;
}


QString Solid3DReader::errorString() const
{
    return QObject::tr("%1\nLine %2, column %3")
            .arg(xml.errorString())
            .arg(xml.lineNumber())
            .arg(xml.columnNumber());
}


void Solid3DReader::readXML()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "femsolid3d");

    while (xml.readNextStartElement()) {
        if(xml.name() == "mesh")
        {
            if(mesh) delete mesh;
            mesh = new Solid3D;

            while (xml.readNextStartElement())
            {

                if (xml.name() == "boundaryconditions")
                    readBoundaryConditions();

                else if (xml.name() == "loading")
                    readLoading();

                else if (xml.name() == "displacements")
                    readDisplacements();

                else if (xml.name() == "materials")
                    readMaterials();

                else if (xml.name() == "nodes")
                    readNodes();

                else if (xml.name() == "elements")
                    readElements();

                else
                    xml.skipCurrentElement();
            }

            mesh->isMounted = true;
        }
    }
}

void Solid3DReader::readBoundaryConditions(void)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "boundaryconditions");

    mesh->nre = xml.attributes().value("count").toInt();
    mesh->restrictions= new bool *[mesh->nre];

    int i = 0;
    while (xml.readNextStartElement()) {
        if (xml.name() == "support")
        {
            mesh->restrictions[i] = new bool[3];
            //int index = xml.attributes().value("index").toInt();

            while (xml.readNextStartElement()) {
                if (xml.name() == "value1d")
                {
                    if(xml.attributes().value("dim").toString() == "x")
                        mesh->restrictions[i][0] = xml.attributes().value("value").toInt()!=0 ? true:false;
                    if(xml.attributes().value("dim").toString() == "y")
                        mesh->restrictions[i][1] = xml.attributes().value("value").toInt()!=0 ? true:false;
                    if(xml.attributes().value("dim").toString() == "z")
                        mesh->restrictions[i][2] = xml.attributes().value("value").toInt()!=0 ? true:false;
                    xml.skipCurrentElement();
                }
                else
                    xml.skipCurrentElement();
            }
            i++;
        }
        else
            xml.skipCurrentElement();
    }

}

void Solid3DReader::readLoading(void)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "loading");

    mesh->nlo = xml.attributes().value("count").toInt();
    mesh->loading = new double *[mesh->nlo];

    int i = 0;
    while (xml.readNextStartElement()) {
        if (xml.name() == "force")
        {

            mesh->loading[i] = new double[3];
            //int index = xml.attributes().value("index").toInt();
            int j=0;
            while (xml.readNextStartElement()) {
                if (xml.name() == "value1d")
                {
                    mesh->loading[i][j++]=xml.attributes().value("value").toDouble();
                    xml.skipCurrentElement();
                }
                else
                    xml.skipCurrentElement();
            }
            i++;
        }
        else if (xml.name() == "pressure")
        {

            //mesh->pressure = new double;
            //int index = xml.attributes().value("index").toInt();
            while (xml.readNextStartElement()) {
                if (xml.name() == "value1d")
                {
                    mesh->pressure0 = 0.0;
                    mesh->pressure1 = xml.attributes().value("value").toDouble();
                    xml.skipCurrentElement();
                }
                else
                    xml.skipCurrentElement();
            }
        }
        else
            xml.skipCurrentElement();
    }

}

void Solid3DReader::readDisplacements(void)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "displacements");

    mesh->ndi = xml.attributes().value("count").toInt();
    mesh->displacements = new double *[mesh->ndi];

    int i = 0;
    while (xml.readNextStartElement()) {
        if (xml.name() == "displacement")
        {
            mesh->displacements[i] = new double[3];
            //int index = xml.attributes().value("index").toInt();
            int j=0;
            while (xml.readNextStartElement()) {
                if (xml.name() == "value1d")
                {
                    mesh->displacements[i][j++]=xml.attributes().value("value").toDouble();
                    xml.skipCurrentElement();
                }
                else
                    xml.skipCurrentElement();
            }
            i++;
        }
        else
            xml.skipCurrentElement();
    }

}

void Solid3DReader::readMaterials(void)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "materials");

    mesh->nma = xml.attributes().value("count").toInt();
    mesh->materials= new Material *[mesh->nma];

    int i = 0;
    while (xml.readNextStartElement()) {
        if (xml.name() == "material")
        {
            int index = xml.attributes().value("index").toInt();
            mesh->materials[i] = new Material(index, "unamed", 0.0, 0.0);

            while (xml.readNextStartElement()) {
                if (xml.name() == "value1d"){
                    if(xml.attributes().value("dim").toString() == "name")
                        mesh->materials[i]->name = xml.attributes().value("value").toString().toStdString();
                    if(xml.attributes().value("dim").toString() == "E")
                        mesh->materials[i]->E = xml.attributes().value("value").toDouble();
                    if(xml.attributes().value("dim").toString() == "density")
                        mesh->materials[i]->density = xml.attributes().value("value").toDouble();
                    if(xml.attributes().value("dim").toString() == "poison")
                        mesh->materials[i]->poisson = xml.attributes().value("value").toDouble();
                    xml.skipCurrentElement();
                }
                else
                    xml.skipCurrentElement();
            }
            i++;
        }
        else
            xml.skipCurrentElement();
    }

}


void Solid3DReader::readNodes(void)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "nodes");

    mesh->nNodes = xml.attributes().value("count").toInt();
    mesh->nodes = new Node3D *[mesh->nNodes];

    int i = 0;

    double *tempCoord = new double[3];
    int til, tid, tir;

    while (xml.readNextStartElement()) {
        if (xml.name() == "node")
        {
            int index = xml.attributes().value("index").toInt();


            while (xml.readNextStartElement()) {
                if (xml.name() == "value1d")
                {
                    if(xml.attributes().value("dim").toString() == "x")
                        tempCoord[0] = xml.attributes().value("value").toDouble();
                    if(xml.attributes().value("dim").toString() == "y")
                        tempCoord[1] = xml.attributes().value("value").toDouble();
                    if(xml.attributes().value("dim").toString() == "z")
                        tempCoord[2] = xml.attributes().value("value").toDouble();
                    if(xml.attributes().value("dim").toString() == "support")
                        tir = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "force")
                        til = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "displacement")
                        tid = xml.attributes().value("value").toInt();
                    xml.skipCurrentElement();

                }
                else
                    xml.skipCurrentElement();
            }

            mesh->nodes[i] = new Node3D(index, tempCoord, mesh->restrictions[tir], mesh->loading[til], mesh->displacements[tid]);
            i++;

        }
        else
            xml.skipCurrentElement();
    }

}

void Solid3DReader::readElements(void)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "elements");

    mesh->nElements = xml.attributes().value("count").toInt();
    mesh->elements = new Solid3DElement *[mesh->nElements];

    int i = 0;


    int inode0, inode1, inode2, inode3;
    int itm;
    int iface;
    int pressure;

    while (xml.readNextStartElement()) {
        if (xml.name() == "element")
        {
            int index = xml.attributes().value("index").toInt();

            while (xml.readNextStartElement()) {
                if (xml.name() == "value1d")
                {
                    if(xml.attributes().value("dim").toString() == "node0")
                        inode0 = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "node1")
                        inode1 = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "node2")
                        inode2 = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "node3")
                        inode3 = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "material")
                        itm = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "pface")
                        iface= xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "pressure")
                        pressure= xml.attributes().value("value").toInt();
                    xml.skipCurrentElement();
                }
                else
                    xml.skipCurrentElement();
            }
            mesh->elements[i] = new  Solid3DElement(index, mesh->nodes[inode0], mesh->nodes[inode1],
                                                    mesh->nodes[inode2], mesh->nodes[inode3],  mesh->materials[itm]);
            mesh->elements[i]->pface = iface;
            if(pressure == 0)
            mesh->elements[i]->pressure = &mesh->pressure0;
            else
                mesh->elements[i]->pressure = &mesh->pressure1;

            i++;
        }
        else
            xml.skipCurrentElement();
    }
}


