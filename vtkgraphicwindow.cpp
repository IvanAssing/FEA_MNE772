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


#include "vtkgraphicwindow.h"

#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkProperty.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkTextProperty.h>

#include <vtkSmartPointer.h>
#include <vtkCellArray.h>
#include <vtkTetra.h>
#include <vtkLine.h>
#include <vtkUnstructuredGrid.h>
#include <vtkExtractEdges.h>
#include <vtkTubeFilter.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkSphereSource.h>
#include <vtkArrowSource.h>
#include <vtkSuperquadricSource.h>
#include <vtkConeSource.h>
#include <vtkGlyph3D.h>


#include <vtkTensorGlyph.h>
#include <vtkHyperStreamline.h>
#include <vtkAppendPolyData.h>

#include <vtkScalarBarWidget.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkInteractorStyleTrackballCamera.h>

#include <vtkContourGrid.h>
#include <vtkPolyDataNormals.h>
#include <vtkStripper.h>
#include <vtkContourFilter.h>
#include <vtkOutlineFilter.h>

#include <vtkPlane.h>
#include <vtkCutter.h>
#include <vtkClipDataSet.h>


#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>

#include <QString>
#include <QDateTime>
#include <QElapsedTimer>

#include "msglog.h"


const char strResults[14][50] = {
    "normal stress x\n",
    "normal stress y\n",
    "normal stress z\n",
    "shear stress xy\n",
    "shear stress yz\n",
    "shear stress zx\n",
    "von Mises \nstress\n",
    "displacement ux\n",
    "displacement uy\n",
    "displacement uz\n",
    "absolute \ndisplacement\n",
    "principal \nstress 1\n",
    "principal \nstress 2\n",
    "principal \nstress 3\n",
};




vtkGraphicWindow::vtkGraphicWindow(QWidget *parent)
    : QVTKOpenGLWidget(parent)
{
    vtkNew<vtkGenericOpenGLRenderWindow> window;
    SetRenderWindow(window.Get());

    // Camera
    vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
    camera->SetViewUp(0,0,1);
    camera->SetPosition(1,1,1);
    camera->SetFocalPoint(0,0,0);

    // Renderer
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetActiveCamera(camera);
    m_renderer->GradientBackgroundOn();
    //    m_renderer->SetBackground(0.4, 0.5, 1.0);
    //    m_renderer->SetBackground2(0.1, 0.2, 0.9);

    m_renderer->SetBackground2(0.9, 0.9, 1.0);
    m_renderer->SetBackground(0.2, 0.2, 0.25);

    //    m_renderer->SetBackground2(1.0, 1.0, 1.0);
    //    m_renderer->SetBackground(1.0, 1.0, 1.0);


    GetRenderWindow()->AddRenderer(m_renderer);

    vtkSmartPointer<vtkInteractorStyleTrackballCamera> iStyle =
            vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

    GetInteractor()->SetInteractorStyle(iStyle);

    step = 0;
    nSteps = 100;
    amplification = 50.;
    s3d_result = 9;
    s3d_mesh = nullptr;

    timer = new QTimer(this);


    // add axes
    vtkSmartPointer<vtkAxesActor> axes = vtkAxesActor::New();

    axes_widget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    axes_widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    axes_widget->SetOrientationMarker( axes );
    axes_widget->SetInteractor(GetInteractor());
    axes_widget->SetViewport( 0.0, 0.0, 0.2, 0.2 );
    axes_widget->SetEnabled( 1 );
    axes_widget->InteractiveOn();


    // add scalarBar
    scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();

    vtkSmartPointer<vtkTextProperty> text = vtkSmartPointer<vtkTextProperty>::New();
    text->SetFontFamilyToArial();
    text->SetFontSize(24);
    text->SetColor(1.0, 1.0, 1.0);
    text->ShadowOn();

    scalarBar->SetLabelTextProperty(text);
    scalarBar->SetTitleTextProperty(text);

    scalarBar->SetTitle("default title\n");
    scalarBar->SetWidth(0.001);
    scalarBar->SetNumberOfLabels(7);
    scalarBar->SetLabelFormat("%5.3E");
    scalarBar->SetBarRatio(0.15);
    scalarBar->SetMaximumWidthInPixels(120);



    scalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();

    scalarBarWidget->SetInteractor(this->GetRenderWindow()->GetInteractor());
    scalarBarWidget->SetCurrentRenderer(m_renderer);
    scalarBarWidget->SetScalarBarActor(scalarBar);

}

void vtkGraphicWindow::addDataSet(Truss3D *mesh)
{
    removeDataSet();

    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<mesh->nNodes; i++)
        points->InsertNextPoint(
                    mesh->nodes[i]->coordinates[0],
                mesh->nodes[i]->coordinates[1],
                mesh->nodes[i]->coordinates[2]);


    vtkSmartPointer<vtkCellArray> lines
            = vtkSmartPointer<vtkCellArray>::New();


    for(int i=0; i<mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {mesh->elements[i]->node1->index,
                             mesh->elements[i]->node2->index};

        lines->InsertNextCell(2, ptIds );
    }

    vtkSmartPointer<vtkPolyData> dataSet =
            vtkSmartPointer<vtkPolyData>::New();
    dataSet->SetPoints(points);
    dataSet->SetLines(lines);

    // Actor
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

    // Mapper
    vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputData(dataSet);
    actor->SetMapper(mapper);


    // Create a tube (cylinder) around the line
    vtkSmartPointer<vtkTubeFilter> tubeFilter =
            vtkSmartPointer<vtkTubeFilter>::New();
    //tubeFilter->SetInputConnection(edges->GetOutputPort());
    tubeFilter->SetInputData(dataSet);
    tubeFilter->SetRadius(.1); //default is .5
    tubeFilter->SetNumberOfSides(16);
    tubeFilter->Update();


    // Actor
    vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();

    // Mapper
    vtkSmartPointer<vtkPolyDataMapper> mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper2->SetInputConnection(tubeFilter->GetOutputPort());
    //mapper2->ScalarVisibilityOff();
    actor2->SetMapper(mapper2);
    actor2->GetProperty()->SetOpacity(0.9);
    //actor2->GetProperty()->SetRepresentationToPoints();
    actor2->GetProperty()->SetColor(98./255, 189./255, 24/255.);

    //m_renderer->AddActor(actor);
    m_renderer->AddActor(actor2);
    m_renderer->ResetCamera(dataSet->GetBounds());


    MsgLog::information(QString("Initial model rendered"));

    renderVTK();
}

void vtkGraphicWindow::addDataSet_solved(Truss3D *mesh)
{

    removeDataSet();

    // Create the color map
    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    double smax;
    double smin;

    mesh->stresslimits(smin, smax);

    lut->SetTableRange(smin, smax);
    lut->SetHueRange(2./3.,0.);
    lut->Build();


    vtkSmartPointer<vtkDoubleArray> scalars =
            vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(mesh->nElements);

    for(int i=0; i<mesh->nElements; i++)
        scalars->SetValue(i, mesh->stress(i));

    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<mesh->nNodes; i++)
        points->InsertNextPoint(
                    mesh->nodes[i]->coordinates[0],
                mesh->nodes[i]->coordinates[1],
                mesh->nodes[i]->coordinates[2]);


    vtkSmartPointer<vtkCellArray> lines
            = vtkSmartPointer<vtkCellArray>::New();


    for(int i=0; i<mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {mesh->elements[i]->node1->index,
                             mesh->elements[i]->node2->index};

        lines->InsertNextCell(2, ptIds );
    }


    vtkSmartPointer<vtkPolyData> dataSet =
            vtkSmartPointer<vtkPolyData>::New();
    dataSet->SetPoints(points);
    dataSet->SetLines(lines);

    dataSet->GetCellData()->SetScalars(scalars);


    // Actor
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

    // Mapper
    vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputData(dataSet);
    actor->SetMapper(mapper);



    // Create a tube (cylinder) around the line
    vtkSmartPointer<vtkTubeFilter> tubeFilter =
            vtkSmartPointer<vtkTubeFilter>::New();
    //tubeFilter->SetInputConnection(edges->GetOutputPort());
    tubeFilter->SetInputData(dataSet);
    tubeFilter->SetRadius(.1); //default is .5
    tubeFilter->SetNumberOfSides(16);
    tubeFilter->SetCapping(1);
    tubeFilter->Update();





    // Actor
    vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();

    // Mapper
    vtkSmartPointer<vtkPolyDataMapper> mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper2->SetInputConnection(tubeFilter->GetOutputPort());
    mapper2->SetLookupTable(lut);
    mapper2->SetScalarRange(smin, smax);

    //mapper2->ScalarVisibilityOff();
    actor2->SetMapper(mapper2);
    //actor2->GetProperty()->SetOpacity(0.9);
    //actor2->GetProperty()->SetRepresentationToPoints();
    //actor2->GetProperty()->SetColor(0.5, 0.5, 0.5);

    //m_renderer->AddActor(actor);
    m_renderer->AddActor(actor2);
    m_renderer->ResetCamera(dataSet->GetBounds());


    scalarBarWidget->EnabledOn();
    scalarBar->SetLookupTable(lut);
    scalarBar->SetTitle("Normal stress");

    MsgLog::information(QString("Solved model rendered"));
    MsgLog::result(QString("Color map visualization of Normal Stress"));


    renderVTK();
}

