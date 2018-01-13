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

#ifndef VTKGRAPHICWINDOW_H
#define VTKGRAPHICWINDOW_H

#include <QVTKOpenGLWidget.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkDataSet.h>
#include <vtkUnstructuredGrid.h>
#include <vtkScalarBarActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkScalarBarWidget.h>


#include "truss3d.h"
#include "solid3d.h"
#include <QTimer>

#include <QVector3D>



class vtkGraphicWindow : public QVTKOpenGLWidget
{
    Q_OBJECT
public:
    explicit vtkGraphicWindow(QWidget *parent = 0);

    void addDataSet(Truss3D *mesh);
    void addDataSet_solved(Truss3D *mesh);


    void reset(void);

    Solid3D *s3d_mesh;

    void removeDataSet();
    int step;
    int s3d_result;

    unsigned int nSteps;




    // parameters
    bool elg_colorbyeigenvalues;
    double elg_scalefactor;
    bool sqg_colorbyeigenvalues;
    double sqg_scalefactor;
    double sqg_gamma;
    bool sqg_absoluteeigenvalues;

    double amplification;
    bool showElements;
    bool showNodes;
    bool showTranslucentModel;
    bool showUndeformedModel;
    bool showLoading;
    bool showRestrictions;

    double hsl_radiusfactor;
    bool hsl_spLoading;
    bool hsl_spPlanez0;
    bool hsl_spRandom;
    bool hsl_spRestrictions;
    bool hsl_v1;
    bool hsl_v2;
    bool hsl_v3;


    int nIsoSurfaceSlices;

    QVector3D cutterNormalPlane;
    double cutterPosition;
    bool showLeftPart;


public slots:
    void zoomToExtent();

    void addDataSet(/*Solid3D *mesh*/ void);
    void addDataSet_solved(/*Solid3D *mesh*/ void);
    void addDataSet_simulation(/*Solid3D *mesh*/ void);

    void startSimulation(void);
    void stopSimulation(void);
    void pauseSimulation(void);
    void drawNextStep(void);
    void setParallelProjection(void);
    void setPerspectiveProjection(void);

    void setXYTopView(void);
    void setXYBottomView(void);
    void setYZTopView(void);
    void setYZBottomView(void);
    void setXZTopView(void);
    void setXZBottomView(void);
    void setIsometricView(void);

    void screenshot(void);

    void setupInitialModel(void);
    void setupShowLoading(void);
    void setupShowRestrictions(void);

    void addComplements(void);

    void ellipsoidGlyphsVisualization(void);
    void hyperstreamlinesVisualization(void);
    void isoSurfaceVisualization(void);
    void superquadricsGlyphsVisualization(void);
    void displacementVisualization(void);
    void planeCutter(void);


private:
        double refSize;
    QTimer  *timer;
    vtkSmartPointer<vtkRenderer> m_renderer;

    vtkSmartPointer<vtkActor> *simulation_actors;
    vtkSmartPointer<vtkScalarBarActor> scalarBar;
    vtkSmartPointer<vtkScalarBarWidget> scalarBarWidget;
    vtkSmartPointer<vtkOrientationMarkerWidget> axes_widget;


    vtkSmartPointer<vtkActor> initialModelActor;
    vtkSmartPointer<vtkActor> loadingActor;
    vtkSmartPointer<vtkActor> restrictionActor;
    vtkSmartPointer<vtkActor> outlineActor;

    vtkSmartPointer<vtkActor> actor_0;
    vtkSmartPointer<vtkActor> actor_1;

    vtkSmartPointer<vtkUnstructuredGrid> dataSet_0;
    vtkSmartPointer<vtkUnstructuredGrid> dataSet_1;



};

#endif // VTKGRAPHICWINDOW_H
