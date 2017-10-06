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

#ifndef SOLID3DELEMENT_H
#define SOLID3DELEMENT_H

#include "node3d.h"
#include "material.h"
#include <mth/matrix.h>
#include <QVector3D>

// according ANSYS spec
//const int idf[4][3] = {1, 0, 2,
//                       0, 1, 3,
//                       1, 2, 3,
//                       2, 0, 3};

// modified
const int idf[4][3] = {1, 2, 3,
                       2, 0, 3,
                       0, 1, 3,
                       1, 0, 2};

///
/// \brief The Solid3DElement class
///
class Solid3DElement
{
public:
    int index;
    Node3D **nodes;
    Material *material;
    Mth::Matrix B;
    double *pressure;
    double V;
    int pface;

    QVector3D *normals;
    double *areas;

    Solid3DElement();
    Solid3DElement(int index, Solid3DElement *element);
    Solid3DElement(int index, Node3D *node0, Node3D *node1, Node3D *node2, Node3D *node3, Material *material);
    void getStress(Mth::Vector &ue, Mth::Vector &se);
    void getStiffnessMatrix(Mth::Matrix &ke);
    void evaluateNormals(void);


//protected:
    virtual ~Solid3DElement();

};

#endif // SOLID3DELEMENT_H
