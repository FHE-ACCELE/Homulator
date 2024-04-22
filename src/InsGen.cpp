/*****
 * InsGen
 */

#include "InsGen.h"
#include "Config.h"


InsGen::InsGen(Config *cfg) {
  batchSize = cfg->getValue("batchSize");

  batchCount = uint32_t(cfg->getValue("N") / batchSize);
}

// This function will be call when the above layer
// it will be call for both c0 and c1
std::vector<std::vector<Instruction *>> InsGen::GenNTT(
    uint32_t levelId, std::string name,
    std::vector<std::vector<Instruction *>> *depInsGroup, // [batch, insGroup]
    bool ntt, // Indicates ntt or intt
    AddrType op1AddrStart, AddrType opOutAddrStart) {
  // Append appropriate suffix to name based on NTT or INTT
  name += ntt ? "_NTT" : "INTT";

  // Dynamically allocate a vector of vectors to store instructions
  std::vector<std::vector<Instruction *>> insList;

  for (uint32_t b = 0; b < batchCount; b++) {
    // Create a new instruction with appropriate parameters
    auto ins = new Instruction(name + "batch(" + std::to_string(b) + ")_",
                               ntt ? NTT : INTT, levelId, b, op1AddrStart + b,
                               opOutAddrStart + b);

    setDependencyAndFormat(ins, depInsGroup, b, 0);

    // Update DataPool and DataInsMap
    std::vector<AddrType> op1list;
    op1list.push_back(op1AddrStart);
    updateDataStructures(ins, op1list, opOutAddrStart, b);
    insList.push_back({ins});
  }

  return insList;
}

std::vector<std::vector<Instruction *>> InsGen::GenAUTO(
    uint32_t levelId, std::string name,
    std::vector<std::vector<Instruction *>> *depInsGroup, // [batch, insGroup]
    AddrType op1AddrStart, AddrType opOutAddrStart) {
  name += "AUTO";

  // Dynamically allocate a vector of vectors to store instructions
  std::vector<std::vector<Instruction *>> insList;

  for (uint32_t b = 0; b < batchCount; b++) {
    // Create a new instruction with appropriate parameters
    auto ins =
        new Instruction(name + "batch(" + std::to_string(b) + ")_", AUTO,
                        levelId, b, op1AddrStart + b, opOutAddrStart + b);

    setDependencyAndFormat(ins, depInsGroup, b, 0);

    // Update DataPool and DataInsMap
    std::vector<AddrType> op1list;
    op1list.push_back(op1AddrStart);
    updateDataStructures(ins, op1list, opOutAddrStart, b);
    insList.push_back({ins});
  }

  return insList;
}

// 生成两组指令输入
// 每个指令有四个输入 一个输出
// Note: 如果MAC只有一个输出的话，可以通过op3=op1 op4=op2来表示
// 重新生成指令
std::vector<std::vector<Instruction *>> InsGen::GenEWE(
    uint32_t levelId, std::string name,
    std::vector<std::vector<Instruction *>> *depInsGroup,
    std::vector<std::vector<Instruction *>> *depInsGroup2, // [batch, insGroup]
    std::vector<std::vector<Instruction *>> *depInsGroup3,
    std::vector<std::vector<Instruction *>> *depInsGroup4,
    AddrType op1AddrStart, AddrType op2AddrStart, AddrType op3AddrStart,
    AddrType op4AddrStart, AddrType opOutAddrStart) {
  name += "EWE";
  std::vector<std::vector<Instruction *>> insList;

  for (uint32_t b = 0; b < static_cast<uint32_t>(batchCount / 2); b++) {
    for (int i = 0; i < 2; ++i) {
      /**** The behevious for one instruction
       * The ewe compute unit will contains two this adder tree
       *   op1   op2  op3   op4
       *       x          x
       *           x
       *************************************/

      std::vector<Instruction *> insGroup;
      uint32_t index = 2 * b + i;
      auto ins = new Instruction(name + "batch(" + std::to_string(index) + ")_",
                                 MULT, levelId, index, op1AddrStart + index,
                                 op2AddrStart + index, op3AddrStart + index,
                                 op4AddrStart + index, opOutAddrStart + index);

      // if(op2AddrStart + index > 56850){
      //   ins->ShowIns();
      // }

      // Set dependencies and formats for each operand
      setDependencyAndFormat(ins, depInsGroup, index, 0);
      setDependencyAndFormat(ins, depInsGroup2, index, 1);
      setDependencyAndFormat(ins, depInsGroup3, index, 2);
      setDependencyAndFormat(ins, depInsGroup4, index, 3);

      // Update DataPool and DataInsMap
      updateDataStructures(
          ins, {op1AddrStart, op2AddrStart, op3AddrStart, op4AddrStart},
          opOutAddrStart, index);

      insGroup.push_back(ins);
      insList.push_back(insGroup);
    }
  }

  return insList;
}

