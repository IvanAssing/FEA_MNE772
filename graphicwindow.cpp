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

#include "graphicwindow.h"

#include <cmath>
#include <iomanip>
#include <string>
#include <cstdio>

#include <qmath.h>
#include <QImage>
#include <QString>
#include <QImageWriter>
#include <QDateTime>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QFont>

#include <QOpenGLTexture>
#include <QBitmap>

#include <GL/gl.h>
#include <GL/glu.h>
#include <FTGL/ftgl.h>
#include FT_FREETYPE_H

#include <mth/vector.h>
#include "dxfreader.h"
#include "truss3dreader.h"
#include "solid3d.h"

const char strResults[10][50] = {
    "normal stress x",
    "normal stress y",
    "normal stress z",
    "shear stress xy",
    "shear stress yz",
    "shear stress zx",
    "Von Mises ",
    "displacement ux",
    "displacement uy",
    "displacement uz",
};


struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    //    QOpenGLTexture *texture;
    QVector2D Size;    // Size of glyph
    QVector2D Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;



GraphicWindow::GraphicWindow(QWidget *parent) :
    QGLWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAutoBufferSwap(true);
    setFormat(QGLFormat(QGL::SampleBuffers));

    view = new ViewController(this);
    view->setFovMax(150.f);

    view->sceneLimitsInf = QVector3D(-1.f, -1.f, -1.f);
    view->sceneLimitsSup = QVector3D(1.f, 1.f, 1.f);

    view->center = QVector3D(0.f, 0.f, 0.f);

    view->position = QVector3D(view->center.x(),2.*view->sceneLimitsInf.y(),view->sceneLimitsSup.z());
    view->up = QVector3D(0,1,0);
    view->mview.setToIdentity();
    view->mview.lookAt(view->position, view->center, view->up);

    amplification = 50.0f;
    id_selectedElement = -1;
    nFrames = 100;
    s3d_result=0;

    isSelectionActived = false;
    isGridActived = false;
    isInitialModelActived = false;
    isSolvedModelActived = false;
    isSimulationModelActived = false;
    isLegendActived = false;
    isShowMesh = true;
    isShowNumbering = false;

    isLightingOn =true;

    timer = new QTimer(this);
    model = Model::notdefined;

    strLegend = new QStringList[10];;

    //    QGLFormat currentFormat = format();
    //    currentFormat.setSampleBuffers(true);
    //    currentFormat.setSamples(16);
    //    setFormat(currentFormat);


    //    QGLWidget::setFormat(QGLFormat(QGL::SampleBuffers));
    //    this->format().setSamples(8);

    //    label=nullptr;

}



void GraphicWindow::initializeGL()
{
    glShadeModel(GL_SMOOTH);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_DEPTH_TEST);
    //glDepthMask(GL_TRUE);
    //glDepthFunc(GL_ALWAYS);
    glDepthRange(0.0f, 1.0f);

    //    glEnable(GL_COLOR_MATERIAL);
    //    glEnable(GL_LIGHTING);
    //    glEnable(GL_LIGHT0);


    glEnable(GL_MULTISAMPLE);
    //    GLint bufs;
    //    GLint samples;
    //    glGetIntegerv(GL_SAMPLE_BUFFERS, &bufs);
    //    glGetIntegerv(GL_SAMPLES, &samples);
    //    qDebug("Have %d buffers and %d samples", bufs, samples);

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClearDepth(1.0f);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);


    projection_matrix.setToIdentity();
    model_matrix.setToIdentity();
    view_matrix.setToIdentity();

    vao = new QOpenGLVertexArrayObject(this);
    vao->create();
    vao->bind();

    //    nodes = new Drawing(vao);
    //    elements = new Drawing(vao);
    grid = new Drawing(vao);
    axis = new Drawing(vao);
    legend = new Drawing(vao);

    initialModel = new Drawing(vao);
    solvedModel = new Drawing(vao);
    simulationModel = new Drawing(vao);
    selectionModel = new Drawing(vao);
    textRender = new Drawing(vao);

    loading= new Drawing(vao);
    restrictions = new Drawing(vao);

    //    solid3d = new Drawing(vao);
    //    solid3dSolvedModel = new Drawing(vao);


    // setupGrid();
    setupTextRender();


}

void GraphicWindow::resizeGL(int width, int height)
{
    glViewport( 0, 0, (GLint)width, (GLint)height );
    view->width = float(width);
    view->height = float(height);
    updateGL();
}

void GraphicWindow::wheelEvent(QWheelEvent *event)
{

    view->setZoomInOut(event->delta(), event->pos());
    //view->pivot2 = view->getViewPos(event->pos());
    //qDebug()<<view->pivot2;
    updateGL();

}

void GraphicWindow::mousePressEvent(QMouseEvent *event)
{

    updateGL();

    if(event->buttons() & Qt::RightButton)
    {
        view->startArcball(event->pos());
        //setCursor(Qt::SizeAllCursor);
        updateGL();

    }

    if(event->buttons() & Qt::LeftButton)
    {
        view->pivot = QVector3D(view->pixelPosToViewPos(event->pos()));

        isSelectionActived = false;

    }

    if(event->buttons() & Qt::MiddleButton)
    {
        updateGL();
        view->startPanning(event->pos());
        //setCursor(Qt::SizeAllCursor);
        //updateGL();
    }

}

void GraphicWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::RightButton)
    {
        view->stopArcball(event->pos());
        //setCursor(Qt::ArrowCursor);
        updateGL();
    }
    if(event->buttons() & Qt::MiddleButton)
    {
        updateGL();
        view->stopPanning(event->pos());
        //setCursor(Qt::ArrowCursor);
        //updateGL();
    }
}


void GraphicWindow::takePicture(void)
{
    QDateTime now = QDateTime::currentDateTime();

    QString filename = QString("../screenshots/screenshoot-")
            + now.toString("yyyyMMddhhmmsszzz") + QString(".png");

    updateGL();

    this->grabFrameBuffer(true).save(filename, "PNG", 100);
}


void GraphicWindow::mouseMoveEvent(QMouseEvent *event)
{
    view->mousept = event->pos();
    updateGL();

    if(event->buttons() & Qt::RightButton)
    {
        view->runArcball(event->pos());
        updateGL();
    }

    if(event->buttons() & Qt::MiddleButton)
    {
        updateGL();
        view->runPanning(event->pos());
    }
    if(isSelectionActived)
        updateGL();
}




void GraphicWindow::keyPressEvent(QKeyEvent *event)
{


    int deltaTime = 1.;

    if (event->key() == Qt::Key_W)
        view->ProcessKeyboard(FORWARD, deltaTime);

    if (event->key() == Qt::Key_S)
        view->ProcessKeyboard(BACKWARD, deltaTime);

    if (event->key() == Qt::Key_A)
        view->ProcessKeyboard(LEFT, deltaTime);

    if (event->key() == Qt::Key_D)
        view->ProcessKeyboard(RIGHT, deltaTime);

    if (event->key() == Qt::Key_Up)
        view->ProcessKeyboard(UP, deltaTime);

    if (event->key() == Qt::Key_Down)
        view->ProcessKeyboard(DOWN, deltaTime);

    if (event->key() == Qt::Key_O)
        view->orthographic = !view->orthographic;

    if (event->key() == Qt::Key_N)
        isShowNumbering = !isShowNumbering;

    if (event->key() == Qt::Key_Right)
    {
        step++;
        if(step>=nFrames) step=0;
    }
    if (event->key() == Qt::Key_Left)
    {
        step--;
        if(step<0) step=nFrames-1;
    }

    if (event->key() == Qt::Key_R)
        setupView();

    updateGL();
}


