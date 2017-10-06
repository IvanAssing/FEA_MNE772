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

#include "truss3dreader.h"



Truss3DReader::Truss3DReader(Truss3D *parent)
{
    mesh = parent;

}


Truss3D* Truss3DReader::read(Truss3DFileManager *truss3dfile)
{
    QFile file(truss3dfile->currentfilename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(truss3dfile->treeWidget->parentWidget(), QObject::tr("Truss3D File Manager"),
                             QObject::tr("Cannot open file %1:\n%2.")
                             .arg(truss3dfile->currentfilename)
                             .arg(file.errorString()));
        return nullptr;
    }

    xml.setDevice(&file);

    if (xml.readNextStartElement()) {
        if (xml.name() == "femtruss3d" && xml.attributes().value("version") == "1.0")
            readXML();
        else
            xml.raiseError(QObject::tr("The file is not an FEMTruss3D version 1.0 file."));
    }

    return this->mesh;
}


QString Truss3DReader::errorString() const
{
    return QObject::tr("%1\nLine %2, column %3")
            .arg(xml.errorString())
            .arg(xml.lineNumber())
            .arg(xml.columnNumber());
}


void Truss3DReader::readXML()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "femtruss3d");

    while (xml.readNextStartElement()) {
        if(xml.name() == "mesh")
        {
            delete mesh;
            mesh = new Truss3D;

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

void Truss3DReader::readBoundaryConditions(void)
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

void Truss3DReader::readLoading(void)
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
        else
            xml.skipCurrentElement();
    }

}

void Truss3DReader::readDisplacements(void)
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

void Truss3DReader::readMaterials(void)
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
                    if(xml.attributes().value("dim").toString() == "A")
                        mesh->materials[i]->A = xml.attributes().value("value").toDouble();
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


void Truss3DReader::readNodes(void)
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

void Truss3DReader::readElements(void)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "elements");

    mesh->nElements = xml.attributes().value("count").toInt();
    mesh->elements = new Truss3DElement *[mesh->nElements];

    int i = 0;


    int index_no1, index_no2;
    int itm;

    while (xml.readNextStartElement()) {
        if (xml.name() == "element")
        {
            int index = xml.attributes().value("index").toInt();

            while (xml.readNextStartElement()) {
                if (xml.name() == "value1d")
                {
                    if(xml.attributes().value("dim").toString() == "node1")
                        index_no1 = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "node2")
                        index_no2 = xml.attributes().value("value").toInt();
                    if(xml.attributes().value("dim").toString() == "material")
                        itm = xml.attributes().value("value").toInt();
                    xml.skipCurrentElement();
                }
                else
                    xml.skipCurrentElement();
            }
            //qDebug()<<"\n"<<index<<" "<<index_no1<<" "<<index_no2<<" "<<itm;
            mesh->elements[i] = new Truss3DElement(index, mesh->nodes[index_no1], mesh->nodes[index_no2], mesh->materials[itm]);
            i++;
        }
        else
            xml.skipCurrentElement();
    }
}


