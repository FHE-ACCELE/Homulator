#ifndef _STATISTIC
#define _STATISTIC

#include "Basic.h"

class Statistic {
private:
  std::map<std::string, uint32_t> statMap;

public:
  Statistic() = default;

  void increaseStat(const std::string &key) {
    auto it = statMap.find(key);
    if (it != statMap.end()) {
      it->second++;
    } else {
      statMap[key] = 1;
    }
  };
  void increaseStat(const std::string &key, uint32_t count) {
    auto it = statMap.find(key);
    if (it != statMap.end()) {
      it->second += count;
    } else {
      statMap[key] = 1;
    }
  };

  void showStat() {
    std::cout << "Start outPut statistic informations:\n";
    std::cout << "=====================================\n";
    for (auto &key : statMap) {
      std::cout << key.first << " :\t" << key.second << "\n";
    }
  };

  void setStat(std::string key, uint32_t infor) { statMap[key] = infor; };

  ~Statistic() = default;
};

#endif