std::vector<std::vector<Instruction *>> InsGen::GenEWE(
    uint32_t levelId, std::string name,
    std::vector<std::vector<Instruction *>> *depInsGroup,
    std::vector<std::vector<Instruction *>> *depInsGroup2,
    std::vector<std::vector<Instruction *>> *depInsGroup3,
    std::vector<std::vector<Instruction *>> *depInsGroup4,
    std::vector<AddrType> op1AddrStart, std::vector<AddrType> op2AddrStart,
    std::vector<AddrType> op3AddrStart, std::vector<AddrType> op4AddrStart,
    std::vector<AddrType> opOutAddrStart) {
  name += "EWE";
  std::vector<std::vector<Instruction *>> insList;

  for (uint32_t b = 0; b < static_cast<uint32_t>(batchCount / 2); b++) {
    for (int i = 0; i < 2; ++i) {
      /**** The behevious for one instruction
       * The ewe compute unit will contains two this adder tree
       *   op1   op2  op3   op4
       *       x          x
       *           x
       *************************************/

      std::vector<Instruction *> insGroup;
      uint32_t index = 2 * b + i;
      auto ins = new Instruction(name + "batch(" + std::to_string(index) + ")_",
                                 MULT, levelId, index, op1AddrStart[index],
                                 op2AddrStart[index], op3AddrStart[index],
                                 op4AddrStart[index], opOutAddrStart[index]);

      // Set dependencies and formats for each operand
      setDependencyAndFormat(ins, depInsGroup, index, 0);
      setDependencyAndFormat(ins, depInsGroup2, index, 1);
      setDependencyAndFormat(ins, depInsGroup3, index, 2);
      setDependencyAndFormat(ins, depInsGroup4, index, 3);

      // Update DataPool and DataInsMap
      updateDataStructures(ins,
                           {op1AddrStart[index], op2AddrStart[index],
                            op3AddrStart[index], op4AddrStart[index]},
                           opOutAddrStart[index]);

      insGroup.push_back(ins);
      insList.push_back(insGroup);
    }
  }

  return insList;
}

void InsGen::setDependencyAndFormat(
    Instruction *ins, std::vector<std::vector<Instruction *>> *depGroup,
    uint32_t index, int operandNum) {

  if (depGroup && index < depGroup->size() && !(*depGroup)[index].empty()) {
    Instruction *depIns = (*depGroup)[index][0];

    ins->setDepIns(depIns, operandNum);
    depIns->getOperandOutFormat() ? ins->setNttForOperand(operandNum)
                                  : ins->setINttForOperand(operandNum);
  }
}

