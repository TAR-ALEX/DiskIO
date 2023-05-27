#include "AspectRatioWidget.hpp"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCore/QObject>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QtWidgets>
#include <estd/ptr.hpp>
#include <estd/string_util.h>
#include <estd/thread_pool.hpp>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <thread>
#include <vector>
#include <QtCharts/QDateTimeAxis>
namespace fs = std::filesystem;
QT_CHARTS_USE_NAMESPACE

class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        // Create the tab widget
        QTabWidget* tabWidget = new QTabWidget(this);

        // Create the CPU usage tab
        QWidget* cpuTab = new QWidget();
        createCpuTab(cpuTab);

        // Create the memory usage tab
        QWidget* memoryTab = new QWidget();
        createMemoryTab(memoryTab);

        // Add the tabs to the tab widget
        tabWidget->addTab(cpuTab, "CPU Usage");
        tabWidget->addTab(memoryTab, "Memory Usage");

        // Set the tab widget as the central widget
        setCentralWidget(tabWidget);

        // Set up the timer to update the charts
        m_timer = new QTimer(this);
        m_timer->setInterval(m_updateIntervalMs);
        connect(m_timer, &QTimer::timeout, this, [this]() {
            updateCharts();
        });
        m_timer->start();

        // Initialize the CPU timer and previous usage
        m_cpuTimer.start();
        m_previousTotalTime = 0;
        m_previousIdle = 0;
        m_previousTotalMemory = 0;
        m_previousFreeMemory = 0;
    }

