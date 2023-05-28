#include "AspectRatioWidget.hpp"
#include <QApplication>
#include <QChartView>
#include <QColorDialog>
#include <QGridLayout>
#include <QLineSeries>
#include <QMainWindow>
#include <QObject>
#include <QPainter>
#include <QPushButton>
#include <QValueAxis>
#include <QtWidgets>
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


#include "./DiskUsageWidget.hpp"
#include "./SystemThemedChart.hpp"
#include "./systemstats.hpp"
#include "./networkstats.hpp"
#include "SystemOverviewWidgets.hpp"


int main(int argc, char** argv) {
    cptr<QApplication> app = new QApplication(argc, argv);
    QMainWindow* mw = new QMainWindow();

    QTabWidget* tabWidget = new QTabWidget(mw);

    QLambdaTimer qlt(250);

    DiskUsageWidget<DiskStats>* duw = new DiskUsageWidget<DiskStats>();
    DiskUsageWidget<NetworkStats>* nuw = new DiskUsageWidget<NetworkStats>();
    OverviewWidget* ovr = new OverviewWidget();

    tabWidget->addTab(ovr, "Summary");
    tabWidget->addTab(duw, "Disk");
    tabWidget->addTab(nuw, "Network");

    mw->setCentralWidget(tabWidget);
    mw->resize(600, 600);

    mw->show();

    duw->attachTo(qlt);
    nuw->attachTo(qlt);
    ovr->attachTo(qlt);
    qlt.start();

    int code = app->exec();
    return code;
}



// int main() {
//     NetworkStats monitor;

//     while (true) {
//         try {
//             // monitor.getRate();
//             printMap(monitor.getRate());
            
//         }
//         catch (const std::exception& e) {
//             std::cerr << "Error: " << e.what() << std::endl;
//             return 1;
//         }

//         std::this_thread::sleep_for(std::chrono::seconds(1));
//     }

//     return 0;
// }
