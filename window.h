#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>

QT_CHARTS_USE_NAMESPACE

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);
    ~Window();

private slots:
    void updateMemoryInfo();
    void handleKillButtonClicked();  // Slot per gestire il click sul bottone Blocca

private:
    QLabel* totalMemLabel;
    QLabel* usedMemLabel;
    QChart* chart;
    QBarSet* barSet;
    QChartView* chartView;
    QTableWidget* processTable;
    QTimer* timer;

    quint64 getTotalMemoryMB();
    quint64 getUsedMemoryMB();

    QList<QPair<quint64, QString>> getTopProcesses(); // Restituisce PID + nome processo
    void killProcessByPid(quint64 pid);               // Termina un processo
};

#endif // WINDOW_H
