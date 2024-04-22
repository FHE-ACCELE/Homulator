
#include "Arch.h"
#include "Staistics.h"

bool mem::checkData(AddrType addr, bool ntt) {
  // for (auto &line : chipMem) {
  for (auto id : usedLineId) {
    auto line = chipMem[id];
    if (!line->getValid() && line->getAddr() == addr) {

      if (arch->MemLineStatic()) {
        auto cycle = arch->getCycle();
        line->insertTimeForLine(cycle);
      }

      return true;
    }
  }
  return false;
}

bool MemController::checkMemReady(std::vector<Instruction *> insg,
                                  std::string stall_key) {

  for (auto &ins : insg) {
    uint32_t inputCount = ins->getinputCount();

    for (uint32_t i = 0; i < inputCount; i++) {

      if (ins->getOperand(i) == 0) { // 0 is the fake input
        continue;
      }

      if (globalDataMap->isInOutputMap(
              ins->getOperand(i))) { // 需要的数据是另一个计算的输出。
        return false;
      }

      if (!onChipMem->checkData(ins->getOperand(i), ins->getOperandFormat(i))) {

        if (globalDataMap->getInputTimes(ins->getOperand(i)) ==
            0) { // the input data has beed used
          continue;
        }

        auto flag = checkOtherMem(ins->getOperand(i), ins->getOperandFormat(i));

        stat->increaseStat(stall_key);

        return false; // Return early if data is not ready
      }
    }
  }

  for (auto &ins : insg) {
    uint32_t inputCout = ins->getinputCount();

    for (uint32_t i = 0; i < inputCout; i++) {
      onChipMem->validAddr(ins->getOperand(i),
                           ins->getOperandFormat(i)); // fetch data
      validOtMem(ins->getOperand(i),
                 ins->getOperandFormat(i)); //其他的mem同样需要处理 times

      globalDataMap->modifidInputTimes( //重要操作
          ins->getOperand(i),
          globalDataMap->getInputTimes(ins->getOperand(i)) - 1);
    }
    stat->increaseStat("MEM_(" + std::to_string(cluseterId) + ")",
                       inputCout * batchSize);
  }

  // increaseStat

  return true;
}

// Insert data from units to memory
bool MemController::insert2MemFromNoC(AddrType addr) {

  for (auto &a : addrFromNoC) { // 正在队列中的不需要再额外增加了。
    if (a.first == addr) {
      return true;
    }
  }

  if (globalDataMap->getInputTimes(addr) ==
      0) { //如果在未来不会再被需作为输出了，则直接输出，不需要写入mem了。
    return true;
  }

  if (addrFromNoC.size() >= fifoDepthFromNoC) { // fifo full了。
    return false;
  }

  stat->increaseStat("NoC_Mem_Chip");
  addrFromNoC.push_back(
      std::make_pair(addr, globalDataMap->getInputTimes(addr)));

  return true;
}

void MemController::update() {

  fetcherFromDram->update();
  if (fetcherFromDram->getExecuted()) {
    stat->increaseStat("HBM_(" + std::to_string(cluseterId) + ")");
  }

  if (!addrFromNoC.empty()) {
    auto firstEntry = addrFromNoC.begin();
    if (onChipMem->insertFromNoc(firstEntry->first, firstEntry->second)) {
      addrFromNoC.erase(firstEntry);
    } else {
      onChipMem->updateLines();
    }
  }

  if (!addrFromUnits.empty()) {
    auto firstEntry = addrFromUnits.begin();
    if (onChipMem->insertFromUnits(firstEntry->first, firstEntry->second)) {
      addrFromUnits.erase(firstEntry);
    } else {
      onChipMem->updateLines();
    }
  }

  // Process data from DRAM
  if (!addrFromDram.empty()) {
    auto firstEntryList = addrFromDram.begin();
    for (const auto &entry : *firstEntryList) {
      if (onChipMem->insertFromDram(entry.first, entry.second)) {
        addrFromDram.erase(addrFromDram.begin());
        break; // Ensure we only process one entry per update
      } else {
        onChipMem->updateLines();
      }
    }
  }

  // Check output signal from DRAM fetcher
  if (fetcherFromDram->getOutSignal()) {
    if (insert2MemFromDram(fetcherFromDram->getOutput())) {
      fetcherFromDram->setOutSignal();
    } else {
    }
  }
}
