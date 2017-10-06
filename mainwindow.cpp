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


#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QElapsedTimer>
#include <QtWidgets>
#include <QFile>
#include <QIODevice>

#include "truss3dreader.h"
#include "solid3dreader.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // interface configurations
    ui->setupUi(this);
    wgl = ui->openGLWidget;


    connect(ui->actionTopView, SIGNAL(triggered(bool)), wgl->view, SLOT(setXYTopView(void)));
    connect(ui->actionBottomView, SIGNAL(triggered(bool)), wgl->view, SLOT(setXYBottomView(void)));
    connect(ui->actionRightView, SIGNAL(triggered(bool)), wgl->view, SLOT(setYZTopView(void)));
    connect(ui->actionLeftView, SIGNAL(triggered(bool)), wgl->view, SLOT(setYZBottomView(void)));
    connect(ui->actionFrontView, SIGNAL(triggered(bool)), wgl->view, SLOT(setXZTopView(void)));
    connect(ui->actionBackView, SIGNAL(triggered(bool)), wgl->view, SLOT(setXZBottomView(void)));
    connect(ui->actionIsoView, SIGNAL(triggered(bool)), wgl->view, SLOT(setIsometricView(void)));

    connect(ui->actionOrthographicProjection, SIGNAL(triggered(bool)), wgl->view, SLOT(setOrthografic(void)));
    connect(ui->actionPerpectiveProjection, SIGNAL(triggered(bool)), wgl->view, SLOT(setPerpective(void)));

    connect(ui->actiontakeSnapshoot, SIGNAL(triggered(bool)), wgl, SLOT(takePicture(void)));
    connect(ui->actionSelect, SIGNAL(triggered(bool)), wgl, SLOT(select(void)));
    connect(ui->actionSolved_Mesh, SIGNAL(triggered(bool)), wgl, SLOT(showSolvedModel(void)));
    connect(ui->actionInitialMesh, SIGNAL(triggered(bool)), wgl, SLOT(showInitialModel(void)));
    connect(ui->actionGrid, SIGNAL(triggered(bool)), wgl, SLOT(showGrid(void)));
    connect(ui->actionLegend, SIGNAL(triggered(bool)), wgl, SLOT(showLegend(void)));

    connect(ui->actionTurnOffLighting, SIGNAL(triggered(bool)), wgl, SLOT(turnOffLighting(void)));
    ui->actionTurnOffLighting->setChecked(true);
    connect(ui->actionShowMesh, SIGNAL(triggered(bool)), wgl, SLOT(showMesh(void)));
    ui->actionShowMesh->setChecked(true);

    connect(ui->action_Open, SIGNAL(triggered(bool)), this, SLOT(openFile()));
    connect(ui->action_Save, SIGNAL(triggered(bool)), this, SLOT(saveFile()));
    connect(ui->actionSave_As, SIGNAL(triggered(bool)), this, SLOT(saveAs()));
    connect(ui->action_Solver, SIGNAL(triggered(bool)), this, SLOT(solver()));
    connect(ui->action_Exit, SIGNAL(triggered(bool)), this, SLOT(close()));

    connect(ui->actionstartSimulation, SIGNAL(triggered(bool)), wgl, SLOT(startSimulation()));
    connect(ui->actionstopSimulation, SIGNAL(triggered(bool)), wgl, SLOT(stopSimulation()));
    connect(ui->actionpauseSimulation, SIGNAL(triggered(bool)), wgl, SLOT(pauseSimulation()));

    connect(ui->actionstressxx, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionstressyy, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionstresszz, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionstressxy, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionstressyz, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionstresszx, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionstressvonmises, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionresultux, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionresultuy, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionresultuz, SIGNAL(triggered(bool)), this, SLOT(setResult()));

    connect(ui->actionNodesReport, SIGNAL(triggered(bool)), this, SLOT(nodesReport()));
    connect(ui->actionElementsReport, SIGNAL(triggered(bool)), this, SLOT(elementsReport()));

    //connect(ui->actionTest, SIGNAL(triggered(bool)), wgl, SLOT(tests()));

    t3d_fileManager = new Truss3DFileManager(ui->treeWidget);
    s3d_fileManager = new Solid3DFileManager(ui->treeWidget);

    // Mesh configurations
    t3d_mesh = new Truss3D;
    s3d_mesh = new Solid3D;

    model = truss3d;

    ui->actionresultuz->setChecked(true);
    lastAction = ui->actionresultuz;
    wgl->s3d_result = 9; // default uz
    ui->resultsToolBar->setDisabled(true);

        QDir::setCurrent("../FEA_MNE715/models");

}

