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

#include "solid3delement.h"
#include <mth/matrix.h>

#define NN 4

Solid3DElement::Solid3DElement()
{
    index = -1;

    nodes = nullptr;
    material = nullptr;
    areas = nullptr;
    normals = nullptr;
    pface = -1;

}


Solid3DElement::Solid3DElement(int index, Node3D *node0, Node3D *node1, Node3D *node2, Node3D *node3, Material *material)
{
    this->index = index;

    nodes = new Node3D*[NN];
    nodes[0] = node0;
    nodes[1] = node1;
    nodes[2] = node2;
    nodes[3] = node3;

    this->material = material;
    pface = -1;

    areas = nullptr;
    normals = nullptr;

}


Solid3DElement::Solid3DElement(int index, Solid3DElement *element)
{
    this->index = index;
    nodes = new Node3D*[NN];
    nodes[0] = element->nodes[0];
    nodes[1] = element->nodes[1];
    nodes[2] = element->nodes[2];
    nodes[3] = element->nodes[3];

    this->material = element->material;

    areas = nullptr;
    normals = nullptr;
}


void Solid3DElement::getStiffnessMatrix(Mth::Matrix &ke)
{
    B.resize(6,12);
    B = 0.0;

    double x[4];
    double y[4];
    double z[4];

    double /*a,*/b,c,d;
    int id[4];

    for(int i=0; i<4; i++)
    {
        id[0] = i;
        id[1] = id[0]+1>3? 0:id[0]+1;
        id[2] = id[1]+1>3? 0:id[1]+1;
        id[3] = id[2]+1>3? 0:id[2]+1;

        for(int j=0;j<4;j++)
        {
            x[j] = nodes[id[j]]->coordinates[0];
            y[j] = nodes[id[j]]->coordinates[1];
            z[j] = nodes[id[j]]->coordinates[2];
        }


        b = -(y[2]*z[3]+y[1]*z[2]+y[3]*z[1])+(y[2]*z[1]+y[1]*z[3]+y[3]*z[2]);
        c = -(x[1]*z[3]+x[2]*z[1]+x[3]*z[2])+(x[3]*z[1]+x[2]*z[3]+x[1]*z[2]);
        d = -(x[1]*y[2]+x[2]*y[3]+x[3]*y[1])+(x[3]*y[2]+x[2]*y[1]+x[1]*y[3]);

        // erro no livro do zienkiewicz and taylor??????
        b *= ((i+2)%2)? -1:1;
        c *= ((i+1)%2)?  1:-1;
        d *= ((i+2)%2)? -1:1;

        if(i==0)
        {
            //a =  x[1]*y[2]*z[3]+x[3]*y[1]*z[2]+x[2]*y[3]*z[1]-(x[3]*y[2]*z[1]+x[2]*y[1]*z[3]+x[1]*y[3]*z[2]);

            V = (x[1]-x[0])*(y[2]-y[0])*(z[3]-z[0]) + (x[2]-x[0])*(y[3]-y[0])*(z[1]-z[0]) + (x[3]-x[0])*(y[1]-y[0])*(z[2]-z[0])
                    - (x[3]-x[0])*(y[2]-y[0])*(z[1]-z[0]) - (x[2]-x[0])*(y[1]-y[0])*(z[3]-z[0]) - (x[1]-x[0])*(y[3]-y[0])*(z[2]-z[0]);
            V /= 6.0;

        }

        B(0,3*i+0) = b;
        B(1,3*i+1) = c;
        B(2,3*i+2) = d;

        B(3,3*i+0) = c;
        B(3,3*i+1) = b;

        B(4,3*i+1) = d;
        B(4,3*i+2) = c;

        B(5,3*i+0) = d;
        B(5,3*i+2) = b;

    }

    B *= 1.0/(6.0*V);

    Mth::Matrix temp(6,12);
    Mth::Matrix Bt(12,6);

    for(int i=0; i<12; i++)
        for(int j=0; j<6; j++)
            Bt(i,j) = B(j,i);

    temp = material->D*B;
    //ke = B.ATxB(temp);
    ke = Bt*temp;

    ke *= V;

}


void Solid3DElement::getStress(Mth::Vector &ue, Mth::Vector &se)
{
    se = material->D*B*ue;
}



void Solid3DElement::evaluateNormals(void)
{
    if(areas) delete [] areas;
    if(normals) delete [] normals;

    normals = new QVector3D[4];
    areas = new double[4];


    for(int i=0; i<4; i++)
    {
        QVector3D v1(nodes[idf[i][1]]->coordinates[0] - nodes[idf[i][0]]->coordinates[0],
                nodes[idf[i][1]]->coordinates[1] - nodes[idf[i][0]]->coordinates[1],
                nodes[idf[i][1]]->coordinates[2] - nodes[idf[i][0]]->coordinates[2]);

        QVector3D v2(nodes[idf[i][2]]->coordinates[0] - nodes[idf[i][1]]->coordinates[0],
                nodes[idf[i][2]]->coordinates[1] - nodes[idf[i][1]]->coordinates[1],
                nodes[idf[i][2]]->coordinates[2] - nodes[idf[i][1]]->coordinates[2]);

        normals[i] = QVector3D::crossProduct(v1, v2);

        areas[i] = 0.5* normals[i].length();

        normals[i].normalize();
    }
}

Solid3DElement::~Solid3DElement()
{
    if(nodes) delete [] nodes;
    if(areas) delete [] areas;
    if(normals) delete [] normals;
}