void vtkGraphicWindow::addDataSet(/*Solid3D *mesh*/ void)
{
    if(s3d_mesh==nullptr)
    {
        MsgLog::error(QString("Model was not loaded."));
        return;
    }

    removeDataSet();

    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<s3d_mesh->nNodes; i++)
        points->InsertNextPoint(
                    s3d_mesh->nodes[i]->coordinates[0],
                s3d_mesh->nodes[i]->coordinates[1],
                s3d_mesh->nodes[i]->coordinates[2]);

    dataSet_0 = vtkSmartPointer<vtkUnstructuredGrid>::New();
    dataSet_0->SetPoints(points);

    refSize = 0.;

    for(int i=0; i<s3d_mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                             s3d_mesh->elements[i]->nodes[1]->index,
                             s3d_mesh->elements[i]->nodes[2]->index,
                             s3d_mesh->elements[i]->nodes[3]->index};

        dataSet_0->InsertNextCell( VTK_TETRA, 4, ptIds );

        refSize += s3d_mesh->elements[i]->V;
    }

    refSize = pow(refSize/s3d_mesh->nElements, 1./3.);


    // Actor
    actor_0 = vtkSmartPointer<vtkActor>::New();

    // Mapper
    vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputData(dataSet_0);
    actor_0->SetMapper(mapper);
    actor_0->GetProperty()->SetColor(1.0, 1.0, 1.0);


    if(showElements)
    {
        actor_0->GetProperty()->SetEdgeColor(0.5, 0.5, 0.8);
        actor_0->GetProperty()->EdgeVisibilityOn();
    }
    if(showNodes)
    {
        actor_0->GetProperty()->SetVertexColor(0.7, 0.7, 1.0);
        actor_0->GetProperty()->VertexVisibilityOn();
        actor_0->GetProperty()->SetPointSize(5.);
    }

    setupShowLoading();
    setupShowRestrictions();
    setupInitialModel();
    addComplements();

    m_renderer->AddActor(actor_0);
    m_renderer->ResetCamera(dataSet_0->GetBounds());

    MsgLog::information(QString("Original model rendered"));

    addComplements();
    renderVTK();
}

void vtkGraphicWindow::addDataSet_solved(/*Solid3D *mesh*/ void)
{
    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }

    removeDataSet();

    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<s3d_mesh->nNodes; i++)
        points->InsertNextPoint(
                    s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i),
                s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1),
                s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2));

    dataSet_1 = vtkSmartPointer<vtkUnstructuredGrid>::New();
    dataSet_1->SetPoints(points);

    for(int i=0; i<s3d_mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                             s3d_mesh->elements[i]->nodes[1]->index,
                             s3d_mesh->elements[i]->nodes[2]->index,
                             s3d_mesh->elements[i]->nodes[3]->index};

        dataSet_1->InsertNextCell( VTK_TETRA, 4, ptIds );
    }

    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    lut->SetTableRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));
    lut->SetHueRange(2./3.,0.);
    lut->Build();

    vtkSmartPointer<vtkDoubleArray> scalars =
            vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(s3d_mesh->nNodes);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        scalars->SetValue(i, s3d_mesh->Snodes(i,s3d_result));

    dataSet_1->GetPointData()->SetScalars(scalars);

    // Actor
    actor_1 = vtkSmartPointer<vtkActor>::New();

    // Mapper
    vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputData(dataSet_1);
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));

    actor_1->SetMapper(mapper);

    if(showElements)
    {
        actor_1->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
        actor_1->GetProperty()->EdgeVisibilityOn();
    }
    if(showNodes)
    {
        actor_1->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
        actor_1->GetProperty()->VertexVisibilityOn();
        actor_1->GetProperty()->SetPointSize(5.);
    }

    scalarBarWidget->EnabledOn();
    scalarBar->SetLookupTable(lut);
    scalarBar->SetTitle(strResults[s3d_result]);

    m_renderer->AddActor(actor_1);
    m_renderer->ResetCamera(actor_1->GetBounds());

    MsgLog::information(QString("Solved model rendered"));
    MsgLog::result(QString("Color map of %1").arg(QString(strResults[s3d_result]).replace("\n", "")));

    addComplements();
    renderVTK();
}

void vtkGraphicWindow::addDataSet_simulation(/*Solid3D *mesh*/ void)
{

    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }

    removeDataSet();

    simulation_actors = new vtkSmartPointer<vtkActor>[nSteps];

    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    double Smin = s3d_mesh->Smin(s3d_result);
    double Smax = s3d_mesh->Smax(s3d_result);

    lut->SetTableRange(Smin, Smax);
    lut->SetHueRange(2./3.,0.);
    lut->Build();

    for(int t=0; t<nSteps; t++)
    {

        vtkSmartPointer< vtkPoints > points =
                vtkSmartPointer< vtkPoints > :: New();

        for(int i=0; i<s3d_mesh->nNodes; i++)
            points->InsertNextPoint(
                        s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i)*t/double(nSteps),
                    s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1)*t/double(nSteps),
                    s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2)*t/double(nSteps));

        vtkSmartPointer<vtkUnstructuredGrid> dataSet =
                vtkSmartPointer<vtkUnstructuredGrid>::New();
        dataSet->SetPoints(points);

        for(int i=0; i<s3d_mesh->nElements; i++)
        {
            vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                                 s3d_mesh->elements[i]->nodes[1]->index,
                                 s3d_mesh->elements[i]->nodes[2]->index,
                                 s3d_mesh->elements[i]->nodes[3]->index};

            dataSet->InsertNextCell( VTK_TETRA, 4, ptIds );
        }


        vtkSmartPointer<vtkDoubleArray> scalars =
                vtkSmartPointer<vtkDoubleArray>::New();
        scalars->SetNumberOfValues(s3d_mesh->nNodes);

        for(int i=0; i<s3d_mesh->nNodes; i++)
            scalars->SetValue(i, s3d_mesh->Snodes(i,s3d_result)*t/double(nSteps));

        dataSet->GetPointData()->SetScalars(scalars);

        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputData(dataSet);
        mapper->SetLookupTable(lut);
        mapper->SetScalarRange(Smin, Smax);

        simulation_actors[t] = vtkSmartPointer<vtkActor>::New();
        simulation_actors[t]->SetMapper(mapper);


        if(showElements)
        {
            simulation_actors[t]->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
            simulation_actors[t]->GetProperty()->EdgeVisibilityOn();
        }
        if(showNodes)
        {
            simulation_actors[t]->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
            simulation_actors[t]->GetProperty()->VertexVisibilityOn();
            simulation_actors[t]->GetProperty()->SetPointSize(5.);
        }

    }


    m_renderer->AddActor(simulation_actors[nSteps-1]);
    m_renderer->ResetCamera(m_renderer->GetActors()->GetLastActor()->GetBounds());

    scalarBarWidget->EnabledOn();
    scalarBar->SetLookupTable(lut);
    scalarBar->SetTitle(strResults[s3d_result]);

    MsgLog::information(QString("Solved model rendered"));
    MsgLog::result(QString("Color map of %1").arg(QString(strResults[s3d_result]).replace("\n", "")));


    addComplements();
    renderVTK();
}

void vtkGraphicWindow::ellipsoidGlyphsVisualization(void)
{
    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }

    removeDataSet();


    vtkSmartPointer<vtkDoubleArray> tensors = vtkSmartPointer<vtkDoubleArray>::New();
    tensors->SetNumberOfTuples(s3d_mesh->nNodes);
    tensors->SetNumberOfComponents(6);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        //n1, n2, n3, n12, n23, n31, von mises, ux, uy, uz
                tensors->InsertTuple6(i,
                                      s3d_mesh->Snodes(i,0), //n1
                                      s3d_mesh->Snodes(i,1), //n2
                                      s3d_mesh->Snodes(i,2), //n3
                                      s3d_mesh->Snodes(i,3), //n12
                                      s3d_mesh->Snodes(i,4), //n23
                                      s3d_mesh->Snodes(i,5)); //n31

