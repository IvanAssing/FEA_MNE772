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

#include "solid3d.h"

#include <fstream>
#include <iostream>
#include <istream>
#include <string>
#include <cmath>
#include <QString>
#include <QFile>
#include <QStringList>
#include <QElapsedTimer>
#include <QTextStream>

#include <mth/matrix.h>

#define buffersize 10


Solid3D::Solid3D()
{
    nNodes = 0;
    nElements = 0;
    isSolved = false;
    isMounted = false;
    isSolved_simulation = false;
}


Solid3D::Solid3D(QString filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        std::cerr<<"error in opening file";

    QTextStream text(&file);

    QString line;
    QStringList str;

    nre = 2;
    nlo = 2;
    ndi = 2;

    // default bondary condition
    restrictions = new bool*[nre];
    restrictions[0] = new bool[3];
    restrictions[0][0] = false;
    restrictions[0][1] = false;
    restrictions[0][2] = false;

    // default displacements
    displacements = new double*[ndi];
    displacements[0] = new double[3];
    displacements[0][0] = 0.0;
    displacements[0][1] = 0.0;
    displacements[0][2] = 0.0;


    // default loading
    loading = new double*[nlo];
    loading [0] = new double[3];
    loading [0][0] = 0.0;
    loading [0][1] = 0.0;
    loading [0][2] = 0.0;

    restrictions[1] = new bool[3];
    displacements[1] = new double[3];
    loading[1] = new double[3];

    do {
        line = text.readLine();
        if(line.isNull())
            break;
        str = line.split(",", QString::SkipEmptyParts);

        if(str[0]=="NUMOFF")
        {
            if(str[1]=="NODE")
            {
                nNodes = str[2].toInt();
                nodes = new Node3D*[nNodes];
            }
            else if(str[1]=="ELEM")
            {
                nElements = str[2].toInt();
                elements = new Solid3DElement*[nElements];
            }
            else if(str[1]=="MAT ")
            {
                nma= str[2].toInt();
                materials = new Material*[nma];

                for(int i=0; i<nma; i++)
                {
                    materials[i] = new Material;
                    materials[i]->index = i;
                    materials[i]->name = QString("material %1").arg(i).toStdString();
                }
            }
        }

        if(str[0]=="MP")
        {
            do {
                int index = str[2].toInt()-1;

                if(str[1]=="DENS")
                    materials[index]->density = str[3].toDouble();
                else if(str[1]=="EX")
                    materials[index]->E = str[3].toDouble();
                else if(str[1]=="PRXY")
                    materials[index]->poisson = str[3].toDouble();

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);
            }while(str[0]=="MP");
        }

        if(str[0]=="N")
        {
            double coord[3];
            for(int i=0; i<nNodes; i++)
            {
                if(str[0] != "N")
                    break;
                if(str[3].toInt()-1 != i) std::cerr<<"error in nodes indexing";
                coord[0] = str[4].toDouble();
                coord[1] = str[5].toDouble();
                coord[2] = str[6].toDouble();

                nodes[i] = new Node3D(i, coord, restrictions[0],
                        loading[0], displacements[0]);

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);
            }
        }

        if(str[0]=="EN")
        {
            int ima;
            for(int i=0; i<nElements+1; i++)
            {
                str = line.split(",", QString::SkipEmptyParts);
                if(str[0] != "EN")
                    break;
                //std::cout<<"\n:"<<i<<"<<<< "<<str[0].toStdString()<<" >>>>"<<line.toStdString();

                ima = str[4].toInt()-1;
                if(str[7].toInt()-1 != i) std::cerr<<"error in elementss indexing";

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);

                elements[i] = new Solid3DElement(
                            i,
                            nodes[str[3].toInt()-1],
                        nodes[str[4].toInt()-1],
                        nodes[str[5].toInt()-1],
                        nodes[str[6].toInt()-1],
                        materials[ima]);

                line = text.readLine();
                //str = line.split(",", QString::SkipEmptyParts);

            }
        }


        if(str[0]=="SFE")
        {
            int iel;
            do {
                iel = str[1].toInt()-1;
                elements[iel]->pface = str[2].toInt()-1;
                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);
                pressure0 = 0.0;
                pressure1 = str[0].toDouble();
                elements[iel]->pressure = &pressure1;

                line = text.readLine();
                str = line.split(",", QString::SkipEmptyParts);
            }while(str[0]=="SFE");
        }


        if(str[0]=="D")
        {
            //            restrictions[1] = new bool[3];
            //            displacements[1] = new double[3];

            restrictions[1][0] = true;
            restrictions[1][1] = true;
            restrictions[1][2] = true;

            displacements[1][0] = str[3].toDouble();
            displacements[1][1] = 0.0;
            displacements[1][2] = 0.0;

            int inode;

            do {
                inode = str[1].toInt()-1;
                nodes[inode]->restrictions = restrictions[1];
                nodes[inode]->displacements = displacements[1];
                line = text.readLine();
                if(line.isNull())
                    break;
                str = line.split(",", QString::SkipEmptyParts);
            }while(str[0]=="D");
        }


    } while (!line.isNull());

}


