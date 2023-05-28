#pragma once

#include <QTimer>
#include <functional>
#include <vector>

class QLambdaTimer : public QObject {
public:
    QLambdaTimer(int intervalMs) : m_timer(new QTimer()) {
        QObject::connect(m_timer, &QTimer::timeout, this, &QLambdaTimer::executeLambdas);
        m_timer->setInterval(intervalMs);
    }

    void start() { m_timer->start(); }

    void stop() { m_timer->stop(); }

    void addLambda(const std::function<void()>& lambda) { m_lambdas.push_back(lambda); }

private:
    QTimer* m_timer;
    std::vector<std::function<void()>> m_lambdas;

    void executeLambdas() {
        for (const auto& lambda : m_lambdas) { lambda(); }
    }
};