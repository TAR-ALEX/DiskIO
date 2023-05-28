
#pragma once
void printMap(const std::map<std::string, std::pair<double, double>>& myMap) {
    for (const auto& entry : myMap) {
        std::cout << entry.first << ": (" << entry.second.first << ", " << entry.second.second << ")" << std::endl;
    }
}

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

class DiskStats {
private:
    uint64_t getSectorSize(std::string dev) {
        std::string s = readfile("/sys/block/" + dev + "/queue/hw_sector_size");
        return stoll(s);
    }

    std::vector<std::string> getDevices() {
        std::vector<std::string> devices = getPaths("/sys/block/");
        std::vector<std::string> result;
        for (auto d : devices) {
            if (!contains(d, "loop", true) && !contains(d, "zram", true) && !contains(d, "dm", true)) {
                auto sd = splitAll(d, "/");
                result.push_back(sd.at(sd.size() - 1)); //will throw this way vs back or operator[]
            }
        }
        return result;
    }

    std::pair<uint64_t, uint64_t> getDevStats(std::string dev) {
        // Field 3 -- # of sectors read
        // Field 7 -- # of sectors written

        std::string f = readfile("/sys/block/" + dev + "/stat");
        auto tok = splitAll(f, " ", false);
        if (tok.size() < 17) throw std::runtime_error("filestats invalid " + std::to_string(tok.size()));
        uint64_t sectorSize = getSectorSize(dev);
        return {stoll(tok[2]) * sectorSize, stoll(tok[6]) * sectorSize};
    }

public:
    uint64_t lastTime = 0;
    std::map<std::string, std::pair<uint64_t, uint64_t>> lastBytes;
    std::map<std::string, std::pair<double, double>> getRate() {
        auto millisec_since_epoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
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

        if (lastTime != 0) {
            lastTime = millisec_since_epoch;
            return mbps;
        }
        lastTime = millisec_since_epoch;
        return {};
    }
};