//        tensors->InsertTuple6(i,
//                              fabs(s3d_mesh->Snodes(i,0)), //n1
//                              fabs(s3d_mesh->Snodes(i,1)), //n2
//                              fabs(s3d_mesh->Snodes(i,2)), //n3
//                              s3d_mesh->Snodes(i,3), //n12
//                              s3d_mesh->Snodes(i,4), //n23
//                              s3d_mesh->Snodes(i,5)); //n31


    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<s3d_mesh->nNodes; i++)
        points->InsertNextPoint(
                    s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i),
                s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1),
                s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2));

    vtkSmartPointer<vtkUnstructuredGrid> dataSet =
            vtkSmartPointer<vtkUnstructuredGrid>::New();
    dataSet->SetPoints(points);


    for(int i=0; i<s3d_mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                             s3d_mesh->elements[i]->nodes[1]->index,
                             s3d_mesh->elements[i]->nodes[2]->index,
                             s3d_mesh->elements[i]->nodes[3]->index};



        dataSet->InsertNextCell( VTK_TETRA, 4, ptIds );
    }


    vtkSmartPointer<vtkDoubleArray> scalars =
            vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(s3d_mesh->nNodes);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        scalars->SetValue(i, s3d_mesh->Snodes(i,s3d_result));


    dataSet->GetPointData()->SetScalars(scalars);
    dataSet->GetPointData()->SetTensors(tensors);


    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetPhiResolution(16);
    sphereSource->SetThetaResolution(16);
    sphereSource->Update();

    vtkSmartPointer<vtkTensorGlyph> tensorGlyph = vtkSmartPointer<vtkTensorGlyph>::New();
    tensorGlyph->SetInputData(dataSet);

    tensorGlyph->SetSourceConnection(sphereSource->GetOutputPort());
    if(elg_colorbyeigenvalues)
        tensorGlyph->SetColorMode(vtkTensorGlyph::COLOR_BY_EIGENVALUES);
    else
        tensorGlyph->SetColorMode(vtkTensorGlyph::COLOR_BY_SCALARS);
    tensorGlyph->ExtractEigenvaluesOn();
    tensorGlyph->SetMaxScaleFactor(2.*refSize*(1+elg_scalefactor));
    //tensorGlyph->SetScaleFactor(2.*refSize*(1+elg_scalefactor));
    //tensorGlyph->ScalingOn();
    //tensorGlyph->ThreeGlyphsOn();
    tensorGlyph->ClampScalingOn();
    tensorGlyph->Update();

    // Actor
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

    // Mapper
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(tensorGlyph->GetOutput());
    mapper->SetScalarRange(tensorGlyph->GetOutput()->GetScalarRange());
    actor->SetMapper(mapper);


    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    lut->SetTableRange(tensorGlyph->GetOutput()->GetScalarRange());
    lut->SetHueRange(2./3.,0.);
    lut->Build();

    scalarBar->SetLookupTable(lut);

    if(elg_colorbyeigenvalues)
        scalarBar->SetTitle("Major Eigenvalue\n");
    else
        scalarBar->SetTitle(strResults[s3d_result]);

    m_renderer->AddActor(actor);


    if(showTranslucentModel)
    {
        // Actor
        vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();

        // Mapper
        vtkSmartPointer<vtkDataSetMapper> mapper2 = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper2->SetInputData(dataSet);
        mapper2->ScalarVisibilityOff();
        actor2->SetMapper(mapper2);

        actor2->GetProperty()->SetColor(1.0, 1.0, 1.0);

        actor2->GetProperty()->SetOpacity(0.25);

        if(showElements)
        {
            actor2->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
            actor2->GetProperty()->EdgeVisibilityOn();
        }
        if(showNodes)
        {
            actor2->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
            actor2->GetProperty()->VertexVisibilityOn();
            actor2->GetProperty()->SetPointSize(5.);
        }

        m_renderer->AddActor(actor2);
    }

    MsgLog::information(QString("Tensor Ellipsoid Glyph rendered"));
    if(!elg_colorbyeigenvalues)
        MsgLog::result(QString("Color map of %1").arg(QString(strResults[s3d_result]).replace("\n", "")));
    else
        MsgLog::result(QString("Color map of Major Eigenvalue"));

    m_renderer->ResetCamera(dataSet->GetBounds());

    renderVTK();
}

void vtkGraphicWindow::hyperstreamlinesVisualization(void)
{

    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }


    removeDataSet();

    vtkSmartPointer<vtkDoubleArray> tensors = vtkSmartPointer<vtkDoubleArray>::New();
    tensors->SetNumberOfTuples(s3d_mesh->nNodes);
    tensors->SetNumberOfComponents(6);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        //n1, n2, n3, n12, n23, n31, von mises, ux, uy, uz
        tensors->InsertTuple6(i,
                              s3d_mesh->Snodes(i,0), //n1
                              s3d_mesh->Snodes(i,1), //n2
                              s3d_mesh->Snodes(i,2), //n3
                              s3d_mesh->Snodes(i,3), //n12
                              s3d_mesh->Snodes(i,4), //n23
                              s3d_mesh->Snodes(i,5)); //n31


    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<s3d_mesh->nNodes; i++)
        points->InsertNextPoint(
                    s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i),
                s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1),
                s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2));

    vtkSmartPointer<vtkUnstructuredGrid> dataSet =
            vtkSmartPointer<vtkUnstructuredGrid>::New();
    dataSet->SetPoints(points);

    for(int i=0; i<s3d_mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                             s3d_mesh->elements[i]->nodes[1]->index,
                             s3d_mesh->elements[i]->nodes[2]->index,
                             s3d_mesh->elements[i]->nodes[3]->index};

        dataSet->InsertNextCell( VTK_TETRA, 4, ptIds );
    }


    // Create the color map
    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    lut->SetTableRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));
    lut->SetHueRange(2./3.,0.);
    lut->Build();


    vtkSmartPointer<vtkDoubleArray> scalars =
            vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(s3d_mesh->nNodes);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        scalars->SetValue(i, s3d_mesh->Snodes(i,s3d_result));


    dataSet->GetPointData()->SetScalars(scalars);
    dataSet->GetPointData()->SetTensors(tensors);

    // Generate hyperstreamlines
    int count = 0;
    int k = 0;
    int r_count = 0;
    try{
        for(int i=0;i<s3d_mesh->nNodes;i++)
        {
            k=-1;
            if(hsl_spPlanez0 && fabs(s3d_mesh->nodes[i]->coordinates[2])<1e-5)
                k = i;
            if(hsl_spRestrictions && (s3d_mesh->nodes[i]->restrictions[0] || s3d_mesh->nodes[i]->restrictions[1] || s3d_mesh->nodes[i]->restrictions[2]))
                k = i;
            if(hsl_spLoading && Mth::norm(s3d_mesh->nodes[i]->loading)>1.e-4)
                k = i;
            if(hsl_spRandom && k==-1 && r_count<100)
            {
                k = vtkMath::Round(vtkMath::Random(i, s3d_mesh->nNodes));
                r_count++;
            }
            if(k==-1)
                continue;

            vtkSmartPointer<vtkHyperStreamline> s1 =
                    vtkSmartPointer<vtkHyperStreamline>::New();
            s1->SetInputData(dataSet);
            s1->SetStartPosition(
                        s3d_mesh->nodes[k]->coordinates[0]+amplification*s3d_mesh->u(3*k),
                    s3d_mesh->nodes[k]->coordinates[1]+amplification*s3d_mesh->u(3*k+1),
                    s3d_mesh->nodes[k]->coordinates[2]+amplification*s3d_mesh->u(3*k+2));
            if(hsl_v1)
                s1->IntegrateMajorEigenvector();
            else if(hsl_v2)
                s1->IntegrateMediumEigenvector();
            else
                s1->IntegrateMinorEigenvector();
            s1->SetMaximumPropagationDistance(500.0);
            s1->SetIntegrationStepLength(0.1);
            s1->SetStepLength(0.01);
            s1->SetRadius(0.2*refSize*(1+hsl_radiusfactor));
            s1->SetNumberOfSides(18);
            s1->SetIntegrationDirectionToIntegrateBothDirections();
            //s1->SetIntegrationDirectionToForward();
            s1->Update();


            vtkSmartPointer<vtkPolyDataMapper> s1Mapper =
                    vtkSmartPointer<vtkPolyDataMapper>::New();
            s1Mapper->SetInputData(s1->GetOutput());
            s1Mapper->SetLookupTable(lut);
            s1Mapper->SetScalarRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));
            s1Mapper->Update();

            vtkSmartPointer<vtkActor> s1Actor =
                    vtkSmartPointer<vtkActor>::New();
            s1Actor->SetMapper(s1Mapper);
            s1Actor->GetProperty()->LightingOff();

            m_renderer->AddActor(s1Actor);
            count++;

            if(count >= 500)
            {
                MsgLog::error(QString("Total Hyperstreamlines limited to 500"));
                break;
            }
        }



        //std::cerr<<"Hyper Streamlines: "<<count;

        if(showTranslucentModel)
        {
            // Actor
            vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();

            // Mapper
            vtkSmartPointer<vtkDataSetMapper> mapper2 = vtkSmartPointer<vtkDataSetMapper>::New();
            mapper2->SetInputData(dataSet);
            mapper2->ScalarVisibilityOff();
            actor2->SetMapper(mapper2);

            actor2->GetProperty()->SetColor(1.0, 1.0, 1.0);

            actor2->GetProperty()->SetOpacity(0.25);

            if(showElements)
            {
                actor2->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
                actor2->GetProperty()->EdgeVisibilityOn();
            }
            if(showNodes)
            {
                actor2->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
                actor2->GetProperty()->VertexVisibilityOn();
                actor2->GetProperty()->SetPointSize(5.);
            }

            m_renderer->AddActor(actor2);
        }


        m_renderer->ResetCamera(dataSet->GetBounds());
        scalarBar->SetLookupTable(lut);
        scalarBar->SetTitle(strResults[s3d_result]);

        MsgLog::information(QString("Tensor Hyperstreamlines rendered"));
        MsgLog::information(QString("Total Hyperstreamlines: %1").arg(count));
        MsgLog::result(QString("Color map of %1").arg(QString(strResults[s3d_result]).replace("\n", "")));

        addComplements();
        renderVTK();


    }
    catch(...)
    {
        MsgLog::error(QString("Tensor Hyperstreamlines process failed"));
        addDataSet();
        //std::cerr<<"\n error in HyperStreamlines construction";
        return;
    }


}

