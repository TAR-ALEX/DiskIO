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


int main(int argc, char** argv) {
    cptr<QApplication> app = new QApplication(argc, argv);
    rptr<QMainWindow> mw = new QMainWindow();

    DiskUsageWidget* duw = new DiskUsageWidget();
    QLambdaTimer qlt(250);
    qlt.start();
    duw->attachTo(qlt);

    mw->setCentralWidget(duw);
    mw->resize(600, 600);

    mw->show();

    int code = app->exec();
    return code;
}