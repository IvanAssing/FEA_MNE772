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

#include "truss3delement.h"
#include <mth/matrix.h>
#include <GL/gl.h>


Truss3DElement::Truss3DElement()
{
    this ->index = -1;

    node1 = nullptr;
    node2 = nullptr;
    material = nullptr;

}


Truss3DElement::Truss3DElement(int index, Node3D *node1, Node3D *node2, Material *material)
{
    this->index = index;

    this->node1 = node1;
    this->node2 = node2;

    this->material = material;

}


Truss3DElement::Truss3DElement(int index, Truss3DElement *element)
{
    this->index = index;
    this->node1 = element->node1;
    this->node2 = element->node2;
    this->material = element->material;
}


void Truss3DElement::getStiffnessMatrix(Mth::Matrix &ke)
{
    double c[3];


    for(int i =0; i<3; i++)
        c[i] = (node2->coordinates[i] - node1->coordinates[i]); //ci

    double inv_l = 1.0/sqrt(c[0]*c[0]+c[1]*c[1]+c[2]*c[2]);



    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++)
        {
            ke(i,j) = material->E*material->A*c[i]*c[j]*inv_l*inv_l*inv_l;
            ke(i+3, j+3) = ke(i,j);
            ke(i+3,j) = -ke(i,j);
            ke(i, j+3) = -ke(i,j);
        }
}


double Truss3DElement::getStress(Mth::Matrix &ue)
{
    double c[3];


    for(int i =0; i<3; i++)
        c[i] = (node2->coordinates[i] - node1->coordinates[i]);

    double inv_l = 1.0/sqrt(c[0]*c[0]+c[1]*c[1]+c[2]*c[2]);

    double u[2];

    u[0] = c[0]*ue(0) + c[1]*ue(1) + c[2]*ue(2);
    u[1] = c[0]*ue(3) + c[1]*ue(4) + c[2]*ue(5);

    return (u[1]-u[0])*material->E*inv_l*inv_l;

}


void Truss3DElement::draw(void)
{
    glBegin(GL_LINES);
    glVertex3d(node1->coordinates[0],
            node1->coordinates[1],
            node1->coordinates[2]);
    glVertex3d(node2->coordinates[0],
            node2->coordinates[1],
            node2->coordinates[2]);
    glEnd();
}
