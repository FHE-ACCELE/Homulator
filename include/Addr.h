#ifndef ADDRH
#define ADDRH

#include "Basic.h"

class AddrManage {
private:
  AddrType addr;
  uint32_t BatchSize;
  std::map<std::string, std::vector<AddrType>> dataMap;

  AddrType offsetAddr;

  // All generated data address wil put into this pool
  // std::map<std::string, std::vector<AddrType>> *DataPool;
  std::vector<AddrType> *DataPool;

  void updateBatchAddr() { addr += BatchSize; }
  void updatePolyAddr(uint32_t count) { addr += count * BatchSize; }

public:
  void setGlobalDatapPoll(std::vector<AddrType> *pool) { DataPool = pool; };

  AddrManage(AddrType PreAddr, uint32_t BSCount)
      : addr(PreAddr), BatchSize(BSCount) {}

  AddrManage(uint32_t BSCount) : addr(0), BatchSize(BSCount) {}

  void MallocMem(const std::string &name, uint32_t polyCount) {
    if (dataMap.find(name) != dataMap.end()) {
      std::cerr << "Error: Name already exists in dataMap." << std::endl;
      return;
    }

    // addr = (*DataPool)[DataPool->size() - 1] + 1;

    std::vector<AddrType> temp;
    for (uint32_t p = 0; p < polyCount; ++p) {
      temp.push_back(addr);
      DataPool->push_back(addr);
      updatePolyAddr(1);
      offsetAddr = addr;
    }
    dataMap[name] = temp;

    std::cout << "Malloc " << name << " from " << temp[0] << " to "
              << temp.back() << std::endl;
  }

  void MallocMemOneBatch(const std::string &name, uint32_t BatchCount) {
    if (dataMap.find(name) != dataMap.end()) {
      std::cerr << "Error: Name already exists in dataMap." << std::endl;
      return;
    }

    std::vector<AddrType> temp;
    for (uint32_t p = 0; p < BatchCount; ++p) {
      temp.push_back(addr);
      DataPool->push_back(addr);
      updateBatchAddr();
    }
    dataMap[name] = temp;

    std::cout << "Malloc " << name << " from " << temp[0] << " to "
              << temp.back() << std::endl;
  }

  // [polyId, START] OR [batchId, start]
  std::vector<AddrType> getAddr(const std::string &name) const {
    auto it = dataMap.find(name);
    if (it != dataMap.end()) {
      return it->second;
    }
    std::cout << name << std::endl;
    throw std::runtime_error("Cannot find this data key, please confirm!\n");
    // The function will end here if the key is not found, due to the
    // exception being thrown.
  }

  AddrType getLatestAddr() const { return addr; }

  // Rule of Five: If you are managing resources, define the destructor, copy
  // constructor, copy assignment operator, move constructor, and move
  // assignment operator.
  ~AddrManage() {
    // Implement cleanup if necessary
  }

  // Delete copy constructor and copy assignment operator to prevent copying
  // if not needed
  AddrManage(const AddrManage &) = delete;
  AddrManage &operator=(const AddrManage &) = delete;
};

#endif