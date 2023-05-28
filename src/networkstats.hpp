
#pragma once
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

#include "./diskstats.hpp"


class NetworkStats {
private:
    std::map<std::string, std::pair<uint64_t, uint64_t>> getDeviceStats() {
        std::map<std::string, std::pair<uint64_t, uint64_t>> result;
        std::string fileStr = readfile("/proc/net/dev");
        auto lines = estd::string_util::splitAll(fileStr, "\n", false);


        for (size_t i = 2; i < lines.size(); i++) {
            auto namePlusTokens = estd::string_util::splitAll(lines.at(i), ":", false);
            auto name = estd::string_util::splitAll(namePlusTokens.at(0), " ", false).at(0);
            auto tokens = estd::string_util::splitAll(namePlusTokens.at(1), " ", false);

            if (name != "lo") { result[name] = {stoll(tokens.at(0)), stoll(tokens.at(8))}; }
        }
        // printMap(result);
        return result;
    }

public:
    uint64_t lastTime = 0;
    std::map<std::string, std::pair<uint64_t, uint64_t>> lastBytes;
    std::map<std::string, std::pair<double, double>> getRate() {
        auto millisec_since_epoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        std::map<std::string, std::pair<uint64_t, uint64_t>> currentBytes = getDeviceStats();
        std::map<std::string, std::pair<double, double>> mbps;
        for (auto& [k, v] : currentBytes) {
            if (lastBytes.count(k))
                mbps[k] = {
                    (v.first - lastBytes[k].first) / 1000000.0 * 1000.0 / (millisec_since_epoch - lastTime),
                    (v.second - lastBytes[k].second) / 1000000.0 * 1000.0 / (millisec_since_epoch - lastTime),
                };
        }
        lastBytes = currentBytes;

        if (lastTime != 0) {
            lastTime = millisec_since_epoch;
            return mbps;
        }
        lastTime = millisec_since_epoch;
        return {};
    }
};