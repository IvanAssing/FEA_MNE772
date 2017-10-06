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

#include "dxfreader.h"
#include <iomanip>

#define ptol 1.e-5
#define ndof 4
#define buffersize 100000
#define bc_index 5

///
/// \brief DXFReader::DXFReader
///
DXFReader::DXFReader()
{
    pbuffer = new Point3DBuffer(buffersize);
    lbuffer = new Line3DBuffer(buffersize);

}

///
/// \brief DXFReader::readfile
/// \param file
/// \return
///
bool DXFReader::readfile(const char *file)
{
    dxf = new DL_Dxf();
    if (!dxf->in(file, this))
    {
        std::cerr << file << " could not be opened.\n";
        return true;
    }
    return false;
}


///
/// \brief DXFReader::writeFT3Dfile
/// \param file
/// \return
///
bool DXFReader::writeFT3Dfile(const char *filename)
{
    std::ofstream file(filename);

    // boundary conditions
    int *fixed_index_nre = new int[pbuffer->layerBuffer->count+1];
    int *fixed_index_nlo = new int[pbuffer->layerBuffer->count+1];

    //default value
    fixed_index_nre[0] = 0;
    fixed_index_nlo[0] = 0;

    int i_nre = 1;
    int i_nlo = 1;
    int count = 0;

    for(int i=1; i<pbuffer->layerBuffer->count; i++)
    {
        if(pbuffer->layerBuffer->index[i]<bc_index)
        {
            fixed_index_nre[i] = i_nre++;
            fixed_index_nlo[i] = 0;
        }
        else
        {
            fixed_index_nre[i] = 0;
            fixed_index_nlo[i] = i_nlo++;
        }
    }

    if(i_nre==0) i_nre=1;
    file<<i_nre<<std::endl;
    for(int i=0;i<i_nre;i++)
        file<<i<<" "<<1<<" "<<1<<" "<<1<<std::endl;

    // loading
    if(i_nlo==0) i_nlo=1;
    file<<i_nlo<<std::endl;
    for(int i=0;i<i_nlo;i++)
        file<<i<<" "<<0<<" "<<0<<" "<<0<<std::endl;

    // displacements
    file<<1<<std::endl;
    file<<0<<" "<<0<<" "<<0<<" "<<0<<std::endl;

    // nodes
    file<<pbuffer->count<<std::endl;
    for(int i=0;i<pbuffer->count;i++)
        file<<i<<" "<<pbuffer->data[ndof*i]<<" "<<pbuffer->data[ndof*i+1]<<" "<<pbuffer->data[ndof*i+2]<<" "<<
              fixed_index_nre[(int)pbuffer->data[ndof*i+3]]<<" "<<fixed_index_nlo[(int)pbuffer->data[ndof*i+3]]<<" 0"<<std::endl;

    delete [] fixed_index_nlo;
    delete [] fixed_index_nre;

    // materials
    count=lbuffer->layerBuffer->count;
    if(count==0) count=1;
    file<<count<<std::endl;
    for(int i=0;i<count;i++)
        file<<i<<" "<<"unamed"<<" "<<1<<" "<<1<<std::endl;

    // elements
    file<<lbuffer->count<<std::endl;
    for(int i=0;i<lbuffer->count;i++)
        file<<i<<" "<<lbuffer->data[3*i]<<" "<<lbuffer->data[3*i+1]<<" "<<lbuffer->data[3*i+2]<<std::endl;
    file.close();

    return true;
}

///
/// \brief operator <<
/// \param stream
/// \param reader
/// \return
///
std::ostream& operator<< (std::ostream& stream, const DXFReader& reader)
{
    stream<<"\n POINTS:\n";
    stream<<*(reader.pbuffer);
    stream<<"\n LINES:\n";
    stream<<*(reader.lbuffer);
    return stream;
}

///
/// \brief DXFReader::~DXFReader
///
DXFReader::~DXFReader()
{
    if(dxf) delete dxf;
    delete pbuffer;
    delete lbuffer;
}


///
/// \brief DXFReader::addPoint
/// \param data
///
void DXFReader::addPoint(const DL_PointData& data) {
    int layer = attributes.getColor();
    if(layer>0 && layer<256) // basic color AutoCAD
    {
        pbuffer->addPoint(data.x, data.y, data.z,layer);
    }
}

