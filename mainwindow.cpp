#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    srand(QDateTime::currentDateTime().toTime_t());
    ui->setupUi(this);

    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iSelectPlottables);
    ui->customPlot->xAxis->setRange(-8, 8);
    ui->customPlot->yAxis->setRange(-5, 5);
    ui->customPlot->axisRect()->setupFullAxesBox();

    ui->customPlot->plotLayout()->insertRow(0);
    MainWindow::title = new QCPTextElement(ui->customPlot, "Случайный график", QFont("sans", 17, QFont::Bold));
    ui->customPlot->plotLayout()->addElement(0, 0, MainWindow::title);

    ui->customPlot->xAxis->setLabel("Ось X");
    ui->customPlot->yAxis->setLabel("Ось Y");

    addRandomGraph();
    ui->customPlot->rescaleAxes();

    // connect slot that ties some axis selections together (especially opposite axes):
    connect(ui->customPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
    connect(ui->customPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress()));
    connect(ui->customPlot, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));

    // make bottom and left axes transfer their ranges to top and right axes:
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->yAxis2, SLOT(setRange(QCPRange)));

    // connect some interaction slots:
    connect(ui->customPlot, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)), this, SLOT(axisLabelDoubleClick(QCPAxis*,QCPAxis::SelectablePart)));
    connect(MainWindow::title, SIGNAL(doubleClicked(QMouseEvent*)), this, SLOT(titleDoubleClick(QMouseEvent*)));

    // connect slot that shows a message in the status bar when a graph is clicked:
    connect(ui->customPlot, SIGNAL(plottableDoubleClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphDoubleClick(QCPAbstractPlottable*,int)));
    // connect menu actions
    connect(ui->actionDelete, SIGNAL(triggered(bool)), this, SLOT(removeAllGraphs()));
    connect(ui->actionAddRandom, SIGNAL(triggered(bool)), this, SLOT(addRandomGraph()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::titleDoubleClick(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (QCPTextElement *title = qobject_cast<QCPTextElement*>(sender()))
    {
        // Set the plot title by double clicking on it
        bool ok;
        QString newTitle = QInputDialog::getText(this, "Новое имя графика", "Новое имя графика:", QLineEdit::Normal, title->text(), &ok);
        if (ok)
        {
            title->setText(newTitle);
            ui->customPlot->replot();
        }
    }
}

void MainWindow::axisLabelDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
    // Set an axis label by double clicking on it
    if (part == QCPAxis::spAxisLabel) // only react when the actual axis label is clicked, not tick label or axis backbone
    {
        bool ok;
        QString newLabel = QInputDialog::getText(this, "Новое имя оси", "Новое имя оси:", QLineEdit::Normal, axis->label(), &ok);
        if (ok)
        {
            axis->setLabel(newLabel);
            ui->customPlot->replot();
        }
    }
}


void MainWindow::selectionChanged()
{
    /*
    normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
    the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
    and the axis base line together. However, the axis label shall be selectable individually.
   
    The selection state of the left and right axes shall be synchronized as well as the state of the
    bottom and top axes.
    */

    // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
    if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
            ui->customPlot->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
        ui->customPlot->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
        ui->customPlot->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    }
    // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
    if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
            ui->customPlot->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
        ui->customPlot->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
        ui->customPlot->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    }
}

void MainWindow::mousePress()
{
    // if an axis is selected, only allow the direction of that axis to be dragged
    // if no axis is selected, both directions may be dragged

    if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        ui->customPlot->axisRect()->setRangeDrag(ui->customPlot->xAxis->orientation());
    else if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        ui->customPlot->axisRect()->setRangeDrag(ui->customPlot->yAxis->orientation());
    else
        ui->customPlot->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::mouseWheel()
{
    // if an axis is selected, only allow the direction of that axis to be zoomed
    // if no axis is selected, both directions may be zoomed

    if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        ui->customPlot->axisRect()->setRangeZoom(ui->customPlot->xAxis->orientation());
    else if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        ui->customPlot->axisRect()->setRangeZoom(ui->customPlot->yAxis->orientation());
    else
        ui->customPlot->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::addGraph()
{
    MainWindow::currentGraph.plotted = true;
    MainWindow::title->setText(MainWindow::currentGraph.title);
    ui->customPlot->xAxis->setLabel(MainWindow::currentGraph.xaxisname);
    ui->customPlot->yAxis->setLabel(MainWindow::currentGraph.yaxisname);
    ui->customPlot->addGraph();
    ui->customPlot->graph()->setData(MainWindow::currentGraph.x, MainWindow::currentGraph.y);
    QCPScatterStyle scatter;
    scatter.setShape(QCPScatterStyle::ssCircle);
    scatter.setPen(QPen(Qt::blue));
    scatter.setBrush(Qt::white);
    scatter.setSize(5);
    ui->customPlot->graph()->setPen(QPen(Qt::green));
    ui->customPlot->graph()->setScatterStyle(QCPScatterStyle(scatter));
    ui->customPlot->graph()->setSelectable(QCP::stSingleData);
    ui->customPlot->rescaleAxes();
    ui->customPlot->replot();
}

void MainWindow::addRandomGraph()
{
    int n = 100; // number of points in graph
    double xScale = (rand()/(double)RAND_MAX + 0.5)*2;
    double yScale = (rand()/(double)RAND_MAX + 0.5)*2;
    double xOffset = (rand()/(double)RAND_MAX - 0.5)*4;
    double yOffset = (rand()/(double)RAND_MAX - 0.5)*10;
    double r1 = (rand()/(double)RAND_MAX - 0.5)*2;
    double r2 = (rand()/(double)RAND_MAX - 0.5)*2;
    double r3 = (rand()/(double)RAND_MAX - 0.5)*2;
    double r4 = (rand()/(double)RAND_MAX - 0.5)*2;
    for (int i=0; i<n; i++)
    {
        MainWindow::currentGraph.x.append((i/(double)n-0.5)*10.0*xScale + xOffset);
        MainWindow::currentGraph.y.append((qSin(MainWindow::currentGraph.x[i]*r1*5)*qSin(qCos(MainWindow::currentGraph.x[i]*r2)*r4*3)+r3*qCos(qSin(MainWindow::currentGraph.x[i])*r4*2))*yScale + yOffset);
    }
    if (not MainWindow::currentGraph.plotted){
        this->addGraph();
    } else {
        QMessageBox::warning(this, "Внимание","Сначала удалите график");
    }
}

void MainWindow::removeAllGraphs()
{
    MainWindow::currentGraph.x.clear();
    MainWindow::currentGraph.y.clear();
    MainWindow::currentGraph.plotted = false;
    ui->customPlot->clearGraphs();
    ui->customPlot->replot();
}

void MainWindow::graphDoubleClick(QCPAbstractPlottable *plottable, int dataIndex)
{
    double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
    bool ok;
    double newDataValue = QInputDialog::getText(this, "Изменение значения", "Новое значение:", QLineEdit::Normal, QString::number(dataValue), &ok).toDouble();
    if (ok)
    {
        MainWindow::currentGraph.y[dataIndex] = newDataValue;
        ui->customPlot->graph()->setData(MainWindow::currentGraph.x, MainWindow::currentGraph.y);
        ui->customPlot->replot();
    }
}