void GraphicWindow::updateGL()
{
    glClearColor(0.7f, 0.7f, 0.7f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if(model==truss3d || model==solid3d)
    {
        projection_matrix = view->getProjectionMatrix();
        view_matrix = view->getViewMatrix();
        model_matrix = view->getModelMatrix();


        QMatrix4x4 mvp;
        //mvp = projection_matrix*model_matrix*rotation*pan*view_matrix;
        mvp = projection_matrix*view_matrix*model_matrix;

        if(model == Model::truss3d)
        {

            if(isSelectionActived && selectionModel->ready)
            {
                // framebuffer for picking
                QOpenGLFramebufferObjectFormat fboFormat;
                fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
                QOpenGLFramebufferObject fbo_selection(view->width, view->height, fboFormat);

                fbo_selection.bind();

                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                selectionModel->bind();
                selectionModel->program.setUniformValue("mvp_matrix", mvp);

                glLineWidth(5.f);
                glDrawArrays(GL_LINES, 0, selectionModel->nVertex);

                glFlush();
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                unsigned char data[4];
                glReadPixels(view->mousept.x(), view->height-view->mousept.y(), 1,1,GL_RGBA, GL_UNSIGNED_BYTE, data);
                id_selectedElement = data[0]+data[1]*256+data[2]*256*256;
                if(id_selectedElement>t3d_mesh->nElements) id_selectedElement = -1;
                //fbo_selection.toImage().save("selection_framebuffer.png","PNG", 100);


                fbo_selection.release();

                // end of framebuffer
            }


            glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black
            //glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // white
            //glClearColor(0.8f, 0.8f, 0.8f, 1.0f); // dark gray
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            if(grid->ready && isGridActived)
                drawGrid(mvp);

            glLineWidth(1.f);

            if(initialModel->ready && isInitialModelActived)
            {
                initialModel->bind();
                initialModel->vboi.bind();
                initialModel->program.setUniformValue("mvp_matrix", mvp);
                glDrawElements(GL_LINES, initialModel->nIndex, GL_UNSIGNED_INT, 0);

                glPointSize(5.f);
                glDrawArrays(GL_POINTS, 0, initialModel->nVertex);

                loading->bind();
                loading->program.setUniformValue("mvp_matrix", mvp);
                glLineWidth(4.f);
                glDrawArrays(GL_LINES, 0, loading->nVertex);

                restrictions->bind();
                restrictions->program.setUniformValue("mvp_matrix", mvp);
                glPointSize(5.f);
                glDrawArrays(GL_POINTS, 0, restrictions->nVertex);
            }


            if(solvedModel->ready && isSolvedModelActived  && !isSimulationModelActived)
            {
                solvedModel->bind();
                solvedModel->program.setUniformValue("mvp_matrix", mvp);
                glDrawArrays(GL_LINES, 0, solvedModel->nVertex);

            }


            if(simulationModel->ready && isSimulationModelActived)
            {
                simulationModel->bind();
                simulationModel->program.setUniformValue("mvp_matrix", mvp);
                glDrawArrays(GL_LINES, step*simulationModel->nVertex, simulationModel->nVertex);
            }

            if(isSelectionActived && selectionModel->ready && id_selectedElement!=-1)
                drawSelectedElement();

            if(legend->ready && isLegendActived)
            {
                drawLegend(0);
            }



            if(isShowNumbering)
            {
                QMatrix4x4 mvp_legend;
                for(int i=0; i<t3d_mesh->nNodes; i++)
                {


                    mvp_legend = mvp;
                    mvp_legend.translate(t3d_mesh->nodes[i]->coordinates[0], t3d_mesh->nodes[i]->coordinates[1], t3d_mesh->nodes[i]->coordinates[2]);

                    //            QVector3D a(0.0, 0.0, 1.0);
                    //            QVector3D b= (view->position - view->center);
                    //            a = mvp_legend*a;
                    //            a.normalize();
                    //            b.normalize();

                    //            QVector3D v = QVector3D::crossProduct(a,b);
                    //            float c = QVector3D::dotProduct(a,b);

                    //            QMatrix4x4 vx;
                    //            vx.fill(0.0);
                    //            vx(3,3) = 1.0;

                    //            vx(0,1) = -v[2];
                    //            vx(1,0) = v[2];
                    //            vx(0,2) = v[1];
                    //            vx(2,0) = -v[1];
                    //            vx(1,2) = -v[0];
                    //            vx(2,1) = v[0];

                    //            QMatrix4x4 R;

                    //            R.setToIdentity();
                    //            R = R + vx + (vx*vx)*(1.f/(1.f+c));

                    //            mvp_legend = R*mvp;


                    textRender->bind();
                    textRender->program.setUniformValue("projection",  mvp_legend);

                    renderTextLine(QString("%1").arg(t3d_mesh->nodes[i]->index), 0.0, 0.0, 0.005, QVector3D(1,1,1));
                }


                for(int i=0; i<t3d_mesh->nElements; i++)
                {
                    mvp_legend = mvp;
                    mvp_legend.translate(0.5*(t3d_mesh->elements[i]->node1->coordinates[0] + t3d_mesh->elements[i]->node2->coordinates[0]),
                            0.5*(t3d_mesh->elements[i]->node1->coordinates[1] + t3d_mesh->elements[i]->node2->coordinates[1]),
                            0.5*(t3d_mesh->elements[i]->node1->coordinates[2] + t3d_mesh->elements[i]->node2->coordinates[2]));

                    textRender->bind();
                    textRender->program.setUniformValue("projection",  mvp_legend);

                    renderTextLine(QString("%1").arg(t3d_mesh->elements[i]->index), 0.0, 0.0, 0.005, QVector3D(1,0,0));
                }
            }


        }

        else if(model == Model::solid3d)
        {

            if(isSelectionActived && selectionModel->ready)
            {
                // framebuffer for picking
                QOpenGLFramebufferObjectFormat fboFormat;
                fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
                QOpenGLFramebufferObject fbo_selection(view->width, view->height, fboFormat);

                fbo_selection.bind();

                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                selectionModel->bind();
                selectionModel->program.setUniformValue("mvp_matrix", mvp);

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDrawArrays(GL_TRIANGLES, 0, selectionModel->nVertex);

                glFlush();
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                unsigned char data[4];
                glReadPixels(view->mousept.x(), view->height-view->mousept.y(), 1,1,GL_RGBA, GL_UNSIGNED_BYTE, data);
                id_selectedElement = data[0]+data[1]*256+data[2]*256*256;
                if(id_selectedElement>s3d_mesh->nElements) id_selectedElement = -1;
                //fbo_selection.toImage().save("selection_framebuffer.png","PNG", 100);
                //qDebug()<<id_selectedElement;


                fbo_selection.release();
                //isSelectionActived = false;

                // end of framebuffer
            }

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black
            //glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // white
            //glClearColor(0.4f, 0.4f, 0.4f, 1.0f); // dark gray
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);


            if(grid->ready && isGridActived)
                drawGrid(mvp);

            glLineWidth(1.f);


            if(simulationModel->ready && isSimulationModelActived)
            {
                simulationModel->bind(step, 10*step+s3d_result);
                simulationModel->vboi.bind();
                simulationModel->program.setUniformValue("mvp_matrix", mvp);


                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                simulationModel->program.setUniformValue("isSolidColor", bool(false));
                glDrawElements(GL_TRIANGLES, simulationModel->nIndex, GL_UNSIGNED_INT, 0);

                if(isShowMesh)
                {
                    simulationModel->program.setUniformValue("isSolidColor", bool(true));
                    glDrawElements(GL_LINES, simulationModel->nIndex, GL_UNSIGNED_INT, 0);

                    glPointSize(3.f);
                    glDrawArrays(GL_POINTS, 0, simulationModel->nVertex);
                }
            }

            if(solvedModel->ready && isSolvedModelActived && !isSimulationModelActived)
            {
                solvedModel->bind(s3d_result);

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                solvedModel->program.setUniformValue("model", model_matrix);
                solvedModel->program.setUniformValue("view", view_matrix);
                solvedModel->program.setUniformValue("projection", projection_matrix);

                solvedModel->program.setUniformValue("viewPos", view->position);
                solvedModel->program.setUniformValue("isLightingOn", bool(isLightingOn));

                solvedModel->program.setUniformValue("isSolidColor", bool(false));
                glDrawArrays(GL_TRIANGLES, 0, solvedModel->nVertex);

                if(isShowMesh)
                {
                    solvedModel->program.setUniformValue("isSolidColor", bool(true));
                    glDrawArrays(GL_LINES, 0, solvedModel->nVertex);
                }


            }



            if(initialModel->ready && isInitialModelActived)
            {
                initialModel->bind();
                initialModel->vboi.bind();
                initialModel->program.setUniformValue("mvp_matrix", mvp);

                if(!isSolvedModelActived && !isSimulationModelActived)
                {
                    initialModel->program.setUniformValue("color", QVector4D(0.9f, 0.95f, 0.9f, 1.0f));
                    glDrawElements(GL_TRIANGLES, initialModel->nIndex, GL_UNSIGNED_INT, 0);
                }

                if(isShowMesh)
                {
                    glLineWidth(2.f);
                    initialModel->program.setUniformValue("color", QVector4D(0.7f, 0.3f, 0.2f, 1.0f));
                    glDrawElements(GL_LINES, initialModel->nIndex, GL_UNSIGNED_INT, 0);

                    glPointSize(3.f);
                    glDrawArrays(GL_POINTS, 0, initialModel->nVertex);
                }

                if(loading->ready)
                {
                    loading->bind();
                    loading->program.setUniformValue("mvp_matrix", mvp);
                    glLineWidth(4.f);
                    glDrawArrays(GL_LINES, 0, loading->nVertex);

                    restrictions->bind();
                    restrictions->program.setUniformValue("mvp_matrix", mvp);
                    glPointSize(5.f);
                    glDrawArrays(GL_POINTS, 0, restrictions->nVertex);
                }
            }

            if(isSelectionActived && selectionModel->ready && id_selectedElement!=-1)
                drawSelectedElement();

            if(legend->ready && isLegendActived)
            {
                glDisable(GL_CULL_FACE);

                drawLegend(s3d_result);

                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);
            }


        }

        // read z-buffer
        glReadPixels(view->mousept.x(), view->height-view->mousept.y(), 1,1,GL_DEPTH_COMPONENT, GL_FLOAT, &view->zView);
        //qDebug()<<view->zView;


        swapBuffers();

    }

}