void Solid3D::evalStiffnessMatrix(void)
{
    k.resize(3*nNodes, 3*nNodes);
    k = 0.0;

    for(int i=0; i<nma; i++)
        materials[i]->updateMatrixD();

    // global stiffness matrix
    Mth::Matrix ke(12,12);

    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
    for(int iel=0; iel<nElements; iel++)
    {
        elements[iel]->getStiffnessMatrix(ke);

        for(int i=0; i<4; i++)
            for(int j=0; j<4; j++)
                for(int ii=0; ii<3; ii++)
                    for(int jj=0; jj<3; jj++)
                        k(3*elements[iel]->nodes[i]->index+ii, 3*elements[iel]->nodes[j]->index+jj) += ke(3*i+ii, 3*j+jj);
    }
}


void Solid3D::evalLoadVector(double factor)
{
    f.resize(3*nNodes);
    f = 0.0;

    for(int i=0; i<nElements; i++)
        elements[i]->evaluateNormals();

    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
    for(int i=0; i<nNodes; i++)
    {
        f(3*nodes[i]->index) = factor * nodes[i]->loading[0];
        f(3*nodes[i]->index+1) = factor * nodes[i]->loading[1];
        f(3*nodes[i]->index+2) = factor * nodes[i]->loading[2];
    }

    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
    for(int i=0; i<nElements; i++)
        if(elements[i]->pface!=-1)
        {
            int iface = elements[i]->pface;

            double force = factor * *(elements[i]->pressure)*elements[i]->areas[iface];
            //force *= -100000;

            for(int j=0; j<3; j++)
            {
                f(3*elements[i]->nodes[idf[iface][j]]->index+0) += -force*elements[i]->normals[iface].x();
                f(3*elements[i]->nodes[idf[iface][j]]->index+1) += -force*elements[i]->normals[iface].y();
                f(3*elements[i]->nodes[idf[iface][j]]->index+2) += -force*elements[i]->normals[iface].z();
            }
        }
}


