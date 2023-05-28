#pragma once

#include <QAreaSeries>
#include <QBrush>
#include <QChart>
#include <QChartView>
#include <QColor>
#include <QFont>
#include <QLineSeries>
#include <QLinearGradient>
#include <QPalette>
#include <QPen>
#include <QValueAxis>

class SystemThemedChart : public QtCharts::QChart {
private:
    static QList<QColor> generateSequentialColors(int count) {
        QList<QColor> colors;

        for (int i = 0; i < count; ++i) {
            QColor color;
            color.setHsv(i * (255.0 / count), 255, 255);
            colors.append(color);
        }

        return colors;
    }

public:
    QList<QColor> sequentialColors = generateSequentialColors(10);
    SystemThemedChart() {
        using namespace QtCharts;
        QColor bgColor = palette().color(QPalette::Background);
        QColor bgColorInactive = palette().color(QPalette::Base);

        QPen pen1(QBrush(QColor(0, 0, 255, 255)), 2);
        QPen pen2(QBrush(QColor(255, 0, 0, 255)), 2);

        if (bgColor.toHsv().value() < 155) {
            pen1 = QPen(QBrush(QColor(0, 230, 255, 255)), 2);
            pen2 = QPen(QBrush(QColor(255, 70, 70, 255)), 2);
        }

        legend()->hide();

        QValueAxis* axisY = new QValueAxis();
        QValueAxis* axisX = new QValueAxis();

        createDefaultAxes();

        QFont font = titleFont();
        font.setBold(true);
        font.setPointSizeF(font.pointSizeF() * 10 / 9);

        QColor fgColor = palette().color(QPalette::Foreground);
        QColor gridColor = palette().color(QPalette::Disabled, QPalette::Foreground);

        setTitleBrush(QBrush(fgColor));
        setTitleFont(font);

        axisX->setLabelsVisible(false);
        axisY->setTitleText("MB/s");

        font = axisY->titleFont();
        font.setPointSizeF(font.pointSizeF() * 4 / 5);

        setPlotAreaBackgroundBrush(QBrush(bgColorInactive));
        setPlotAreaBackgroundVisible(true);
        setBackgroundBrush(QBrush(bgColor));



        setBackgroundRoundness(0);

        setMargins(QMargins(0, 0, 0, -20));
    }

    int addedAxisCounter = 0;

    void addAxis(QtCharts::QAbstractAxis* axis, Qt::Alignment alignment) {
        QColor fgColor = palette().color(QPalette::Foreground);
        QColor gridColor = palette().color(QPalette::Disabled, QPalette::Foreground);

        axis->setTitleBrush(QBrush(fgColor));
        axis->setGridLineColor(gridColor);
        axis->setLabelsColor(fgColor);
        axis->setShadesColor(gridColor);
        axis->setMinorGridLineColor(gridColor);
        axis->setShadesBorderColor(gridColor);
        axis->setLinePenColor(gridColor);
        QChart::addAxis(axis, alignment);
        if (alignment & Qt::AlignBottom && axis->labelsVisible()) { setMargins(QMargins(0, 0, 0, 0)); }
    }

    void addSeries(QtCharts::QLineSeries* series) {
        series->color().setAlpha(255);
        if (!series->color().isValid() || series->color() == QColor(0, 0, 0, 255)) {
            QPen linePen = series->pen();
            linePen.setColor(sequentialColors[(addedAxisCounter++) % sequentialColors.size()]);
            linePen.setWidth(2);
            series->setPen(linePen);
        }
        QChart::addSeries(series);
    }

    void addLineSeriesWithArea(QtCharts::QLineSeries* series) {
        series->pen().color().setAlpha(255);
        if (!series->pen().color().isValid() || series->pen().color() == QColor(0, 0, 0, 255)) {
            QPen linePen = series->pen();
            linePen.setColor(sequentialColors[(addedAxisCounter++) % sequentialColors.size()]);
            linePen.setWidth(2);
            series->setPen(linePen);
        }


        QPen linePen = series->pen();
        QColor lineColor = linePen.color();
        QColor shadedColor = lineColor;
        shadedColor.setAlpha(63); // 25% opacity (255 * 0.25 = 63)

        QLinearGradient gradient;
        gradient.setColorAt(0.0, shadedColor);
        gradient.setColorAt(1.0, shadedColor);

        QtCharts::QAreaSeries* areaSeries = new QtCharts::QAreaSeries(series);
        areaSeries->setBorderColor(Qt::transparent);
        areaSeries->setBrush(QBrush(gradient));

        QChart::addSeries(areaSeries);
        QChart::addSeries(series);

        createDefaultAxes();
        for (auto ax : axes(Qt::Horizontal | Qt::Vertical, areaSeries)) { ax->hide(); }
    }
};