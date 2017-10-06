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


#ifndef NODE3D_H
#define NODE3D_H

///
/// \brief The Node3D class
///
class Node3D
{
public:
    int index;
    double *coordinates;
    bool *restrictions;
    double *loading;
    double*displacements;



    Node3D();
    Node3D(int index, double *coordinates, bool *restrictions = nullptr, double *loading = nullptr, double *displacements = nullptr);
    ~Node3D();

};

#endif // NODE3D_H