void InsGen::updateDataStructures(Instruction *ins,
                                  const std::vector<AddrType> &addrStarts,
                                  AddrType outAddr, uint32_t index,
                                  bool noOut) {
  // Make a copy of the addrStarts vector for modification
  std::vector<AddrType> uniqueAddrStarts = addrStarts;

  // Sort the vector
  std::sort(uniqueAddrStarts.begin(), uniqueAddrStarts.end());

  // Remove duplicates
  auto last = std::unique(uniqueAddrStarts.begin(), uniqueAddrStarts.end());
  uniqueAddrStarts.erase(last, uniqueAddrStarts.end());

  for (AddrType addrStart : uniqueAddrStarts) {
    AddrType addr = addrStart + index;
    DataPool->push_back(addr);
    datamap->addInputAddr(addr);
  }

  if (!noOut) {
    // ins->ShowIns();

    DataPool->push_back(outAddr + index);
    datamap->addOutAddr(outAddr + index);

    (*DataInsMap)[outAddr + index].push_back(ins);
  }
}

void InsGen::updateDataStructuresWithOutOutput(
    Instruction *ins, const std::vector<AddrType> &addrStarts, uint32_t index) {
  // Make a copy of the addrStarts vector for modification
  std::vector<AddrType> uniqueAddrStarts = addrStarts;

  // Sort the vector
  std::sort(uniqueAddrStarts.begin(), uniqueAddrStarts.end());

  // Remove duplicates
  auto last = std::unique(uniqueAddrStarts.begin(), uniqueAddrStarts.end());
  uniqueAddrStarts.erase(last, uniqueAddrStarts.end());

  for (AddrType addrStart : uniqueAddrStarts) {
    AddrType addr = addrStart + index;
    DataPool->push_back(addr);
    datamap->addInputAddr(addr);
    // (*DataInsMap)[addr].push_back(ins);
  }
}

void InsGen::updateDataStructures(Instruction *ins,
                                  const std::vector<AddrType> &addr,
                                  AddrType outAddr, bool noOut) {
  // Make a copy of the addrStarts vector for modification
  std::vector<AddrType> uniqueAddrStarts = addr;

  // Sort the vector
  std::sort(uniqueAddrStarts.begin(), uniqueAddrStarts.end());

  // Remove duplicates
  auto last = std::unique(uniqueAddrStarts.begin(), uniqueAddrStarts.end());
  uniqueAddrStarts.erase(last, uniqueAddrStarts.end());

  for (AddrType addrStart : uniqueAddrStarts) {
    AddrType addr = addrStart;
    DataPool->push_back(addr);
    datamap->addInputAddr(addr);
  }
  if (!noOut) {
    DataPool->push_back(outAddr);
    datamap->addOutAddr(outAddr);
    (*DataInsMap)[outAddr].push_back(ins);
  }
}

std::vector<std::vector<Instruction *>>
InsGen::GenBCONV(uint32_t levelId, uint32_t InLevel, uint32_t offset,
                 std::string name,
                 std::vector<std::vector<std::vector<Instruction *>>>
                     depInsGroupList, // [inlevel, batch, insgroup]
                 std::vector<AddrType> op1AddrStartList,
                 AddrType op2AddrStartList, // [in, out]
                 AddrType opOutAddrStart) {

  name += "BCONV";

  std::vector<std::vector<Instruction *>> insList; // [batch, insgroup]

  auto &dataPollRef = *DataPool;
  auto &dataInsMapRef = *DataInsMap;

  for (uint32_t b = 0; b < batchCount; b++) {
    std::vector<Instruction *> insGroup;

    std::vector<AddrType> inAddrList;
    std::vector<AddrType> outAddrList;
    for (uint32_t il = 0; il < InLevel;
         il++) { // inlevel = alpha in the modup stage

      Instruction *ins =
          new Instruction(name + "inputLevel(" + std::to_string(il) + ")" +
                              "_batch(" + std::to_string(b) + ")_",
                          BCONV_STEP2, levelId, b, op1AddrStartList[il] + b,
                          op2AddrStartList, // + (il * offset + levelId),
                          opOutAddrStart + b);

      setDependencyAndFormat(ins, &depInsGroupList[il], b, 0);

      // Update DataPool and DataInsMap
      updateDataStructuresWithOutOutput(ins, {op1AddrStartList[il]}, b);

      updateDataStructuresWithOutOutput(
          ins,
          {
              op2AddrStartList // + (il * offset + levelId)
          },
          0);

      insGroup.push_back(ins);
    }
    updateDataStructures(nullptr, {}, opOutAddrStart, b);

    insList.push_back(insGroup);
  }
  return insList;
}