void GraphicWindow::setupGrid(void)
{
    if(grid) delete grid;
    grid = new Drawing(vao);

    float delta = view->world.radius/10.f;

    int nx = static_cast<int>((view->sceneLimitsSup.x()-view->sceneLimitsInf.x())/delta)+1;
    int ny = static_cast<int>((view->sceneLimitsSup.y()-view->sceneLimitsInf.y())/delta)+1;

    nx *= 1.5f;
    ny *= 1.5f;

    int nt = 2*(nx+1)+2*(ny+1);

    GLfloat *vertex = new GLfloat[2*nt];

    GLfloat sx = 0.5f*delta*nx;
    GLfloat sy = 0.5f*delta*ny;
    grid_sx = sx;
    grid_sy = sy;
    grid_delta = delta;

    for(int i=0; i<=nx; i++)
    {
        vertex[4*i] = -sx + i*delta+view->world.center.x();
        vertex[4*i+1] = -sy+view->world.center.y();
        vertex[4*i+2] = vertex[4*i];
        vertex[4*i+3] = sy+view->world.center.y();;
    }
    int inc = 4*(nx+1);
    for(int i=0; i<=ny; i++)
    {
        vertex[inc+4*i] = -sx+view->world.center.x();
        vertex[inc+4*i+1] = -sy + i*delta+view->world.center.y();
        vertex[inc+4*i+2] = sx+view->world.center.x();
        vertex[inc+4*i+3] = vertex[inc+4*i+1];
    }

    grid->vao.bind();
    grid->vbov.bind();
    grid->vbov.allocate(vertex, 2*nt*sizeof(GL_FLOAT));

    grid->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/gridxy.vs");
    grid->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/gridxy.fs");
    grid->program.link();
    grid->program.bind();

    grid->vbov.bind();
    int vertexLocation = grid->program.attributeLocation("position");
    grid->program.enableAttributeArray(vertexLocation);
    grid->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 2, 2*sizeof(GLfloat));
    QVector4D color(.75f, .75f, .75f, .75f);
    grid->program.setUniformValue("color", color);

    grid->program.setUniformValue("posz", view->sceneLimitsInf.z());

    grid->nVertex = nt;
    grid->ready = true;


    if(axis) delete axis;
    axis = new Drawing(vao);

    for(int i=0; i<6; i++)
    {
        vertex[3*i] = -grid_sx+view->world.center.x();
        vertex[3*i+1] = -grid_sy+view->world.center.y();
        vertex[3*i+2] = view->sceneLimitsInf.z();
    }

    vertex[3] += 1.5*delta; // axis X
    vertex[10] += 1.5*delta; // axis Y
    vertex[17] += 1.5*delta; // axis Z

    axis->vao.bind();
    axis->vbov.bind();
    axis->vbov.allocate(vertex, 3*6*sizeof(GL_FLOAT));

    axis->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/initialmodel.vs");
    axis->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/initialmodel.fs");
    axis->program.link();
    axis->program.bind();

    axis->vbov.bind();
    vertexLocation = axis->program.attributeLocation("position");
    axis->program.enableAttributeArray(vertexLocation);
    axis->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    axis->ready = true;

    delete [] vertex;

}

void GraphicWindow::setupInitialModel(Truss3D *updatedMesh)
{

    t3d_mesh = updatedMesh;
    if(!t3d_mesh->isMounted)
        return;

    if(initialModel) delete initialModel;

    setupView();
    setupGrid();

    GLfloat *vertexposition = new GLfloat[3*t3d_mesh->nNodes];

    for(int i=0; i<t3d_mesh->nNodes; i++)
    {
        vertexposition[3*i] = t3d_mesh->nodes[i]->coordinates[0]; // x
        vertexposition[3*i+1] = t3d_mesh->nodes[i]->coordinates[1]; // y
        vertexposition[3*i+2] = t3d_mesh->nodes[i]->coordinates[2]; // z
    }

    initialModel = new Drawing(vao);

    initialModel->vao.bind();
    initialModel->vbov.bind();
    initialModel->vbov.allocate(vertexposition, 3*t3d_mesh->nNodes*sizeof(GL_FLOAT));


    initialModel->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/initialmodel.vs");
    initialModel->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/initialmodel.fs");
    initialModel->program.link();
    initialModel->program.bind();

    initialModel->vbov.bind();
    int vertexLocation = initialModel->program.attributeLocation("position");
    initialModel->program.enableAttributeArray(vertexLocation);
    initialModel->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    QVector4D color(1.0f, 1.0f, 1.0f, 1.0f);
    initialModel->program.setUniformValue("color", color);

    GLuint *vertexIndex = new GLuint[2*t3d_mesh->nElements];

    for(int i=0; i<t3d_mesh->nElements; i++)
    {
        vertexIndex[2*i] = t3d_mesh->elements[i]->node1->index;
        vertexIndex[2*i+1] = t3d_mesh->elements[i]->node2->index;
    }
    initialModel->nIndex = 2*t3d_mesh->nElements;
    initialModel->nVertex = t3d_mesh->nNodes;

    initialModel->vboi.bind();
    initialModel->vboi.allocate(vertexIndex, 2*t3d_mesh->nElements*sizeof(GLuint));
    initialModel->ready = true;



    /// LOADING
    QVector3D volume = view->sceneLimitsSup - view->sceneLimitsInf;
    float delta = std::min(std::min(volume.x(), volume.y()), volume.z())/20.f;
    int count = 0;
    for(int i=0; i<t3d_mesh->nNodes; i++)
    {
        QVector3D normal(t3d_mesh->nodes[i]->loading[0], t3d_mesh->nodes[i]->loading[1], t3d_mesh->nodes[i]->loading[2]);
        if(normal.length()>1e-5)
        {
            normal.normalize();
            normal*=-delta;

            vertexposition[6*count+0] = t3d_mesh->nodes[i]->coordinates[0];
            vertexposition[6*count+1] = t3d_mesh->nodes[i]->coordinates[1];
            vertexposition[6*count+2] = t3d_mesh->nodes[i]->coordinates[2];

            vertexposition[6*count+3] = t3d_mesh->nodes[i]->coordinates[0]+normal.x();
            vertexposition[6*count+4] = t3d_mesh->nodes[i]->coordinates[1]+normal.y();
            vertexposition[6*count+5] = t3d_mesh->nodes[i]->coordinates[2]+normal.z();
            count++;
        }
    }



    if(loading) delete loading;
    loading = new Drawing(vao);

    loading->vao.bind();
    loading->vbov.bind();
    loading->vbov.allocate(vertexposition, 6*count*sizeof(GL_FLOAT));


    loading->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solid3d.vs");
    loading->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solid3d.fs");
    loading->program.link();
    loading->program.bind();

    loading->vbov.bind();
    vertexLocation = loading->program.attributeLocation("position");
    loading->program.enableAttributeArray(vertexLocation);
    loading->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    QVector4D color2(1.0f, .0f, 0.0f, 1.0f);
    loading->program.setUniformValue("color", color2);

    loading->nVertex = count*2;
    loading->ready = true;


    /// RESTRICTION

    count = 0;
    for(int i=0; i<t3d_mesh->nNodes; i++)
    {
        if(t3d_mesh->nodes[i]->restrictions[0])
        {
            vertexposition[3*count+0] = t3d_mesh->nodes[i]->coordinates[0];
            vertexposition[3*count+1] = t3d_mesh->nodes[i]->coordinates[1];
            vertexposition[3*count+2] = t3d_mesh->nodes[i]->coordinates[2];
            count++;
        }
    }


    if(restrictions) delete restrictions;
    restrictions = new Drawing(vao);

    restrictions->vao.bind();
    restrictions->vbov.bind();
    restrictions->vbov.allocate(vertexposition, 3*count*sizeof(GL_FLOAT));


    restrictions->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solid3d.vs");
    restrictions->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solid3d.fs");
    restrictions->program.link();
    restrictions->program.bind();

    restrictions->vbov.bind();
    vertexLocation = restrictions->program.attributeLocation("position");
    restrictions->program.enableAttributeArray(vertexLocation);
    restrictions->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    QVector4D color3(0.3f, 1.0f, 0.3f, 1.0f);
    restrictions->program.setUniformValue("color", color3);

    restrictions->nVertex = count;
    restrictions->ready = true;


    delete [] vertexIndex;
    delete [] vertexposition;
}