void vtkGraphicWindow::isoSurfaceVisualization(void)
{
    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }
    removeDataSet();


    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<s3d_mesh->nNodes; i++)
        points->InsertNextPoint(
                    s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i),
                s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1),
                s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2));

    vtkSmartPointer<vtkUnstructuredGrid> dataSet =
            vtkSmartPointer<vtkUnstructuredGrid>::New();
    dataSet->SetPoints(points);

    for(int i=0; i<s3d_mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                             s3d_mesh->elements[i]->nodes[1]->index,
                             s3d_mesh->elements[i]->nodes[2]->index,
                             s3d_mesh->elements[i]->nodes[3]->index};

        dataSet->InsertNextCell( VTK_TETRA, 4, ptIds );
    }


    vtkSmartPointer<vtkDoubleArray> scalars =
            vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(s3d_mesh->nNodes);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        scalars->SetValue(i, s3d_mesh->Snodes(i,s3d_result));

    dataSet->GetPointData()->SetScalars(scalars);

    // Create the color map
    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    lut->SetTableRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));
    lut->SetHueRange(2./3.,0.);
    lut->Build();


    vtkSmartPointer<vtkContourFilter> contour =
            vtkSmartPointer<vtkContourFilter>::New();
    contour->SetInputData(dataSet);
    double *range = dataSet->GetScalarRange();
    contour->GenerateValues(nIsoSurfaceSlices, range[0], range[1]);

    contour->ComputeScalarsOn();
    contour->ComputeNormalsOn();
    contour->Update();


    vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputData(contour->GetOutput());
    mapper->SetScalarRange(dataSet->GetScalarRange());


    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    //actor->GetProperty()->SetInterpolationToPhong();

    if(showTranslucentModel)
    {
        // Actor
        vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();

        // Mapper
        vtkSmartPointer<vtkDataSetMapper> mapper2 = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper2->SetInputData(dataSet);
        mapper2->ScalarVisibilityOff();
        actor2->SetMapper(mapper2);

        actor2->GetProperty()->SetColor(1.0, 1.0, 1.0);

        actor2->GetProperty()->SetOpacity(0.25);

        if(showElements)
        {
            actor2->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
            actor2->GetProperty()->EdgeVisibilityOn();
        }
        if(showNodes)
        {
            actor2->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
            actor2->GetProperty()->VertexVisibilityOn();
            actor2->GetProperty()->SetPointSize(5.);
        }

        m_renderer->AddActor(actor2);
    }

    scalarBar->SetLookupTable(lut);
    scalarBar->SetTitle(strResults[s3d_result]);

    m_renderer->AddActor(actor);
    m_renderer->ResetCamera(dataSet->GetBounds());

    setupShowLoading();
    setupShowRestrictions();
    setupInitialModel();
    addComplements();

    MsgLog::result(QString("Isosurfaces for %1").arg(QString(strResults[s3d_result]).replace("\n", "")));
    MsgLog::information(QString("Total surfaces: %1").arg(nIsoSurfaceSlices));

    renderVTK();
}

