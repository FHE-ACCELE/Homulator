#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "Basic.h"

class Config {
private:
  // 用于存储键值对的映射
  std::map<std::string, uint32_t> configMap;

public:
  Config(std::string file);

  uint32_t getValue(std::string key) {
    auto it = configMap.find(key);
    if (it == configMap.end()) {
      throw std::runtime_error("Can not find this key!\n");
    }
    return it->second;
  };

  void setValue(std::string name, uint32_t count){
    configMap[name] = count;
  };
};

#endif