void MainWindow::openFile(void)
{

    QString filename =
            QFileDialog::getOpenFileName(this, QObject::tr("Open FEA File"),
                                         QDir::currentPath(),
                                         QObject::tr("FEA Files (*.fsxl *.ftxl *.ft3d);;Ansys CDB File (*.cdb);;Truss3D Files (*.ftxl *.ft3d);;DXF File (*.dxf)"));
    if (filename.isEmpty())
        return;

//    QString filename = "../models/cube.fsxl";

    QFileInfo file(filename);
    QString type = file.completeSuffix();
    if(type == "ftxl" || type == "ft3d" || type == "dxf")
    {
        model = truss3d;
        t3d_fileManager->currentfilename = filename;
        if(!t3d_fileManager->openFile())
            return;
        wgl->restart();
        this->setWindowTitle(QString("Truss 3D Model [%1]").arg(t3d_fileManager->currentfilename));

        Truss3DReader reader(t3d_mesh);
        t3d_mesh = reader.read(t3d_fileManager);

        wgl->model = model;
        wgl->solvedModel->ready = false;
        wgl->simulationModel->ready = false;

        wgl->setupInitialModel(t3d_mesh);
        wgl->isInitialModelActived = true;
        ui->actionInitialMesh->setChecked(true);
        wgl->updateGL();

        ui->resultsToolBar->setDisabled(true);

    }
    else if (type == "fsxl" || type == "cdb")
    {
        model = solid3d;
        s3d_fileManager->currentfilename = filename;
        if(!s3d_fileManager->openFile())
            return;
        wgl->restart();
        this->setWindowTitle(QString("Solid 3D Model [%1]").arg(s3d_fileManager->currentfilename));

        Solid3DReader reader(s3d_mesh);
        s3d_mesh = reader.read(s3d_fileManager);

        for(int i=0; i<s3d_mesh->nElements; i++)
            s3d_mesh->elements[i]->evaluateNormals();


        wgl->model = model;
        wgl->solvedModel->ready = false;
        wgl->simulationModel->ready = false;

        wgl->setupInitialModel(s3d_mesh);
        wgl->isInitialModelActived = true;
        ui->actionInitialMesh->setChecked(true);

        wgl->updateGL();

        ui->resultsToolBar->setDisabled(true);

    }
    else
        return;

    wgl->updateGL();

}

void MainWindow::saveFile(void)
{
    if(model == truss3d)
        t3d_fileManager->saveFile();
    else
        s3d_fileManager->saveFile();
}


void MainWindow::saveAs(void)
{
    if(model == truss3d)
    {
        t3d_fileManager->currentfilename =
                QFileDialog::getSaveFileName(this, QObject::tr("Save As Truss3D File"),
                                             QDir::currentPath(),
                                             QObject::tr("Truss3D File (*.ftxl)"));
        if (t3d_fileManager->currentfilename.isEmpty())
            return;

        t3d_fileManager->saveFile();

        this->setWindowTitle(QString("Truss 3D Model [%1]").arg(t3d_fileManager->currentfilename));
    }
    else
    {
        s3d_fileManager->currentfilename =
                QFileDialog::getSaveFileName(this, QObject::tr("Save As Solid3D File"),
                                             QDir::currentPath(),
                                             QObject::tr("Solid3D File (*.fsxl)"));
        if (s3d_fileManager->currentfilename.isEmpty())
            return;

        s3d_fileManager->saveFile();

        this->setWindowTitle(QString("Solid 3D Model [%1]").arg(s3d_fileManager->currentfilename));
    }
}

void MainWindow::solver(void)
{

    if(model == truss3d)
    {
        if(t3d_fileManager->currentfilename == "") return;
        t3d_fileManager->saveFile();

        Truss3DReader reader(t3d_mesh);
        t3d_mesh = reader.read(t3d_fileManager);

        QString str;
        QElapsedTimer timer;
        timer.start();


        if(t3d_mesh->isMounted)
        {
            str = QString("Solving the Truss3D model... (%1 nodes, %2 elements)...").
                    arg(t3d_mesh->nNodes).arg(t3d_mesh->nElements);
            ui->statusBar->showMessage(str, 60000);
            t3d_mesh->evalStiffnessMatrix();
            t3d_mesh->solve_simulation(wgl->nFrames);
            t3d_mesh->isSolved = true;

            str += QString(" ready! %1 s").arg(timer.elapsed()/1000.);
            ui->statusBar->showMessage(str, 60000);

            wgl->setupSimulationModel(t3d_mesh);
            wgl->setupSolvedModel(t3d_mesh);


            wgl->isSolvedModelActived = true;
            wgl->isSimulationModelActived = false;
            ui->actionSolved_Mesh->setChecked(true);

            wgl->isInitialModelActived = false;
            ui->actionInitialMesh->setChecked(false);

            wgl->updateGL();
        }
    }
    else
    {
        if(s3d_fileManager->currentfilename == "") return;
        s3d_fileManager->saveFile();

        Solid3DReader reader(s3d_mesh);
        s3d_mesh = reader.read(s3d_fileManager);

        QString str;
        QElapsedTimer timer;
        timer.start();


        if(s3d_mesh->isMounted)
        {
            str = QString("Solving the Solid3D model... (%1 nodes, %2 elements)...").
                    arg(s3d_mesh->nNodes).arg(s3d_mesh->nElements);
            ui->statusBar->showMessage(str, 60000);
            s3d_mesh->evalStiffnessMatrix();
            s3d_mesh->evalLoadVector();
            //s3d_mesh->evalLoadVector();
            //s3d_mesh->solve();
            s3d_mesh->solve_simulation(wgl->nFrames);
            //s3d_mesh->isSolved = true;

            str += QString(" ready! %1 s").arg(timer.elapsed()/1000.);
            ui->statusBar->showMessage(str, 60000);
            ui->resultsToolBar->setDisabled(false);

            wgl->setupSimulationModel(s3d_mesh);
            wgl->setupSolvedModel(s3d_mesh);

            wgl->isSolvedModelActived = true;
            wgl->isSimulationModelActived = false;
            ui->actionSolved_Mesh->setChecked(true);

            wgl->isInitialModelActived = false;
            ui->actionInitialMesh->setChecked(false);

            wgl->updateGL();
        }
    }
}

