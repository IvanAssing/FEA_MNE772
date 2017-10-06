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

#include "truss3d.h"

#include <fstream>
#include <iostream>
#include <istream>
#include <string>
#include <cmath>

#include <QString>

#include <mth/matrix.h>

Truss3D::Truss3D(char *filename)
{
    this->isSolved = false;

    std::ifstream file(filename, std::ios::in);
    if(file.fail()) std::cerr<<"Error in file loading: "<<filename;
    int index;

    // Dados de condições de apoio
    file>>nre;
    restrictions= new bool *[nre];

    for(int i=0; i<nre; i++)
    {
        restrictions[i] = new bool[3];
        file>>index>>restrictions[i][0]>>restrictions[i][1]>>restrictions[i][2];
    }

    // Dados de carregamento preescrito
    file>>nlo;
    loading = new double *[nlo];

    for(int i=0; i<nlo; i++)
    {
        loading[i] = new double[3];
        file>>index>>loading[i][0]>>loading[i][1]>>loading[i][2];
    }

    // Dados de deslocamento preescrito
    file>>ndi;
    displacements = new double *[ndi];

    for(int i=0; i<ndi; i++)
    {
        displacements[i] = new double[3];
        file>>index>>displacements[i][0]>>displacements[i][1]>>displacements[i][2];
    }

    // Dados dos nos
    file>>nNodes;
    this->nodes = new Node3D *[nNodes];

    double *tempCoord = new double[3];
    int til, tid, tir;

    for(int i=0; i<nNodes; i++)
    {
        file>>index>>tempCoord[0]>>tempCoord[1]>>tempCoord[2]>>tir>>til>>tid;
        nodes[i] = new Node3D(index, tempCoord, restrictions[tir], loading[til], displacements[tid]);
    }

    // Dados de material
    file>>nma;
    materials = new Material *[nma];
    std::string tname;
    double te, ta;

    for(int i=0; i<nma; i++)
    {
        file>>index>>tname>>te>>ta;
        materials[i] = new Material(index, tname, te, ta);
    }

    // Dados dos Elementos
    file>>nElements;
    this->elements = new Truss3DElement *[nElements];

    int index_el, index_no1, index_no2;
    int itm;

    for(int i=0; i<nElements; i++)
    {
        file>>index_el>>index_no1>>index_no2>>itm;
        elements[i] = new Truss3DElement(index_el, nodes[index_no1], nodes[index_no2], materials[itm]); // cria o elementos
    }

    isMounted = true;
}


Truss3D::Truss3D(DXFReader *file)
{

    // Dados de condições de apoio
    nre = 1+1;
    restrictions= new bool *[nre];
    restrictions[0] = new bool[3];
    restrictions[1] = new bool[3];

    restrictions[0][0]=restrictions[0][1]=restrictions[0][2] = false;

    // Dados de carregamento preescrito
    nlo = 1+1;
    loading = new double *[nlo];

    loading[0] = new double[3];
    loading[1] = new double[3];

    loading[0][0]=loading[0][1]=loading[0][2] = 0.0;


    // Dados de deslocamento preescrito
    ndi = 1;
    displacements = new double *[ndi];
    displacements[0] = new double[3];

    displacements[0][0]=displacements[0][1]=displacements[0][2] = 0.0;

    // Dados dos nos
    nNodes = file->pbuffer->count;
    this->nodes = new Node3D *[nNodes];

    double *tempCoord = new double[3];

    for(int i=0; i<nNodes; i++)
    {
        tempCoord[0] = file->pbuffer->data[3*i];
        tempCoord[1] = file->pbuffer->data[3*i+1];
        tempCoord[2] = file->pbuffer->data[3*i+2];

        nodes[i] = new Node3D(i, tempCoord, restrictions[0], loading[0], displacements[0]);
    }

    // Dados de material
    nma = 1;
    materials = new Material *[nma];

    materials[0] = new Material(0, "unamed", 1.0, 1.0);

    // Dados dos Elementos
    nElements = file->lbuffer->count;
    this->elements = new Truss3DElement *[nElements];

    int index_no1, index_no2;

    for(int i=0; i<nElements; i++)
    {
        index_no1 = file->lbuffer->data[3*i];
        index_no2 = file->lbuffer->data[3*i+1];
        elements[i] = new Truss3DElement(i, nodes[index_no1], nodes[index_no2], materials[0]); // cria o elementos
    }

    isMounted = true;

}


Truss3D::Truss3D()
{
    nNodes = 0;
    nElements = 0;
    isSolved = false;
    isMounted = false;
    isSolved_simulation = false;
}


