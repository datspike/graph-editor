#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMap>
#include <QList>
#include "float.h"
#include <math.h>
#include <random>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    srand(QDateTime::currentDateTime().toTime_t());
    ui->setupUi(this);

    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iSelectPlottables);
    ui->customPlot->xAxis->setRange(0, 10);
    ui->customPlot->yAxis->setRange(-1, 11);
    ui->customPlot->axisRect()->setupFullAxesBox();

    ui->customPlot->plotLayout()->insertRow(0);
    MainWindow::title = new QCPTextElement(ui->customPlot, "Случайный график", QFont("sans", 17, QFont::Bold));
    ui->customPlot->plotLayout()->addElement(0, 0, MainWindow::title);

    ui->customPlot->xAxis->setLabel("Номер измерения");
    ui->customPlot->yAxis->setLabel("Результат");

    addRandomGraph(3);
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
    connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(saveGraph()));
    connect(ui->actionOpen, SIGNAL(triggered(bool)), this, SLOT(loadGraph()));
    connect(ui->actionScreenshot, SIGNAL(triggered(bool)), this, SLOT(saveScreenshot()));

    QSignalMapper* signalMapper = new QSignalMapper (this) ;
    connect (ui->action_sin, SIGNAL(triggered()), signalMapper, SLOT(map())) ;
    connect (ui->action_sqrt, SIGNAL(triggered()), signalMapper, SLOT(map())) ;
    connect (ui->action_power, SIGNAL(triggered()), signalMapper, SLOT(map())) ;
    connect (ui->action_line, SIGNAL(triggered()), signalMapper, SLOT(map())) ;

    signalMapper -> setMapping (ui->action_sin, 0) ;
    signalMapper -> setMapping (ui->action_sqrt, 1) ;
    signalMapper -> setMapping (ui->action_power, 2) ;
    signalMapper -> setMapping (ui->action_line, 3) ;

    connect (signalMapper, SIGNAL(mapped(int)), this, SLOT(addRandomGraph(int))) ;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::saveScreenshot()
{
    QRect rectangle = QRect(QPoint(1,1), QPoint(ui->customPlot->width(),ui->customPlot->height()));
    QPixmap pixmap(rectangle.size());
    ui->customPlot->render(&pixmap, QPoint(), QRegion(rectangle));
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить скриншот графика"), QDir::homePath(), tr("Jpeg (*.jpg);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    else
    {
        pixmap.save(fileName);
    }
}

void MainWindow::saveGraph()
{
    QJsonObject infoObject;
    infoObject.insert("title", QJsonValue::fromVariant(MainWindow::title->text()));
    infoObject.insert("xaxisname", QJsonValue::fromVariant(ui->customPlot->xAxis->label()));
    infoObject.insert("yaxisname", QJsonValue::fromVariant(ui->customPlot->yAxis->label()));
    QJsonArray graphDataX;
    for (int i=0; i<MainWindow::currentGraph.x.length(); i++)
    {
        graphDataX.push_back(MainWindow::currentGraph.x[i]);
    }
    infoObject.insert("data_x", graphDataX);
    QJsonArray graphDataY;
    for (int i=0; i<MainWindow::currentGraph.y.length(); i++)
    {
        graphDataY.push_back(MainWindow::currentGraph.y[i]);
    }
    infoObject.insert("data_y", graphDataY);

    QJsonDocument json_doc(infoObject);

    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить данные графика"), QDir::homePath(), tr("Json (*.json);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    else
    {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::information(this, tr("Не удалось сохранить файл"), file.errorString());
            return;
        }
        QTextStream stream( &file );
        stream << json_doc.toJson() << endl;
    }
}

void MainWindow::loadGraph()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Открыть данные графика"), QDir::homePath(), tr("Json (*.json);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    else
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::information(this, tr("Не удалось открыть файл"), file.errorString());
            return;
        }
        QString json_text = file.readAll();
        file.close();
        MainWindow::removeAllGraphs();
        QJsonDocument json_doc = QJsonDocument::fromJson(json_text.toUtf8());
        QJsonObject infoObject = json_doc.object();
        MainWindow::currentGraph.title = infoObject["title"].toString();
        MainWindow::currentGraph.xaxisname = infoObject["xaxisname"].toString();
        MainWindow::currentGraph.yaxisname = infoObject["yaxisname"].toString();
        QJsonArray data_x = infoObject["data_x"].toArray();
        QJsonArray data_y = infoObject["data_y"].toArray();
        QJsonValue item;
        foreach (item, data_x) {
            MainWindow::currentGraph.x.append(item.toDouble());
        }
        foreach (item, data_y) {
            MainWindow::currentGraph.y.append(item.toDouble());
        }
        MainWindow::addGraph();
    }
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
            calculateExpectedValue();
            calculateStudent();
            plotDistrPlot();
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
            calculateExpectedValue();
            calculateStudent();
            plotDistrPlot();
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
    calculateExpectedValue();
    calculateStudent();
    plotDistrPlot();
}

void MainWindow::plotDistrPlot()
{
    ui->plotDistribution->clearGraphs();
    ui->plotDistribution->addGraph();
    ui->plotDistribution->graph()->setData(MainWindow::currentGraph.xd, MainWindow::currentGraph.yd);
    QCPScatterStyle scatter;
    scatter.setShape(QCPScatterStyle::ssCircle);
    scatter.setPen(QPen(Qt::blue));
    scatter.setBrush(Qt::white);
    scatter.setSize(5);
    ui->plotDistribution->graph()->setPen(QPen(Qt::green));
    ui->plotDistribution->graph()->setScatterStyle(QCPScatterStyle(scatter));
    ui->plotDistribution->graph()->setSelectable(QCP::stSingleData);
    ui->plotDistribution->rescaleAxes();
    ui->plotDistribution->replot();
}

