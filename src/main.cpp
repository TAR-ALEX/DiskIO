#include "AspectRatioWidget.hpp"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
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
namespace fs = std::filesystem;

using namespace QtCharts;
using namespace estd::shortnames;
using namespace estd::string_util;

std::string readfile(std::string path) {
    std::ifstream t(path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

std::vector<std::string> getPaths(std::string path) {
    std::set<std::string> resultset;
    std::vector<std::string> result;
    for (const auto& entry : fs::directory_iterator(path)) resultset.insert(entry.path());
    for (const auto& entry : resultset) result.push_back(entry);
    return result;
}

uint64_t getSectorSize(std::string dev) {
    std::string s = readfile("/sys/block/" + dev + "/queue/hw_sector_size");
    return stoll(s);
}

std::vector<std::string> getDevices() {
    std::vector<std::string> devices = getPaths("/sys/block/");
    std::vector<std::string> result;
    for (auto d : devices) {
        if (!contains(d, "loop", true)) {
            auto sd = splitAll(d, "/");
            result.push_back(sd.at(sd.size() - 1)); //will throw this way vs back or []
        }
    }
    return result;
}

// <read, write>
std::pair<uint64_t, uint64_t> getDevStats(std::string dev) {
    // Field 3 -- # of sectors read
    // Field 7 -- # of sectors written

    std::string f = readfile("/sys/block/" + dev + "/stat");
    auto tok = splitAll(f, " ", false);
    if (tok.size() < 17) throw std::runtime_error("filestats invalid " + std::to_string(tok.size()));
    uint64_t sectorSize = getSectorSize(dev);
    return {stoll(tok[2]) * sectorSize, stoll(tok[6]) * sectorSize};
}

class DiskStats {
public:
    uint64_t intervalms = 250;
    uint64_t lastTime = 0;
    std::map<std::string, std::pair<uint64_t, uint64_t>> lastBytes;
    std::function<void(std::map<std::string, std::pair<double, double>>)> onData = [](auto) {};
    void run() {
        std::thread t([&]() {
            while (true) {
                auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                std::chrono::system_clock::now().time_since_epoch()
                )
                                                .count();

                std::map<std::string, std::pair<uint64_t, uint64_t>> currentBytes;
                std::map<std::string, std::pair<double, double>> mbps;
                auto devs = getDevices();
                for (auto& dev : devs) {
                    auto stat = getDevStats(dev);
                    currentBytes[dev] = stat;
                }
                for (auto& [k, v] : currentBytes) {
                    if (lastBytes.count(k))
                        mbps[k] = {
                            (v.first - lastBytes[k].first) / 1000000.0 * 1000.0 / (millisec_since_epoch - lastTime),
                            (v.second - lastBytes[k].second) / 1000000.0 * 1000.0 / (millisec_since_epoch - lastTime),
                        };
                }
                lastBytes = currentBytes;
                if (lastTime != 0) onData(mbps);
                lastTime = millisec_since_epoch;
                std::this_thread::sleep_for(std::chrono::milliseconds(intervalms));
            }
        });
        t.detach();
    }
    void stop() {}
};

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

int main(int argc, char** argv) {
    DiskStats dstats;

    std::map<std::string, std::pair<rptr<QLineSeries>, rptr<QLineSeries>>> series;
    std::map<std::string, rptr<QChartView>> chart;

    auto createChart = [&](std::string name) {
        rptr<QChart> c = new QChart();
        c->layout()->setContentsMargins(0, 0, 0, 0);
        rptr<QLineSeries> s1 = new QLineSeries();
        rptr<QLineSeries> s2 = new QLineSeries();
        s1->setPen(QPen{QBrush{QColor{0, 0, 255, 255}}, 2});
        s2->setPen(QPen{QBrush{QColor{255, 0, 0, 255}}, 2});
        c->legend()->hide();
        c->addSeries(s1.get());
        c->addSeries(s2.get());
        c->createDefaultAxes();
        QFont font = c->titleFont();
        font.setBold(true);
        font.setPointSizeF(font.pointSizeF() * 10 / 9);
        c->setTitleFont(font);
        c->setTitle(name.c_str());
        c->axisX()->setLabelsVisible(false);
        c->axisY()->setTitleText("MB/s");
        font = c->axisY()->titleFont();
        font.setPointSizeF(font.pointSizeF() * 4 / 5);
        c->axisY()->setTitleFont(font);
        c->axisX()->setRange(0, 100);
        c->axisY()->setRange(0, 100);

        // QChart::ChartTheme theme = QChart::ChartTheme::ChartThemeQt;
        // c->setTheme(theme);
        // c->setAutoFillBackground(false);
        c->setBackgroundVisible(false);


        c->setMargins(QMargins(0, 0, 0, 0));

        rptr<QChartView> cv = new QChartView(c.get());

        cv->setRenderHint(QPainter::Antialiasing);

        return std::tuple{cv, s1, s2};
    };

    cptr<QApplication> app = new QApplication(argc, argv);
    cptr<QMainWindow> mw = new QMainWindow();

    cptr<EQLayoutWidget<QGridLayout>> w = new EQLayoutWidget<QGridLayout>();

    cptr<EQLayoutWidget<QVBoxLayout>> wLegend = new EQLayoutWidget<QVBoxLayout>();
    // wLegend->setContentsMargins(0, 0, 0, 0);
    // w->setContentsMargins(0, 0, 0, 0);
    // w->layout->setContentsMargins(0, 0, 0, 0);
    // wLegend->layout->setContentsMargins(0, 0, 0, 0);
    // wLegend->layout->setSpacing(0);

    wLegend->addWidget(w.get());
    QLabel* legend = new QLabel(
        "<b><font color='#0000FF' font_size=6>• READ&nbsp;&nbsp;&nbsp;<b><font color='#ff0000' font_size=6>• WRITE"
    );
    wLegend->addWidget(legend);
    legend->setAlignment(Qt::AlignCenter);

    auto updateStrech = [&]() {
        for (auto& [dev, _] : chart) { w->layout->removeWidget(chart[dev].get()); }
        int itemNum = 0;
        int itemsInRow = 5;
        if (itemsInRow <= 10) itemsInRow = (chart.size() + 1) / 2;
        for (auto& [dev, _] : chart) {
            w->layout->addWidget(chart[dev].get(), itemNum % itemsInRow, itemNum / itemsInRow);
            itemNum++;
        }
        for (int i = 0; i < w->layout->rowCount(); i++) { w->layout->setRowStretch(i, 1); }
        for (int i = 0; i < w->layout->columnCount(); i++) { w->layout->setColumnStretch(i, 1); }
    };


    mw->setCentralWidget(wLegend.get());
    mw->resize(600, 600);

    mw->show();
    int idx = 0;

    dstats.onData = [&](auto mbps) {
        estd::Semaphore sem;
        QMetaObject::invokeMethod(app.get(), [&, mbps] {
            for (auto& [dev, _] : mbps) {
                if (!chart.count(dev)) {
                    std::tie(chart[dev], series[dev].first, series[dev].second) = createChart(dev);
                    updateStrech();
                }
            }
            for (auto& [dev, stat] : mbps) {
                // std::cout << dev << "\n";
                // std::cout << "\t" << stat.first << " " << stat.second << "\n";
                series[dev].second->append(idx, stat.second);
                series[dev].first->append(idx, stat.first);
                double yMax = 20.0 + fmax(getMaxY(series[dev].first.get()), getMaxY(series[dev].second.get()));
                yMax = uint64_t(yMax / 25.0) * 25 + 25;
                chart[dev]->chart()->axisY()->setRange(0, yMax);
                keepLastSeries(series[dev].first.get(), 600);
                keepLastSeries(series[dev].second.get(), 600);
                QLineSeries ss;
                ss.points();
                chart[dev]->chart()->axisX()->setRange(idx - 300, idx);
            }
            idx++;
            sem.notify();
        });
        sem.wait();
    };
    dstats.run();
    return app->exec();
}