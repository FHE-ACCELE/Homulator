#ifndef MEM_H_
#define MEM_H_

// #include "Arch.h"
#include "Config.h"
#include "Instruction.h"

class Arch;
class Statistic;

// DRAM DataMap
class DataMap {
private:
  // addr->usedtimes
  std::map<AddrType, uint32_t> inputDataAddr;
  std::map<AddrType, uint32_t> outDataAddr;

  uint32_t
      moveTimes; // this value indites the movetime from one mem to other mem.

public:
  DataMap() {
    // Empty constructor
    moveTimes = 0;
  };

  // Adds or increments the usage count of an input address
  void addInputAddr(AddrType addr) {
    auto iter = inputDataAddr.find(addr);
    if (iter != inputDataAddr.end()) {
      iter->second += 1;
    } else {
      inputDataAddr[addr] = 1;
    }
  };

  // Adds an output address, throws if address already exists
  void addOutAddr(AddrType addr) {
    auto iter = outDataAddr.find(addr);

    if (iter != outDataAddr.end()) {
      // std::cout<<addr<<"\n\n";
      // throw std::runtime_error("Please confirm the out addr, WAW!"); //
      // modified, because the ewe will shared same outaddr
    } else {
      outDataAddr[addr] = 1;
    }
  };

  // Returns the usage times of an input address, throws if not found
  uint32_t getInputTimes(AddrType addr) {
    auto iter = inputDataAddr.find(addr);
    if (iter == inputDataAddr.end()) {
      // std::cout << "Error: Input address " << addr << " not found in
      // DataMap\n"; throw std::runtime_error("Error! Not find the input addr in
      // the DataMap");
      return 0;
    }
    return iter->second;
  };

  void modifidInputTimes(AddrType addr, uint32_t times) {
    auto iter = inputDataAddr.find(addr);

    if (iter != inputDataAddr.end()) {
      if (inputDataAddr[addr] > 0 && times >= 0)
        inputDataAddr[addr] = times;
    }
  };

  // Checks if an address is in the input map
  bool isInInputMap(AddrType addr) {
    return inputDataAddr.find(addr) != inputDataAddr.end();
  };

  // Checks if an address is in the output map
  bool isInOutputMap(AddrType addr) {
    return outDataAddr.find(addr) != outDataAddr.end();
  };

  // Erases an address from the output map, throws if not found
  void eraseOutMap(AddrType addr) {
    // if(addr == 5892){
    //   std::cout<<"DEBUG: this inser datamap\n";
    // }
    auto iter = outDataAddr.find(addr);
    if (iter != outDataAddr.end()) {
      outDataAddr.erase(iter);
    } else {
      moveTimes += 1;
      // std::cout << "Error: Output address " << addr
      //           << " not found in DataMap\n";
      // throw std::runtime_error(
      //     "This out Addr not exist in the map, please confirm!\n");
    }
  };

  // Erases an address from the input map, throws if not found
  void eraseInputMap(AddrType addr) {
    auto iter = inputDataAddr.find(addr);
    if (iter != inputDataAddr.end()) {
      inputDataAddr.erase(iter);
    } else {
      std::cout << "Error: Input address " << addr << " not found in DataMap\n";
      throw std::runtime_error(
          "This input Addr not exist in the map, please confirm!\n");
    }
  };
};

class HBMEntry {
private:
  /* data */
  std::vector<AddrType> storge;

public:
  HBMEntry(std::vector<AddrType> data) { storge = data; };

  std::vector<AddrType> getData() { return storge; };

  void show() {
    std::cout << "\n";
    for (auto &add : storge) {
      std::cout << add << " | ";
    }
    std::cout << "\n";
  };
};

class HbmPort {
private:
  uint32_t delay;
  uint32_t entryCount;
  std::vector<HBMEntry *> pipeline; // [pipelineDepth, bandwidth]

  uint32_t used_pipeline; // Consider renaming to usedPipeline for consistency

  bool outSignal; // Corrected typo in naming
  HBMEntry *outPointer;

  bool inputSignal;
  HBMEntry *inPointer;

  bool executed = false;

public:
  // Constructor for HbmPort
  HbmPort(uint32_t pipeline_delay, uint32_t transEntryCount)
      : delay(pipeline_delay), entryCount(transEntryCount), used_pipeline(0),
        outSignal(false), inputSignal(false) {
    pipeline.resize(delay);
    outPointer = nullptr;
  };

  // Moves data along the pipeline
  void movePipeline() {
    if (used_pipeline > 0 && pipeline.back() == nullptr) {
      // Rotate the pipeline to simulate data movement
      std::rotate(pipeline.begin(), pipeline.end() - 1, pipeline.end());
      executed = true;
    }
  };

