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

#include "material.h"

Material::Material(int index_, std::string name, double E, double A, double I)
    :index(index_), name(name), E(E), A(A), I(I)
{

}

Material::Material()
{
    index = 0;
    name = "unamed";
    E = 0.0;
    A = 0.0;
    I = 0.0;
    poisson = 0;
}

void Material::updateMatrixD(void)
{
    D.resize(6,6);
    D = 0.0;

    D(0,0) = 1.0-poisson;
    D(0,1) = poisson;
    D(0,2) = poisson;

    D(1,0) = poisson;
    D(1,1) = 1.0-poisson;
    D(1,2) = poisson;

    D(2,0) = poisson;
    D(2,1) = poisson;
    D(2,2) = 1.0-poisson;

    D(3,3) = 0.5-poisson;
    D(4,4) = 0.5-poisson;
    D(5,5) = 0.5-poisson;

    D *= E/((1.0+poisson)*(1.0-2.0*poisson));
}

