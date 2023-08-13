#pragma once

#include "./systemstats.hpp"

#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

using namespace QtCharts;

class ValueUsageWidget : public ContainerWidget {
private:
    std::function<std::vector<double>()> getDataFunction;
    std::vector<QLineSeries*> m_series;
    QDateTimeAxis* m_xAxis;
    QString title;
    QValueAxis* m_yAxis;

    QWidget* createChart(QList<QColor> colors = QList<QColor>{}) {
        // Create the line series for the data
        for (size_t i = 0; i < getDataFunction().size(); i++) { m_series.push_back(new QLineSeries()); }

        // Create the chart and add the line series to it
        SystemThemedChart* chart = new SystemThemedChart();
        chart->legend()->hide();
        chart->setTitle(title);
        if(colors.size() != 0)
            chart->sequentialColors = colors;
        for (auto* series : m_series) { chart->addLineSeriesWithArea(series); }

        // Create the X-axis and set it to a time axis
        m_xAxis = new QDateTimeAxis();
        m_xAxis->setTickCount(10);
        m_xAxis->setFormat("hh:mm:ss");
        m_xAxis->setLabelsVisible(false);
        chart->addAxis(m_xAxis, Qt::AlignBottom);
        for (auto* series : m_series) { series->attachAxis(m_xAxis); }

        m_yAxis = new QValueAxis();
        m_yAxis->setLabelFormat("%.2f");
        m_yAxis->setRange(0, 100); // Initial range, will be updated in updateData
        chart->addAxis(m_yAxis, Qt::AlignLeft);

        // Create the Y-axes and set the range
        for (auto* series : m_series) {
            series->attachAxis(m_yAxis);
        }

        // Create the chart view and add it to the layout
        QChartView* chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        return chartView;
    }

public:
    int m_maxDataPoints = 600;
    qint64 m_xAxisRangeMs = 60000; // 1 minute

    ValueUsageWidget(std::function<std::vector<double>()> dataFunction, QString title, QList<QColor> colors, QWidget* parent = nullptr) :
        ContainerWidget(parent), getDataFunction(std::move(dataFunction)), title(title) {
        setWidget(createChart(colors));
    }

    ValueUsageWidget(std::function<std::vector<double>()> dataFunction, QString title, QWidget* parent = nullptr) :
        ValueUsageWidget(dataFunction, title, QList<QColor>{}, parent) {}

    void attachTo(QLambdaTimer& t) {
        t.addLambda([&]() { updateData(); });
    }

    void updateData() {
        // Update chart
        std::vector<double> usages = getDataFunction();

        double minThreshold = m_yAxis->max() * 0.70;
        double maxThreshold = m_yAxis->max();
        double maxDataPoint = 0;

        for (size_t i = 0; i < usages.size(); i++) {
            m_series[i]->append(QDateTime::currentDateTime().toMSecsSinceEpoch(), usages[i]);
        }

        for (size_t i = 0; i < m_series.size(); i++) {
            // Remove data points that exceed the maximum
            if (m_series[i]->count() > m_maxDataPoints) m_series[i]->remove(0);
            for (const QPointF& point : m_series[i]->points()) {
                maxDataPoint = std::max(maxDataPoint, point.y());
            }
        }

        if(maxDataPoint > maxThreshold){
            m_yAxis->setRange(0, maxDataPoint);
        }else if(maxDataPoint < 1.0){
            m_yAxis->setRange(0, 1.0);
        }else if(maxDataPoint < minThreshold){
            m_yAxis->setRange(0, maxDataPoint * 1.25);
        }

        // Adjust X-axis range
        qint64 maxX = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qint64 minX = maxX - m_xAxisRangeMs;
        m_xAxis->setRange(QDateTime::fromMSecsSinceEpoch(minX), QDateTime::fromMSecsSinceEpoch(maxX));
    }
};

class PercentUsageWidget : public ContainerWidget {
private:
    std::function<int()> getDataFunction;
    QString title;
    QLineSeries* m_series;
    QDateTimeAxis* m_xAxis;

    QWidget* createChart() {
        // Create the line series for the usage data
        m_series = new QLineSeries();

        // Create the chart and add the line series to it
        SystemThemedChart* chart = new SystemThemedChart();
        chart->legend()->hide();
        chart->addLineSeriesWithArea(m_series);
        chart->setTitle(title);

        // Create the X-axis and set it to a time axis
        m_xAxis = new QDateTimeAxis();
        m_xAxis->setTickCount(10);
        m_xAxis->setFormat("hh:mm:ss");
        m_xAxis->setLabelsVisible(false);
        chart->addAxis(m_xAxis, Qt::AlignBottom);
        m_series->attachAxis(m_xAxis);

        // Create the Y-axis and set the range
        QValueAxis* yAxis = new QValueAxis();
        yAxis->setLabelFormat("%.0f%%");
        yAxis->setRange(0, 100);
        chart->addAxis(yAxis, Qt::AlignLeft);
        m_series->attachAxis(yAxis);

        // Create the chart view and add it to the layout
        QChartView* chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        return chartView;
    }

    void updateData() {
        // Update chart
        int usage = getDataFunction();
        m_series->append(QDateTime::currentDateTime().toMSecsSinceEpoch(), usage);
        if (m_series->count() > m_maxDataPoints) m_series->remove(0);
        qint64 maxX = QDateTime::currentDateTime().toMSecsSinceEpoch();
        qint64 minX = maxX - m_xAxisRangeMs;
        m_xAxis->setRange(QDateTime::fromMSecsSinceEpoch(minX), QDateTime::fromMSecsSinceEpoch(maxX));
    }

public:
    int m_maxDataPoints = 600;
    qint64 m_xAxisRangeMs = 60000; // 1 minute
    PercentUsageWidget(std::function<int()> getDataFunction, QString title, QWidget* parent = nullptr) :
        ContainerWidget(parent), getDataFunction(getDataFunction), title(title) {
        setWidget(createChart());
    }
    void attachTo(QLambdaTimer& t) {
        t.addLambda([&]() { updateData(); });
    }
};

class CpuUsageWidget : public PercentUsageWidget {
private:
    SystemStats sysstats;

public:
    CpuUsageWidget(QWidget* parent = nullptr) :
        PercentUsageWidget([&]() { return sysstats.getCpuUsage(); }, "CPU Usage", parent) {}
};

class MemoryUsageWidget : public PercentUsageWidget {
private:
    SystemStats sysstats;

public:
    MemoryUsageWidget(QWidget* parent = nullptr) :
        PercentUsageWidget([&]() { return sysstats.getMemoryUsage(); }, "Memory Usage", parent) {}
};

class MemoryValueWidget : public ValueUsageWidget {
private:
    SystemStats sysstats;

public:
    MemoryValueWidget(QWidget* parent = nullptr) :
        ValueUsageWidget([&]() { return std::vector<double>{(double)sysstats.getMemoryUsage()}; }, "Memory Usage", parent) {}
};

class OverviewWidget : public EQLayoutWidget<QVBoxLayout> {
private:
    MemoryValueWidget* mem = new MemoryValueWidget();
    CpuUsageWidget* cpu = new CpuUsageWidget();

public:
    OverviewWidget(QWidget* parent = nullptr) : EQLayoutWidget(parent) {
        setAutoFillBackground(true);
        layout->setSpacing(0);
        layout->addWidget(cpu);
        layout->addWidget(mem);
    }
    void attachTo(QLambdaTimer& t) {
        cpu->attachTo(t);
        mem->attachTo(t);
    }
};