void vtkGraphicWindow::superquadricsGlyphsVisualization(void)
{
    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }
    removeDataSet();

    vtkSmartPointer<vtkDoubleArray> tensors = vtkSmartPointer<vtkDoubleArray>::New();
    tensors->SetNumberOfTuples(s3d_mesh->nNodes);
    tensors->SetNumberOfComponents(9);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        //n1, n2, n3, n12, n23, n31, von mises, ux, uy, uz
        tensors->InsertTuple9(i,
                              s3d_mesh->Snodes(i,0), //n1
                              s3d_mesh->Snodes(i,3), //n12
                              s3d_mesh->Snodes(i,5), //n31
                              s3d_mesh->Snodes(i,3), //n12
                              s3d_mesh->Snodes(i,1), //n2
                              s3d_mesh->Snodes(i,4), //n23
                              s3d_mesh->Snodes(i,5), //n31
                              s3d_mesh->Snodes(i,4), //n23
                              s3d_mesh->Snodes(i,2) //n3
                              );



    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<s3d_mesh->nNodes; i++)
        points->InsertNextPoint(
                    s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i),
                s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1),
                s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2));

    vtkSmartPointer<vtkUnstructuredGrid> dataSet =
            vtkSmartPointer<vtkUnstructuredGrid>::New();
    dataSet->SetPoints(points);


    for(int i=0; i<s3d_mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                             s3d_mesh->elements[i]->nodes[1]->index,
                             s3d_mesh->elements[i]->nodes[2]->index,
                             s3d_mesh->elements[i]->nodes[3]->index};



        dataSet->InsertNextCell( VTK_TETRA, 4, ptIds );
    }



    // Create the color map
    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    lut->SetTableRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));
    lut->SetHueRange(2./3.,0.);
    lut->Build();

    vtkSmartPointer<vtkDoubleArray> scalars =
            vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(s3d_mesh->nNodes);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        scalars->SetValue(i, s3d_mesh->Snodes(i,s3d_result));


    dataSet->GetPointData()->SetScalars(scalars);
    dataSet->GetPointData()->SetTensors(tensors);

    double tensor[9], w[3];
    int axis;
    double sum, cl, cp, theta, phi, gamma;

    double **m = new double*[3];
    double **v = new double*[3];
    for(int i=0; i<3;i++)
    {
        m[i] = new double[3];
        v[i] = new double[3];
    }


    vtkSmartPointer<vtkAppendPolyData> append = vtkSmartPointer<vtkAppendPolyData>::New();


    vtkSmartPointer<vtkDoubleArray> iTensor = vtkSmartPointer<vtkDoubleArray>::New();
    iTensor->SetNumberOfTuples(1);
    iTensor->SetNumberOfComponents(9);

    vtkSmartPointer<vtkDoubleArray> iScalar = vtkSmartPointer<vtkDoubleArray>::New();
    iScalar->SetNumberOfTuples(1);
    iScalar->SetNumberOfComponents(3);

    vtkSmartPointer<vtkPolyData> node = vtkSmartPointer<vtkPolyData>::New();

    vtkSmartPointer< vtkPoints > point = vtkSmartPointer< vtkPoints > :: New();


    for(int i=0; i<s3d_mesh->nNodes; i++)
    {

        point->InsertPoint(0,
                           s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i),
                s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1),
                s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2));

        node->SetPoints(point);


        iTensor->InsertTuple9(0,
                              s3d_mesh->Snodes(i,0), //n1
                              s3d_mesh->Snodes(i,3), //n12
                              s3d_mesh->Snodes(i,5), //n31
                              s3d_mesh->Snodes(i,3), //n12
                              s3d_mesh->Snodes(i,1), //n2
                              s3d_mesh->Snodes(i,4), //n23
                              s3d_mesh->Snodes(i,5), //n31
                              s3d_mesh->Snodes(i,4), //n23
                              s3d_mesh->Snodes(i,2) //n3
                              );


        node->GetPointData()->SetTensors(iTensor);
        iTensor->GetTuple(0, tensor);


        for (int j=0; j<3; j++)
            for (int k=0; k<3; k++)
                m[k][j] = tensor[k+3*j];

        vtkMath::Jacobi(m, w, v);
        //std::cerr<<"\nw0="<<w[0]<<" w1="<<w[1]<<" w2="<<w[2];

        // Calculation according to equation (7) from
        // Gordon Kindlmann, Superquadric Tensor Glyphs, Joint
        // EUROGRAPHICS - IEEE TCVG Symposium on Visualization (2004),
        // http://www.cs.utah.edu/~gk/papers/vissym04/vissym04kindlmann.pdf

        int mv = 0;
        ///modified part
        if(sqg_absoluteeigenvalues)
        {
            w[0] = fabs(w[0]);
            w[1] = fabs(w[1]);
            w[2] = fabs(w[2]);

            int order[3] = {0,1,2};
            // sort the eigenvalues
            if(w[0]<w[1]) std::swap(w[0], w[1]), std::swap(order[0], order[1]);
            if(w[1]<w[2]) std::swap(w[1], w[2]), std::swap(order[1], order[2]);
            if(w[0]<w[1]) std::swap(w[0], w[1]), std::swap(order[0], order[1]);

            mv = order[0];
        }

        if (w[2] > 0.)
        {
            sum = w[0] + w[1] + w[2];
            cl = (w[0] - w[1]) / sum;
            cp = 2.*(w[1] - w[2]) / sum;
            //cs = 3*w[2] / sum;

            if (cl >= cp)
            {
                axis  = 0;  // superquadric with rotational symmetriy around the x axis
                theta = pow(1 - cp, sqg_gamma);
                phi   = pow(1 - cl, sqg_gamma);
            }
            else
            {
                axis  = 2;  // superquadric with rotational symmetriy around the z axis
                theta = pow(1 - cl, sqg_gamma);
                phi   = pow(1 - cp, sqg_gamma);
            }
        }
        else
        {
            std::cerr<<"\nPoint " << i << " has at least one negative tensor eigenvalue. Superquadric tensor glyphing not feasible.";
            continue;
        }

        QVector3D e1(v[0][mv], v[1][mv], v[2][mv]);
        e1.normalize();

        if(sqg_colorbyeigenvalues)
            iScalar->InsertTuple3(0, cl*fabs(e1.x())-cl+1., cl*fabs(e1.y())-cl+1., cl*fabs(e1.z())-cl+1.);
        else
        {
            double color[3];
            lut->GetColor(s3d_mesh->Snodes(i,s3d_result), color);
            iScalar->InsertTuple3(0, color[0], color[1], color[2]);
        }

        node->GetPointData()->SetScalars(iScalar);


        //std::cerr<<"\n phi:"<<phi<<" theta:"<<theta<<" cl:"<<cl<<" cp:"<<cp;

        vtkSmartPointer<vtkSuperquadricSource> superquadric = vtkSmartPointer<vtkSuperquadricSource>::New();

        superquadric->SetAxisOfSymmetry(axis);
        superquadric->SetThetaRoundness(theta);
        superquadric->SetPhiRoundness(phi);

        vtkSmartPointer<vtkTensorGlyph> tensorGlyph = vtkSmartPointer<vtkTensorGlyph>::New();
        tensorGlyph->SetInputData(node);

        tensorGlyph->SetSourceConnection(superquadric->GetOutputPort());

        //tensorGlyph->SetColorMode(vtkTensorGlyph::COLOR_BY_EIGENVALUES);
        tensorGlyph->ColorGlyphsOff();

        tensorGlyph->ExtractEigenvaluesOn();
        tensorGlyph->SetMaxScaleFactor(2.*refSize*(1+sqg_scalefactor));
        //        tensorGlyph->SetScaleFactor(10);
        //        tensorGlyph->ScalingOn();
        tensorGlyph->ClampScalingOn();
        tensorGlyph->Update();

        //        append->AddInputData(vtkPolyData::SafeDownCast(tensorGlyph->GetOutput()));
        //        append->Update();


        vtkSmartPointer<vtkActor> actor3 = vtkSmartPointer<vtkActor>::New();
        actor3->GetProperty()->SetColor(iScalar->GetTuple3(0));

        vtkSmartPointer<vtkPolyDataMapper> mapper3 = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper3->SetInputData(tensorGlyph->GetOutput());
        mapper3->SetLookupTable(lut);
        actor3->SetMapper(mapper3);



        m_renderer->AddActor(actor3);

    }


    //    // Actor
    //    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

    //    // Mapper
    //    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //    mapper->SetInputData(append->GetOutput());
    //    //mapper->SetLookupTable(lut);
    //    mapper->SetScalarRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));

    //    actor->SetMapper(mapper);
    //    m_renderer->AddActor(actor);

    scalarBar->SetLookupTable(lut);
    if(sqg_colorbyeigenvalues)
        scalarBar->SetTitle("Major Eigenvalue\n (ref. G.Kindlmann)\n");
    else
        scalarBar->SetTitle(strResults[s3d_result]);


    if(showTranslucentModel)
    {
        // Actor
        vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();

        // Mapper
        vtkSmartPointer<vtkDataSetMapper> mapper2 = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper2->SetInputData(dataSet);
        mapper2->ScalarVisibilityOff();
        actor2->SetMapper(mapper2);

        actor2->GetProperty()->SetColor(1.0, 1.0, 1.0);

        actor2->GetProperty()->SetOpacity(0.25);

        if(showElements)
        {
            actor2->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
            actor2->GetProperty()->EdgeVisibilityOn();
        }
        if(showNodes)
        {
            actor2->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
            actor2->GetProperty()->VertexVisibilityOn();
            actor2->GetProperty()->SetPointSize(5.);
        }

        m_renderer->AddActor(actor2);
    }


    m_renderer->ResetCamera(dataSet->GetBounds());

    MsgLog::information(QString("Tensor Superquadric Glyph rendered"));
    if(!sqg_colorbyeigenvalues)
        MsgLog::result(QString("Color map of %1").arg(QString(strResults[s3d_result]).replace("\n", "")));
    else
        MsgLog::result(QString("Color map of Major Eigenvalue (ref. G.Kindlmann)"));

    addComplements();
    renderVTK();
}