  // Updates the state of the port
  void update() {
    executed = false;
    if (pipeline[delay - 1] != nullptr && !outSignal && outPointer == nullptr) {
      outPointer = pipeline[delay - 1];
      outSignal = true;
      pipeline[delay - 1] = nullptr;
      used_pipeline--;
    }

    movePipeline();

    if (pipeline.front() == nullptr && inputSignal) {
      pipeline.front() = inPointer;
      used_pipeline++;
      inputSignal = false;
    }
  };

  bool getExecuted() { return executed; };

  // Sets the input for the port
  bool setInput(HBMEntry *addres) {
    if (!inputSignal) {
      inPointer = addres;
      // std::cout<<addres->getData();
      // addres->show();
      inputSignal = true;
      return true;
    }
    return false;
  }

  bool getOutSignal() const { return outSignal; };
  void setOutSignal() {
    outSignal = false;
    outPointer = nullptr;
  };

  // Gets the output from the port and clears the pointer
  std::vector<AddrType> getOutput() {
    if (outPointer == nullptr) {
      throw std::runtime_error("Output pointer is null");
    }
    std::vector<AddrType> temp = outPointer->getData();

    return temp;
  };
};

class MemLine {
private:
  AddrType dataAddr;
  bool valid;         // Indicates if this line will be out
  uint32_t usedTimes; // Number of times this line will be used
  bool unitsOut;      // Indicates if the data comes from units
  bool format;        // true for NTT, false otherwise

  uint32_t accessTimes; // The total access times from the chip
  std::vector<unsigned long long> accessTime;

public:
  MemLine() : valid(true), usedTimes(0), unitsOut(false), format(false) {
    dataAddr = -1;
  };

  // Setter and getter methods
  void setAddr(AddrType addr) { dataAddr = addr; };
  void setTimes(uint32_t times) { usedTimes = times; };
  uint32_t getTimes() const { return usedTimes; };
  void decreaseTimes() {
    if (usedTimes > 0) {
      usedTimes--;
    } else {
      std::cout << usedTimes << "\n";
      throw std::runtime_error("Attempted to decrease times below zero");
    }
  };

  bool getValid() const { return valid; };
  void setValid(bool f) { valid = f; };

  bool getUnits() const { return unitsOut; };
  void setUnit(bool f) { unitsOut = f; };

  void setNTT(bool ntt) { format = ntt; };
  bool getNTTState() const { return format; };

  AddrType getAddr() const { return dataAddr; };

  void insertTimeForLine(unsigned long long time) {
    accessTime.push_back(time);
  };
};

class mem {
private:
  uint32_t lineCount;
  uint32_t id;
  std::string name;
  std::vector<MemLine *> chipMem;

  DataMap *globalDataMap;

  Arch *arch;

  std::vector<uint32_t> usedLineId;
  std::vector<uint32_t> freeLineId;

public:
  mem(uint32_t size, Arch *_arch) : lineCount(size), arch(_arch) {
    for (uint32_t i = 0; i < lineCount; ++i) {
      chipMem.push_back(new MemLine());
      freeLineId.push_back(i); // Store the free Id.
    }
  };

  // Destructor to prevent memory leaks
  ~mem() {
    for (auto &line : chipMem) {
      delete line;
    }
  }

  // Setter methods
  void setId(uint32_t i) { id = i; };
  void setName(const std::string &n) { name = n; };

  // Insertion methods
  bool insertFromDram(AddrType addr, uint32_t times, bool ntt = true) {
    return insert(addr, times, ntt, false);
  };

  bool insertFromUnits(AddrType addr, uint32_t times, bool ntt = true,
                       bool final = false) {
    return insert(addr, times, ntt, true);
  }

  bool insertFromNoc(AddrType addr, uint32_t times, bool ntt = true,
                     bool final = false) {
    return insert(addr, times, ntt, true);
  }

  void setGlobalDataMap(DataMap *map) { globalDataMap = map; }

  void updateLines() {
    for (auto &line : chipMem) {
      auto addr = line->getAddr();
      if (!globalDataMap->isInInputMap(
              addr)) { // Mem 中的非输入的line需要被output
        line->setValid(false);
        line->setTimes(0);
      }
    }
  };

  // Common insertion logic
  bool insert(AddrType addr, uint32_t times, bool ntt, bool fromUnits) {

    if (times != globalDataMap->getInputTimes(addr)) { // 检查一致性
      // std::cout << " " << addr << " times " << times << " "
      //           << globalDataMap->getInputTimes(addr)<<"\n\n";
      // throw std::runtime_error("Debug: Error not equal for this addr!\n");
      times = globalDataMap->getInputTimes(addr);
    }

    if (insertCheckData(addr, ntt)) { //已经有这个数据了，就不需要再增加了
      return true;
    }

    if (!freeLineId.empty()) {
      auto id = freeLineId[0];
      auto line = chipMem[id];

      if (line->getValid() || line->getTimes() == 0) {
        line->setAddr(addr);
        line->setValid(false);
        line->setTimes(times);
        line->setUnit(fromUnits);
        line->setNTT(ntt);

        usedLineId.push_back(id);
        freeLineId.erase(freeLineId.begin());

        return true;
      }
    }

    return false;
  }