private:
    void createCpuTab(QWidget* parent)
    {
        // Create the line series for the CPU usage data
        m_cpuSeries = new QLineSeries(parent);

        // Create the chart and add the line series to it
        QChart* cpuChart = new QChart();
        cpuChart->legend()->hide();
        cpuChart->addSeries(m_cpuSeries);
        cpuChart->setTitle("CPU Usage");

        // Create the X-axis and set it to a time axis
        m_cpuXAxis = new QDateTimeAxis();
        m_cpuXAxis->setTickCount(10);
        m_cpuXAxis->setFormat("hh:mm:ss");
        cpuChart->addAxis(m_cpuXAxis, Qt::AlignBottom);
        m_cpuSeries->attachAxis(m_cpuXAxis);

        // Create the Y-axis and set the range
        QValueAxis* cpuYAxis = new QValueAxis();
        cpuYAxis->setLabelFormat("%.0f%%");
        cpuYAxis->setRange(0, 100);
        cpuChart->addAxis(cpuYAxis, Qt::AlignLeft);
        m_cpuSeries->attachAxis(cpuYAxis);

        // Create the chart view and set it as the layout for the CPU tab
        QChartView* cpuChartView = new QChartView(cpuChart);
        cpuChartView->setRenderHint(QPainter::Antialiasing);

        QVBoxLayout* layout = new QVBoxLayout(parent);
        layout->addWidget(cpuChartView);
    }

    void createMemoryTab(QWidget* parent)
    {
        // Create the line series for the memory usage data
        m_memorySeries = new QLineSeries(parent);

        // Create the chart and add the line series to it
        QChart* memoryChart = new QChart();
        memoryChart->legend()->hide();
        memoryChart->addSeries(m_memorySeries);
        memoryChart->setTitle("Memory Usage");

        // Create the X-axis and set it to a time axis
        m_memoryXAxis = new QDateTimeAxis();
        m_memoryXAxis->setTickCount(10);
        m_memoryXAxis->setFormat("hh:mm:ss");
        memoryChart->addAxis(m_memoryXAxis, Qt::AlignBottom);
        m_memorySeries->attachAxis(m_memoryXAxis);

        // Create the Y-axis and set the range
        QValueAxis* memoryYAxis = new QValueAxis();
        memoryYAxis->setLabelFormat("%.0f%%");
        memoryYAxis->setRange(0, 100);
        memoryChart->addAxis(memoryYAxis, Qt::AlignLeft);
        m_memorySeries->attachAxis(memoryYAxis);

        // Create the chart view and set it as the layout for the memory tab
        QChartView* memoryChartView = new QChartView(memoryChart);
        memoryChartView->setRenderHint(QPainter::Antialiasing);

        QVBoxLayout* layout = new QVBoxLayout(parent);
        layout->addWidget(memoryChartView);
    }

    void updateCharts()
    {
        // Update CPU chart
        int cpuUsage = getCpuUsage();
        m_cpuSeries->append(QDateTime::currentDateTime().toMSecsSinceEpoch(), cpuUsage);
        if (m_cpuSeries->count() > m_maxDataPoints)
            m_cpuSeries->remove(0);
        qint64 cpuMaxX = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qint64 cpuMinX = cpuMaxX - m_xAxisRangeMs;
        m_cpuXAxis->setRange(QDateTime::fromMSecsSinceEpoch(cpuMinX), QDateTime::fromMSecsSinceEpoch(cpuMaxX));

        // Update Memory chart
        int memoryUsage = getMemoryUsage();
        m_memorySeries->append(QDateTime::currentDateTime().toMSecsSinceEpoch(), memoryUsage);
        if (m_memorySeries->count() > m_maxDataPoints)
            m_memorySeries->remove(0);
        qint64 memoryMaxX = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qint64 memoryMinX = memoryMaxX - m_xAxisRangeMs;
        m_memoryXAxis->setRange(QDateTime::fromMSecsSinceEpoch(memoryMinX), QDateTime::fromMSecsSinceEpoch(memoryMaxX));
    }

    int getCpuUsage()
    {
        QFile statFile("/proc/stat");
        if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return -1;

        QTextStream in(&statFile);
        QString line;
        do {
            line = in.readLine();
            if (line.startsWith("cpu ")) {
                QStringList values = line.split(' ', QString::SkipEmptyParts);
                if (values.size() >= 8) {
                    int user = values[1].toInt();
                    int nice = values[2].toInt();
                    int system = values[3].toInt();
                    int idle = values[4].toInt();

                    int totalTime = user + nice + system + idle;
                    int elapsedTime = m_cpuTimer.elapsed();

                    int deltaTime = totalTime - m_previousTotalTime;
                    int currentUsage = (deltaTime > 0) ? ((deltaTime - (idle - m_previousIdle)) * 100) / deltaTime : 0;

                    m_previousTotalTime = totalTime;
                    m_previousIdle = idle;

                    if (currentUsage < 0)
                        currentUsage = 0;
                    else if (currentUsage > 100)
                        currentUsage = 100;

                    statFile.close();
                    return currentUsage;
                }
            }
        } while (!line.isNull());

        statFile.close();

        return -1;
    }

    int getMemoryUsage()
{
    QFile meminfoFile("/proc/meminfo");
    if (!meminfoFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;

    QTextStream in(&meminfoFile);
    QString line;
    int totalMemory = 0;
    int freeMemory = 0;

    do {
        line = in.readLine();
        if (line.startsWith("MemTotal:")) {
            totalMemory = line.split(" ", QString::SkipEmptyParts)[1].toInt();
        } else if (line.startsWith("MemAvailable:")) {
            freeMemory = line.split(" ", QString::SkipEmptyParts)[1].toInt();
        }
    } while (!line.isNull());

    meminfoFile.close();

    if (totalMemory > 0) {
        int usedMemory = totalMemory - freeMemory;
        int currentUsage = (usedMemory * 100) / totalMemory;

        if (currentUsage < 0)
            currentUsage = 0;
        else if (currentUsage > 100)
            currentUsage = 100;

        return currentUsage;
    }

    return -1;
}

private:
    QLineSeries* m_cpuSeries;
    QLineSeries* m_memorySeries;
    QChart* m_cpuChart;
    QChart* m_memoryChart;
    QTimer* m_timer;
    QDateTimeAxis* m_cpuXAxis;
    QDateTimeAxis* m_memoryXAxis;
    const int m_maxDataPoints = 100;
    const int m_updateIntervalMs = 1000;
    const qint64 m_xAxisRangeMs = 60000; // 1 minute
    QElapsedTimer m_cpuTimer;
    int m_previousTotalTime;
    int m_previousIdle;
    int m_previousTotalMemory;
    int m_previousFreeMemory;
};

int main2(int argc, char* argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}