std::vector<std::vector<Instruction *>> InsGen::GenBCONV(
    uint32_t levelId, uint32_t InLevel, std::string name,
    std::vector<std::vector<std::vector<Instruction *>>>
        depInsGroupList, // [inlevel, batch, insgroup]
    std::vector<AddrType> op1AddrStartList,
    std::vector<std::vector<AddrType>> op2AddrStartList, // [in, out]
    AddrType opOutAddrStart) {

  name += "BCONV";

  std::vector<std::vector<Instruction *>> insList; // [batch, insgroup]

  auto &dataPollRef = *DataPool;
  auto &dataInsMapRef = *DataInsMap;

  for (uint32_t b = 0; b < batchCount; b++) {
    std::vector<Instruction *> insGroup;
    for (uint32_t il = 0; il < InLevel;
         il++) { // inlevel = alpha in the modup stage

      Instruction *ins =
          new Instruction(name + "inputLevel(" + std::to_string(il) + ")" +
                              "_batch(" + std::to_string(b) + ")_",
                          BCONV_STEP2, levelId, b, op1AddrStartList[il] + b,
                          op2AddrStartList[il][levelId], opOutAddrStart + b);

      setDependencyAndFormat(ins, &depInsGroupList[il], b, 0);

      // Update DataPool and DataInsMap
      updateDataStructures(ins, {op1AddrStartList[il]}, opOutAddrStart, b);
      updateDataStructures(ins, {op2AddrStartList[il][levelId]}, opOutAddrStart,
                           0, true);

      insGroup.push_back(ins);
    }
    insList.push_back(insGroup);
  }

  return insList;
}

std::vector<Instruction *>
InsGen::GenHPIP(uint32_t levelId, std::string name,
                std::vector<Instruction *> *depInsGroup = nullptr,
                std::vector<Instruction *> *depInsGroup2 = nullptr,
                AddrType op1AddrStart = NONEADDRESS,
                AddrType op2AddrStart = NONEADDRESS,
                AddrType opOutAddrStart = NONEADDRESS) {

  name += "HPIP";

  std::vector<Instruction *> insList;

  auto &dataPollRef = *DataPool;
  auto &dataInsMapRef = *DataInsMap;

  for (uint32_t b = 0; b < batchCount; b++) {
    Instruction *ins = new Instruction(
        name + "batch(" + std::to_string(b) + ")_", IP, levelId, b,
        op1AddrStart + b, op2AddrStart + b, opOutAddrStart + b);
    if (depInsGroup != nullptr) {
      ins->setDepIns((*depInsGroup)[b], 1);
      if ((*depInsGroup)[b]->getOperandOutFormat()) {
        ins->setNttForOperand(1);
      } else {
        ins->setINttForOperand(1);
      }
    }

    if (depInsGroup2 != nullptr) {
      ins->setDepIns((*depInsGroup2)[b], 2);
      if ((*depInsGroup2)[b]->getOperandOutFormat()) {
        ins->setNttForOperand(2);
      } else {
        ins->setINttForOperand(2);
      }
    }

    datamap->addInputAddr(op1AddrStart + b);
    datamap->addInputAddr(op2AddrStart + b);

    datamap->addOutAddr(opOutAddrStart + b);

    dataPollRef.push_back(op1AddrStart + b);
    dataPollRef.push_back(op2AddrStart + b);

    dataInsMapRef[op1AddrStart + b].push_back(ins);
    dataInsMapRef[op2AddrStart + b].push_back(ins);

    insList.push_back(ins);
  }
}
