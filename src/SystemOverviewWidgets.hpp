#pragma once

#include "./systemstats.hpp"

#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

using namespace QtCharts;

class CpuUsageWidget : public ContainerWidget {
private:
    SystemStats sysstats;
    QLineSeries* m_cpuSeries;
    QDateTimeAxis* m_cpuXAxis;
    QWidget* createCpuChart() {
        // Create the line series for the CPU usage data
        m_cpuSeries = new QLineSeries();

        // Create the chart and add the line series to it
        SystemThemedChart* cpuChart = new SystemThemedChart();
        cpuChart->legend()->hide();
        cpuChart->addLineSeriesWithArea(m_cpuSeries);
        cpuChart->setTitle("CPU Usage");

        // Create the X-axis and set it to a time axis
        m_cpuXAxis = new QDateTimeAxis();
        m_cpuXAxis->setTickCount(10);
        m_cpuXAxis->setFormat("hh:mm:ss");
        m_cpuXAxis->setLabelsVisible(false);
        cpuChart->addAxis(m_cpuXAxis, Qt::AlignBottom);
        m_cpuSeries->attachAxis(m_cpuXAxis);

        // Create the Y-axis and set the range
        QValueAxis* cpuYAxis = new QValueAxis();
        cpuYAxis->setLabelFormat("%.0f%%");
        cpuYAxis->setRange(0, 100);
        cpuChart->addAxis(cpuYAxis, Qt::AlignLeft);
        m_cpuSeries->attachAxis(cpuYAxis);

        // Create the chart view and add it to the layout
        QChartView* cpuChartView = new QChartView(cpuChart);
        cpuChartView->setRenderHint(QPainter::Antialiasing);
        return cpuChartView;
    }

    void updateData() {
        // Update CPU chart
        int cpuUsage = sysstats.getCpuUsage();
        m_cpuSeries->append(QDateTime::currentDateTime().toMSecsSinceEpoch(), cpuUsage);
        if (m_cpuSeries->count() > m_maxDataPoints) m_cpuSeries->remove(0);
        qint64 cpuMaxX = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qint64 cpuMinX = cpuMaxX - m_xAxisRangeMs;
        m_cpuXAxis->setRange(QDateTime::fromMSecsSinceEpoch(cpuMinX), QDateTime::fromMSecsSinceEpoch(cpuMaxX));
    }

public:
    int m_maxDataPoints = 600;
    qint64 m_xAxisRangeMs = 60000; // 1 minute
    CpuUsageWidget(QWidget* parent = nullptr) : ContainerWidget(parent) { setWidget(createCpuChart()); }
    void attachTo(QLambdaTimer& t){
        t.addLambda([&](){updateData();});
    }
};

class MemoryUsageWidget : public ContainerWidget {
private:
    SystemStats sysstats;
    QLineSeries* m_memorySeries;
    QDateTimeAxis* m_memoryXAxis;
    QWidget* createMemoryChart() {
        // Create the line series for the memory usage data
        m_memorySeries = new QLineSeries();

        // Create the chart and add the line series to it
        SystemThemedChart* memoryChart = new SystemThemedChart();
        memoryChart->legend()->hide();
        memoryChart->addLineSeriesWithArea(m_memorySeries);
        memoryChart->setTitle("Memory Usage");

        // Create the X-axis and set it to a time axis
        m_memoryXAxis = new QDateTimeAxis();
        m_memoryXAxis->setTickCount(10);
        m_memoryXAxis->setFormat("hh:mm:ss");
        m_memoryXAxis->setLabelsVisible(false);
        memoryChart->addAxis(m_memoryXAxis, Qt::AlignBottom);
        m_memorySeries->attachAxis(m_memoryXAxis);

        // Create the Y-axis and set the range
        QValueAxis* memoryYAxis = new QValueAxis();
        memoryYAxis->setLabelFormat("%.0f%%");
        memoryYAxis->setRange(0, 100);
        memoryChart->addAxis(memoryYAxis, Qt::AlignLeft);
        m_memorySeries->attachAxis(memoryYAxis);

        // Create the chart view and return it
        QChartView* memoryChartView = new QChartView(memoryChart);
        memoryChartView->setRenderHint(QPainter::Antialiasing);
        return memoryChartView;
    }

    void updateData() {
        int memoryUsage = sysstats.getMemoryUsage();
        m_memorySeries->append(QDateTime::currentDateTime().toMSecsSinceEpoch(), memoryUsage);
        if (m_memorySeries->count() > m_maxDataPoints) m_memorySeries->remove(0);
        qint64 memoryMaxX = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qint64 memoryMinX = memoryMaxX - m_xAxisRangeMs;
        m_memoryXAxis->setRange(QDateTime::fromMSecsSinceEpoch(memoryMinX), QDateTime::fromMSecsSinceEpoch(memoryMaxX));
    }

public:
    int m_maxDataPoints = 600;
    qint64 m_xAxisRangeMs = 60000; // 1 minute
    MemoryUsageWidget(QWidget* parent = nullptr) : ContainerWidget(parent) { setWidget(createMemoryChart()); }
    void attachTo(QLambdaTimer& t){
        t.addLambda([&](){updateData();});
    }
};

class OverviewWidget : public EQLayoutWidget<QVBoxLayout>
{
    private:
    MemoryUsageWidget* mem = new MemoryUsageWidget();
    CpuUsageWidget* cpu = new CpuUsageWidget();
public:
    OverviewWidget(QWidget *parent = nullptr) : EQLayoutWidget(parent)
    {
        setAutoFillBackground(true);
        layout->setSpacing(0);
        layout->addWidget(cpu);
        layout->addWidget(mem);
    }
    void attachTo(QLambdaTimer& t){
        cpu->attachTo(t);
        mem->attachTo(t);
    }
};