void GraphicWindow::setupInitialModel(Solid3D *updatedMesh)
{
    s3d_mesh = updatedMesh;
    if(!s3d_mesh->isMounted)
        return;

    if(initialModel) delete initialModel;

    setupView();
    setupGrid();

    // Vertex
    GLfloat *vertexposition = new GLfloat[3*s3d_mesh->nNodes];

    for(int i=0; i<s3d_mesh->nNodes; i++)
    {
        vertexposition[3*i] = s3d_mesh->nodes[i]->coordinates[0]; // x
        vertexposition[3*i+1] = s3d_mesh->nodes[i]->coordinates[1]; // y
        vertexposition[3*i+2] = s3d_mesh->nodes[i]->coordinates[2]; // z
    }

    // Index
    unsigned int *vertexindex = new unsigned int[12*s3d_mesh->nElements];

    for(int i=0; i<s3d_mesh->nElements; i++)
        for(int j=0; j<4; j++)
            for(int k=0; k<3; k++)
                vertexindex[12*i+3*j+k] = s3d_mesh->elements[i]->nodes[idf[j][k]]->index;

    initialModel = new Drawing(vao);

    initialModel->vao.bind();
    initialModel->vbov.bind();
    initialModel->vbov.allocate(vertexposition, 3*s3d_mesh->nNodes*sizeof(GL_FLOAT));


    initialModel->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solid3d.vs");
    initialModel->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solid3d.fs");
    initialModel->program.link();
    initialModel->program.bind();

    initialModel->vbov.bind();
    int vertexLocation = initialModel->program.attributeLocation("position");
    initialModel->program.enableAttributeArray(vertexLocation);
    initialModel->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    QVector4D color(1.0f, 1.0f, 1.0f, 1.0f);
    initialModel->program.setUniformValue("color", color);

    initialModel->vboi.bind();
    initialModel->vboi.allocate(vertexindex, 12*s3d_mesh->nElements*sizeof(unsigned int));

    //qDebug()<<GL_MAX_ELEMENT_INDEX;

    initialModel->nIndex = 12*s3d_mesh->nElements;
    initialModel->nVertex = s3d_mesh->nNodes;
    initialModel->ready = true;


    delete [] vertexindex;
    delete [] vertexposition;

    // Vertex
    vertexposition = new GLfloat[6*s3d_mesh->nElements];

    /// LOADING
    QVector3D volume = view->sceneLimitsSup - view->sceneLimitsInf;
    float delta = std::min(std::min(volume.x(), volume.y()), volume.z())/20.f;
    int count = 0;
    for(int i=0; i<s3d_mesh->nElements; i++)
        if(s3d_mesh->elements[i]->pface!=-1)
        {
            int iface = s3d_mesh->elements[i]->pface;
            //qDebug()<<i<<" "<<iface;

            QVector3D normal(s3d_mesh->elements[i]->normals[iface]*delta);

            for(int j=0; j<3; j++)
            {
                vertexposition[6*count+0] = s3d_mesh->elements[i]->nodes[idf[iface][j]]->coordinates[0];
                vertexposition[6*count+1] = s3d_mesh->elements[i]->nodes[idf[iface][j]]->coordinates[1];
                vertexposition[6*count+2] = s3d_mesh->elements[i]->nodes[idf[iface][j]]->coordinates[2];

                vertexposition[6*count+3] = s3d_mesh->elements[i]->nodes[idf[iface][j]]->coordinates[0]+normal.x();
                vertexposition[6*count+4] = s3d_mesh->elements[i]->nodes[idf[iface][j]]->coordinates[1]+normal.y();
                vertexposition[6*count+5] = s3d_mesh->elements[i]->nodes[idf[iface][j]]->coordinates[2]+normal.z();
                count++;
            }
        }


    if(loading) delete loading;
    loading = new Drawing(vao);

    loading->vao.bind();
    loading->vbov.bind();
    loading->vbov.allocate(vertexposition, 6*count*sizeof(GL_FLOAT));


    loading->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solid3d.vs");
    loading->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solid3d.fs");
    loading->program.link();
    loading->program.bind();

    loading->vbov.bind();
    vertexLocation = loading->program.attributeLocation("position");
    loading->program.enableAttributeArray(vertexLocation);
    loading->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    QVector4D color2(1.0f, .0f, 0.0f, 1.0f);
    loading->program.setUniformValue("color", color2);

    loading->nVertex = count*2;
    loading->ready = true;


    /// RESTRICTION

    count = 0;
    for(int i=0; i<s3d_mesh->nNodes; i++)
    {
        if(s3d_mesh->nodes[i]->restrictions[0])
        {
            vertexposition[3*count+0] = s3d_mesh->nodes[i]->coordinates[0];
            vertexposition[3*count+1] = s3d_mesh->nodes[i]->coordinates[1];
            vertexposition[3*count+2] = s3d_mesh->nodes[i]->coordinates[2];
            count++;
        }
    }


    if(restrictions) delete restrictions;
    restrictions = new Drawing(vao);

    restrictions->vao.bind();
    restrictions->vbov.bind();
    restrictions->vbov.allocate(vertexposition, 3*count*sizeof(GL_FLOAT));


    restrictions->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solid3d.vs");
    restrictions->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solid3d.fs");
    restrictions->program.link();
    restrictions->program.bind();

    restrictions->vbov.bind();
    vertexLocation = restrictions->program.attributeLocation("position");
    restrictions->program.enableAttributeArray(vertexLocation);
    restrictions->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    QVector4D color3(0.3f, 1.0f, 0.3f, 1.0f);
    restrictions->program.setUniformValue("color", color3);

    restrictions->nVertex = count;
    restrictions->ready = true;


    delete [] vertexposition;
}


void GraphicWindow::setupSolvedModel(Solid3D *updatedMesh)
{
    s3d_mesh = updatedMesh;

    if(!s3d_mesh->isMounted || !s3d_mesh->isSolved)
        return;

    if(solvedModel) delete solvedModel;

    //    setupView();
    //    setupGrid();
    setupLegend();

    // Vertex
    int nVertex = s3d_mesh->nElements*4*3; // number of tetrahedrons * number of faces * number of vertex per face
    GLfloat *vertexposition = new GLfloat[3*nVertex];
    GLfloat *vertexnormal = new GLfloat[3*nVertex];


    //#pragma omp parallel for num_threads(FEM_NUM_THREADS)
    for(int i=0; i<s3d_mesh->nElements; i++) // for elements
        for(int j=0; j<4; j++) // for faces
            for(int k=0; k<3; k++) // for vertex
            {
                vertexposition[36*i+9*j+3*k  ] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[0]+
                        amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index  )); // x

                vertexposition[36*i+9*j+3*k+1] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[1]+
                        amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index+1)); // y

                vertexposition[36*i+9*j+3*k+2] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[2]+
                        amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index+2)); // z

                vertexnormal[36*i+9*j+3*k  ] = s3d_mesh->elements[i]->normals[j].x();
                vertexnormal[36*i+9*j+3*k+1] = s3d_mesh->elements[i]->normals[j].y();
                vertexnormal[36*i+9*j+3*k+2] = s3d_mesh->elements[i]->normals[j].z();
            }


    solvedModel = new Drawing(vao,10);
    solvedModel->vao.bind();

    GLfloat **vertexcolor = new GLfloat*[10];

    for(int ids=0; ids<10; ids++)
        vertexcolor[ids] = new GLfloat[3*nVertex];



    for(int ids=0; ids<10; ids++)
    {
        double T0, T1, T2, T3, T4, Tn, R, G, B;

        T0 = s3d_mesh->Smin(ids);
        T4 = s3d_mesh->Smax(ids);

        T2 = (T0+T4)/2.;
        T1 = (T0+T2)/2.;
        T3 = (T2+T4)/2.;

        strLegend[ids].append(QString(strResults[ids]));
        strLegend[ids].append(QString("%1").arg(T0, 0,'E',3));
        strLegend[ids].append(QString("%1").arg(T1, 0,'E',3));
        strLegend[ids].append(QString("%1").arg(T2, 0,'E',3));
        strLegend[ids].append(QString("%1").arg(T3, 0,'E',3));
        strLegend[ids].append(QString("%1").arg(T4, 0,'E',3));

        for(int i=0; i<s3d_mesh->nElements; i++)
            for(int j=0; j<4; j++)
                for(int k=0; k<3; k++)
                {
                    Tn = s3d_mesh->Snodes(s3d_mesh->elements[i]->nodes[idf[j][k]]->index,ids);
                    R = Tn<T2?  0. : (Tn>T3? 1. : (Tn-T2)/(T3-T2));
                    B = Tn>T2?  0. : (Tn<T1? 1. : (T2-Tn)/(T2-T1));
                    G = Tn<T1? (Tn-T0)/(T1-T0) : Tn>T3 ? (T4-Tn)/(T4-T3) : 1.;

                    vertexcolor[ids][36*i+9*j+3*k  ] = R;
                    vertexcolor[ids][36*i+9*j+3*k+1] = G;
                    vertexcolor[ids][36*i+9*j+3*k+2] = B;
                }
        solvedModel->vboc_array[ids].bind();
        solvedModel->vboc_array[ids].allocate(vertexcolor[ids], 3*nVertex*sizeof(GL_FLOAT));
    }



    solvedModel->vbov.bind();
    solvedModel->vbov.allocate(vertexposition, 3*nVertex*sizeof(GL_FLOAT));

    solvedModel->vbon.bind();
    solvedModel->vbon.allocate(vertexnormal, 3*nVertex*sizeof(GL_FLOAT));

    solvedModel->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solid3dsolvedmodel.vs");
    solvedModel->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solid3dsolvedmodel.fs");
    solvedModel->program.link();
    solvedModel->program.bind();

    solvedModel->vbov.bind();
    int vertexLocation1 = solvedModel->program.attributeLocation("position");
    solvedModel->program.enableAttributeArray(vertexLocation1);
    solvedModel->program.setAttributeBuffer(vertexLocation1, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    solvedModel->vbon.bind();
    int vertexLocation2 = solvedModel->program.attributeLocation("normal");
    solvedModel->program.enableAttributeArray(vertexLocation2);
    solvedModel->program.setAttributeBuffer(vertexLocation2, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));


    // material
    solvedModel->program.setUniformValue("material.ambient", QVector3D(1.0f, 1.0f, 1.0f));
    solvedModel->program.setUniformValue("material.diffuse", QVector3D(1.0f, 1.0f, 1.0f));
    solvedModel->program.setUniformValue("material.specular", QVector3D(1.0f, 1.0f, 1.0f));
    solvedModel->program.setUniformValue("material.shininess", float(32.f));

    // light
    solvedModel->program.setUniformValue("light.position", QVector3D(5.0, 5.0, 5.0));
    solvedModel->program.setUniformValue("light.ambient", QVector3D(1.0, 1.0, 1.0));
    solvedModel->program.setUniformValue("light.diffuse", QVector3D(1.0, 1.0, 1.0));
    solvedModel->program.setUniformValue("light.specular", QVector3D(1.0, 1.0, 1.0));

    solvedModel->program.setUniformValue("isSolidColor", bool(false));
    solvedModel->program.setUniformValue("isLightingOn", bool(isLightingOn));


    solvedModel->nVertex = nVertex;
    solvedModel->ready = true;

    for(int ids=0; ids<10; ids++)
        delete [] vertexcolor[ids];

    delete [] vertexcolor;
    delete [] vertexnormal;
    delete [] vertexposition;

    selectionModel->ready = false;

}

