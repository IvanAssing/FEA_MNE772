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

#ifndef TRUSS3D_H
#define TRUSS3D_H

#include "node3d.h"
#include "truss3delement.h"

#include "dxfreader.h"

#include <mth/matrix.h>
#include <mth/vector.h>

class Truss3DReader;

///
/// \brief The Truss3D class
///
class Truss3D
{
    friend class Truss3DReader;

private:
    int ndi, nlo, nre, nma;

public:
    Node3D **nodes;
    Truss3DElement **elements;
    Material **materials;


    double **displacements, **loading;
    bool **restrictions;

    int nNodes;
    int nElements;
    bool isMounted,isSolved;

    Mth::Matrix k;
    Mth::Vector f;
    Mth::Vector u;

    Mth::Vector reactions;
    Mth::Vector stress;

    Truss3D();
    Truss3D(char *filename);
    Truss3D(DXFReader *file);
    void loadfile(char *filename);
    void report(QString filename, bool isNodesInfo=true);

    void evalStiffnessMatrix(void);

    void stresslimits(double &min, double &max);

    void solve(void);

    // variables for ramp computation
    int nSteps;
    Mth::Matrix f_simulation;
    Mth::Matrix u_simulation;

    Mth::Matrix reactions_simulation;
    Mth::Matrix stress_simulation;

    void solve_simulation(int nSteps);
    void stresslimits_simulation(double &min, double &max);
    bool isSolved_simulation;

    virtual ~Truss3D();
};

#endif // TRUSS3D_H