  void processAddr(AddrType addr) { // 检查目的addr

    for (auto it = usedLineId.begin(); it != usedLineId.end();) {
      auto line = chipMem[*it];
      if (line->getAddr() == addr) {
        if (line->getTimes() > 0) {
          line->decreaseTimes();
        } else {
          line->setValid(true);
          line->setTimes(0);
          freeLineId.push_back(*it); // Store the value before erasing
          it = usedLineId.erase(it); // Erase the element and update iterator
          continue; // Continue to next iteration without incrementing iterator
        }
      }
      ++it; // Increment iterator
    }
  };

  void checkingTimes() {
    // for (auto &line : chipMem) {
    for (auto it = usedLineId.begin(); it != usedLineId.end(); ++it) {
      auto line = chipMem[*it];
      if (line->getTimes() != globalDataMap->getInputTimes(line->getAddr())) {
        std::cout << " " << line->getAddr() << " times " << line->getTimes()
                  << " " << globalDataMap->getInputTimes(line->getAddr())
                  << "\n\n";
        throw std::runtime_error("Debug: Error not equal for this addr!\n");
      }
    }
  };

  void shownState() {
    std::cout << "\n\n";
    // for (auto &line : chipMem) {
    for (auto it = usedLineId.begin(); it != usedLineId.end(); ++it) {
      auto line = chipMem[*it];
      if (line->getTimes() != 0) {
        std::cout << " " << line->getAddr() << " times " << line->getTimes()
                  << " " << globalDataMap->getInputTimes(line->getAddr())
                  << " || ";
      }
    }
  };

  // Validation method
  void validAddr(AddrType addr, bool ntt) {

    // for (auto &line : chipMem) {
    // for (auto it = usedLineId.begin(); it != usedLineId.end();) {
    //   auto line = chipMem[*it];
    //   if (line->getTimes() > 0 &&
    //       line->getAddr() == addr) { //! line->getValid() &&
    //     // This version not check the ntt state
    //     // if (ntt != line->getNTTState()) {
    //     //   throw std::runtime_error("Incorrect format representation");
    //     // }

    //     line->decreaseTimes();
    //     if (line->getTimes() == 0) {
    //       line->setValid(true);
    //       line->setTimes(0);
    //       freeLineId.push_back(*it); // Store the value before erasing
    //       it = usedLineId.erase(it); // Erase the element and update iterator
    //       continue; // Continue to next iteration without incrementing
    //       iterator
    //     }
    //     return;
    //   }
    // }

    for (auto it = usedLineId.begin(); it != usedLineId.end();) {
      auto line = chipMem[*it];
      if (line->getTimes() > 0 && line->getAddr() == addr) {
        // Check if additional state needs verification
        // if (ntt != line->getNTTState()) {
        //     throw std::runtime_error("Incorrect format representation");
        // }

        line->decreaseTimes();
        if (line->getTimes() == 0) {
          line->setValid(true);
          line->setTimes(0);
          freeLineId.push_back(*it);
          it = usedLineId.erase(it);
          continue;
        }
        return; // Assuming only one match is processed at a time
      }
      ++it;
    }

    // throw std::runtime_error("Address not found");
  };

  bool checkData(AddrType addr, bool ntt);

  bool insertCheckData(AddrType addr, bool ntt) {

    for (auto id : usedLineId) {
      auto line = chipMem[id];
      if (!line->getValid() && line->getAddr() == addr) {
        return true;
      }
    }

    return false;
  };
};

class MemController {
private:
  mem *onChipMem;

  uint32_t memCount;
  uint32_t memSize;
  uint32_t memLineCount;

  uint32_t fetchDelay;

  // addr->times
  std::vector<std::vector<std::pair<AddrType, uint32_t>>> addrFromDram;
  std::vector<std::pair<AddrType, uint32_t>> addrFromUnits;
  std::vector<std::pair<AddrType, uint32_t>> addrFromNoC;

  uint32_t entryCountForFifoDram;

  uint32_t fifoDepthFromDram;
  uint32_t fifoDepthFromUnits;
  uint32_t fifoDepthFromNoC;

  DataMap *globalDataMap;

  HbmPort *fetcherFromDram;

  uint32_t fetchdelayFromDram;

