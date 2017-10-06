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

#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <mth/matrix.h>

///
/// \brief The Material class
///
class Material
{
public:
    int index;
    std::string name;
    double E;
    double A;
    double I;

    double poisson;
    double density;

    Mth::Matrix D;


    Material();
    Material(int index, std::string name, double E, double A, double I = 0.0);
    void updateMatrixD(void);
};

#endif // MATERIAL_H