void Solid3D::solve(void)
{
    std::ofstream flog("/home/ivan/Projects/data3/log_solver.txt");

    Mth::Matrix kc(k); // cópias
    Mth::Vector fc(f);

    // Aplica as condicoes de contorno
    for(int i=0; i<nNodes; i++)
        for(int j=0; j<3; j++)
            if(nodes[i]->restrictions[j]==true)
            {
                //flog<<"\n"<<i<<" "<<nodes[i]->restrictions[j];
                int n = 3*nodes[i]->index+j;
                for(int t=0; t<3*nNodes; t++)
                {
                    kc(n, t) = 0.0;
                    //kc(t, n) = 0.0;
                }
                kc(n, n) = 1.0;
                fc(n) = nodes[i]->displacements[j];
            }


    //    flog<<"\n\n Matriz de rigidez\n";
    //    Mth::Matrix print(kc);
    //    print *= 1e-8;
    //    //kc *= 1e-8;
    //    flog<<print;
    //    //kc *= 1e+8;

    flog<<"\n\n Vetor de carga\n";
    flog<<f;

    //    Mth::Matrix kcc(kc);
    //    Mth::Vector fcc(fc);

    //    std::cerr<<"\n\n Teste\n";
    //    kcc.inverse();
    //    std::cerr<<kcc*fcc;

    // Aloca vetor para resultados
    u.resize(3*nNodes);

    kc.solve(fc, u);


    //    kc.inverse();
    //    u = kc * f;

    u.clear(1e-11);

    flog<<"\n\n Solução\n";
    flog<<u;

    //    flog<<"\n\n Error\n";
    //    Mth::Vector error = fcc-kcc*u;
    //    error.clear(1e-10);
    //    flog<<error;


    //    std::cerr<<"\n\n Matriz de rigidez\n";
    //    std::cerr<<k;
    reactions.resize(3*nNodes);

    reactions = k*u;

    reactions.clear(1.e-10);

    //    flog<<"\n\n Reações\n";
    //    flog<<reactions;


    Mth::Vector ue(12);
    Mth::Vector se(12);

    Mth::Matrix S(nElements, 7); //n1, n2, n3, n12, n23, n31, von mises

    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)

    for(int i=0; i<nElements; i++)
    {
        for(int j=0; j<4; j++)
        {
            ue(3*j+0) = u(3*elements[i]->nodes[j]->index+0);
            ue(3*j+1) = u(3*elements[i]->nodes[j]->index+1);
            ue(3*j+2) = u(3*elements[i]->nodes[j]->index+2);
        }

        elements[i]->getStress(ue,se);

        for(int j=0; j<6; j++)
            S(i,j) = se(j);

        // Von Mises stress
        S(i,6) = sqrt(0.5*((se(0)-se(1))*(se(0)-se(1)) + (se(1)-se(2))*(se(1)-se(2)) +
                           (se(2)-se(0))*(se(2)-se(0)) +
                           6.0*(se(3)*se(3) + se(4)*se(4) + se(5)*se(5))));
    }

    //    flog<<"\n\n Tensoes normais (elementos)\n";
    //    flog<<S;

    Snodes.resize(nNodes, 10); //n1, n2, n3, n12, n23, n31, von mises, ux, uy, uz
    Mth::Vector contribution(nNodes);

    Snodes = 0.0;
    contribution = 0.0;

    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
    for(int i=0; i<nElements; i++)
    {
        for(int j=0; j<4; j++)
        {
            int nid = elements[i]->nodes[j]->index;
            contribution(nid) += 1.0;
            for(int t=0; t<7; t++)
                Snodes(nid,t) += S(i,t);
        }
    }

    for(int i=0; i<nNodes; i++)
        for(int t=0; t<7; t++)
            Snodes(i,t) /= contribution(i);

    for(int i=0; i<nNodes; i++)
    {
        Snodes(i,7) = u(3*i); // ux
        Snodes(i,8) = u(3*i+1); // uy
        Snodes(i,9) = u(3*i+2); // uz
    }


    //    //    stress.clear();
    //    flog<<"\n\n Tensoes normais (nodes)\n";
    //    flog<<Snodes;



    Smax.resize(10);
    Smin.resize(10);

    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
    for(int t=0; t<10; t++)
    {
        Smax(t) = Snodes(0,t);
        Smin(t) = Snodes(0,t);
        for(int i=0; i<nNodes; i++)
        {
            if(Snodes(i,t)>Smax(t)) Smax(t) = Snodes(i,t);
            if(Snodes(i,t)<Smin(t)) Smin(t) = Snodes(i,t);
        }
    }

    flog<<"\n\n Tensoes maximas\n";
    flog<<Smax;

    flog<<"\n\n Tensoes minimas\n";
    flog<<Smin;


    flog.close();

    isSolved = true;
}


