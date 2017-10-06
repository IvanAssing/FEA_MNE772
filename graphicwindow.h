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

#ifndef GRAPHICWINDOW_H
#define GRAPHICWINDOW_H


#include <QGLWidget>
#include <QMouseEvent>
#include <QCursor>
#include <QTimer>
#include <QLabel>

#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>

#include "truss3d.h"
#include "solid3d.h"
#include "viewcontroller.h"
#include "drawing.h"



enum Model{
    truss3d,
    solid3d,
    notdefined
};

///
/// \brief The GraphicWindow class
///
class GraphicWindow : public QGLWidget
{
    Q_OBJECT
public:
    explicit GraphicWindow(QWidget *parent = 0);

    QTimer  *timer;
    Truss3D *t3d_mesh;
    Solid3D *s3d_mesh;

    Model model;

    ViewController *view;

    QOpenGLVertexArrayObject *vao/*,*vao_selection*/;

    Drawing *grid;
    Drawing *axis;
    Drawing *initialModel;
    Drawing *solvedModel;
    Drawing *simulationModel;
    Drawing *selectionModel;
    Drawing *legend;
    Drawing *textRender;

    Drawing *loading;
    Drawing *restrictions;


    QOpenGLShaderProgram program;

    QMatrix4x4 projection_matrix;
    QMatrix4x4 model_matrix;
    QMatrix4x4 view_matrix;

    QVector2D mousePressPosition;

    void setupGrid(void);
    void setupInitialModel(Truss3D *updatedMesh);
    void setupSolvedModel(Truss3D *updatedMesh);
    void setupSimulationModel(Truss3D *updatedMesh);

    void setupInitialModel(Solid3D *updatedMesh);
    void setupSolvedModel(Solid3D *updatedMesh);
    void setupSimulationModel(Solid3D *updatedMesh);

    void setupSelectionModel(void);
    void setupView(void);

    QStringList *strLegend;
    void setupLegend(void);
    void drawLegend(int n_result);
    void drawGrid(QMatrix4x4 mvp);
    void setupTextRender(void);
    void renderTextLine(QString text, GLfloat x, GLfloat y, GLfloat scale, QVector3D color);
    void drawSelectedElement(void);

    void restart(void);

    // miscellaneous
    int step;
    int nFrames;
    int id_selectedElement;
    float amplification;

    bool isSelectionActived;
    bool isGridActived;
    bool isInitialModelActived;
    bool isSolvedModelActived;
    bool isSimulationModelActived;
    bool isLegendActived;
    bool isShowMesh;
    bool isShowNumbering;

    bool isLightingOn;

    int s3d_result;

    float grid_sx, grid_sy, grid_delta;

signals:

public slots:
    void initializeGL();
    void resizeGL(int width, int height);
    void updateGL();


    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    //void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

    void select(void);
    void showGrid(void);
    void showInitialModel(void);
    void showSolvedModel(void);
    void showSimulationModel(void);
    void showLegend(void);
    void showMesh(void);

    void startSimulation(void);
    void stopSimulation(void);
    void pauseSimulation(void);
    void drawNextStep(void);

    void takePicture(void);

    void turnOffLighting(void);

};

#endif // GRAPHICWINDOW_H


