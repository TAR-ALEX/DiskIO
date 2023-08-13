#pragma once

#include <QFile>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QTextStream>

class SystemStats {
private:
    int m_previousTotalTime = 0;
    int m_previousIdle = 0;

public:
    int getCpuUsage() {
        QFile statFile("/proc/stat");
        if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;

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

                    int deltaTime = totalTime - m_previousTotalTime;
                    int currentUsage = (deltaTime > 0) ? ((deltaTime - (idle - m_previousIdle)) * 100) / deltaTime : 0;

                    m_previousTotalTime = totalTime;
                    m_previousIdle = idle;

                    if (currentUsage < 0) currentUsage = 0;
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

    int getMemoryUsage() {
        QFile meminfoFile("/proc/meminfo");
        if (!meminfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;

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
            uint64_t usedMemory = totalMemory - freeMemory;
            uint64_t currentUsage = (usedMemory * 100) / totalMemory;

            if (currentUsage < 0) currentUsage = 0;
            else if (currentUsage > 100)
                currentUsage = 100;

            return currentUsage;
        }

        return -1;
    }
};