/*****
 * Generate instruction
 * only input some required information
 *
 */

#ifndef INSGEN_H
#define INSGEN_H

#include "Basic.h"
#include "Components.h"
#include "Config.h"
#include "Instruction.h"
#include "mem.h"

#define BASEADDRESS 0x00000001
#define NONEADDRESS 0xffffffff

class InsGen {
private:
  uint32_t batchSize;

  uint32_t batchCount;
  DataMap *datamap;

  // All generated data address wil put into this pool
  // std::map<std::string, std::vector<AddrType>> *DataPool;
  std::vector<AddrType> *DataPool;

  // Generate the all data->intruction map, which will be used when
  // fetch one data on the OnChipMem and the memoryControl will fetch this one
  // new corresponding instrution in the intructiuon buffer
  std::map<AddrType, std::vector<Instruction *>> *DataInsMap;

public:
  InsGen(Config *cfg);

  void setGlobalDataMap(DataMap *map) { datamap = map; };

  void setGlobalDatapPoll(std::vector<AddrType> *pool) { DataPool = pool; };
  void
  setGlobalDataInsMap(std::map<AddrType, std::vector<Instruction *>> *map) {
    DataInsMap = map;
  };

  std::vector<std::vector<Instruction *>> GenNTT(
      uint32_t levelId, std::string name,
      std::vector<std::vector<Instruction *>> *depInsGroup, // [batch, insGroup]
      bool ntt, // Indicates ntt or intt
      AddrType op1AddrStart, AddrType op3AddrStart);

  std::vector<std::vector<Instruction *>> GenAUTO(
      uint32_t levelId, std::string name,
      std::vector<std::vector<Instruction *>> *depInsGroup, // [batch, insGroup]
      AddrType op1AddrStart, AddrType opOutAddrStart);

  std::vector<std::vector<Instruction *>>
  GenEWE(uint32_t levelId, std::string name,
         std::vector<std::vector<Instruction *>> *depInsGroup,
         std::vector<std::vector<Instruction *>> *depInsGroup2,
         std::vector<std::vector<Instruction *>> *depInsGroup3,
         std::vector<std::vector<Instruction *>> *depInsGroup4,
         AddrType op1AddrStart, AddrType op2AddrStart, AddrType op3AddrStart,
         AddrType op4AddrStart, AddrType opOutAddrStart);

  std::vector<std::vector<Instruction *>>
  GenEWE(uint32_t levelId, std::string name,
         std::vector<std::vector<Instruction *>> *depInsGroup,
         std::vector<std::vector<Instruction *>> *depInsGroup2,
         std::vector<std::vector<Instruction *>> *depInsGroup3,
         std::vector<std::vector<Instruction *>> *depInsGroup4,
         std::vector<AddrType> op1AddrStart, std::vector<AddrType> op2AddrStart,
         std::vector<AddrType> op3AddrStart, std::vector<AddrType> op4AddrStart,
         std::vector<AddrType> opOutAddrStart);

  std::vector<std::vector<Instruction *>>
  GenBCONV(uint32_t levelId, uint32_t InLevel, uint32_t offset,
           std::string name,
           std::vector<std::vector<std::vector<Instruction *>>>
               depInsGroupList, // [inlevel, batch, insgroup]
           std::vector<AddrType> op1AddrStartList,
           AddrType op2AddrStartList, // [in, out]
           AddrType opOutAddrStart);

  std::vector<std::vector<Instruction *>>
  GenBCONV(uint32_t levelId, uint32_t InLevel, std::string name,
           std::vector<std::vector<std::vector<Instruction *>>> depInsGroupList,
           // [inlevel, batch, insgroup]
           std::vector<AddrType> op1AddrStartList,
           std::vector<std::vector<AddrType>> op2AddrStartList, // [in, out]
           AddrType opOutAddrStart);

  std::vector<Instruction *> GenHPIP(uint32_t levelId, std::string name,
                                     std::vector<Instruction *> *depInsGroup,
                                     std::vector<Instruction *> *depInsGroup2,
                                     AddrType op1AddrStart,
                                     AddrType op2AddrStart,
                                     AddrType opOutAddrStart);

  uint32_t getbatchCount() { return batchCount; };

  // void updateDataStructures(Instruction *ins, const std::vector<AddrType>
  // &addrStarts,
  //                           AddrType outAddr, uint32_t index, bool noOut =
  //                           false);
  void updateDataStructures(Instruction *ins,
                            const std::vector<AddrType> &addrStarts,
                            AddrType outAddr, uint32_t index,
                            bool noOut = false);

  void
  updateDataStructuresWithOutOutput(Instruction *ins,
                                    const std::vector<AddrType> &addrStarts,
                                    uint32_t index);

  void updateDataStructures(Instruction *ins, const std::vector<AddrType> &addr,
                            AddrType outAddr, bool noOut = false);

  void setDependencyAndFormat(Instruction *ins,
                              std::vector<std::vector<Instruction *>> *depGroup,
                              uint32_t index, int operandNum);
  // 然后将指令全部发送给 指令关系分析器 决定数据由DRAM发送到Accelerator的顺序
};

#endif // INSGEN_H