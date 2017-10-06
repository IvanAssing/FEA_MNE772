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

#ifndef SOLID3D_H
#define SOLID3D_H

#include "node3d.h"
#include "solid3delement.h"

#include <mth/matrix.h>
#include <mth/vector.h>

class Solid3DReader;

///
/// \brief The Solid3D class
///
class Solid3D
{
    friend class Solid3DReader;

private:
    int ndi, nlo, nre, nma;

    Mth::Matrix k;
    Mth::Vector f;

public:
    Node3D **nodes;
    Solid3DElement **elements;
    Material **materials;


    double **displacements, **loading;
    double pressure0;
    double pressure1;
    bool **restrictions;

    int nNodes;
    int nElements;
    bool isMounted,isSolved;


    Mth::Vector u;

    Mth::Vector reactions;
    Mth::Matrix Snodes;
    Mth::Matrix *Snodes_simulation;


    Mth::Vector Smax;
    Mth::Vector Smin;

    Solid3D();
    Solid3D(QString filename);


    void report(QString filename, bool isNodesInfo=true);

    void evalStiffnessMatrix(void);
    void evalLoadVector(double factor=1.0);

    void evalStressLimits(void);

    void solve(void);

    // variables for ramp computation
    int nSteps;
    Mth::Matrix f_simulation;
    Mth::Matrix u_simulation;

    Mth::Matrix reactions_simulation;
    Mth::Matrix *stress_simulation;

    void solve_simulation(int nSteps);
    //void stresslimits_simulation(double &min, double &max);
    bool isSolved_simulation;


//protected:
    virtual ~Solid3D();
};

#endif // SOLID3D_H