void MainWindow::addRandomGraph(int type)
{
    int n = 100; // number of points in graph
    double xScale = (rand()/(double)RAND_MAX + 0.5)*2;
    double yScale = (rand()/(double)RAND_MAX + 0.5)*2;
    double xOffset = (rand()/(double)RAND_MAX - 0.5)*4;
    double yOffset = (rand()/(double)RAND_MAX + 0.5)*10;
    double r1 = (rand()/(double)RAND_MAX - 0.5)*2;
    double r2 = (rand()/(double)RAND_MAX - 0.5)*2;
    double r3 = (rand()/(double)RAND_MAX - 0.5)*2;
    double r4 = (rand()/(double)RAND_MAX - 0.5)*2;
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(yOffset, 0.5);
    for (int i=0; i<n; i++)
    {
        switch(type) {
        // sin
        case 0:
            MainWindow::currentGraph.x.append((i/(double)n-0.5)*10.0*xScale + xOffset);
            MainWindow::currentGraph.y.append((qSin(MainWindow::currentGraph.x[i]*r1*5)*qSin(qCos(MainWindow::currentGraph.x[i]*r2)*r4*3)+r3*qCos(qSin(MainWindow::currentGraph.x[i])*r4*2))*yScale + yOffset);
            break;
        // sqrt
        case 1:
            MainWindow::currentGraph.x.append((i/(double)n-0.5)*10.0*xScale + xOffset);
            MainWindow::currentGraph.y.append(sqrt(MainWindow::currentGraph.x[i])*yScale + yOffset);
            break;
        // power
        case 2:
            MainWindow::currentGraph.x.append((i/(double)n-0.5)*10.0*xScale + xOffset);
            MainWindow::currentGraph.y.append(pow(MainWindow::currentGraph.x[i],2)*yScale + yOffset);
            break;
        // line
        case 3:
            MainWindow::currentGraph.x.append(i);
            MainWindow::currentGraph.y.append(distribution(generator));
            break;
        }
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
    MainWindow::currentGraph.xd.clear();
    MainWindow::currentGraph.yd.clear();
    MainWindow::currentGraph.plotted = false;
    ui->customPlot->clearGraphs();
    ui->customPlot->replot();
    calculateExpectedValue();
    calculateStudent();
    plotDistrPlot();
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
        calculateExpectedValue();
        calculateStudent();
    }
}

double MainWindow::calculateExpectedValue(QVector<double> values)
{
    double result = 0;
    double max = -DBL_MAX;
    double min = DBL_MAX;
    double item;
    foreach(item, values)
    {
        if(item>max)
            max = item;
        if(item<min)
            min = item;
    }
    double delta = (max - min) / 10;
    double p[10];
    MainWindow::currentGraph.yd.clear();
    MainWindow::currentGraph.xd.clear();
    QMap<double, int> groups;;
    for(int i=0;i<10;i++)
    {
        int temp = 0;
        foreach(item, values)
        {
            if(item>min+i*delta && item<=min+(i+1)*delta)
            {
                temp++;
                groups.insert(item, i);
            }
        }
        if(i==0)
            temp++;
        p[i] = temp/(double)(values.length());
        MainWindow::currentGraph.yd.append(p[i]);
        MainWindow::currentGraph.xd.append(i);
    }

    for(int i=0;i<10;i++)
    {
        int temp = 0;
        foreach(item, values)
        {
            if(item>min+i*delta && item<=min+(i+1)*delta)
            {
                temp++;
                groups.insert(item, i);
            }
        }
        if(i==0)
            temp++;
        p[i] = temp/(double)(values.length());
        if(MainWindow::currentGraph.yd.length()<10)
        {
            MainWindow::currentGraph.yd.append(p[i]);
            MainWindow::currentGraph.xd.append(i);
        }
        result += (min + i*delta + delta/2) * p[i];
    }

   /* foreach(item, MainWindow::currentGraph.y)
    {
        result += item * MainWindow::currentGraph.yd[groups.value(item)];
    }*/

    //ui->label_mat->setText(QString("Мат. ожидание: ").append(QString::number(result)));
    return result;
}

double MainWindow::calculateStudent(QVector<double> values)
{
    double mean,sum,squaredErrorsSum=0,meanSquaredError;
    int n=MainWindow::currentGraph.y.length();
    double item;
    foreach(item, MainWindow::currentGraph.y)
    {
        sum+=item;
    }
    mean = sum/n;

    QList<double> errors;
    foreach(item, MainWindow::currentGraph.y)
    {
        errors.append(mean-item);
        squaredErrorsSum+=(mean-item)*(mean-item);
    }

    meanSquaredError = sqrt(squaredErrorsSum/(n*(n-1)));

    double trustedInterval = meanSquaredError * 1.9840; //1.984 - Коэффициент для n=100 и надежности 0,95
    double percentErrorInterval = fabs(trustedInterval/mean*100);

    return trustedInterval;
    //ui->label_mean->setText(QString("Среднеквадратическое отклонение: ").append(QString::number(meanSquaredError)));
    //ui->label_final->setText("Абсолютное значение с учетом Стьюдента: " + QString::number(mean, 'f', 5).append(" ± ").append(QString::number(trustedInterval, 'f', 5)));
    //ui->label_percent->setText(QString("Относительная погрешность: ").append(QString::number(percentErrorInterval)).append("%"));
}
