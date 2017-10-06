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

#ifndef TRUSS3DELEMENT_H
#define TRUSS3DELEMENT_H

#include "node3d.h"
#include "material.h"
#include <mth/matrix.h>

///
/// \brief The Truss3DElement class
///
class Truss3DElement
{
public:
    int index;
    Node3D *node1;
    Node3D *node2;
    Material *material;
    double E, A;

    Truss3DElement();
    Truss3DElement(int index, Truss3DElement *element);
    Truss3DElement(int index, Node3D *node1, Node3D *node2, Material *material);
    double getStress(Mth::Matrix &ue);
    void getStiffnessMatrix(Mth::Matrix &ke);
    void draw(void);

};

#endif // TRUSS3DELEMENT_H