void Solid3D::solve_simulation(int steps)
{
    nSteps = steps;

    f_simulation.resize(3*nNodes, nSteps);
    //f_simulation = 0.0;

    double delta = 1.0/nSteps;
    double factor;

    for(int k=0; k<nSteps; k++)
    {
        factor = k*delta;
        for(int i=0; i<3*nNodes; i++)
            f_simulation(i, k) = factor * f(i);
    }



    Mth::Matrix kc(k); // cópias
    Mth::Matrix fc(f_simulation);

    // Aplica as condicoes de contorno
    for(int i=0; i<nNodes; i++)
        for(int j=0; j<3; j++)
            if(nodes[i]->restrictions[j]==true)
            {
                int n = 3*nodes[i]->index+j;
                for(int t=0; t<3*nNodes; t++)
                    kc(n, t) = kc(t, n) =  0.0;

                kc(n, n) = 1.0;
                for(int k=0; k<nSteps; k++)
                    fc(n,k) = nodes[i]->displacements[j];
            }
    //    std::cerr<<"ckecking symmetry...";
    //    for(int i=0; i<3*nNodes; i++)
    //        for(int j=0; j<3*nNodes; j++)
    //            if(fabs(kc(i,j)-kc(j,i))>1e-5)
    //                std::cerr<<"\n"<<i<<" "<<j<<" - "<<fabs(kc(i,j)-kc(j,i));

    u_simulation.resize(3*nNodes, nSteps);


    QElapsedTimer timer;
    timer.start();
    std::cerr<<"start solving linear system...\n";
    //kc.solve(fc, u_simulation);
    //kc.solve_symmetric(fc, u_simulation);
    //kc.solve_gpu(fc, u_simulation);
    //*
    /////////////////////////////
    Mth::Matrix fcc(3*nNodes);
    Mth::Matrix ucc(3*nNodes);
    for(int i=0; i<3*nNodes; i++)
    {
        fcc(i) = f_simulation(i,nSteps-1);
    }
    kc.solve_sparse(fcc, ucc);

    for(int k=0; k<nSteps; k++)
    {
        factor = k*delta;
        for(int i=0; i<3*nNodes; i++)
            u_simulation(i,k) = ucc(i)*factor;
    }
    /////////////////////////////
    //*/

    std::cerr<<"timing 0: "<<timer.elapsed()/1000.;

    //reactions_simulation.resize(3*nNodes, nSteps);

    //reactions_simulation = k*u_simulation;


    //std::cerr<<"timing 1: "<<timer.elapsed()/1000.;

    Mth::Vector ue(12);
    Mth::Vector se(12);

    Mth::Matrix *S = new Mth::Matrix[nSteps];

    for(int k=0; k<nSteps; k++)
    {

        S[k].resize(nElements, 7); //n1, n2, n3, n12, n23, n31, von mises

    }

    //std::cerr<<"timing 2: "<<timer.elapsed()/1000.;

    //#pragma omp parallel for schedule(static) num_threads(8)
    for(int i=0; i<nElements; i++)
        for(int k=0; k<nSteps; k++)
        {
            for(int j=0; j<4; j++)
            {
                ue(3*j+0) = u_simulation(3*elements[i]->nodes[j]->index+0,k);
                ue(3*j+1) = u_simulation(3*elements[i]->nodes[j]->index+1,k);
                ue(3*j+2) = u_simulation(3*elements[i]->nodes[j]->index+2,k);
            }

            elements[i]->getStress(ue,se);

            for(int j=0; j<6; j++)
                S[k](i,j) = se(j);

            // Von Mises stress
            S[k](i,6) = sqrt(0.5*((se(0)-se(1))*(se(0)-se(1)) + (se(1)-se(2))*(se(1)-se(2)) +
                                  (se(2)-se(0))*(se(2)-se(0)) +
                                  6.0*(se(3)*se(3) + se(4)*se(4) + se(5)*se(5))));
        }

    //std::cerr<<"timing 3: "<<timer.elapsed()/1000.;

    Snodes_simulation = new Mth::Matrix[nSteps];
    Mth::Vector contribution(nNodes);

    for(int k=0; k<nSteps; k++)
    {
        Snodes_simulation[k].resize(nNodes, 10); //n1, n2, n3, n12, n23, n31, von mises, ux, uy, uz

        Snodes_simulation[k] = 0.0;
        contribution = 0.0;

        //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
        for(int i=0; i<nElements; i++)
        {
            for(int j=0; j<4; j++)
            {
                int nid = elements[i]->nodes[j]->index;
                contribution(nid) += 1.0;
                for(int t=0; t<7; t++)
                    Snodes_simulation[k](nid,t) += S[k](i,t);
            }
        }

        for(int i=0; i<nNodes; i++)
            for(int t=0; t<7; t++)
                Snodes_simulation[k](i,t) /= contribution(i);

        for(int i=0; i<nNodes; i++)
        {
            Snodes_simulation[k](i,7) = u_simulation(3*i, k); // ux
            Snodes_simulation[k](i,8) = u_simulation(3*i+1, k); // uy
            Snodes_simulation[k](i,9) = u_simulation(3*i+2, k); // uz
        }

    }

    Smax.resize(10);
    Smin.resize(10);

    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
    for(int t=0; t<10; t++)
    {
        Smax(t) = Snodes_simulation[0](0,t);
        Smin(t) = Snodes_simulation[0](0,t);
        for(int k=0; k<nSteps; k++)
            for(int i=0; i<nNodes; i++)
            {
                if(Snodes_simulation[k](i,t)>Smax(t)) Smax(t) = Snodes_simulation[k](i,t);
                if(Snodes_simulation[k](i,t)<Smin(t)) Smin(t) = Snodes_simulation[k](i,t);
            }
    }

    isSolved_simulation = true;


    u.resize(3*nNodes);
    reactions.resize(3*nNodes);
    Snodes.resize(nElements,10);

    for(int i=0; i<3*nNodes; i++)
    {
        u(i) = u_simulation(i,nSteps-1);
        reactions(i) = reactions_simulation(i,nSteps-1);
    }


    for(int t=0; t<10; t++)
        for(int i=0; i<nElements; i++)
        {
            Snodes(i,t) = Snodes_simulation[nSteps-1](i, t);
        }

    //std::cerr<<"timing 4: "<<timer.elapsed()/1000.;

    isSolved = true;

}


