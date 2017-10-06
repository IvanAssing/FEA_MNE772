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

#ifndef DRAWING_H
#define DRAWING_H

#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>

///
/// \brief The Drawing class
///
class Drawing
{
public:
    bool ready;
    QOpenGLVertexArrayObject vao;

    QOpenGLShaderProgram program;
    QOpenGLBuffer vbov, vboi, vboc, vbon;
    int nVertex, nIndex;

    QOpenGLBuffer *vboc_array;
    QOpenGLBuffer *vbov_array;
    int nvboc;
    int nvbov;

    Drawing(QOpenGLVertexArrayObject *parent);

    Drawing(QOpenGLVertexArrayObject *parent, int nColorBuffers);
    Drawing(QOpenGLVertexArrayObject *parent, int nVertexBuffers, int nColorBuffers);

    void bind(int nvboColor);
    void bind(int nvboVertex, int nvboColor);

    void bind(void);
    virtual ~Drawing();
};

#endif // DRAWING_H
