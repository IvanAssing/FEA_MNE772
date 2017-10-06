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

#ifndef TRUSS3DREADER_H
#define TRUSS3DREADER_H

#include <QIcon>
#include <QXmlStreamReader>
#include "truss3d.h"
#include "truss3dfilemanager.h"

///
/// \brief The Truss3DReader class
///
class Truss3DReader
{
public:
    Truss3DReader(Truss3D *parent);
    Truss3D *mesh;

    Truss3D* read(Truss3DFileManager *file);

    QString errorString() const;

private:
    void readXML();
    void readBoundaryConditions(void);
    void readLoading(void);
    void readDisplacements(void);
    void readMaterials(void);
    void readNodes(void);
    void readElements(void);

    QXmlStreamReader xml;

};
//! [0]

#endif
