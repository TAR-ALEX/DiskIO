#pragma once

#include "./diskstats.hpp"
#include <QtWidgets>
#include "./SystemThemedChart.hpp"
#include "./QLambdaTimer.hpp"
#include "./SystemOverviewWidgets.hpp"

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

template<class STAT_TYPE = DiskStats>
class DiskUsageWidget : public ContainerWidget {
private:
    STAT_TYPE dstats;

    std::map<std::string, rptr<ValueUsageWidget>> chart;
    rptr<EQLayoutWidget<QGridLayout>> w = new EQLayoutWidget<QGridLayout>();

    rptr<ValueUsageWidget> createChart(std::string name) {
        rptr<ValueUsageWidget> cw = new ValueUsageWidget([=](){
            // std::cout << name << " " << mbps[name].first << std::endl;
            return std::vector<double>{mbps[name].second, mbps[name].first};
        }, name.c_str(), {red, blue});
        return cw;
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

    std::map<std::string, std::pair<double, double>> mbps;

    void updateData(){
        mbps = dstats.getRate();
        for (auto& [dev, _] : mbps) {
            // std::cout << dev << std::endl;
            if (!chart.count(dev)) {
                chart[dev] = createChart(dev);
                updateStrech();
            }
        }
        // std::cout <<"----------"<< std::endl;
        for(auto& chrt: chart){
            chrt.second->updateData();
        }
    }

    QColor red;
    QColor blue;

public:
    DiskUsageWidget(QWidget* parent = nullptr) : ContainerWidget(parent) {
        
        w->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

        rptr<EQLayoutWidget<QVBoxLayout>> wLegend = new EQLayoutWidget<QVBoxLayout>();

        QColor bgColor = w->palette().color(QPalette::Background);
        blue = QColor{0, 0, 255, 255};
        red = QColor{255, 0, 0, 255};
        if (bgColor.toHsv().value() < 155) {
            blue = QColor{0, 200, 255, 255};
            red = QColor{255, 70, 70, 255};
        }

        wLegend->setAutoFillBackground(true);

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