///
/// \brief GraphicWindow::setupView
///
void GraphicWindow::setupView(void)
{

    double xsup, ysup, zsup, xinf, yinf, zinf;

    if(model == Model::truss3d)
    {
        xsup = xinf = t3d_mesh->nodes[0]->coordinates[0];
        ysup = yinf = t3d_mesh->nodes[0]->coordinates[1];
        zsup = zinf = t3d_mesh->nodes[0]->coordinates[2];

        for(int i=0; i<t3d_mesh->nNodes; i++)
        {
            if(xsup < t3d_mesh->nodes[i]->coordinates[0]) xsup = t3d_mesh->nodes[i]->coordinates[0];
            if(ysup < t3d_mesh->nodes[i]->coordinates[1]) ysup = t3d_mesh->nodes[i]->coordinates[1];
            if(zsup < t3d_mesh->nodes[i]->coordinates[2]) zsup = t3d_mesh->nodes[i]->coordinates[2];
            if(xinf > t3d_mesh->nodes[i]->coordinates[0]) xinf = t3d_mesh->nodes[i]->coordinates[0];
            if(yinf > t3d_mesh->nodes[i]->coordinates[1]) yinf = t3d_mesh->nodes[i]->coordinates[1];
            if(zinf > t3d_mesh->nodes[i]->coordinates[2]) zinf = t3d_mesh->nodes[i]->coordinates[2];
        }
    }
    else if(model == Model::solid3d)
    {
        xsup = xinf = s3d_mesh->nodes[0]->coordinates[0];
        ysup = yinf = s3d_mesh->nodes[0]->coordinates[1];
        zsup = zinf = s3d_mesh->nodes[0]->coordinates[2];

        for(int i=0; i<s3d_mesh->nNodes; i++)
        {
            if(xsup < s3d_mesh->nodes[i]->coordinates[0]) xsup = s3d_mesh->nodes[i]->coordinates[0];
            if(ysup < s3d_mesh->nodes[i]->coordinates[1]) ysup = s3d_mesh->nodes[i]->coordinates[1];
            if(zsup < s3d_mesh->nodes[i]->coordinates[2]) zsup = s3d_mesh->nodes[i]->coordinates[2];
            if(xinf > s3d_mesh->nodes[i]->coordinates[0]) xinf = s3d_mesh->nodes[i]->coordinates[0];
            if(yinf > s3d_mesh->nodes[i]->coordinates[1]) yinf = s3d_mesh->nodes[i]->coordinates[1];
            if(zinf > s3d_mesh->nodes[i]->coordinates[2]) zinf = s3d_mesh->nodes[i]->coordinates[2];
        }
    }


    view->world.center = 0.5f*QVector3D(xinf+xsup, yinf+ysup, zinf+zsup);
    view->world.radius = 0.5f*QVector3D(xinf-xsup, yinf-ysup, zinf-zsup).length();

    view->sceneLimitsInf = QVector3D(xinf, yinf, zinf);
    view->sceneLimitsSup = QVector3D(xsup, ysup, zsup);

    view->center = view->world.center;
    view->panning = QVector3D();
    view->rotation = QQuaternion();
    view->zoom = 1.0f;

    view->position = view->center + 2*QVector3D(0,-view->world.radius,view->world.radius);
    view->up = QVector3D(0,1,0);

}


