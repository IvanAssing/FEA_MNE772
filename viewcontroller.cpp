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

#include "viewcontroller.h"
#include <math.h>
#include <qmath.h>

#define PAN_INTENSITY 1.0f
#define VIEW_POINT_FACTOR 2.5f
#define ZOOM_FACTOR 0.001f

ViewController::ViewController(QGLWidget *parent_)
    :parent(parent_)
{
    parent = parent_;
    runningArcball = false;
    rotation = QQuaternion(); // unitary
    axis = QVector3D(0.0f, 1.0f, 0.0f); // axis y

    mview.setToIdentity();
    mmodel.setToIdentity();
    mprojection.setToIdentity();

    fov = 45.f;
    zoom = 1.0f;

    panning = QVector3D(0.0,0.0,0.0);
    orthographic = false;

    world.center = QVector3D(0,0,0);
    world.radius = 1.0f;

}

void ViewController::startArcball(const QPoint &point)
{
    runningArcball = true;

    lastPos2D = pixelPosToViewPos(point);

    // start vector
    lastPos3D = QVector3D(lastPos2D.x(), lastPos2D.y(), 0.0f);
    float z_2 = 1.0f - QVector3D::dotProduct(lastPos3D, lastPos3D); // z=sqrt(1-x2-y2)
    if (z_2 > 0)
        lastPos3D.setZ(std::sqrt(z_2));
    else
        lastPos3D.normalize();
}


void ViewController::runArcball(const QPoint &point)
{
    if(!runningArcball) return;

    QPointF currentPos2D = pixelPosToViewPos(point);

    // current vector
    QVector3D currentPos3D = QVector3D(currentPos2D.x(), currentPos2D.y(), 0.0f);
    float z_2 = 1.0f - QVector3D::dotProduct(currentPos3D, currentPos3D);
    if (z_2 > 0)
        currentPos3D.setZ(std::sqrt(z_2));
    else
        currentPos3D.normalize();

    axis = QVector3D::crossProduct(lastPos3D, currentPos3D);
    angle = qRadiansToDegrees(std::asin(std::sqrt(QVector3D::dotProduct(axis, axis))));

    axis.normalize();
    rotation = QQuaternion::fromAxisAndAngle(axis, angle) * rotation;

    lastPos2D = currentPos2D;
    lastPos3D = currentPos3D;
}

void ViewController::stopArcball(const QPoint &point)
{
    runArcball(point);
    runningArcball = false;
}


QPointF ViewController::pixelPosToViewPos(const QPoint &point)
{
    return QPointF(2.0f * float(point.x()) / width - 1.0f,
                   1.0f - 2.0f * float(point.y()) / height);
}

void ViewController::setZoomInOut(int delta, const QPoint &point)
{
    //    if(!orthografic)
    //    {
    //        float offset = delta/250.0f;

    //        if (fov >= 1.0f && fov <= fovMax)
    //            fov -= offset;
    //        if (fov <= 1.0f)
    //            fov = 1.0f;
    //        if (fov >= fovMax)
    //            fov = fovMax;
    //    }
    //    else
    //    {
    //        float offset = -delta/50.0f;
    //        QVector3D path = position - center;

    //        //center+= offset * path.normalized();
    //        qDebug()<<offset*path.normalized();
    //        position += offset * path.normalized();


    //    }

    pivot = getViewPos(point);

    zoom *= (1+delta*ZOOM_FACTOR);
    //qDebug()<<zoom<<" "<<delta;

}

float ViewController::aspect(void)
{
    return float(width)/float(height);
}

void ViewController::setFovMax(float _fovMax)
{
    this->fovMax = _fovMax;
}

void ViewController::ProcessKeyboard(CameraMovement direction, float deltaTime)
{

    QVector3D front = QVector3D((position-center).normalized());
    QVector3D right = QVector3D::crossProduct(front, up).normalized();

    float velocity = world.radius/20.f;

    if (direction == BACKWARD)
    {
        position += up * velocity;
        center += up * velocity;
    }
    if (direction == FORWARD)
    {
        position -= up * velocity;
        center -= up * velocity;
    }
    if (direction == LEFT)
    {
        position -= right * velocity;
        center -= right * velocity;
    }
    if (direction == RIGHT)
    {
        position += right * velocity;
        center += right * velocity;
    }
    if (direction == UP)
    {
        position -= front * velocity;
        center -= front * velocity;
    }
    if (direction == DOWN)
    {
        position += front * velocity;
        center += front * velocity;
    }

}


QVector3D ViewController::getViewPos(const QPoint& point)
{
    float x = 2.0f * float(point.x()) / width - 1.0f;
    float y = 1.0f - 2.0f * float(point.y()) / height;
    float m33 = -(1-zFar*iez)/(zNear-zFar);
    float z = (zView+m33*zNear) / (zView*iez+m33);


    return QVector3D(x, y, z);
}

