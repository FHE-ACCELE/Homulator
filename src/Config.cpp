#include "Config.h"
#include "Basic.h"

Config::Config(std::string file) {
  // 打开配置文件
  std::ifstream configFile(file);

  // 检查文件是否成功打开
  if (!configFile.is_open()) {
    std::cerr << "Error opening config file." << std::endl;
    return;
  }

  // 逐行读取配置文件
  std::string line;
  while (std::getline(configFile, line)) {
    // 忽略注释和空行
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // 查找等号位置，分割键值对
    size_t equalSignPos = line.find('=');
    if (equalSignPos != std::string::npos) {
      std::string key = line.substr(0, equalSignPos);
      std::string value = line.substr(equalSignPos + 1);

      // 移除键和值的前后空格
      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      // 将键值对插入映射
      configMap[key] = std::stoi(value);
    }
  }

  // 输出配置信息
  std::cout << "Configuration details are as "
               "follow:\n\n*****************************************\n\n";
  for (const auto &pair : configMap) {
    // std::cout << pair.first << "\t" << pair.second << std::endl;
    // 设置字段宽度为 10，并左对齐
    std::cout << std::left << std::setw(20) << pair.first << " ";

    // 设置字段宽度为 10，并右对齐
    std::cout << std::right << std::setw(20) << pair.second << "\n";
  }

  std::cout << "\n*****************************************\n\n";
}