void GraphicWindow::setupSolvedModel(Truss3D *updatedMesh)
{
    t3d_mesh = updatedMesh;

    if(!t3d_mesh->isMounted || !t3d_mesh->isSolved)
        return;

    if(solvedModel) delete solvedModel;

    //    setupView();
    //    setupGrid();
    setupLegend();

    GLfloat *vertexposition = new GLfloat[6*t3d_mesh->nElements];

    for(int i=0; i<t3d_mesh->nElements; i++)
        for(int j=0; j<3; j++)
        {
            vertexposition [6*i+j] = static_cast<GLfloat>(t3d_mesh->elements[i]->node1->coordinates[j] + amplification*t3d_mesh->u(3*t3d_mesh->elements[i]->node1->index+j));
            vertexposition [6*i+j+3] = static_cast<GLfloat>(t3d_mesh->elements[i]->node2->coordinates[j] + amplification*t3d_mesh->u(3*t3d_mesh->elements[i]->node2->index+j));
        }

    GLfloat *vertexcolor = new GLfloat[6*t3d_mesh->nElements];

    double T0, T1, T2, T3, T4, Tn, R, G, B;
    t3d_mesh->stresslimits(T0, T4);



    T0 *= 0.15;
    T4 *= 0.15;

    T2 = (T0+T4)/2.;
    T1 = (T0+T2)/2.;
    T3 = (T2+T4)/2.;


    strLegend[0].append(QString("Normal Stress"));
    strLegend[0].append(QString("%1").arg(T0, 0,'E',3));
    strLegend[0].append(QString("%1").arg(T1, 0,'E',3));
    strLegend[0].append(QString("%1").arg(T2, 0,'E',3));
    strLegend[0].append(QString("%1").arg(T3, 0,'E',3));
    strLegend[0].append(QString("%1").arg(T4, 0,'E',3));


    for(int i=0; i<t3d_mesh->nElements; i++)
    {
        Tn = t3d_mesh->stress(i);
        R = Tn<T2?  0. : (Tn>T3? 1. : (Tn-T2)/(T3-T2));
        B = Tn>T2?  0. : (Tn<T1? 1. : (T2-Tn)/(T2-T1));
        G = Tn<T1? (Tn-T0)/(T1-T0) : Tn>T3 ? (T4-Tn)/(T4-T3) : 1.;

        vertexcolor [6*i] = vertexcolor [6*i+3] = R;
        vertexcolor [6*i+1] = vertexcolor [6*i+4] = G;
        vertexcolor [6*i+2] = vertexcolor [6*i+5] = B;
    }


    solvedModel = new Drawing(vao);

    solvedModel->vao.bind();
    solvedModel->vbov.bind();
    solvedModel->vbov.allocate(vertexposition, 6*t3d_mesh->nElements*sizeof(GL_FLOAT));

    solvedModel->vboc.bind();
    solvedModel->vboc.allocate(vertexcolor, 6*t3d_mesh->nElements*sizeof(GL_FLOAT));


    solvedModel->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solvedmodel.vs");
    solvedModel->program.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/solvedmodel.gs");
    solvedModel->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solvedmodel.fs");
    solvedModel->program.link();
    solvedModel->program.bind();

    solvedModel->vbov.bind();
    int vertexLocation = solvedModel->program.attributeLocation("position");
    solvedModel->program.enableAttributeArray(vertexLocation);
    solvedModel->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    solvedModel->vboc.bind();
    int vertexColorLocation = solvedModel->program.attributeLocation("color");
    solvedModel->program.enableAttributeArray(vertexColorLocation);
    solvedModel->program.setAttributeBuffer(vertexColorLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    solvedModel->ready = true;
    solvedModel->nVertex = 2*t3d_mesh->nElements;

    delete [] vertexposition;
    delete [] vertexcolor;

    selectionModel->ready = false;
}


void GraphicWindow::setupSimulationModel(Truss3D *updatedMesh)
{
    t3d_mesh = updatedMesh;

    if(!t3d_mesh->isMounted || !t3d_mesh->isSolved || !t3d_mesh->isSolved_simulation)
        return;

    if(simulationModel) delete simulationModel;

    //setupView();
    //setupGrid();

    int stride = 6*t3d_mesh->nElements;
    GLfloat *vertexposition = new GLfloat[stride*t3d_mesh->nSteps];

    for(int k=0; k<t3d_mesh->nSteps; k++)
        for(int i=0; i<t3d_mesh->nElements; i++)
            for(int j=0; j<3; j++)
            {
                vertexposition [k*stride+6*i+j] = static_cast<GLfloat>(t3d_mesh->elements[i]->node1->coordinates[j] + amplification*t3d_mesh->u_simulation(3*t3d_mesh->elements[i]->node1->index+j, k));
                vertexposition [k*stride+6*i+j+3] = static_cast<GLfloat>(t3d_mesh->elements[i]->node2->coordinates[j] + amplification*t3d_mesh->u_simulation(3*t3d_mesh->elements[i]->node2->index+j, k));
            }

    GLfloat *vertexcolor = new GLfloat[stride*t3d_mesh->nSteps];

    double T0, T1, T2, T3, T4, Tn, R, G, B;
    t3d_mesh->stresslimits_simulation(T0, T4);

    T0 *= 0.15;
    T4 *= 0.15;

    T2 = (T0+T4)/2.;
    T1 = (T0+T2)/2.;
    T3 = (T2+T4)/2.;

    for(int k=0; k<t3d_mesh->nSteps; k++)
        for(int i=0; i<t3d_mesh->nElements; i++)
        {
            Tn = t3d_mesh->stress_simulation(i,k);
            R = Tn<T2?  0. : (Tn>T3? 1. : (Tn-T2)/(T3-T2));
            B = Tn>T2?  0. : (Tn<T1? 1. : (T2-Tn)/(T2-T1));
            G = Tn<T1? (Tn-T0)/(T1-T0) : Tn>T3 ? (T4-Tn)/(T4-T3) : 1.;

            vertexcolor [k*stride+6*i] = vertexcolor [k*stride+6*i+3] = R;
            vertexcolor [k*stride+6*i+1] = vertexcolor [k*stride+6*i+4] = G;
            vertexcolor [k*stride+6*i+2] = vertexcolor [k*stride+6*i+5] = B;
        }

    simulationModel = new Drawing(vao);

    simulationModel->vao.bind();
    simulationModel->vbov.bind();
    simulationModel->vbov.allocate(vertexposition, stride*t3d_mesh->nSteps*sizeof(GL_FLOAT));

    simulationModel->vboc.bind();
    simulationModel->vboc.allocate(vertexcolor, stride*t3d_mesh->nSteps*sizeof(GL_FLOAT));


    simulationModel->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solvedmodel.vs");
    simulationModel->program.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/solvedmodel.gs");
    simulationModel->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solvedmodel.fs");
    simulationModel->program.link();
    simulationModel->program.bind();

    simulationModel->vbov.bind();
    int vertexLocation = simulationModel->program.attributeLocation("position");
    simulationModel->program.enableAttributeArray(vertexLocation);
    simulationModel->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    simulationModel->vboc.bind();
    int vertexColorLocation = simulationModel->program.attributeLocation("color");
    simulationModel->program.enableAttributeArray(vertexColorLocation);
    simulationModel->program.setAttributeBuffer(vertexColorLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    simulationModel->ready = true;
    isSimulationModelActived = false;
    simulationModel->nVertex = 2*t3d_mesh->nElements;

    delete [] vertexposition;
    delete [] vertexcolor;
}



void GraphicWindow::setupSimulationModel(Solid3D *updatedMesh)
{
    s3d_mesh = updatedMesh;
    model = Model::solid3d;

    if(!s3d_mesh->isMounted || !s3d_mesh->isSolved || !s3d_mesh->isSolved_simulation)
        return;

    if(s3d_mesh->nSteps!=nFrames)
    {
        std::cerr<<"error";
        return;
    }

    if(simulationModel) delete simulationModel;

    //    setupView();
    //    setupGrid();

    // Vertex
    GLfloat *vertexposition = new GLfloat[3*s3d_mesh->nNodes];

    // Colors
    GLfloat *vertexcolor = new GLfloat[3*s3d_mesh->nNodes];

    double T0, T1, T2, T3, T4, Tn, R, G, B;



    simulationModel = new Drawing(vao, s3d_mesh->nSteps, 10*s3d_mesh->nSteps);
    simulationModel->vao.bind();


    for(int k=0; k<s3d_mesh->nSteps; k++)
    {
        for(int i=0; i<s3d_mesh->nNodes; i++)
        {
            vertexposition[3*i  ] = s3d_mesh->nodes[i]->coordinates[0] + amplification*s3d_mesh->u_simulation(3*s3d_mesh->nodes[i]->index, k); // x
            vertexposition[3*i+1] = s3d_mesh->nodes[i]->coordinates[1] + amplification*s3d_mesh->u_simulation(3*s3d_mesh->nodes[i]->index+1, k); // y
            vertexposition[3*i+2] = s3d_mesh->nodes[i]->coordinates[2] + amplification*s3d_mesh->u_simulation(3*s3d_mesh->nodes[i]->index+2, k); // z
        }

        simulationModel->vbov_array[k].bind();
        simulationModel->vbov_array[k].allocate(vertexposition, 3*s3d_mesh->nNodes*sizeof(GL_FLOAT));


        for(int ids=0; ids<10; ids++)
        {
            T0 = s3d_mesh->Smin(ids);
            T4 = s3d_mesh->Smax(ids);

            //        T0 *= 0.15;
            //        T4 *= 0.15;
            //setupLegend(T0, T4);

            T2 = (T0+T4)/2.;
            T1 = (T0+T2)/2.;
            T3 = (T2+T4)/2.;

            for(int i=0; i<s3d_mesh->nNodes; i++)
            {
                Tn = s3d_mesh->Snodes_simulation[k](i,ids);
                R = Tn<T2?  0. : (Tn>T3? 1. : (Tn-T2)/(T3-T2));
                B = Tn>T2?  0. : (Tn<T1? 1. : (T2-Tn)/(T2-T1));
                G = Tn<T1? (Tn-T0)/(T1-T0) : Tn>T3 ? (T4-Tn)/(T4-T3) : 1.;

                vertexcolor [3*i]  = R;
                vertexcolor [3*i+1] = G;
                vertexcolor [3*i+2] = B;
            }

            simulationModel->vboc_array[10*k+ids].bind();
            simulationModel->vboc_array[10*k+ids].allocate(vertexcolor, 3*s3d_mesh->nNodes*sizeof(GL_FLOAT));
        }

    }


    // Index
    GLuint *vertexindex = new GLuint[12*s3d_mesh->nElements];

    for(int i=0; i<s3d_mesh->nElements; i++)
        for(int j=0; j<4; j++)
            for(int k=0; k<3; k++)
                vertexindex[12*i+3*j+k] = s3d_mesh->elements[i]->nodes[idf[j][k]]->index;

    simulationModel->vboi.bind();
    simulationModel->vboi.allocate(vertexindex, 12*s3d_mesh->nElements*sizeof(GLuint));


    simulationModel->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solid3dsimulationmodel.vs");
    simulationModel->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solid3dsimulationmodel.fs");
    simulationModel->program.link();
    simulationModel->program.bind();



    simulationModel->nIndex = 12*s3d_mesh->nElements;
    simulationModel->nVertex = s3d_mesh->nNodes;

    simulationModel->ready = true;
    isSimulationModelActived = false;

    delete [] vertexindex;
    delete [] vertexcolor;
    delete [] vertexposition;

    updateGL();
}



void GraphicWindow::setupSelectionModel(void)
{
    if(model == Model::truss3d)
    {
        if(!t3d_mesh->isMounted)
            return;

        if(selectionModel) delete selectionModel;

        //    setupView();
        //    setupGrid();

        GLfloat *vertexposition = new GLfloat[6*t3d_mesh->nElements];

        if(t3d_mesh->isSolved)
        {
            for(int i=0; i<t3d_mesh->nElements; i++)
                for(int j=0; j<3; j++)
                {
                    vertexposition [6*i+j] = static_cast<GLfloat>(t3d_mesh->elements[i]->node1->coordinates[j] + amplification*t3d_mesh->u(3*t3d_mesh->elements[i]->node1->index+j));
                    vertexposition [6*i+j+3] = static_cast<GLfloat>(t3d_mesh->elements[i]->node2->coordinates[j] + amplification*t3d_mesh->u(3*t3d_mesh->elements[i]->node2->index+j));
                }
        }
        else
        {
            for(int i=0; i<t3d_mesh->nElements; i++)
                for(int j=0; j<3; j++)
                {
                    vertexposition [6*i+j] = static_cast<GLfloat>(t3d_mesh->elements[i]->node1->coordinates[j]);
                    vertexposition [6*i+j+3] = static_cast<GLfloat>(t3d_mesh->elements[i]->node2->coordinates[j]);
                }
        }

        GLfloat *vertexcolor = new GLfloat[6*t3d_mesh->nElements];


        for(int i=0; i<t3d_mesh->nElements; i++){

            vertexcolor[6*i] = vertexcolor[6*i+3] = ((i & 0x000000FF) >> 0)/255.f;
            vertexcolor[6*i+1] = vertexcolor[6*i+4] = ((i & 0x0000FF00) >> 8)/255.f;
            vertexcolor[6*i+2] = vertexcolor[6*i+5] = ((i & 0x00FF0000) >> 16)/255.f;
        }

        selectionModel = new Drawing(vao);

        selectionModel->vao.bind();
        selectionModel->vbov.bind();
        selectionModel->vbov.allocate(vertexposition, 6*t3d_mesh->nElements*sizeof(GL_FLOAT));

        selectionModel->vboc.bind();
        selectionModel->vboc.allocate(vertexcolor, 6*t3d_mesh->nElements*sizeof(GL_FLOAT));


        selectionModel->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solvedmodel.vs");
        selectionModel->program.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/solvedmodel.gs");
        selectionModel->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solvedmodel.fs");
        selectionModel->program.link();
        selectionModel->program.bind();

        selectionModel->vbov.bind();
        int vertexLocation = selectionModel->program.attributeLocation("position");
        selectionModel->program.enableAttributeArray(vertexLocation);
        selectionModel->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

        selectionModel->vboc.bind();
        int vertexColorLocation = selectionModel->program.attributeLocation("color");
        selectionModel->program.enableAttributeArray(vertexColorLocation);
        selectionModel->program.setAttributeBuffer(vertexColorLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

        selectionModel->ready = true;
        selectionModel->nVertex = 2*t3d_mesh->nElements;

        delete [] vertexposition;
        delete [] vertexcolor;
    }

    else if(model == Model::solid3d)
    {
        if(!s3d_mesh->isMounted)
            return;

        if(selectionModel) delete selectionModel;

        // Vertex
        int nVertex = s3d_mesh->nElements*4*3; // number of tetrahedrons * number of faces * number of vertex per face
        GLfloat *vertexposition = new GLfloat[3*nVertex];



        if(s3d_mesh->isSolved)
        {
            for(int i=0; i<s3d_mesh->nElements; i++) // for elements
                for(int j=0; j<4; j++) // for faces
                    for(int k=0; k<3; k++) // for vertex
                    {
                        vertexposition[36*i+9*j+3*k  ] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[0]+
                                amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index  )); // x

                        vertexposition[36*i+9*j+3*k+1] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[1]+
                                amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index+1)); // y

                        vertexposition[36*i+9*j+3*k+2] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[2]+
                                amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index+2)); // z
                    }
        }
        else
        {
            for(int i=0; i<s3d_mesh->nElements; i++) // for elements
                for(int j=0; j<4; j++) // for faces
                    for(int k=0; k<3; k++) // for vertex
                    {
                        vertexposition[36*i+9*j+3*k  ] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[0]);//+
                        //amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index  )); // x

                        vertexposition[36*i+9*j+3*k+1] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[1]);//+
                        //amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index+1)); // y

                        vertexposition[36*i+9*j+3*k+2] = static_cast<GLfloat>(s3d_mesh->elements[i]->nodes[idf[j][k]]->coordinates[2]);//+
                        //amplification*s3d_mesh->u(3*s3d_mesh->elements[i]->nodes[idf[j][k]]->index+2)); // z
                    }
        }



        GLfloat *vertexcolor = new GLfloat[3*nVertex];


        for(int i=0; i<s3d_mesh->nElements; i++)
            for(int j=0; j<4; j++)
                for(int k=0; k<3; k++)
                {

                    vertexcolor[36*i+9*j+3*k  ] = ((i & 0x000000FF) >> 0)/255.f;
                    vertexcolor[36*i+9*j+3*k+1] = ((i & 0x0000FF00) >> 8)/255.f;
                    vertexcolor[36*i+9*j+3*k+2] = ((i & 0x00FF0000) >> 16)/255.f;

                }



        selectionModel = new Drawing(vao);

        selectionModel->vao.bind();
        selectionModel->vbov.bind();
        selectionModel->vbov.allocate(vertexposition, 3*nVertex*sizeof(GL_FLOAT));

        selectionModel->vboc.bind();
        selectionModel->vboc.allocate(vertexcolor, 3*nVertex*sizeof(GL_FLOAT));


        selectionModel->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/solid3dselection.vs");
        selectionModel->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/solid3dselection.fs");
        selectionModel->program.link();
        selectionModel->program.bind();

        selectionModel->vbov.bind();
        int vertexLocation = selectionModel->program.attributeLocation("position");
        selectionModel->program.enableAttributeArray(vertexLocation);
        selectionModel->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

        selectionModel->vboc.bind();
        int colorLocation = selectionModel->program.attributeLocation("color");
        selectionModel->program.enableAttributeArray(colorLocation);
        selectionModel->program.setAttributeBuffer(colorLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

        selectionModel->nVertex = nVertex;
        selectionModel->ready = true;


        delete [] vertexcolor;
        delete [] vertexposition;
    }

}


