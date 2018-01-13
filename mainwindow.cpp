/****************************************************************************
** Copyright (C) 2017 Ivan Assing da Silva
** Contact: ivanassing@gmail.com
**
** This file is part of the FEA_MNE772 project.
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

#include "msglog.h"

//#include "vtk.h"
//#include "vtkmainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // interface configurations

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    ui->setupUi(this);
    vtkRenderer = ui->openGLWidget;



    connect(ui->actionTopView, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setXYTopView(void)));
    connect(ui->actionBottomView, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setXYBottomView(void)));
    connect(ui->actionRightView, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setYZTopView(void)));
    connect(ui->actionLeftView, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setYZBottomView(void)));
    connect(ui->actionFrontView, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setXZTopView(void)));
    connect(ui->actionBackView, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setXZBottomView(void)));
    connect(ui->actionIsoView, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setIsometricView(void)));

    connect(ui->actionOrthographicProjection, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setParallelProjection(void)));
    connect(ui->actionPerpectiveProjection, SIGNAL(triggered(bool)), vtkRenderer, SLOT(setPerspectiveProjection(void)));

    connect(ui->actionScreenshoot, SIGNAL(triggered(bool)), vtkRenderer, SLOT(screenshot(void)));

    //    connect(ui->actionSelect, SIGNAL(triggered(bool)), wgl, SLOT(select(void)));
    //    connect(ui->actionSolved_Mesh, SIGNAL(triggered(bool)), wgl, SLOT(showSolvedModel(void)));
    //    connect(ui->actionInitialMesh, SIGNAL(triggered(bool)), wgl, SLOT(showInitialModel(void)));
    //    connect(ui->actionGrid, SIGNAL(triggered(bool)), wgl, SLOT(showGrid(void)));
    //    connect(ui->actionLegend, SIGNAL(triggered(bool)), wgl, SLOT(showLegend(void)));

    //    connect(ui->actionTurnOffLighting, SIGNAL(triggered(bool)), wgl, SLOT(turnOffLighting(void)));
    //    ui->actionTurnOffLighting->setChecked(true);
    //    connect(ui->actionShowMesh, SIGNAL(triggered(bool)), wgl, SLOT(showMesh(void)));
    //    ui->actionShowMesh->setChecked(true);

    connect(ui->action_Open, SIGNAL(triggered(bool)), this, SLOT(openFile()));
    connect(ui->action_Save, SIGNAL(triggered(bool)), this, SLOT(saveFile()));
    connect(ui->actionSave_As, SIGNAL(triggered(bool)), this, SLOT(saveAs()));
    connect(ui->action_Solver, SIGNAL(triggered(bool)), this, SLOT(solver()));
    connect(ui->action_Exit, SIGNAL(triggered(bool)), this, SLOT(close()));

    connect(ui->actionstartSimulation, SIGNAL(triggered(bool)), vtkRenderer, SLOT(startSimulation()));
    connect(ui->actionstopSimulation, SIGNAL(triggered(bool)), vtkRenderer, SLOT(stopSimulation()));
    connect(ui->actionpauseSimulation, SIGNAL(triggered(bool)), vtkRenderer, SLOT(pauseSimulation()));

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
    connect(ui->actionresultabsu, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionpstress1, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionpstress2, SIGNAL(triggered(bool)), this, SLOT(setResult()));
    connect(ui->actionpstress3, SIGNAL(triggered(bool)), this, SLOT(setResult()));

    connect(ui->actionNodesReport, SIGNAL(triggered(bool)), this, SLOT(nodesReport()));
    connect(ui->actionElementsReport, SIGNAL(triggered(bool)), this, SLOT(elementsReport()));

    connect(ui->actionEllipsoid, SIGNAL(triggered(bool)), vtkRenderer, SLOT(ellipsoidGlyphsVisualization()));
    connect(ui->actionHyperstreamline, SIGNAL(triggered(bool)), vtkRenderer, SLOT(hyperstreamlinesVisualization()));
    connect(ui->actionIsosurface, SIGNAL(triggered(bool)), vtkRenderer, SLOT(isoSurfaceVisualization()));
    connect(ui->actionSuperquadricGlyph, SIGNAL(triggered(bool)), vtkRenderer, SLOT(superquadricsGlyphsVisualization()));
    connect(ui->actionVectorDisplacement, SIGNAL(triggered(bool)), vtkRenderer, SLOT(displacementVisualization()));
    connect(ui->actionCutterScalar, SIGNAL(triggered(bool)), vtkRenderer, SLOT(planeCutter()));
    connect(ui->actionOriginalMesh, SIGNAL(triggered(bool)), vtkRenderer, SLOT(addDataSet()));
    connect(ui->actionScalarColorMap, SIGNAL(triggered(bool)), vtkRenderer, SLOT(addDataSet_solved()));


    connect(ui->action_Solver_2, SIGNAL(triggered(bool)), this, SLOT(iterative_solver()));
    connect(ui->action_Solver_3, SIGNAL(triggered(bool)), this, SLOT(direct_solver()));


    // setup output widget for MsgLog
    MsgLog::output = ui->listWidget;
    ui->listWidget->setAutoScroll(true);

    ui->cutter_nx->setText(QString("-1.0"));
    ui->cutter_ny->setText(QString("0.0"));
    ui->cutter_nz->setText(QString("0.0"));
    ui->sqg_gamma->setText(QString("3.0"));

    connect(ui->cutter_nx, SIGNAL(editingFinished()), this, SLOT(updateCutter()));
    connect(ui->cutter_ny, SIGNAL(editingFinished()), this, SLOT(updateCutter()));
    connect(ui->cutter_nz, SIGNAL(editingFinished()), this, SLOT(updateCutter()));
    connect(ui->cutter_position, SIGNAL(valueChanged(int)), this, SLOT(updateCutter()));


    t3d_fileManager = new Truss3DFileManager(ui->treeWidget);
    s3d_fileManager = new Solid3DFileManager(ui->treeWidget);

    // Mesh configurations
    t3d_mesh = new Truss3D;
    s3d_mesh = new Solid3D;

    model = notdefined;

    ui->actionresultuz->setChecked(true);
    lastAction = ui->actionresultuz;
    vtkRenderer->s3d_result = 9; // default uz
    ui->resultsToolBar->setDisabled(true);

    QDir::setCurrent("../FEA_MNE772/models");

    isIterativeSolver = true;
    updateParameters();

}

void MainWindow::openFile(void)
{

    QString filename =
            QFileDialog::getOpenFileName(this, QObject::tr("Open FEA File"),
                                         QDir::currentPath(),
                                         QObject::tr("Solid 3D Files (*.fsxl);;Ansys CDB File (*.cdb);;Truss3D Files (*.ftxl *.ft3d);;DXF File (*.dxf)"));
    if (filename.isEmpty())
        return;

    QFileInfo file(filename);
    QString type = file.completeSuffix();
    if(type == "ftxl" || type == "ft3d" || type == "dxf")
    {
        model = truss3d;
        t3d_fileManager->currentfilename = filename;
        if(!t3d_fileManager->openFile())
            return;

        this->setWindowTitle(QString("Truss 3D Model [%1]").arg(t3d_fileManager->currentfilename));

        Truss3DReader reader(t3d_mesh);
        t3d_mesh = reader.read(t3d_fileManager);

        MsgLog::information("Truss 3D Model loaded");
        MsgLog::result(QString("%1 nodes, %2 elements").arg(t3d_mesh->nNodes, t3d_mesh->nElements));

        ui->resultsToolBar->setDisabled(true);


        //        double volume, weight;
        //        s3d_mesh->infoGeometry(volume, weight);
        //        MsgLog::result(QString("Model's volume: %1").arg(volume));
        //        MsgLog::result(QString("Model's weight: %1").arg(weight));

        vtkRenderer->removeDataSet();
        vtkRenderer->addDataSet(t3d_mesh);

    }
    else if (type == "fsxl" || type == "cdb")
    {
        model = solid3d;
        s3d_fileManager->currentfilename = filename;
        if(!s3d_fileManager->openFile())
            return;

        this->setWindowTitle(QString("Solid 3D Model [%1]").arg(s3d_fileManager->currentfilename));

        Solid3DReader reader(s3d_mesh);
        s3d_mesh = reader.read(s3d_fileManager);

        MsgLog::information("Solid 3D Model loaded");
        MsgLog::result(QString("%1 nodes, %2 elements").arg(s3d_mesh->nNodes).arg(s3d_mesh->nElements));


        ui->resultsToolBar->setDisabled(false);

        s3d_mesh->evalStiffnessMatrix();
        s3d_mesh->evalLoadVector();
        double volume, weight;
        s3d_mesh->infoGeometry(volume, weight);
        MsgLog::result(QString("Model's volume: %1").arg(volume));
        MsgLog::result(QString("Model's weight: %1").arg(weight));

        vtkRenderer->reset();
        vtkRenderer->s3d_mesh = s3d_mesh;
        vtkRenderer->removeDataSet();
        vtkRenderer->addDataSet();
        //vtkWin->show();

    }
    else
        return;

    //wgl->update();

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

            MsgLog::information(QString("Starting the Truss3D Solver"));

            t3d_mesh->evalStiffnessMatrix();
            t3d_mesh->solve();
            //t3d_mesh->solve_simulation(wgl->nFrames);
            t3d_mesh->isSolved = true;

            str += QString(" ready! %1 s").arg(timer.elapsed()/1000.);
            ui->statusBar->showMessage(str, 60000);
            MsgLog::information(QString("Total solver time: %1 s").arg(timer.elapsed()/1000.));


            vtkRenderer->removeDataSet();
            vtkRenderer->addDataSet_solved(t3d_mesh);

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

            MsgLog::information(QString("Starting the Solid3D Solver"));

            s3d_mesh->evalStiffnessMatrix();
            s3d_mesh->evalLoadVector();
            //s3d_mesh->evalLoadVector();

            s3d_mesh->isIterativeSolver = isIterativeSolver;
            s3d_mesh->solve();
            //s3d_mesh->solve_simulation(wgl->nFrames);
            s3d_mesh->isSolved = true;

            str += QString(" ready! %1 s").arg(timer.elapsed()/1000.);
            ui->statusBar->showMessage(str, 60000);
            MsgLog::information(QString("Total solver time was %1 s").arg(timer.elapsed()/1000.));



            vtkRenderer->reset();
            vtkRenderer->s3d_mesh = s3d_mesh;
            vtkRenderer->removeDataSet();
            vtkRenderer->addDataSet_simulation();


        }
    }
}

void MainWindow::setResult(void)
{
    if(ui->actionstressxx->isChecked() && lastAction != ui->actionstressxx)
    {
        vtkRenderer->s3d_result = 0;
        lastAction->setChecked(false);
        lastAction = ui->actionstressxx;
    }
    else if(ui->actionstressyy->isChecked() && lastAction != ui->actionstressyy)
    {
        vtkRenderer->s3d_result = 1;
        lastAction->setChecked(false);
        lastAction = ui->actionstressyy;
    }
    else if(ui->actionstresszz->isChecked() && lastAction != ui->actionstresszz)
    {
        vtkRenderer->s3d_result = 2;
        lastAction->setChecked(false);
        lastAction = ui->actionstresszz;
    }
    else if(ui->actionstressxy->isChecked() && lastAction != ui->actionstressxy)
    {
        vtkRenderer->s3d_result = 3;
        lastAction->setChecked(false);
        lastAction = ui->actionstressxy;
    }
    else if(ui->actionstressyz->isChecked() && lastAction != ui->actionstressyz)
    {
        vtkRenderer->s3d_result = 4;
        lastAction->setChecked(false);
        lastAction = ui->actionstressyz;
    }
    else if(ui->actionstresszx->isChecked() && lastAction != ui->actionstresszx)
    {
        vtkRenderer->s3d_result = 5;
        lastAction->setChecked(false);
        lastAction = ui->actionstresszx;
    }
    else if(ui->actionstressvonmises->isChecked() && lastAction != ui->actionstressvonmises)
    {
        vtkRenderer->s3d_result = 6;
        lastAction->setChecked(false);
        lastAction = ui->actionstressvonmises;
    }
    else if(ui->actionresultux->isChecked() && lastAction != ui->actionresultux)
    {
        vtkRenderer->s3d_result = 7;
        lastAction->setChecked(false);
        lastAction = ui->actionresultux;
    }
    else if(ui->actionresultuy->isChecked() && lastAction != ui->actionresultuy)
    {
        vtkRenderer->s3d_result = 8;
        lastAction->setChecked(false);
        lastAction = ui->actionresultuy;
    }
    else if(ui->actionresultuz->isChecked() && lastAction != ui->actionresultuz)
    {
        vtkRenderer->s3d_result = 9;
        lastAction->setChecked(false);
        lastAction = ui->actionresultuz;
    }
    else if(ui->actionresultabsu->isChecked() && lastAction != ui->actionresultabsu)
    {
        vtkRenderer->s3d_result = 10;
        lastAction->setChecked(false);
        lastAction = ui->actionresultabsu;
    }
    else if(ui->actionpstress1->isChecked() && lastAction != ui->actionpstress1)
    {
        vtkRenderer->s3d_result = 11;
        lastAction->setChecked(false);
        lastAction = ui->actionpstress1;
    }
    else if(ui->actionpstress2->isChecked() && lastAction != ui->actionpstress2)
    {
        vtkRenderer->s3d_result = 12;
        lastAction->setChecked(false);
        lastAction = ui->actionpstress2;
    }
    else if(ui->actionpstress3->isChecked() && lastAction != ui->actionpstress3)
    {
        vtkRenderer->s3d_result = 13;
        lastAction->setChecked(false);
        lastAction = ui->actionpstress3;
    }

    //vtkRenderer->removeDataSet();
    //vtkRenderer->addDataSet_simulation(s3d_mesh);
}

void MainWindow::updateParameters(void)
{

    vtkRenderer->elg_scalefactor = ui->elg_scalefactor->value()/100.;
    vtkRenderer->elg_colorbyeigenvalues = ui->elg_colorbyeigen->isChecked();

    vtkRenderer->sqg_scalefactor = ui->sqg_scalefactor->value()/100.;
    vtkRenderer->sqg_colorbyeigenvalues = ui->sqg_colorbyeigen->isChecked();
    vtkRenderer->sqg_gamma = ui->sqg_gamma->text().toDouble();
    vtkRenderer->sqg_absoluteeigenvalues = ui->sqg_abs_eigenvalues->isChecked();

    vtkRenderer->amplification = 50.*(1+ui->def_factor->value()/100.);
    vtkRenderer->showElements = ui->showElements->isChecked();
    vtkRenderer->showNodes = ui->showNodes->isChecked();
    vtkRenderer->showTranslucentModel = ui->showTranslucentModel->isChecked();

    vtkRenderer->showUndeformedModel = ui->ShowUndeformedModel->isChecked();
    vtkRenderer->showRestrictions = ui->showRestrictions->isChecked();
    vtkRenderer->showLoading = ui->showLoading->isChecked();

    vtkRenderer->hsl_radiusfactor = ui->hsl_radiusfactor->value()/100.;
    vtkRenderer->hsl_spLoading = ui->hsl_spLoading->isChecked();
    vtkRenderer->hsl_spPlanez0 = ui->hsl_spPlanez0->isChecked();
    vtkRenderer->hsl_spRandom = ui->hsl_spRandom->isChecked();
    vtkRenderer->hsl_spRestrictions = ui->hsl_spRestrictions->isChecked();

    vtkRenderer->hsl_v1 = ui->hsl_v1->isChecked();
    vtkRenderer->hsl_v2 = ui->hsl_v2->isChecked();
    vtkRenderer->hsl_v3 = ui->hsl_v3->isChecked();

    vtkRenderer->nIsoSurfaceSlices = ui->nIsoSurfaceSlices->value();

    vtkRenderer->cutterNormalPlane = QVector3D(ui->cutter_nx->text().toDouble(),
                                               ui->cutter_ny->text().toDouble(),
                                               ui->cutter_nz->text().toDouble());

    vtkRenderer->cutterPosition = ui->cutter_position->value()/100.;

    vtkRenderer->showLeftPart = ui->showLeftPart->isChecked();

}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    updateParameters();
    //vtkRenderer->renderVTK();
}


void MainWindow::direct_solver(void)
{
    isIterativeSolver = false;
    solver();
}

void MainWindow::iterative_solver(void)
{
    isIterativeSolver = true;
    solver();
}

void MainWindow::updateCutter(void)
{
    updateParameters();
    vtkRenderer->planeCutter();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_F5)
    {
        delete ui->openGLWidget;

        ui->openGLWidget = new vtkGraphicWindow(ui->splitter_1);
        ui->openGLWidget->setObjectName(QStringLiteral("openGLWidget"));
        ui->openGLWidget->setMouseTracking(true);
        ui->openGLWidget->setFocusPolicy(Qt::StrongFocus);
        ui->openGLWidget->setAutoFillBackground(true);
        ui->splitter_1->addWidget(ui->openGLWidget);

        vtkRenderer = ui->openGLWidget;

        updateParameters();
        vtkRenderer->s3d_mesh = s3d_mesh;
        vtkRenderer->removeDataSet();
        vtkRenderer->addDataSet();


        MsgLog::error(QString("vtkRenderer was restarted"));
    }

    if((event->key() == Qt::Key_F1) || (event->key() == Qt::Key_Space))
        vtkRenderer->screenshot();

    if(event->key() == Qt::Key_F8)
        vtkRenderer->zoomToExtent();
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


