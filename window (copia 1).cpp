#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#include "window.h"
#include <QApplication>
#include <QScreen>
#include <QFile>
#include <QStringList>
#include <QRegExp>
#include <QBrush>

Window::Window(QWidget *parent)
    : QMainWindow(parent),
      chart(new QChart()),
      series(new QLineSeries()),
      axisX(new QValueAxis()),
      axisY(new QValueAxis()),
      timer(new QTimer(this)),
      timeCounter(0.0),
      totalMemoryMB(0.0)
{
    setWindowTitle("Monitor RAM");

    // Dimensioni: a tutto schermo
    resize(QApplication::primaryScreen()->size());

    // --- Calcola memoria totale ---
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status))
        totalMemoryMB = status.ullTotalPhys / (1024.0 * 1024.0);
    else
        totalMemoryMB = 0.0;
#else
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString memTotalLine = file.readLine();
        QStringList totalParts = memTotalLine.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if (totalParts.size() >= 2)
            totalMemoryMB = totalParts[1].toDouble() / 1024.0;
        else
            totalMemoryMB = 0.0;
    } else {
        totalMemoryMB = 0.0;
    }
#endif

    // --- Imposta il grafico ---
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setBackgroundBrush(Qt::black);
    chart->setTitleBrush(QBrush(Qt::white));
    chart->setTitle(QString("RAM totale: %1 MB").arg(totalMemoryMB, 0, 'f', 0));

    axisX->setRange(0, 60);
    axisX->setLabelFormat("%g");
    axisX->setTitleText("Tempo (x 0.1s)");
    axisX->setLabelsBrush(Qt::white);
    axisX->setTitleBrush(Qt::white);

    axisY->setRange(0, 10000);
    axisY->setTitleText("RAM usata (MB)");
    axisY->setLabelsBrush(Qt::white);
    axisY->setTitleBrush(Qt::white);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    series->setColor(Qt::green);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    setCentralWidget(chartView);

    // --- Timer aggiornamento ---
    connect(timer, &QTimer::timeout, this, &Window::updateMemoryUsage);
    timer->start(100);
}

Window::~Window() {}

void Window::updateMemoryUsage() {
    double usedMB = getUsedMemoryMB();

    series->append(timeCounter, usedMB);
    timeCounter += 0.1;

    if (series->count() > 600) {
        series->removePoints(0, 1);
        axisX->setRange(timeCounter - 60, timeCounter);
    }

    if (usedMB > axisY->max())
        axisY->setMax(usedMB + 500);
}

double Window::getUsedMemoryMB() {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status))
        return (status.ullTotalPhys - status.ullAvailPhys) / (1024.0 * 1024.0);
    else
        return 0.0;
#else
    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0.0;

    QString memTotalLine = file.readLine();
    QString memAvailableLine;

    for (int i = 0; i < 2; ++i)
        memAvailableLine = file.readLine();

    QStringList totalParts = memTotalLine.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    QStringList availableParts = memAvailableLine.split(QRegExp("\\s+"), Qt::SkipEmptyParts);

    if (totalParts.size() < 2 || availableParts.size() < 2)
        return 0.0;

    double total = totalParts[1].toDouble();
    double available = availableParts[1].toDouble();
    double used = total - available;

    return used / 1024.0;
#endif
}
