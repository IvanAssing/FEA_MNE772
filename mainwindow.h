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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


#include "truss3d.h"
#include "solid3d.h"

#include "truss3dfilemanager.h"
#include "solid3dfilemanager.h"



#include "graphicwindow.h"

namespace Ui {
class MainWindow;
}



///
/// \brief The MainWindow class
///
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    Truss3DFileManager *t3d_fileManager;
    Solid3DFileManager *s3d_fileManager;

    Truss3D *t3d_mesh;
    Solid3D *s3d_mesh;

    QAction *lastAction;

    Model model;


    GraphicWindow *wgl;

    ~MainWindow();

public slots:
    virtual void openFile(void);
    virtual void saveFile(void);
    virtual void saveAs(void);
    virtual void solver(void);

    virtual void nodesReport(void);
     virtual void elementsReport(void);

    virtual void setResult(void);

    virtual void paintEvent(QPaintEvent *event);


private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