void vtkGraphicWindow::displacementVisualization(void)
{
    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }
    removeDataSet();


    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<s3d_mesh->nNodes; i++)
        points->InsertNextPoint(
                    s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i),
                s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1),
                s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2));

    vtkSmartPointer<vtkUnstructuredGrid> dataSet =
            vtkSmartPointer<vtkUnstructuredGrid>::New();
    dataSet->SetPoints(points);


    for(int i=0; i<s3d_mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                             s3d_mesh->elements[i]->nodes[1]->index,
                             s3d_mesh->elements[i]->nodes[2]->index,
                             s3d_mesh->elements[i]->nodes[3]->index};



        dataSet->InsertNextCell( VTK_TETRA, 4, ptIds );
    }



    vtkSmartPointer<vtkDoubleArray> displacement = vtkSmartPointer<vtkDoubleArray>::New();
    displacement->SetNumberOfComponents(3);


    vtkSmartPointer<vtkDoubleArray> magnitude = vtkSmartPointer<vtkDoubleArray>::New();
    magnitude->SetNumberOfValues(s3d_mesh->nNodes);


    double maxDisp = .0;
    double iDisp = .0;
    double ux, uy, uz;

    for(int i=0;i<s3d_mesh->nNodes;i++)
    {
        ux = s3d_mesh->u(3*i);
        uy = s3d_mesh->u(3*i+1);
        uz = s3d_mesh->u(3*i+2);

        displacement->InsertNextTuple3(ux, uy, uz);
        iDisp = pow(ux*ux+uy*uy+uz*uz, 0.5);
        maxDisp = iDisp > maxDisp? iDisp : maxDisp;

        magnitude->SetValue(i, iDisp);

    }


    dataSet->GetPointData()->SetScalars(magnitude);
    dataSet->GetPointData()->SetVectors(displacement);

    // Create the color map
    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    lut->SetTableRange(0, maxDisp);
    lut->SetHueRange(2./3.,0.);
    lut->Build();


    vtkSmartPointer<vtkArrowSource> arrow =
            vtkSmartPointer<vtkArrowSource>::New();
    arrow->SetShaftResolution(16);
    arrow->SetTipResolution(16);

    double scale = 2.*refSize/maxDisp;

    vtkSmartPointer<vtkGlyph3D> glyphs =
            vtkSmartPointer<vtkGlyph3D>::New();
    glyphs->SetInputData(dataSet);
    glyphs->SetSourceConnection(arrow->GetOutputPort());
    glyphs->ScalingOn();
    glyphs->SetScaleModeToScaleByVector();
    glyphs->SetScaleFactor(scale);
    //glyphs->OrientOn();
    //glyphs->ClampingOn();
    //glyphs->SetVectorModeToUseVector();
    //glyphs->SetIndexModeToOff();


    vtkSmartPointer<vtkPolyDataMapper> glyphMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(glyphs->GetOutputPort());
    glyphMapper->SetLookupTable(lut);
    glyphMapper->SetScalarRange(dataSet->GetScalarRange());

    vtkSmartPointer<vtkActor> glyphActor =
            vtkSmartPointer<vtkActor>::New();
    glyphActor->SetMapper(glyphMapper);

    if(showTranslucentModel)
    {
        // Actor
        vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();

        // Mapper
        vtkSmartPointer<vtkDataSetMapper> mapper2 = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper2->SetInputData(dataSet);
        mapper2->ScalarVisibilityOff();
        actor2->SetMapper(mapper2);

        actor2->GetProperty()->SetColor(1.0, 1.0, 1.0);

        actor2->GetProperty()->SetOpacity(0.25);

        if(showElements)
        {
            actor2->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
            actor2->GetProperty()->EdgeVisibilityOn();
        }
        if(showNodes)
        {
            actor2->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
            actor2->GetProperty()->VertexVisibilityOn();
            actor2->GetProperty()->SetPointSize(5.);
        }

        m_renderer->AddActor(actor2);
    }
    scalarBar->SetLookupTable(lut);
    scalarBar->SetTitle("Absolute \nDisplacement \n");


    m_renderer->AddActor(glyphActor);
    m_renderer->ResetCamera(dataSet->GetBounds());

    MsgLog::information(QString("Displacement Vector Glyph rendered"));
    MsgLog::result(QString("Color map of absolute displacement"));

    addComplements();
    renderVTK();
}

void vtkGraphicWindow::startSimulation(void)
{

    //addDataSet_simulation();
    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }
    addDataSet_simulation();
    connect(timer, SIGNAL(timeout()), this, SLOT(drawNextStep()));
    timer->start(100);
    MsgLog::information(QString("Simulation started"));
}

void vtkGraphicWindow::drawNextStep(void)
{
    step++;

    if(step>=nSteps) step=0;

    removeDataSet();
    m_renderer->AddActor(simulation_actors[step]);

    addComplements();
    renderVTK();
}

void vtkGraphicWindow::pauseSimulation(void)
{
    if(!timer->isActive()) return;
    //disconnect(timer, SIGNAL(timeout()), this, SLOT(drawNextStep()));
    timer->stop();
    MsgLog::information(QString("Simulation paused"));
}

void vtkGraphicWindow::stopSimulation(void)
{
    //if(!timer->isActive()) return;
    //disconnect(timer, SIGNAL(timeout()), this, SLOT(drawNextStep()));
    timer->stop();
    step = nSteps-1;
    drawNextStep();
    MsgLog::information(QString("Simulation stoped"));
}

void vtkGraphicWindow::removeDataSet()
{
    vtkActor *actor = m_renderer->GetActors()->GetLastActor();
    while (actor != nullptr)
    {
        m_renderer->RemoveActor(actor);
        actor = m_renderer->GetActors()->GetLastActor();
    }

    //renderVTK();
}

void vtkGraphicWindow::zoomToExtent()
{

    vtkSmartPointer<vtkActor> actor = m_renderer->GetActors()->GetLastActor();
    if (actor != nullptr)
    {
        m_renderer->ResetCamera(actor->GetBounds());
    }

    renderVTK();
}

void vtkGraphicWindow::setParallelProjection(void)
{
    m_renderer->GetActiveCamera()->ParallelProjectionOn();
    renderVTK();
}

void vtkGraphicWindow::setPerspectiveProjection(void)
{
    m_renderer->GetActiveCamera()->ParallelProjectionOff();
    renderVTK();
}

void vtkGraphicWindow::setXYTopView(void)
{
    m_renderer->GetActiveCamera()->SetPosition(0,0,1);
    m_renderer->GetActiveCamera()->SetViewUp(0,1,0);
    m_renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
    m_renderer->ResetCamera(m_renderer->GetActors()->GetLastActor()->GetBounds());
    renderVTK();
}

void vtkGraphicWindow::setXYBottomView(void)
{
    m_renderer->GetActiveCamera()->SetPosition(0,0,-1);
    m_renderer->GetActiveCamera()->SetViewUp(0,1,0);
    m_renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
    m_renderer->ResetCamera(m_renderer->GetActors()->GetLastActor()->GetBounds());
    renderVTK();
}

void vtkGraphicWindow::setYZTopView(void)
{
    m_renderer->GetActiveCamera()->SetPosition(1,0,0);
    m_renderer->GetActiveCamera()->SetViewUp(0,0,1);
    m_renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
    m_renderer->ResetCamera(m_renderer->GetActors()->GetLastActor()->GetBounds());
    renderVTK();
}

void vtkGraphicWindow::setYZBottomView(void)
{
    m_renderer->GetActiveCamera()->SetPosition(-1,0,0);
    m_renderer->GetActiveCamera()->SetViewUp(0,0,1);
    m_renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
    m_renderer->ResetCamera(m_renderer->GetActors()->GetLastActor()->GetBounds());
    renderVTK();
}

void vtkGraphicWindow::setXZTopView(void)
{
    m_renderer->GetActiveCamera()->SetPosition(0,1,0);
    m_renderer->GetActiveCamera()->SetViewUp(0,0,1);
    m_renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
    m_renderer->ResetCamera(m_renderer->GetActors()->GetLastActor()->GetBounds());
    renderVTK();
}

void vtkGraphicWindow::setXZBottomView(void)
{
    m_renderer->GetActiveCamera()->SetPosition(0,-1,0);
    m_renderer->GetActiveCamera()->SetViewUp(0,0,1);
    m_renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
    m_renderer->ResetCamera(m_renderer->GetActors()->GetLastActor()->GetBounds());
    renderVTK();
}

void vtkGraphicWindow::setIsometricView(void)
{
    m_renderer->GetActiveCamera()->SetPosition(1,1,1);
    m_renderer->GetActiveCamera()->SetViewUp(0,0,1);
    m_renderer->ResetCamera(m_renderer->GetActors()->GetLastActor()->GetBounds());
    renderVTK();
}

void vtkGraphicWindow::screenshot(void)
{
    QDateTime now = QDateTime::currentDateTime();

    QString filename = QString("../screenshots/screenshot-")
            + now.toString("yyyyMMddhhmmsszzz") + QString(".png");

    // Screenshot
    vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter =
            vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImageFilter->SetInput(GetRenderWindow());

    windowToImageFilter->SetScale(1); //image quality
    windowToImageFilter->SetInputBufferTypeToRGBA(); //also record the alpha (transparency) channel
    windowToImageFilter->ReadFrontBufferOff(); // read from the back buffer

    windowToImageFilter->Update();
    renderVTK();

    vtkSmartPointer<vtkPNGWriter> writer =
            vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetFileName(filename.toStdString().c_str());
    writer->SetInputConnection(windowToImageFilter->GetOutputPort());
    writer->Write();

    MsgLog::information(QString("Screenshot saved: %1").arg(filename));
}