void Truss3D::evalStiffnessMatrix(void)
{
    k.resize(3*nNodes, 3*nNodes);
    k = 0.0;

    f.resize(3*nNodes);
    f = 0.0;

    Mth::Matrix ke(6,6);

    Node3D **ptrNodes = new Node3D*[2];

    // Calcula a matriz de rigidez global


    for(int i=0; i<nElements; i++)
    {
        // Calcula a matriz de rigidez local
        elements[i]->getStiffnessMatrix(ke);

        ptrNodes[0] = elements[i]->node1;
        ptrNodes[1] = elements[i]->node2;

        // Posiciona a matriz de rigidez local do elemento i na matriz de rigidez global
        for(int ni=0;ni<2;ni++)
            for(int nj=0;nj<2;nj++)
                for(int ii=0;ii<3;ii++)
                    for(int ij=0;ij<3;ij++)
                    {
                        int indexI = 3*ptrNodes[ni]->index+ii;
                        int indexJ = 3*ptrNodes[nj]->index+ij;
                        k(indexI, indexJ) += ke(3*ni+ii, 3*nj+ij);
                    }

    }

    for(int i=0; i<nNodes; i++)
    {
        f(3*nodes[i]->index) = nodes[i]->loading[0];
        f(3*nodes[i]->index+1) = nodes[i]->loading[1];
        f(3*nodes[i]->index+2) = nodes[i]->loading[2];
    }

    delete [] ptrNodes;

}


void Truss3D::solve(void)
{
    std::ofstream flog("log_solver.txt");

    Mth::Matrix kc(k); // cópias
    Mth::Vector fc(f);

    // Aplica as condicoes de contorno
    for(int i=0; i<nNodes; i++)
        for(int j=0; j<3; j++)
            if(nodes[i]->restrictions[j]==true)
            {
                flog<<"\n"<<i<<" "<<nodes[i]->restrictions[j];
                int n = 3*nodes[i]->index+j;
                for(int t=0; t<3*nNodes; t++)
                    kc(n, t) = 0.0;
                kc(n, n) = 1.0;
                fc(n) = nodes[i]->displacements[j];
            }

    for(int i=0; i<3*nNodes; i++)
    {
        bool check = true;
        for(int j=0; j<3*nNodes; j++)
            if(fabs(kc(i, j))>1.e-10) {check = false ; break;}
        if(check)
        {
            int t = i/3;
            int v = i-3*t;
            flog<<"\n error in line: "<<i<<" node: "<<t<<" v: "<<v;
            //            for(int t=0; t<3*nNodes; t++)
            //                kc(i, t) = kc(t, i) = 0.0;
            //            kc(i, i) = 1.0;
            //            fc(i) = 0.0;

        }
    }

    for(int i=0; i<3*nNodes; i++)
    {
        bool check = true;
        for(int j=0; j<3*nNodes; j++)
            if(fabs(kc(j, i))>1.e-10) {check = false ; break;}
        if(check) flog<<"\n error in column: "<<i;
    }




    //    std::cerr<<"\n\n Matriz de rigidez\n";
    //    std::cerr<<kc;

    //    std::cerr<<"\n\n Vetor de carga\n";
    //    std::cerr<<fc;

    //    Mth::Matrix kcc(kc);
    //    Mth::Vector fcc(fc);

    //    std::cerr<<"\n\n Teste\n";
    //    kcc.inverse();
    //    std::cerr<<kcc*fcc;

    // Aloca vetor para resultados
    u.resize(3*nNodes);

    kc.solve(fc, u);


    u.clear();

    flog<<"\n\n Solução\n";
    flog<<u;


    //    std::cerr<<"\n\n Matriz de rigidez\n";
    //    std::cerr<<k;
    reactions.resize(3*nNodes);

    reactions = k*u;

    reactions.clear(1.e-10);

    flog<<"\n\n Reações\n";
    flog<<reactions;


    Mth::Matrix ue(6);


    stress.resize(nElements);
    for(int i=0; i<nElements; i++)
    {
        ue(0) = u(3*elements[i]->node1->index);
        ue(1) = u(3*elements[i]->node1->index+1);
        ue(2) = u(3*elements[i]->node1->index+2);
        ue(3) = u(3*elements[i]->node2->index);
        ue(4) = u(3*elements[i]->node2->index+1);
        ue(5) = u(3*elements[i]->node2->index+2);

        stress(i) = elements[i]->getStress(ue);
    }

    //    stress.clear();
    flog<<"\n\n Tensoes normais\n";
    flog<<stress;

    flog.close();

    isSolved = true;
}



