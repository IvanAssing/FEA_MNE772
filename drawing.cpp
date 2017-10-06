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

#include "drawing.h"

Drawing::Drawing(QOpenGLVertexArrayObject *parent)
{
   Q_UNUSED(parent);

    ready = false;
    vao.create();
    vao.bind();
    vbov = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbov.create();
    vbov.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboc = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vboc.create();
    vboc.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboi = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    vboi.create();
    vboi.setUsagePattern(QOpenGLBuffer::StaticDraw);
    nVertex = 0;
    nIndex = 0;

    nvboc = 0;
    vboc_array = nullptr;

    nvbov = 0;
    vbov_array = nullptr;

}

Drawing::Drawing(QOpenGLVertexArrayObject *parent, int nColorBuffers)
{
   Q_UNUSED(parent);

    ready = false;
    vao.create();
    vao.bind();
    vbov = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbov.create();
    vbov.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboc = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vboc.create();
    vboc.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboi = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    vboi.create();
    vboi.setUsagePattern(QOpenGLBuffer::StaticDraw);
    nVertex = 0;
    nIndex = 0;

    nvboc = nColorBuffers;
    vboc_array = new QOpenGLBuffer[nvboc];
    for(int i=0; i<nvboc; i++)
    {
    vboc_array[i].create();
    vboc_array[i].setUsagePattern(QOpenGLBuffer::StaticDraw);
    }

    nvbov = 0;
    vbov_array = nullptr;

}



Drawing::Drawing(QOpenGLVertexArrayObject *parent, int nVertexBuffers, int nColorBuffers)
{
   Q_UNUSED(parent);

    ready = false;
    vao.create();
    vao.bind();
    vbov = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbov.create();
    vbov.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboc = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vboc.create();
    vboc.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboi = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    vboi.create();
    vboi.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbon = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbon.create();
    nVertex = 0;
    nIndex = 0;

    nvboc = nColorBuffers;
    vboc_array = new QOpenGLBuffer[nvboc];
    for(int i=0; i<nvboc; i++)
    {
    vboc_array[i].create();
    vboc_array[i].setUsagePattern(QOpenGLBuffer::StaticDraw);
    }

    nvbov = nVertexBuffers;
    vbov_array = new QOpenGLBuffer[nvbov];
    for(int i=0; i<nvbov; i++)
    {
    vbov_array[i].create();
    vbov_array[i].setUsagePattern(QOpenGLBuffer::StaticDraw);
    }

}

void Drawing::bind(void)
{
    vao.bind();
    program.bind();
    //vbon.bind();
    //vbov.bind();
}


void Drawing::bind(int nvboColor)
{
    vao.bind();
    program.bind();

    vboc_array[nvboColor].bind();
    int vertexColorLocation = program.attributeLocation("color");
    program.enableAttributeArray(vertexColorLocation);
    program.setAttributeBuffer(vertexColorLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    vbon.bind();
    vbov.bind();
}

void Drawing::bind(int nvboVertex, int nvboColor)
{
    vao.bind();
    program.bind();

    vbov_array[nvboVertex].bind();
    int vertexLocation = program.attributeLocation("position");
    program.enableAttributeArray(vertexLocation);
    program.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    vboc_array[nvboColor].bind();
    int vertexColorLocation = program.attributeLocation("color");
    program.enableAttributeArray(vertexColorLocation);
    program.setAttributeBuffer(vertexColorLocation, GL_FLOAT, 0, 3, 3*sizeof(GLfloat));

    vbon.bind();
    vbov.bind();
}



Drawing::~Drawing()
{
    if(vboc_array)
    {
        for(int i=0; i<nvboc; i++)
        {vboc_array[i].release();vboc_array[i].destroy();}
        delete [] vboc_array;
    }

    if(vbov_array)
    {
        for(int i=0; i<nvbov; i++)
        {vbov_array[i].release();vbov_array[i].destroy();}
        delete [] vbov_array;
    }

    vbov.release();
    vbov.destroy();
    vbon.release();
    vbon.destroy();
    vboi.release();
    vboi.destroy();
    vboc.release();
    vboc.destroy();
    if(program.isLinked()) program.release();
    vao.release();

}

