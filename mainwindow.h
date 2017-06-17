#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QInputDialog>
#include "qcustomplot.h"

struct Graph {
    bool plotted = false;
    QString title = "Случайный график", xaxisname = "Ось X", yaxisname = "Ось Y";
    QVector<double> x, y_mean, y_min, y_max, student;
    QVector<double> xd, yd;
    QVector<QVector<double>> graphdata;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QCPTextElement *title;

private slots:
    void graphDoubleClick(QCPAbstractPlottable *plottable, int dataIndex);
    void titleDoubleClick(QMouseEvent *event);
    void axisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
    void selectionChanged();
    void mousePress();
    void mouseWheel();
    void addRandomGraph();
    void removeAllGraphs();    
    void addGraph();
    void saveGraph();
    void loadGraph();
    void saveScreenshot();
    double calculateExpectedValue(QVector<double> values);
    double calculateStudent(QVector<double> values);
    void plotDistrPlot();

private:
    Ui::MainWindow *ui;
    Graph currentGraph;
};

#endif // MAINWINDOW_H