void GraphicWindow::select(void)
{

    if(!selectionModel->ready)
        setupSelectionModel();

    isSelectionActived = true;
    updateGL();

}


void GraphicWindow::showGrid(void)
{
    isGridActived = !isGridActived;
    if(!grid->ready)
        setupGrid();
    updateGL();
}

void GraphicWindow::showMesh(void)
{
    isShowMesh = !isShowMesh;

    updateGL();
}


void GraphicWindow::showLegend(void)
{
    isLegendActived = !isLegendActived;
    updateGL();
}

void GraphicWindow::turnOffLighting(void)
{
    isLightingOn = !isLightingOn;
}

void GraphicWindow::showInitialModel(void)
{
    isInitialModelActived = !isInitialModelActived;
    updateGL();
}

void GraphicWindow::showSolvedModel(void)
{
    isSolvedModelActived = !isSolvedModelActived;
    updateGL();
}

void GraphicWindow::showSimulationModel(void)
{
    isSimulationModelActived =!isSimulationModelActived;
    updateGL();
}

void GraphicWindow::startSimulation(void)
{
    if(!isSimulationModelActived) step = 0;
    isSimulationModelActived = true;
    isSolvedModelActived = false;
    if(!simulationModel->ready && model==truss3d)
        setupSimulationModel(t3d_mesh);
    else if(!simulationModel->ready && model==solid3d)
        setupSimulationModel(s3d_mesh);
    connect(timer, SIGNAL(timeout()), this, SLOT(drawNextStep()));
    timer->start(100);
}

void GraphicWindow::drawNextStep(void)
{
    step++;

//    if(step>=nFrames) stopSimulation();

//    QString filename = QString("./pictures/frame_") + QString("%1").arg(step)
//            + QString(".png");

//    updateGL();


//    this->grabFrameBuffer(true).save(filename, "PNG", 50);

    if(step>=nFrames) step=0;
    updateGL();
}

void GraphicWindow::stopSimulation(void)
{
    isSimulationModelActived = false;
    isSolvedModelActived = true;
    disconnect(timer, SIGNAL(timeout()), this, SLOT(drawNextStep()));
    timer->stop();
    step = 0;
    updateGL();
}

void GraphicWindow::pauseSimulation(void)
{

    disconnect(timer, SIGNAL(timeout()), this, SLOT(drawNextStep()));
    timer->stop();
}

void GraphicWindow::setupLegend(void)
{

    //    if(!t3d_mesh->isMounted || !t3d_mesh->isSolved || !t3d_mesh->isSolved_simulation)
    //        return;

    if(legend) delete legend;

    //    if(strLegend) delete strLegend;
    //    if(model==truss3d)
    //        strLegend = new QStringList[1];
    //    else
    //        strLegend = new QStringList[10];


    int npoints = 1000;
    float w = 0.05;
    float h = 1.5;
    float dh = h/npoints;
    GLfloat *vertexposition = new GLfloat[2*npoints];

    for(int i=0; i<npoints; i+=2)
    {
        vertexposition [2*i] = 0.f;
        vertexposition [2*i+1] = i*dh;
        vertexposition [2*i+2] = w;
        vertexposition [2*i+3] = i*dh;
    }

    GLfloat *vertexcolor = new GLfloat[3*npoints];

    float T0, T1, T2, T3, T4, Tn, R, G, B;

    T0 = 0.f;
    T4 = 1.f;

    T2 = 0.5f;
    T1 = 0.25f;
    T3 = 0.75f;

    for(int i=0; i<npoints; i+=2)
    {
        Tn = i*dh/h;
        R = Tn<T2?  0. : (Tn>T3? 1. : (Tn-T2)/(T3-T2));
        B = Tn>T2?  0. : (Tn<T1? 1. : (T2-Tn)/(T2-T1));
        G = Tn<T1? (Tn-T0)/(T1-T0) : Tn>T3 ? (T4-Tn)/(T4-T3) : 1.;

        vertexcolor[3*i] = vertexcolor[3*i+3] = R;
        vertexcolor[3*i+1] = vertexcolor[3*i+4] = G;
        vertexcolor[3*i+2] = vertexcolor[3*i+5] = B;
    }


    legend = new Drawing(vao);

    legend->vao.bind();
    legend->vbov.bind();
    legend->vbov.allocate(vertexposition, 2*npoints*sizeof(GL_FLOAT));

    legend->vboc.bind();
    legend->vboc.allocate(vertexcolor, 3*npoints*sizeof(GL_FLOAT));


    legend->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/legend.vs");
    legend->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/legend.fs");
    legend->program.link();
    legend->program.bind();

    legend->vbov.bind();
    int vertexLocation = legend->program.attributeLocation("position");
    legend->program.enableAttributeArray(vertexLocation);
    legend->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 2, 2*sizeof(GLfloat));

    legend->vboc.bind();
    int vertexColorLocation = legend->program.attributeLocation("color");
    legend->program.enableAttributeArray(vertexColorLocation);
    legend->program.setAttributeBuffer(vertexColorLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    legend->ready = true;
    legend->nVertex = npoints;

    delete [] vertexposition;
    delete [] vertexcolor;
}