  uint32_t cluseterId;

  std::vector<MemController *> memlists;

  Arch *arch;

  Statistic *stat;
  uint32_t batchSize;
  Config *cfg_;

public:
  // Constructor
  MemController(Config *cfg, Arch *_arch) : arch(_arch) {
    memCount = cfg->getValue("memCount");
    memSize = cfg->getValue("memSize");
    batchSize = cfg->getValue("batchSize");
    uint32_t elementBitWidth = cfg->getValue("elementBitWidth");

    cfg_ = cfg;

    memLineCount = uint32_t(
        memSize / (float(batchSize * elementBitWidth) / (8 * 1024 * 1024)));
    onChipMem = new mem(memLineCount, _arch);

    entryCountForFifoDram =
        cfg->getValue("entryCount"); // related with the bandwidth of hbm
    fifoDepthFromDram = cfg->getValue("memDramFifo");
    fifoDepthFromUnits = cfg->getValue("memUnitsFifo");

    fifoDepthFromNoC = cfg->getValue("cluster");

    fetchdelayFromDram =
        cfg->getValue("offDelay"); // This is about the hbm type.
    fetcherFromDram = new HbmPort(fetchdelayFromDram, entryCountForFifoDram);
  };

  // Destructor to properly clean up resources
  ~MemController() {
    delete onChipMem;
    delete fetcherFromDram;
  }

  void setClusterId(uint32_t id) { cluseterId = id; };

  void setMemControlList(std::vector<MemController *> list) {
    memlists = list;
  };

  void setGlobalDataMap(DataMap *map) {
    globalDataMap = map;
    onChipMem->setGlobalDataMap(map);
  }

  // Insert data from units to memory
  bool insert2MemFromUnits(AddrType addr) {

    if (globalDataMap->isInOutputMap(addr)) { //从输出map中删除
      globalDataMap->eraseOutMap(addr);
    }

    for (auto &a : addrFromUnits) { // 正在队列中的不需要再额外增加了。
      if (a.first == addr) {
        return true;
      }
    }

    if (globalDataMap->getInputTimes(addr) ==
        0) { //如果在未来不会再被需作为输出了，则直接输出，不需要写入mem了。
      return true;
    }

    if (addrFromUnits.size() >= fifoDepthFromUnits) { // fifo full了。
      return false;
    }

    addrFromUnits.push_back(
        std::make_pair(addr, globalDataMap->getInputTimes(addr)));

    return true;
  };

  bool insert2MemFromNoC(AddrType addr);

  // Insert data from DRAM to memory
  bool insert2MemFromDram(std::vector<AddrType> addrs) {

    if (addrFromDram.size() >= fifoDepthFromDram) {
      return false; // FIFO is full
    }

    std::vector<std::pair<AddrType, uint32_t>> temp;
    for (const auto &addr : addrs) {
      if (globalDataMap->getInputTimes(addr) == 0) { //如果不再被需要了
        // return true;
        continue;
      }

      temp.push_back(std::make_pair(addr, globalDataMap->getInputTimes(addr)));
    }
    addrFromDram.push_back(temp);
    return true;
  };

  // Access memory from units
  void accessFromUnits(Instruction *ins){
      // return true;
  };

  // check this instruction group is reday?
  bool checkMemReady(std::vector<Instruction *> insg, std::string stall_key);

  bool onlyCheckMem(AddrType addr, bool format) {

    if (onChipMem->checkData(addr, format)) {

      return true; // Return early if data is not ready
    }
    return false;
  };

  bool checkOtherMem(AddrType addr, bool format) {
    for (uint32_t id = 0; id < memlists.size(); id++) {
      if (id != cluseterId) {
        if (memlists[id]->onlyCheckMem(addr, format)) {
            insert2MemFromNoC(addr);
        }
      }
    }
    return false;
  };

  void processAddr(AddrType addr) { onChipMem->processAddr(addr); };

  void validOtMem(AddrType addr, bool format) {
    for (uint32_t id = 0; id < memlists.size(); id++) {
      if (id != cluseterId) {
        if (memlists[id]->onlyCheckMem(addr, format)) {
          memlists[id]->processAddr(addr);
        }
      }
    }
  };

  // Send data to on-chip memory from Dram
  // This function expose to the instruction generator
  bool sentToOnChipMem(std::vector<AddrType> &addrs) {
    return fetcherFromDram->setInput(new HBMEntry(addrs));
  };

  uint32_t getHbmBandwidth() { return entryCountForFifoDram; };

  void checkingTimes() { onChipMem->checkingTimes(); };

  // Update the controller's state
  void update();

  void shownState() { onChipMem->shownState(); };

  void setStat(Statistic *s) { stat = s; };
};

#endif