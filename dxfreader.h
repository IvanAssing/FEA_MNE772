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

#ifndef DXFREADER_H
#define DXFREADER_H

#include <dl_dxf.h>
#include <dl_creationadapter.h>
#include <fstream>

class Point3DBuffer;
class Line3DBuffer;
class LayerBuffer;

// Class DXFReader
///
/// \brief The DXFReader class
///
class DXFReader : public DL_CreationAdapter
{
public:
    DL_Dxf *dxf;
    Point3DBuffer *pbuffer;
    Line3DBuffer *lbuffer;

    DXFReader();
    bool readfile(const char *file);
    bool writeFT3Dfile(const char *filename);
    virtual void addPoint(const DL_PointData& data);
    virtual void addLine(const DL_LineData& data);

    friend std::ostream& operator<< (std::ostream& stream, const DXFReader& reader);

    virtual ~DXFReader();
};


// Class Line3D Buffer
///
/// \brief The Line3DBuffer class
///
class Line3DBuffer
{
public:
    int *data;
    int count;
    int max;

    Line3DBuffer(int size);
    int addLine(int p1, int p2, int layer);

    friend std::ostream& operator<< (std::ostream& stream, const Line3DBuffer& buffer);

    LayerBuffer *layerBuffer;

    virtual ~Line3DBuffer();
};


// Class Point3D Buffer
///
/// \brief The Point3DBuffer class
///
class Point3DBuffer
{
public:
    double *data;
    int count;
    int max;

    Point3DBuffer(int size);
    int addPoint(double x, double y, double z, int layer = 0);

    friend std::ostream& operator<< (std::ostream& stream, const Point3DBuffer& buffer);

    LayerBuffer *layerBuffer;

    virtual ~Point3DBuffer();
};


// Class Layer
///
/// \brief The LayerBuffer class
///
class LayerBuffer
{
    public:
    int count;
    int *index;

    LayerBuffer(int size);

    int addLayer(int value);
    virtual ~LayerBuffer();
};


#endif // DXFREADER_H
