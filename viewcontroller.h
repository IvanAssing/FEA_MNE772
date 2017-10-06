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


#ifndef VIEWCONTROL_H
#define VIEWCONTROL_H

#include <QPointF>
#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QGLWidget>

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};


///
/// \brief The ViewController class
///
class ViewController : public QObject
{
    Q_OBJECT

public:
    explicit ViewController(QGLWidget*parent=0);
    QGLWidget *parent;

    int width;
    int height;
    bool orthographic;
    float fov;
    float fovMax;

    float iez,zView,scale,zoom;

    float xRight, xLeft, yBottom, yTop, zNear, zFar;

    QQuaternion rotation;
    QVector3D axis;
    float angle;

    struct{
    QVector3D center;
    float radius;
    } world;

    // matrix
    QMatrix4x4 mmodel;
    QMatrix4x4 mview;
    QMatrix4x4 mprojection;

    bool runningArcball;
    bool runningPanning;

    QPointF lastPos2D;
    //QVector2D lastPos2DPan;
    QVector3D lastPos3DPan;
    QVector3D lastPos3D;
    QPoint mousept;

    QVector3D pivot;
    QVector3D pivot2;
    QVector3D panning;

    // camera vectors
    QVector3D position;
    QVector3D center;
    QVector3D front;
    QVector3D up;

    // scene dimensions
    QVector3D sceneLimitsSup;
    QVector3D sceneLimitsInf;



    // arcball controllers
    void startArcball(const QPoint &point);
    void runArcball(const QPoint &point);
    void stopArcball(const QPoint &point);

    // pan controllers
    void startPanning(const QPoint &point);
    void runPanning(const QPoint &point);
    void stopPanning(const QPoint &point);

    QPointF pixelPosToViewPos(const QPoint& point);
    QVector3D getViewPos(const QPoint& point);

    void ProcessKeyboard(CameraMovement direction, float deltaTime);

    void setZoomInOut(int delta, const QPoint &point);
    void setFovMax(float fovMax=90.f);
    float aspect(void);

    QMatrix4x4 getProjectionMatrix(void);
    QMatrix4x4 getViewMatrix(void);
    QMatrix4x4 getModelMatrix(void);

public slots:
    virtual void setXYTopView(void);
    virtual void setXYBottomView(void);
    virtual void setYZTopView(void);
    virtual void setYZBottomView(void);
    virtual void setXZTopView(void);
    virtual void setXZBottomView(void);
    virtual void setIsometricView(void);
    virtual void setOrthografic(void);
    virtual void setPerpective(void);


protected:
    virtual ~ViewController();

};

#endif // VIEWCONTROL_H