///
/// \brief DXFReader::addLine
/// \param data
///
void DXFReader::addLine(const DL_LineData& data)
{
    int layer = attributes.getColor();
    if(layer>0 && layer<256) // basic color AutoCAD
    {
        int p1 = pbuffer->addPoint(data.x1, data.y1, data.z1);
        int p2 = pbuffer->addPoint(data.x2, data.y2, data.z2);

        lbuffer->addLine(p1, p2, layer);
    }
}


///
/// \brief Line3DBuffer::Line3DBuffer
/// \param size
///
Line3DBuffer::Line3DBuffer(int size)
{
    count = 0;
    max = size;
    data = new int[ndof*size];
    layerBuffer = new LayerBuffer(size);
    //layerBuffer->addLayer(0);
}

///
/// \brief Line3DBuffer::addLine
/// \param p1
/// \param p2
/// \param layer
/// \return
///
int Line3DBuffer::addLine(int p1, int p2, int layer)
{
    for(int i=0; i<count; i++)
        if(p1 == data[3*i]) // p1
            if(p2 == data[3*i+1]) // p2
                return i;

    // add line
    data[3*count] = p1;
    data[3*count+1] = p2;
    data[3*count+2] = layerBuffer->addLayer(layer);
    count++;

    return count-1;
}

///
/// \brief operator <<
/// \param stream
/// \param buffer
/// \return
///
std::ostream& operator<< (std::ostream& stream, const Line3DBuffer& buffer)
{
    for(int i=0; i<buffer.count; i++)
        stream<<i<<std::setw(10)<<buffer.data[3*i]<<std::setw(10)<<buffer.data[3*i+1]<<std::setw(10)<<buffer.data[3*i+2]<<std::endl;
    return stream;
}

///
/// \brief Line3DBuffer::~Line3DBuffer
///
Line3DBuffer::~Line3DBuffer()
{
    if(data) delete [] data;
    delete layerBuffer;
}


///
/// \brief Point3DBuffer::Point3DBuffer
/// \param size
///
Point3DBuffer::Point3DBuffer(int size)
{
    count = 0;
    max = size;
    data = new double[ndof*size];
    layerBuffer = new LayerBuffer(size);
    layerBuffer->addLayer(0);
}

///
/// \brief Point3DBuffer::addPoint
/// \param x
/// \param y
/// \param z
/// \param layer
/// \return
///
int Point3DBuffer::addPoint(double x, double y, double z, int layer)
{
    for(int i=0; i<count; i++)
        if(fabs(x-data[ndof*i])<ptol) // x
            if(fabs(y-data[ndof*i+1])<ptol) // y
                if(fabs(z-data[ndof*i+2])<ptol) // z
                {
                    if(layer!=0)
                        data[ndof*i+3] = static_cast<double>(layerBuffer->addLayer(layer));
                    return i;
                }

    // add point
    data[ndof*count] = x;
    data[ndof*count+1] = y;
    data[ndof*count+2] = z;
    data[ndof*count+3] = static_cast<double>(layerBuffer->addLayer(layer));
    count++;

    return count-1;

}

///
/// \brief operator <<
/// \param stream
/// \param buffer
/// \return
///
std::ostream& operator<< (std::ostream& stream, const Point3DBuffer& buffer)
{
    for(int i=0; i<buffer.count; i++)
        stream<<i<<std::setw(14)<<buffer.data[ndof*i]<<std::setw(14)<<buffer.data[ndof*i+1]<<std::setw(14)<<buffer.data[ndof*i+2]<<std::setw(14)<<buffer.data[ndof*i+3]<<std::endl;
    return stream;
}

///
/// \brief Point3DBuffer::~Point3DBuffer
///
Point3DBuffer::~Point3DBuffer()
{
    if(data) delete [] data;
    delete layerBuffer;
}


///
/// \brief LayerBuffer::LayerBuffer
/// \param size
///
LayerBuffer::LayerBuffer(int size)
{
    count=0;
    index = new int[size];
}

///
/// \brief LayerBuffer::addLayer
/// \param value
/// \return
///
int LayerBuffer::addLayer(int value)
{
    for(int i=0; i<count; i++)
        if(index[i]==value)
            return i;

    // add layer
    index[count] = value;
    count++;
    return count-1;
}

///
/// \brief LayerBuffer::~LayerBuffer
///
LayerBuffer::~LayerBuffer()
{
    if(index) delete [] index;
}