void ViewController::startPanning(const QPoint &point)
{
    runningPanning = true;
    lastPos3DPan = getViewPos(point);
}


void ViewController::runPanning(const QPoint &point)
{
    if(!runningPanning) return;

    QVector3D currentPos3D = getViewPos(point);

    if(zView = 1.0f)
        currentPos3D.setZ(lastPos3DPan.z());

    QVector3D pan = currentPos3D - lastPos3DPan;
    panning += PAN_INTENSITY*pan;

    lastPos3DPan = currentPos3D;
}

void ViewController::stopPanning(const QPoint &point)
{
    runPanning(point);
    runningPanning = false;
}


QMatrix4x4 ViewController::getProjectionMatrix(void)
{
    if(!orthographic) // perpective
    {

        zNear = 0.1f;
        zFar = 1000.f;

        float length = 0.5f*(zNear+zFar)*tan(qDegreesToRadians(fov/2.0));
        scale = 0.5*length/world.radius;

        float y = zNear*tan(qDegreesToRadians(fov/2.0));
        float x = y * aspect();

        //        length = std::min(y,x);
        //        scale = 0.5*length/world.radius;

        iez = std::min(x,y)/tan(qDegreesToRadians(fov/2.0));


        mprojection.setToIdentity();
        mprojection.frustum(-x, x, -y, y, zNear, zFar);

        mprojection.scale(scale);

    }
    else // orthographic
    {
        zNear = -500.f;
        zFar = 500.f;

        float y = 1.f;
        float x = y * aspect();

        float length = std::min(y,x);
        scale = 0.5*length/world.radius;

        iez = 0.0;

        mprojection.setToIdentity();
        mprojection.ortho(-x,x, -y, y, zNear, zFar);

        mprojection.scale(scale);

    }

    return mprojection;
}

QMatrix4x4 ViewController::getViewMatrix(void)
{

    mview.setToIdentity();
    mview.lookAt(position, center, up);
    return mview;


}

QMatrix4x4 ViewController::getModelMatrix(void)
{


    mmodel.setToIdentity();


    mmodel.translate(world.radius*panning);

    mmodel.translate(center);
    mmodel.rotate(rotation);
    mmodel.translate(-center);

    mmodel.translate(center);
    mmodel.scale(zoom);
    mmodel.translate(-center);


    return mmodel;

}



void ViewController::setXYTopView(void)
{
    rotation = QQuaternion();
    mview.setToIdentity();

    center = world.center;
    position = center + QVector3D(0.0, 0.0, VIEW_POINT_FACTOR*world.radius);
    up = QVector3D(0,1,0);

    parent->updateGL();
}

void ViewController::setXYBottomView(void)
{
    rotation = QQuaternion();

    center = world.center;
    position = center + QVector3D(0.0, 0.0, -VIEW_POINT_FACTOR*world.radius);
    up = QVector3D(0,1,0);

    parent->updateGL();
}

void ViewController::setYZTopView(void)
{
    rotation = QQuaternion();

    center = world.center;
    position = center + QVector3D(VIEW_POINT_FACTOR*world.radius, 0.0, 0.0);
    up = QVector3D(0,0,1);

    parent->updateGL();
}

void ViewController::setYZBottomView(void)
{
    rotation = QQuaternion();

    center = world.center;
    position = center + QVector3D(-VIEW_POINT_FACTOR*world.radius, 0.0, 0.0);
    up = QVector3D(0,0,1);

    parent->updateGL();
}

void ViewController::setXZTopView(void)
{
    rotation = QQuaternion();

    center = world.center;
    position = center + QVector3D(0.0, VIEW_POINT_FACTOR*world.radius, 0.0);
    up = QVector3D(0,0,1);

    parent->updateGL();
}

void ViewController::setXZBottomView(void)
{
    rotation = QQuaternion();

    center = world.center;
    position = center + QVector3D(0.0, -VIEW_POINT_FACTOR*world.radius, 0.0);
    up = QVector3D(0,0,1);

    parent->updateGL();
}


void ViewController::setIsometricView(void)
{
    rotation = QQuaternion();

    center = world.center;
    position = center + QVector3D(-VIEW_POINT_FACTOR*world.radius, -VIEW_POINT_FACTOR*world.radius, VIEW_POINT_FACTOR*world.radius);
    up = QVector3D(0,0,1);

    parent->updateGL();
}

void ViewController::setOrthografic(void)
{
    orthographic = true;
    parent->updateGL();
}

void ViewController::setPerpective(void)
{
    orthographic = false;
    parent->updateGL();
}


ViewController::~ViewController()
{

}