void vtkGraphicWindow::setupShowLoading(void)
{
    if(s3d_mesh==nullptr)
        return;

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    vtkSmartPointer<vtkDoubleArray> loading = vtkSmartPointer<vtkDoubleArray>::New();
    loading->SetNumberOfComponents(3);


    double maxLoad = .0;
    double magnitude = .0;

    double signal = -1.;

    for(int i=0;i<s3d_mesh->nElements;i++)
    {
        if(s3d_mesh->elements[i]->pface != -1)
            if(*s3d_mesh->elements[i]->pressure<0.)
            {
                signal = 1.0;
                break;
            }
    }

    int npt = 0;
    for(int i=0;i<s3d_mesh->nNodes;i++)
    {
        magnitude = Mth::norm(s3d_mesh->nodes[i]->loading);

        if(magnitude>1e-5)
        {
            if(s3d_mesh->nodes[i])
                maxLoad = magnitude > maxLoad? magnitude : maxLoad;
            points->InsertNextPoint(
                        s3d_mesh->nodes[i]->coordinates[0]/*+amplification*s3d_mesh->u(3*i)*/,
                    s3d_mesh->nodes[i]->coordinates[1]/*+amplification*s3d_mesh->u(3*i+1)*/,
                    s3d_mesh->nodes[i]->coordinates[2]/*+amplification*s3d_mesh->u(3*i+2)*/);

            loading->InsertTuple3(npt, signal*s3d_mesh->nodes[i]->loading[0],
                    signal*s3d_mesh->nodes[i]->loading[1], signal*s3d_mesh->nodes[i]->loading[2]);

            npt++;
        }
    }

    vtkSmartPointer<vtkPolyData> dataSet =
            vtkSmartPointer<vtkPolyData>::New();
    dataSet->SetPoints(points);

    dataSet->GetPointData()->SetVectors(loading);

    vtkSmartPointer<vtkArrowSource> arrow =
            vtkSmartPointer<vtkArrowSource>::New();
    arrow->SetShaftResolution(16);
    arrow->SetTipResolution(16);
    if(signal<0.)
        arrow->InvertOn();


    double scale = 2.*refSize/maxLoad;
    //    std::cerr<<"\n scale="<<scale;
    //    std::cerr<<"\n npt="<<npt;

    vtkSmartPointer<vtkGlyph3D> glyphs =
            vtkSmartPointer<vtkGlyph3D>::New();
    glyphs->SetInputData(dataSet);
    glyphs->SetSourceConnection(arrow->GetOutputPort());
    glyphs->ScalingOn();
    glyphs->SetScaleModeToScaleByVector();
    glyphs->SetScaleFactor(scale);
    //glyphs->OrientOn();
    //glyphs->ClampingOn();
    //glyphs->SetVectorModeToUseVector();
    //glyphs->SetIndexModeToOff();


    vtkSmartPointer<vtkPolyDataMapper> glyphMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(glyphs->GetOutputPort());
    glyphMapper->ScalarVisibilityOff();

    loadingActor =vtkSmartPointer<vtkActor>::New();
    loadingActor->SetMapper(glyphMapper);
    loadingActor->GetProperty()->SetColor(1.0, 0.4, 0.0);
}

void vtkGraphicWindow::setupShowRestrictions(void)
{
    if(s3d_mesh==nullptr)
        return;


    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    vtkSmartPointer<vtkDoubleArray> restrictions = vtkSmartPointer<vtkDoubleArray>::New();
    restrictions->SetNumberOfComponents(3);

    vtkSmartPointer<vtkDoubleArray> colors = vtkSmartPointer<vtkDoubleArray>::New();
    colors->SetNumberOfComponents(3);

    for(int i=0;i<s3d_mesh->nNodes;i++)
    {
        // x
        if(s3d_mesh->nodes[i]->restrictions[0])
        {
            points->InsertNextPoint(
                        s3d_mesh->nodes[i]->coordinates[0]/*+amplification*s3d_mesh->u(3*i)*/,
                    s3d_mesh->nodes[i]->coordinates[1]/*+amplification*s3d_mesh->u(3*i+1)*/,
                    s3d_mesh->nodes[i]->coordinates[2]/*+amplification*s3d_mesh->u(3*i+2)*/);
            restrictions->InsertNextTuple3(1,0,0);
            colors->InsertNextTuple3(1,0,0);

            points->InsertNextPoint(
                        s3d_mesh->nodes[i]->coordinates[0]/*+amplification*s3d_mesh->u(3*i)*/,
                    s3d_mesh->nodes[i]->coordinates[1]/*+amplification*s3d_mesh->u(3*i+1)*/,
                    s3d_mesh->nodes[i]->coordinates[2]/*+amplification*s3d_mesh->u(3*i+2)*/);
            restrictions->InsertNextTuple3(-1,0,0);
            colors->InsertNextTuple3(1,0,0);
        }

        // y
        if(s3d_mesh->nodes[i]->restrictions[1])
        {
            points->InsertNextPoint(
                        s3d_mesh->nodes[i]->coordinates[0]/*+amplification*s3d_mesh->u(3*i)*/,
                    s3d_mesh->nodes[i]->coordinates[1]/*+amplification*s3d_mesh->u(3*i+1)*/,
                    s3d_mesh->nodes[i]->coordinates[2]/*+amplification*s3d_mesh->u(3*i+2)*/);
            restrictions->InsertNextTuple3(0,1,0);
            colors->InsertNextTuple3(0,1,0);

            points->InsertNextPoint(
                        s3d_mesh->nodes[i]->coordinates[0]/*+amplification*s3d_mesh->u(3*i)*/,
                    s3d_mesh->nodes[i]->coordinates[1]/*+amplification*s3d_mesh->u(3*i+1)*/,
                    s3d_mesh->nodes[i]->coordinates[2]/*+amplification*s3d_mesh->u(3*i+2)*/);
            restrictions->InsertNextTuple3(0,-1,0);
            colors->InsertNextTuple3(0,1,0);
        }

        // z
        if(s3d_mesh->nodes[i]->restrictions[2])
        {
            points->InsertNextPoint(
                        s3d_mesh->nodes[i]->coordinates[0]/*+amplification*s3d_mesh->u(3*i)*/,
                    s3d_mesh->nodes[i]->coordinates[1]/*+amplification*s3d_mesh->u(3*i+1)*/,
                    s3d_mesh->nodes[i]->coordinates[2]/*+amplification*s3d_mesh->u(3*i+2)*/);
            restrictions->InsertNextTuple3(0,0,1);
            colors->InsertNextTuple3(0,0,1);

            points->InsertNextPoint(
                        s3d_mesh->nodes[i]->coordinates[0]/*+amplification*s3d_mesh->u(3*i)*/,
                    s3d_mesh->nodes[i]->coordinates[1]/*+amplification*s3d_mesh->u(3*i+1)*/,
                    s3d_mesh->nodes[i]->coordinates[2]/*+amplification*s3d_mesh->u(3*i+2)*/);
            restrictions->InsertNextTuple3(0,0,-1);
            colors->InsertNextTuple3(0,0,1);
        }
    }

    vtkSmartPointer<vtkPolyData> dataSet =
            vtkSmartPointer<vtkPolyData>::New();
    dataSet->SetPoints(points);

    dataSet->GetPointData()->SetVectors(restrictions);
    dataSet->GetPointData()->SetScalars(colors);


    vtkSmartPointer<vtkConeSource> cone =
            vtkSmartPointer<vtkConeSource>::New();
    cone->SetResolution(32);
    cone->SetRadius(0.15);

    double scale = 0.4*refSize;

    vtkSmartPointer<vtkGlyph3D> glyphs =
            vtkSmartPointer<vtkGlyph3D>::New();
    glyphs->SetInputData(dataSet);
    glyphs->SetSourceConnection(cone->GetOutputPort());
    glyphs->ScalingOn();
    glyphs->SetScaleModeToScaleByVector();
    glyphs->SetScaleFactor(scale);
    glyphs->OrientOn();
    glyphs->ClampingOn();
    glyphs->SetVectorModeToUseVector();
    glyphs->SetIndexModeToOff();

    vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(glyphs->GetOutputPort());
    //glyphMapper->SetColorModeToDirectScalars();
    //glyphMapper->ScalarVisibilityOff();

    restrictionActor = vtkSmartPointer<vtkActor>::New();
    restrictionActor->SetMapper(glyphMapper);
    restrictionActor->GetProperty()->SetColor(0.0, 0.4, 1.0);

}

