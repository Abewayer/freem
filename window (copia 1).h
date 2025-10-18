#ifndef WINDOW_H
#define WINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QTimer>

QT_CHARTS_USE_NAMESPACE

class Window : public QMainWindow {
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);
    ~Window();

private slots:
    void updateMemoryUsage();

private:
    QChart *chart;
    QLineSeries *series;
    QValueAxis *axisX;
    QValueAxis *axisY;
    QTimer *timer;
    qreal timeCounter;

    double getUsedMemoryMB(); // Legge la memoria reale dal sistema

    double totalMemoryMB;     // ✅ Aggiungi questa riga
};

#endif // WINDOW_H
