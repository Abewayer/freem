#include <QGuiApplication>
#include "window.h"
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDebug>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#else
#include <sys/sysinfo.h>
#include <signal.h>
#include <unistd.h>
#endif

Window::Window(QWidget *parent)
    : QMainWindow(parent)
{
    // Dimensione finestra
    QSize screenSize = QGuiApplication::primaryScreen()->availableGeometry().size();
    resize(screenSize.width() / 1, screenSize.height() / 2);

    // Widget centrale
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);

    // --- Colonna sinistra ---
    QVBoxLayout* leftLayout = new QVBoxLayout();

    totalMemLabel = new QLabel("Totale memoria: -- MB");
    usedMemLabel = new QLabel("Memoria in uso: -- MB");

    totalMemLabel->setStyleSheet("color: white;");
    usedMemLabel->setStyleSheet("color: white;");

    leftLayout->addWidget(totalMemLabel);
    leftLayout->addWidget(usedMemLabel);

    // --- Tabella processi ---
    processTable = new QTableWidget(10, 2);
    processTable->setHorizontalHeaderLabels(QStringList() << "Processo" << "Azione");
    // Fai sì che la colonna 0 (Processo) si allarghi e occupi lo spazio
    processTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    // Imposta la colonna 1 (Azione) a larghezza fissa o minima
    processTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    processTable->verticalHeader()->setVisible(false);
    processTable->horizontalHeader()->setStretchLastSection(true);
    processTable->setStyleSheet("background-color: #222; color: white;");
    leftLayout->addWidget(processTable);

    mainLayout->addLayout(leftLayout, 1);

    // --- Grafico ---
    barSet = new QBarSet("Memoria");
    *barSet << 0;

    QBarSeries* series = new QBarSeries();
    series->append(barSet);

    chart = new QChart();
    chart->addSeries(series);
    chart->setBackgroundBrush(QBrush(QColor("#222")));
    chart->setTitleBrush(QBrush(Qt::white));
    chart->setTitle("Uso Memoria");
    chart->legend()->hide();

    QStringList categories;
    categories << "Memoria";
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%d %%");
    axisY->setTitleText("Percentuale");
    axisY->setTitleBrush(QBrush(Qt::white));
    axisY->setLabelsBrush(QBrush(Qt::white));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background-color: #222;");
    mainLayout->addWidget(chartView, 2);

    // --- Timer ---
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Window::updateMemoryInfo);
    timer->start(1000);

    setStyleSheet("background-color: #222;");
}

Window::~Window() {}

void Window::updateMemoryInfo()
{
    quint64 totalMem = getTotalMemoryMB();
    quint64 usedMem = getUsedMemoryMB();

    totalMemLabel->setText(QString("Totale memoria: %1 MB").arg(totalMem));
    usedMemLabel->setText(QString("Memoria in uso: %1 MB").arg(usedMem));

    double percent = totalMem > 0 ? (double)usedMem / totalMem * 100.0 : 0.0;
    barSet->replace(0, percent);
    barSet->setColor(percent > 80 ? Qt::red : Qt::blue);

    // --- Ottieni i processi principali ---
    QList<QPair<quint64, QString>> topProcs = getTopProcesses();
    processTable->setRowCount(topProcs.size());

    for (int i = 0; i < topProcs.size(); ++i)
    {
        // Colonna 0: nome processo
        QTableWidgetItem *procItem = new QTableWidgetItem(topProcs[i].second);
        processTable->setItem(i, 0, procItem);

        // Colonna 1: pulsante "Blocca"
        QPushButton *killButton = new QPushButton("Blocca");
        killButton->setStyleSheet("background-color: #444; color: white;");
        processTable->setCellWidget(i, 1, killButton);

        // Collega il pulsante alla funzione di kill
        connect(killButton, &QPushButton::clicked, this, [this, pid = topProcs[i].first]() {
#ifdef Q_OS_WIN
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProcess) {
                TerminateProcess(hProcess, 1);
                CloseHandle(hProcess);
            }
#else
            QProcess::execute("kill", QStringList() << "-9" << QString::number(pid));
#endif
        });
    }

    processTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    processTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}


// --- Slot per il click sul bottone Blocca ---
void Window::handleKillButtonClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    quint64 pid = btn->property("pid").toULongLong();
    qDebug() << "Richiesta di terminare PID:" << pid;
    killProcessByPid(pid);
}

// --- Funzioni di sistema ---
quint64 Window::getTotalMemoryMB()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullTotalPhys / (1024 * 1024);
#else
    struct sysinfo info;
    sysinfo(&info);
    return info.totalram * info.mem_unit / (1024 * 1024);
#endif
}

quint64 Window::getUsedMemoryMB()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return (statex.ullTotalPhys - statex.ullAvailPhys) / (1024 * 1024);
#else
    struct sysinfo info;
    sysinfo(&info);
    return (info.totalram - info.freeram) * info.mem_unit / (1024 * 1024);
#endif
}

QList<QPair<quint64, QString>> Window::getTopProcesses()
{
    QList<QPair<quint64, QString>> procs;

#ifdef Q_OS_WIN
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        return procs;

    cProcesses = cbNeeded / sizeof(DWORD);
    QVector<QPair<DWORD, SIZE_T>> memUsage;

    for (unsigned int i = 0; i < cProcesses; i++)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
        if (hProcess)
        {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
                memUsage.append(qMakePair(aProcesses[i], pmc.WorkingSetSize));
            CloseHandle(hProcess);
        }
    }

    std::sort(memUsage.begin(), memUsage.end(), [](auto &a, auto &b){ return a.second > b.second; });

    for (int i = 0; i < qMin(10, memUsage.size()); ++i)
        procs.append(qMakePair((quint64)memUsage[i].first, QString("Processo")));
#else
    // Linux
    QProcess p;
    p.start("bash", QStringList() << "-c" << "ps -eo pid,comm,rss --sort=-rss | head -n 11");
    p.waitForFinished();
    QString output = p.readAllStandardOutput();
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);

    for (int i = 1; i < lines.size(); ++i)
    {
        QStringList parts = lines[i].simplified().split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 3)
        {
            quint64 pid = parts[0].toULongLong();
            QString name = parts[1];
            procs.append(qMakePair(pid, name));
        }
    }
#endif

    return procs;
}

void Window::killProcessByPid(quint64 pid)
{
#ifdef Q_OS_WIN
    QString command = QString("taskkill /PID %1 /F").arg(pid);
    QProcess::execute(command);
#else
    if (::kill((pid_t)pid, SIGKILL) != 0)
        qWarning() << "Errore nel terminare processo PID" << pid << ":" << strerror(errno);
#endif
}