void vtkGraphicWindow::setupInitialModel(void)
{
    if(s3d_mesh==nullptr)
        return;


    // Actor
    initialModelActor = vtkSmartPointer<vtkActor>::New();

    // Mapper
    vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputData(dataSet_0);
    mapper->ScalarVisibilityOff();
    initialModelActor->SetMapper(mapper);
    initialModelActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
    initialModelActor->GetProperty()->SetOpacity(0.25);

    if(showElements)
    {
        initialModelActor->GetProperty()->SetEdgeColor(0.5, 0.5, 0.8);
        initialModelActor->GetProperty()->EdgeVisibilityOn();
    }
    if(showNodes)
    {
        initialModelActor->GetProperty()->SetVertexColor(0.7, 0.7, 1.0);
        initialModelActor->GetProperty()->VertexVisibilityOn();
        initialModelActor->GetProperty()->SetPointSize(5.);
    }



    //    vtkSmartPointer<vtkStructuredGridOutlineFilter> outline = vtkSmartPointer<vtkStructuredGridOutlineFilter>::New();
    //    outline->SetInputData(dataSet_0);

    //    // Actor
    //    outlineActor = vtkSmartPointer<vtkActor>::New();

    //    // Mapper
    //    vtkSmartPointer<vtkPolyDataMapper> mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    //    mapper2->SetInputConnection(outline->GetOutputPort());
    //    //mapper2->ScalarVisibilityOff();
    //    outlineActor->SetMapper(mapper2);
    //    outlineActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    //    //initialModelActor->GetProperty()->SetOpacity(0.25);





}

void vtkGraphicWindow::addComplements(void)
{
    if(showLoading)
        m_renderer->AddActor(loadingActor);
    if(showRestrictions)
        m_renderer->AddActor(restrictionActor);
    if(showUndeformedModel)
        m_renderer->AddActor(initialModelActor);

    //m_renderer->AddActor(outlineActor);
}


void vtkGraphicWindow::planeCutter(void)
{

    if(s3d_mesh==nullptr || s3d_mesh->isSolved==false)
    {
        MsgLog::error(QString("Model was not solved."));
        return;
    }

    removeDataSet();
    vtkSmartPointer< vtkPoints > points =
            vtkSmartPointer< vtkPoints > :: New();

    for(int i=0; i<s3d_mesh->nNodes; i++)
        points->InsertNextPoint(
                    s3d_mesh->nodes[i]->coordinates[0]+amplification*s3d_mesh->u(3*i),
                s3d_mesh->nodes[i]->coordinates[1]+amplification*s3d_mesh->u(3*i+1),
                s3d_mesh->nodes[i]->coordinates[2]+amplification*s3d_mesh->u(3*i+2));

    dataSet_1 = vtkSmartPointer<vtkUnstructuredGrid>::New();
    dataSet_1->SetPoints(points);

    for(int i=0; i<s3d_mesh->nElements; i++)
    {
        vtkIdType ptIds[] = {s3d_mesh->elements[i]->nodes[0]->index,
                             s3d_mesh->elements[i]->nodes[1]->index,
                             s3d_mesh->elements[i]->nodes[2]->index,
                             s3d_mesh->elements[i]->nodes[3]->index};

        dataSet_1->InsertNextCell( VTK_TETRA, 4, ptIds );
    }

    vtkSmartPointer<vtkLookupTable> lut =
            vtkSmartPointer<vtkLookupTable>::New();

    lut->SetTableRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));
    lut->SetHueRange(2./3.,0.);
    lut->Build();

    vtkSmartPointer<vtkDoubleArray> scalars =
            vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(s3d_mesh->nNodes);

    for(int i=0; i<s3d_mesh->nNodes; i++)
        scalars->SetValue(i, s3d_mesh->Snodes(i,s3d_result));

    dataSet_1->GetPointData()->SetScalars(scalars);

    double *bounds = dataSet_1->GetBounds();

    QVector3D p0 = QVector3D(bounds[0], bounds[2], bounds[4]);
    QVector3D p1 = QVector3D(bounds[1], bounds[3], bounds[5]);

    double l = (p1-p0).length();

    QVector3D p = 0.5*(p1+p0)+ 0.5*l*cutterNormalPlane.normalized()*cutterPosition;


    // Create a plane to cut
    vtkSmartPointer<vtkPlane> plane =
            vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(p.x(), p.y(), p.z());
    plane->SetNormal(cutterNormalPlane.x(), cutterNormalPlane.y(), cutterNormalPlane.z());

    // Create cutter
    vtkSmartPointer<vtkCutter> cutter =
            vtkSmartPointer<vtkCutter>::New();
    cutter->SetCutFunction(plane);
    cutter->SetInputData(dataSet_1);
    cutter->Update();

    vtkSmartPointer<vtkPolyDataMapper> cutterMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
    cutterMapper->SetInputConnection( cutter->GetOutputPort());
    cutterMapper->SetLookupTable(lut);
    cutterMapper->SetScalarRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));

    // Create plane actor
    vtkSmartPointer<vtkActor> planeActor =
            vtkSmartPointer<vtkActor>::New();
    //planeActor->GetProperty()->SetColor(1.0,1,0);
    //planeActor->GetProperty()->SetLineWidth(2);
    planeActor->SetMapper(cutterMapper);

    vtkSmartPointer<vtkClipDataSet> clipperLeft =
            vtkSmartPointer<vtkClipDataSet>::New();

    clipperLeft->SetInputData(dataSet_1);
    clipperLeft->SetClipFunction(plane.Get());
    clipperLeft->Update();

    vtkSmartPointer<vtkDataSetMapper> clipperMapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    clipperMapper->SetInputConnection( clipperLeft->GetOutputPort());
    clipperMapper->SetLookupTable(lut);
    clipperMapper->SetScalarRange(s3d_mesh->Smin(s3d_result), s3d_mesh->Smax(s3d_result));

    vtkSmartPointer<vtkActor> clipperActor =
            vtkSmartPointer<vtkActor>::New();
    //planeActor->GetProperty()->SetColor(1.0,1,0);
    //planeActor->GetProperty()->SetLineWidth(2);
    clipperActor->SetMapper(clipperMapper);

    if(showElements)
    {
        clipperActor->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
        clipperActor->GetProperty()->EdgeVisibilityOn();
        planeActor->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
        planeActor->GetProperty()->EdgeVisibilityOn();
    }
    if(showNodes)
    {
        clipperActor->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
        clipperActor->GetProperty()->VertexVisibilityOn();
        clipperActor->GetProperty()->SetPointSize(5.);
        planeActor->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
        planeActor->GetProperty()->VertexVisibilityOn();
        planeActor->GetProperty()->SetPointSize(5.);
    }



    if(showTranslucentModel)
    {
        // Actor
        vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();

        // Mapper
        vtkSmartPointer<vtkDataSetMapper> mapper2 = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper2->SetInputData(dataSet_1);
        mapper2->ScalarVisibilityOff();
        actor2->SetMapper(mapper2);

        actor2->GetProperty()->SetColor(1.0, 1.0, 1.0);

        actor2->GetProperty()->SetOpacity(0.25);

        if(showElements)
        {
            actor2->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
            actor2->GetProperty()->EdgeVisibilityOn();
            //actor2->GetProperty()->SetLineWidth(2.f);
        }
        if(showNodes)
        {
            actor2->GetProperty()->SetVertexColor(0.9, 0.9, 1.0);
            actor2->GetProperty()->VertexVisibilityOn();
            actor2->GetProperty()->SetPointSize(5.);
        }

        m_renderer->AddActor(actor2);
    }

    scalarBar->SetLookupTable(lut);
    scalarBar->SetTitle(strResults[s3d_result]);

    //m_renderer->AddActor(actor_2);

    m_renderer->AddActor(planeActor);
    if(showLeftPart)
        m_renderer->AddActor(clipperActor);

    m_renderer->ResetCamera(dataSet_1->GetBounds());

    MsgLog::information(QString("Cutter plane rendered"));
    MsgLog::information(QString("Origin: %1, %2, %3 - Normal: %4, %5, %6")
                        .arg(p.x()).arg(p.y()).arg(p.z())
                        .arg(cutterNormalPlane.x()).arg(cutterNormalPlane.y()).arg(cutterNormalPlane.z()));
    MsgLog::result(QString("Color map of %1").arg(QString(strResults[s3d_result]).replace("\n", "")));

    addComplements();
    renderVTK();

}

void vtkGraphicWindow::reset(void)
{
    s3d_mesh = nullptr;
    removeDataSet();

    //    delete simulation_actors;

    //    initialModelActor->Delete();
    //    loadingActor->Delete();
    //    restrictionActor->Delete();
    //    outlineActor->Delete();
    //    actor_0->Delete();
    //    actor_1->Delete();
    //    dataSet_0->Delete();
    //    dataSet_1->Delete();
}