void Truss3D::solve_simulation(int steps)
{
    nSteps = steps;

    f_simulation.resize(3*nNodes, nSteps);
    f_simulation = 0.0;

    double delta = 1.0/nSteps;
    double factor;


    for(int k=0; k<nSteps; k++)
    {
        factor = k*delta;
        for(int i=0; i<nNodes; i++)
        {
            f_simulation(3*nodes[i]->index, k) = factor *nodes[i]->loading[0];
            f_simulation(3*nodes[i]->index+1, k) = factor * nodes[i]->loading[1];
            f_simulation(3*nodes[i]->index+2, k) = factor * nodes[i]->loading[2];
        }
    }


//    std::ofstream flog("log_simulation.txt");

//    flog<<f_simulation;

//    flog.close();



    Mth::Matrix kc(k); // cópias
    Mth::Matrix fc(f_simulation);

    // Aplica as condicoes de contorno

    for(int i=0; i<nNodes; i++)
        for(int j=0; j<3; j++)
            if(nodes[i]->restrictions[j]==true)
            {
                int n = 3*nodes[i]->index+j;
                for(int t=0; t<3*nNodes; t++)
                    kc(n, t) = kc(t, n) = 0.0;
                kc(n, n) = 1.0;
                for(int k=0; k<nSteps; k++)
                    fc(n,k) = nodes[i]->displacements[j];
            }

    u_simulation.resize(3*nNodes, nSteps);

    //kc.solve(fc, u_simulation);
    //kc.solve_symmetric(fc, u_simulation);

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




    //u_simulation.clear();

    reactions_simulation.resize(3*nNodes, nSteps);

    reactions_simulation = k*u_simulation;

    //reactions_simulation.clear(1.e-10);


    Mth::Matrix ue(6);

    stress_simulation.resize(nElements, nSteps);

    for(int i=0; i<nElements; i++)
        for(int k=0; k<nSteps; k++)
        {
            ue(0) = u_simulation(3*elements[i]->node1->index, k);
            ue(1) = u_simulation(3*elements[i]->node1->index+1, k);
            ue(2) = u_simulation(3*elements[i]->node1->index+2, k);
            ue(3) = u_simulation(3*elements[i]->node2->index, k);
            ue(4) = u_simulation(3*elements[i]->node2->index+1, k);
            ue(5) = u_simulation(3*elements[i]->node2->index+2, k);

            stress_simulation(i, k) = elements[i]->getStress(ue);
        }

    isSolved_simulation = true;


    u.resize(3*nNodes);
    reactions.resize(3*nNodes);
    stress.resize(nElements);


    for(int i=0; i<3*nNodes; i++)
    {
        u(i) = u_simulation(i,nSteps-1);
        reactions(i) = reactions_simulation(i,nSteps-1);
    }


    for(int i=0; i<nElements; i++)
    {
        stress(i) = stress_simulation(i, nSteps-1);
    }

    isSolved = true;

}


void Truss3D::stresslimits(double &min, double &max)
{
    min = stress(0);
    max = stress(0);

    for(int i=1; i<stress.n; i++)
    {
        if(stress(i)>max) max = stress(i);
        if(stress(i)<min) min = stress(i);
    }

}


void Truss3D::stresslimits_simulation(double &min, double &max)
{
    min = stress_simulation(0,0);
    max = stress_simulation(0,0);

    for(int k=0; k<stress_simulation.n; k++)
        for(int i=1; i<stress_simulation.m; i++)
        {
            double value = stress_simulation(i,k);
            if(value>max) max = value;
            if(value<min) min = value;
        }

}


Truss3D::~Truss3D()
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



void Truss3D::report(QString filename, bool isNodesInfo)
{

    if(isNodesInfo)
    {
    std::ofstream flog(filename.toStdString());

    flog<<"Node"<<","<<"Coordinate x"<<","<<"Coordinate y"<<","<<"Coordinate z";//<<std::endl;
    flog<<","<<"Restriction x"<<","<<"Restriction y"<<","<<"Restriction z";
    flog<<","<<"Loading x"<<","<<"Loading y"<<","<<"Loading z";
    flog<<","<<"Displacement x"<<","<<"Displacement y"<<","<<"Displacement z";

    flog<<std::endl;


    for(int i=0; i<nNodes; i++)
    {
        flog<<i<<","<<nodes[i]->coordinates[0]<<","<<nodes[i]->coordinates[1]<<","<<nodes[i]->coordinates[2];//<<std::endl;
        flog<<","<<nodes[i]->restrictions[0]<<","<<nodes[i]->restrictions[1]<<","<<nodes[i]->restrictions[2];
        flog<<","<<reactions(3*i)<<","<<reactions(3*i+1)<<","<<reactions(3*i+2);
        flog<<","<<u(3*i)<<","<<u(3*i+1)<<","<<u(3*i+2);
        flog<<std::endl;
    }

    flog.close();
    }

    else
    {
        std::ofstream flog(filename.toStdString());

        flog<<"Element"<<","<<"Node 0"<<","<<"Node 1";//<<std::endl;
        flog<<","<<"Material E"<<","<<"Material A";
//        flog<<","<<"Loading x"<<","<<"Loading y"<<","<<"Loading z";
//        flog<<","<<"Displacement x"<<","<<"Displacement y"<<","<<"Displacement z";
        flog<<","<<"Normal stress";

        flog<<std::endl;


        for(int i=0; i<nElements; i++)
        {
            flog<<i<<","<<elements[i]->node1->index<<","<<elements[i]->node2->index;//<<std::endl;
            flog<<","<<elements[i]->material->E<<","<<elements[i]->material->A;
//            flog<<","<<reactions(3*i)<<","<<reactions(3*i+1)<<","<<reactions(3*i+2);
            flog<<","<<stress(i);

            flog<<std::endl;
        }


        flog.close();
    }
}