void GraphicWindow::drawLegend(int n_result)
{

    legend->bind();
    QMatrix4x4 mvp_legend;
    mvp_legend.ortho(-1.*view->aspect(), 1.*view->aspect(), -1, 1, -10, 10);
    mvp_legend.translate(0.7*view->aspect(), -1.5/2., 0.0);
    legend->program.setUniformValue("mvp_matrix", mvp_legend);
    glPolygonMode(GL_FRONT, GL_FILL);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, legend->nVertex);


    textRender->bind();
    textRender->program.setUniformValue("projection",  mvp_legend);

    for(int i=0; i<4;i++)
        renderTextLine(strLegend[n_result][i+1], 0.06, i*1.5/4., 0.0005, QVector3D(1,1,1));
    renderTextLine(strLegend[n_result][5], 0.06, 1.45, 0.0005, QVector3D(1,1,1));

    renderTextLine(strLegend[n_result][0], -0.02, 1.52, 0.0005, QVector3D(1,1,1));

}

///
/// \brief GraphicWindow::setupTextRender
///
void GraphicWindow::setupTextRender(void)
{

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/freefont/FreeSans.ttf", 0, &face))
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 96);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    face->glyph->bitmap.buffer
                    );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        //        QOpenGLTexture *TEXTE = new QOpenGLTexture(QOpenGLTexture::Target2D);
        //        TEXTE->setSize(face->glyph->bitmap.width, face->glyph->bitmap.rows);

        //        TEXTE->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);
        //        TEXTE->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);
        //        TEXTE->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);

        //        TEXTE->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, face->glyph->bitmap.buffer);
        //        TEXTE->allocateStorage();

        // Now store character for later use
        Character character = {
            texture,
            //            TEXTE,
            QVector2D(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            QVector2D(face->glyph->bitmap_left, face->glyph->bitmap_top),
            GLuint(face->glyph->advance.x)
        };

        Characters.insert(std::pair<GLchar, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);


    textRender = new Drawing(vao);

    textRender->program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/text.vs");
    textRender->program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/text.fs");
    textRender->program.link();
    textRender->program.bind();

    textRender->vao.bind();
    textRender->vbov.bind();
    textRender->vbov.allocate(NULL, sizeof(GLfloat) * 6 * 4);

    textRender->vbov.bind();
    int vertexLocation = textRender->program.attributeLocation("vertex");
    textRender->program.enableAttributeArray(vertexLocation);
    textRender->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 4, 4 * sizeof(GLfloat));

}

///
/// \brief GraphicWindow::renderTextLine
/// \param qttext
/// \param x
/// \param y
/// \param scale
/// \param color
///
void GraphicWindow::renderTextLine(QString qttext, GLfloat x, GLfloat y, GLfloat scale, QVector3D color)
{
    textRender->bind();
    textRender->program.setUniformValue("textColor", color);

    glActiveTexture(GL_TEXTURE0);
    textRender->bind();

    std::string text = qttext.toStdString();

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x() * scale;
        GLfloat ypos = y - (ch.Size.y() - ch.Bearing.y()) * scale;

        GLfloat w = ch.Size.x() * scale;
        GLfloat h = ch.Size.y() * scale;
        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };
        // Render glyph texture over quad
        //glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory

        textRender->vbov.bind();
        textRender->vbov.allocate(vertices, sizeof(vertices));


        int vertexLocation = textRender->program.attributeLocation("vertex");
        textRender->program.enableAttributeArray(vertexLocation);
        textRender->program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 4, 4 * sizeof(GLfloat));

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        int text = textRender->program.uniformLocation( "text" );
        textRender->program.setUniformValue( text, 0 );

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

}

///
/// \brief GraphicWindow::drawGrid
/// \param mvp
///
void GraphicWindow::drawGrid(QMatrix4x4 mvp)
{
    glLineWidth(1.f);

    grid->bind();
    grid->program.setUniformValue("mvp_matrix", mvp);
    glDrawArrays(GL_LINES, 0, grid->nVertex);

    glLineWidth(5.f);
    axis->bind();
    axis->program.setUniformValue("mvp_matrix", mvp);
    axis->program.setUniformValue("color", QVector4D(1.0f, 0.0f, 0.0f, 0.9f));
    glDrawArrays(GL_LINES, 0, 2);
    axis->program.setUniformValue("color", QVector4D(0.0f, 1.0f, 0.0f, 0.9f));
    glDrawArrays(GL_LINES, 2, 2);
    axis->program.setUniformValue("color", QVector4D(0.0f, 0.0f, 1.0f, 0.9f));
    glDrawArrays(GL_LINES, 4, 2);

    QMatrix4x4 mvp_legend;
    textRender->bind();

    mvp_legend = mvp;
    mvp_legend.translate(-grid_sx+view->world.center.x()+1.6*grid_delta, -grid_sy+view->world.center.y(), view->sceneLimitsInf.z());
    textRender->program.setUniformValue("projection", mvp_legend);
    renderTextLine(QString("X"), 0.0, 0.0, 0.005*grid_delta, QVector3D(1,0,0));

    mvp_legend = mvp;
    mvp_legend.translate(-grid_sx+view->world.center.x(), -grid_sy+view->world.center.y()+1.6*grid_delta, view->sceneLimitsInf.z());
    textRender->program.setUniformValue("projection", mvp_legend);
    renderTextLine(QString("Y"), 0.0, 0.0, 0.005*grid_delta, QVector3D(0,1,0));

    mvp_legend = mvp;
    mvp_legend.translate(-grid_sx+view->world.center.x(), -grid_sy+view->world.center.y(), 1.6*grid_delta+view->sceneLimitsInf.z());
    textRender->program.setUniformValue("projection", mvp_legend);
    renderTextLine(QString("Z"), 0.0, 0.0, 0.005*grid_delta, QVector3D(0,0,1));

}

void GraphicWindow::restart(void)
{

    isSelectionActived = false;
    isGridActived = false;
    isInitialModelActived = false;
    isSolvedModelActived = false;
    isSimulationModelActived = false;
    isLegendActived = false;

    if(grid) delete grid;
    grid = new Drawing(vao);

    if(axis) delete axis;
    axis = new Drawing(vao);

    if(legend) delete legend;
    legend = new Drawing(vao);

    if(initialModel) delete initialModel;
    initialModel = new Drawing(vao);

    if(solvedModel) delete solvedModel;
    solvedModel = new Drawing(vao);

    if(simulationModel) delete simulationModel;
    simulationModel = new Drawing(vao);

    if(selectionModel) delete selectionModel;
    selectionModel = new Drawing(vao);

    //    if(textRender) delete textRender;
    //    textRender = new Drawing(vao);

    if(loading) delete loading;
    loading= new Drawing(vao);

    if(restrictions) delete restrictions;
    restrictions = new Drawing(vao);

}

void GraphicWindow::drawSelectedElement(void)
{

    if(!solvedModel->ready)
        return;

    if(model==truss3d)
    {


        if(solvedModel->ready)
            solvedModel->bind();
        else
            initialModel->bind();

        glLineWidth(10.f);
        glDrawArrays(GL_LINES, 2*id_selectedElement, 2);
        glLineWidth(1.f);


        QMatrix4x4 mvp_text;
        mvp_text.ortho(-1.*view->aspect(), 1.*view->aspect(), -1, 1, -10, 10);
        mvp_text.translate(-1.*view->aspect(), -1, 0.0);


        textRender->bind();
        textRender->program.setUniformValue("projection",  mvp_text);


        QString str = QString("Element %1 - ").arg(id_selectedElement);

        Truss3DElement *e = t3d_mesh->elements[id_selectedElement];

        str +=  QString("nodes: %1 and %2 - normal stress: %3")
                .arg(e->node1->index).arg(e->node2->index).
                arg(t3d_mesh->stress(id_selectedElement));

        renderTextLine(str, 0.02, 0.02, 0.0005, QVector3D(1,1,1));
    }
    else
    {
        if(solvedModel->ready)
        {

            solvedModel->bind();
            glLineWidth(8.f);
            solvedModel->program.setUniformValue("isSolidColor", bool(true));
            glDrawArrays(GL_LINES, 12*id_selectedElement, 12);
            glLineWidth(1.f);
}
        else
        {
            initialModel->bind();
            glLineWidth(8.f);
            initialModel->program.setUniformValue("color", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
            //glDrawElements(GL_LINES, initialModel->nIndex, GL_UNSIGNED_INT, 0);
            glLineWidth(1.f);
        }




        QMatrix4x4 mvp_text;
        mvp_text.ortho(-1.*view->aspect(), 1.*view->aspect(), -1, 1, -10, 10);
        mvp_text.translate(-1.*view->aspect(), -1, 0.0);


        textRender->bind();
        textRender->program.setUniformValue("projection",  mvp_text);


        QString str = QString("Element %1 - ").arg(id_selectedElement);

        Solid3DElement *e = s3d_mesh->elements[id_selectedElement];

        str += QString("Nodes: %1, %2, %3 and %4")
                .arg(e->nodes[0]->index).arg(e->nodes[1]->index).arg(e->nodes[2]->index).arg(e->nodes[3]->index);

        renderTextLine(str, 0.02, 0.02, 0.0005, QVector3D(1,1,1));

    }

}




