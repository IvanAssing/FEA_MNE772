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

#ifndef MODEL_H
#define MODEL_H

#include "truss3d.h"
#include "solid3d.h"

#include "truss3dfilemanager.h"
#include "solid3dfilemanager.h"




enum ModelType{
    truss3d,
    solid3d
};

class Model
{
public:
    Model();
    Model(QTreeWidget *treeWidget);
    Truss3DFileManager *t3d_fileManager;
    Solid3DFileManager *s3d_fileManager;

    Truss3D *t3d_mesh;
    Solid3D *s3d_mesh;

};

#endif // MODEL_H