void MainWindow::setResult(void)
{
    if(ui->actionstressxx->isChecked() && lastAction != ui->actionstressxx)
    {
        wgl->s3d_result = 0;
        lastAction->setChecked(false);
        lastAction = ui->actionstressxx;
    }
    else if(ui->actionstressyy->isChecked() && lastAction != ui->actionstressyy)
    {
        wgl->s3d_result = 1;
        lastAction->setChecked(false);
        lastAction = ui->actionstressyy;
    }
    else if(ui->actionstresszz->isChecked() && lastAction != ui->actionstresszz)
    {
        wgl->s3d_result = 2;
        lastAction->setChecked(false);
        lastAction = ui->actionstresszz;
    }
    else if(ui->actionstressxy->isChecked() && lastAction != ui->actionstressxy)
    {
        wgl->s3d_result = 3;
        lastAction->setChecked(false);
        lastAction = ui->actionstressxy;
    }
    else if(ui->actionstressyz->isChecked() && lastAction != ui->actionstressyz)
    {
        wgl->s3d_result = 4;
        lastAction->setChecked(false);
        lastAction = ui->actionstressyz;
    }
    else if(ui->actionstresszx->isChecked() && lastAction != ui->actionstresszx)
    {
        wgl->s3d_result = 5;
        lastAction->setChecked(false);
        lastAction = ui->actionstresszx;
    }
    else if(ui->actionstressvonmises->isChecked() && lastAction != ui->actionstressvonmises)
    {
        wgl->s3d_result = 6;
        lastAction->setChecked(false);
        lastAction = ui->actionstressvonmises;
    }
    else if(ui->actionresultux->isChecked() && lastAction != ui->actionresultux)
    {
        wgl->s3d_result = 7;
        lastAction->setChecked(false);
        lastAction = ui->actionresultux;
    }
    else if(ui->actionresultuy->isChecked() && lastAction != ui->actionresultuy)
    {
        wgl->s3d_result = 8;
        lastAction->setChecked(false);
        lastAction = ui->actionresultuy;
    }
    else if(ui->actionresultuz->isChecked() && lastAction != ui->actionresultuz)
    {
        wgl->s3d_result = 9;
        lastAction->setChecked(false);
        lastAction = ui->actionresultuz;
    }

    wgl->updateGL();
}


void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    wgl->updateGL();
}

void MainWindow::nodesReport(void)
{
    if(model == truss3d)
    {
        if(!t3d_mesh->isSolved) return;

        QString file;
        file = QString("%1").arg(t3d_fileManager->currentfilename);
        file.replace(" ","_");
        file.replace(".ftxl","_report_from_nodes.csv");
        t3d_mesh->report(file,true);
        QProcess *process = new QProcess(this);
        QString cmd = "/usr/lib/libreoffice/program/soffice.bin --calc -o ";
        process->start(cmd + file);

    }
    else
    {
        if(!s3d_mesh->isSolved) return;

        QString file;
        file = QString("%1").arg(s3d_fileManager->currentfilename);
        file.replace(" ","_");
        file.replace(".fsxl","_report_from_nodes.csv");
        s3d_mesh->report(file,true);
        QProcess *process = new QProcess(this);
        QString cmd = "/usr/lib/libreoffice/program/soffice.bin --calc -o ";
        process->start(cmd + file);
    }
}

void MainWindow::elementsReport(void)
{
    if(model == truss3d)
    {
        if(!t3d_mesh->isSolved) return;

        QString file;
        file = QString("%1").arg(t3d_fileManager->currentfilename);
        file.replace(" ","_");
        file.replace(".fsxl","_report_from_elements.csv");
        t3d_mesh->report(file,false);
        QProcess *process = new QProcess(this);
        QString cmd = "/usr/lib/libreoffice/program/soffice.bin --calc -o ";
        process->start(cmd + file);

    }
    else
    {
        if(!s3d_mesh->isSolved) return;

        QString file;
        file = QString("%1").arg(s3d_fileManager->currentfilename);
        file.replace(" ","_");
        file.replace(".fsxl","_report_from_elements.csv");
        s3d_mesh->report(file,false);
        QProcess *process = new QProcess(this);
        QString cmd = "/usr/lib/libreoffice/program/soffice.bin --calc -o ";
        process->start(cmd + file);
    }
}




MainWindow::~MainWindow()
{
    delete t3d_fileManager;
    delete s3d_fileManager;
    delete t3d_mesh;
    delete s3d_mesh;
    delete ui;
}