void Solid3D::evalStressLimits(void)
{
    Smax.resize(7);
    Smin.resize(7);

    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
    for(int t=0; t<7; t++)
    {
        Smax(t) = Snodes(0,t);
        Smin(t) = Snodes(0,t);
        for(int i=0; i<nNodes; i++)
        {
            if(Snodes(i,t)>Smax(t)) Smax(t) = Snodes(i,t);
            if(Snodes(i,t)<Smin(t)) Smin(t) = Snodes(i,t);
        }
    }
}


//void Solid3D::stresslimits_simulation(double &min, double &max)
//{
//    min = stress_simulation(0,0);
//    max = stress_simulation(0,0);

//    for(int k=0; k<stress_simulation.n; k++)
//        for(int i=1; i<stress_simulation.m; i++)
//        {
//            double value = stress_simulation(i,k);
//            if(value>max) max = value;
//            if(value<min) min = value;
//        }

//}


Solid3D::~Solid3D()
{
    if(isMounted)
    {
        for(int i=0; i<nElements; i++)
            delete elements[i];
        delete [] elements;

        for(int i=0; i<nNodes; i++)
            delete nodes[i];
        delete [] nodes;
    }
}


void Solid3D::report(QString filename, bool isNodesInfo)
{

    if(isNodesInfo)
    {
        std::ofstream flog(filename.toStdString());

        flog<<"Node"<<","<<"Coordinate x"<<","<<"Coordinate y"<<","<<"Coordinate z";//<<std::endl;
        flog<<","<<"Restriction x"<<","<<"Restriction y"<<","<<"Restriction z";
        flog<<","<<"Loading x"<<","<<"Loading y"<<","<<"Loading z";
        flog<<","<<"Displacement x"<<","<<"Displacement y"<<","<<"Displacement z";
        flog<<","<<"Normal stress x";
        flog<<","<<"Normal stress y";
        flog<<","<<"Normal stress z";
        flog<<","<<"Shear stress xy";
        flog<<","<<"Shear stress yz";
        flog<<","<<"Shear stress zx";
        flog<<","<<"Von Mises ";
        flog<<std::endl;


        for(int i=0; i<nNodes; i++)
        {
            flog<<i<<","<<nodes[i]->coordinates[0]<<","<<nodes[i]->coordinates[1]<<","<<nodes[i]->coordinates[2];//<<std::endl;
            flog<<","<<nodes[i]->restrictions[0]<<","<<nodes[i]->restrictions[1]<<","<<nodes[i]->restrictions[2];
            flog<<","<<reactions(3*i)<<","<<reactions(3*i+1)<<","<<reactions(3*i+2);
            flog<<","<<u(3*i)<<","<<u(3*i+1)<<","<<u(3*i+2);
            for(int j=0;j<7;j++)
                flog<<","<<Snodes(i, j);
            flog<<std::endl;
        }

        flog.close();
    }

    else
    {
        std::ofstream flog(filename.toStdString());

        flog<<"Element"<<","<<"Node 0"<<","<<"Node 1"<<","<<"Node 2"<<","<<"Node 3";//<<std::endl;
        flog<<","<<"E"<<","<<"Poisson";
        //        flog<<","<<"Loading x"<<","<<"Loading y"<<","<<"Loading z";
        //        flog<<","<<"Displacement x"<<","<<"Displacement y"<<","<<"Displacement z";
        //        flog<<","<<"Normal stress x";
        //        flog<<","<<"Normal stress y";
        //        flog<<","<<"Normal stress z";
        //        flog<<","<<"Shear stress xy";
        //        flog<<","<<"Shear stress yz";
        //        flog<<","<<"Shear stress zx";
        //        flog<<","<<"Von Mises ";
        flog<<std::endl;


        for(int i=0; i<nElements; i++)
        {
            flog<<i<<","<<elements[i]->nodes[0]->index<<","<<elements[i]->nodes[1]->index<<","<<elements[i]->nodes[2]->index<<","<<elements[i]->nodes[3]->index;//<<std::endl;
            flog<<","<<elements[i]->material->E<<","<<elements[i]->material->poisson;
            //            flog<<","<<reactions(3*i)<<","<<reactions(3*i+1)<<","<<reactions(3*i+2);
            //            flog<<","<<u(3*i)<<","<<u(3*i+1)<<","<<u(3*i+2);
            //            for(int j=0;j<7;j++)
            //                flog<<","<<Snodes(i, j);
            flog<<std::endl;
        }


        flog.close();
    }
}



