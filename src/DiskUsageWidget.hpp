#pragma once

#include "./diskstats.hpp"
#include <QtWidgets>
#include "./SystemThemedChart.hpp"
#include "./QLambdaTimer.hpp"

double getMaxY(QLineSeries* ser) {
    double maxY = 0;
    for (auto& p : ser->points()) {
        if (p.y() > maxY) maxY = p.y();
    }
    return maxY;
}


void keepLastSeries(QLineSeries* ser, int numKeep = 200) {
    while (ser->count() > numKeep) { ser->remove(0); }
}

class DiskUsageWidget : public ContainerWidget {
private:
    int idx = 0;
    DiskStats dstats;

    std::map<std::string, std::pair<rptr<QLineSeries>, rptr<QLineSeries>>> series;
    std::map<std::string, rptr<ContainerWidget>> chart;
    rptr<EQLayoutWidget<QGridLayout>> w = new EQLayoutWidget<QGridLayout>();

    std::tuple<rptr<ContainerWidget>,rptr<QLineSeries>,rptr<QLineSeries>> createChart(std::string name) {
            rptr<SystemThemedChart> c = new SystemThemedChart();
            rptr<QLineSeries> s1 = new QLineSeries();
            rptr<QLineSeries> s2 = new QLineSeries();

            QColor bgColor = c->palette().color(QPalette::Background);

            s1->setPen(QPen{QBrush{QColor{0, 0, 255, 255}}, 2});
            s2->setPen(QPen{QBrush{QColor{255, 0, 0, 255}}, 2});
            if (bgColor.toHsv().value() < 155) {
                s1->setPen(QPen{QBrush{QColor{0, 230, 255, 255}}, 2});
                s2->setPen(QPen{QBrush{QColor{255, 70, 70, 255}}, 2});
            }
            c->legend()->hide();
            c->addLineSeriesWithArea(s2.get());
            c->addLineSeriesWithArea(s1.get());
            QValueAxis* axisY = new QValueAxis();
            QValueAxis* axisX = new QValueAxis();

            // axisY->setLabelFormat("%03d");
            QFont font = c->titleFont();
            font.setBold(true);
            font.setPointSizeF(font.pointSizeF() * 10 / 9);

            axisX->setLabelsVisible(false);
            axisY->setTitleText("MB/s");
            font = axisY->titleFont();
            font.setPointSizeF(font.pointSizeF() * 4 / 5);
          
            axisX->setRange(0, 100);
            axisY->setRange(0, 100);


            c->addAxis(axisY, Qt::AlignLeft);
            c->addAxis(axisX, Qt::AlignBottom);

            s1->attachAxis(axisY);
            s1->attachAxis(axisX);
            s2->attachAxis(axisY);
            s2->attachAxis(axisX);


            rptr<QChartView> cv = new QChartView(c.get());
            rptr<ContainerWidget> cw = new ContainerWidget(cv, nullptr);

            // cv->setContentsMargins(QMargins(0, 0, 0, -50));
            cv->setRenderHint(QPainter::Antialiasing);
            cv->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
            return std::tuple{cw, s1, s2};
        }

        void updateStrech() {
            for (auto& [dev, _] : chart) { w->layout->removeWidget(chart[dev].get()); }
            int itemNum = 0;
            int itemsInRow = 4;
            if (4 < chart.size()) itemsInRow = (chart.size() + 1) / 2;
            if (10 < chart.size()) itemsInRow = (chart.size() + 2) / 3;
            for (auto& [dev, _] : chart) {
                w->layout->addWidget(chart[dev].get(), itemNum % itemsInRow, itemNum / itemsInRow);
                itemNum++;
            }
            w->setMinimumSize(120, 120);
        }

        void updateData(){
            auto mbps = dstats.getRate();
            for (auto& [dev, _] : mbps) {
                if (!chart.count(dev)) {
                    std::tie(chart[dev], series[dev].first, series[dev].second) = createChart(dev);
                    updateStrech();
                }
            }
            for (auto& [dev, stat] : mbps) {
                series[dev].second->append(idx, stat.second);
                series[dev].first->append(idx, stat.first);
                double yMax = 20.0 + fmax(getMaxY(series[dev].first.get()), getMaxY(series[dev].second.get()));
                yMax = uint64_t(yMax / 25.0) * 25 + 25;
                auto chrt = static_cast<QChartView*>(chart[dev]->getWidget())->chart();
                chrt->axisY()->setRange(0, yMax);
                keepLastSeries(series[dev].first.get(), 600);
                keepLastSeries(series[dev].second.get(), 600);
                QLineSeries ss;
                ss.points();
                chrt->axisX()->setRange(idx - 300, idx);
            }
            idx++;
        }

public:
    DiskUsageWidget(QWidget* parent = nullptr) : ContainerWidget(parent) {
        
        w->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

        rptr<EQLayoutWidget<QVBoxLayout>> wLegend = new EQLayoutWidget<QVBoxLayout>();

        QColor bgColor = w->palette().color(QPalette::Background);
        auto blue = QColor{0, 0, 255, 255};
        auto red = QColor{255, 0, 0, 255};
        if (bgColor.toHsv().value() < 155) {
            blue = QColor{0, 200, 255, 255};
            red = QColor{255, 70, 70, 255};
        }

        wLegend->addWidget(w.get());
        QLabel* legend = new QLabel(
            "<b><font color='" + blue.name(QColor::HexRgb) + "' font_size=6>• READ&nbsp;&nbsp;&nbsp;<b><font color='" +
            red.name(QColor::HexRgb) + "' font_size=6>• WRITE"
        );
        wLegend->addWidget(legend);
        legend->setAlignment(Qt::AlignCenter);
        legend->setContentsMargins(QMargins(0, 0, 0, 5));
        w->layout->setVerticalSpacing(0);

        ContainerWidget::setWidget(wLegend.get());
    }

    void attachTo(QLambdaTimer& t){
        t.addLambda([&](){updateData();});
    }
};