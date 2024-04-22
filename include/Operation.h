#ifndef _OPERATION_
#define _OPERATION_

#include "Addr.h"
#include "Basic.h"
#include "Context.h"
// #include "Driver.h"
#include "InsGen.h"
#include "mem.h"

class Arch;
class Driver;

/**
 * 生成指令以及数据地址
 */

class KeySwitch {
private:
  /* data */
  // All generated data address wil put into this pool
  // std::map<std::string, std::vector<AddrType>> *DataPool;
  std::vector<AddrType> *DataPool;

  // Generate the all data->intruction map, which will be used when
  // fetch one data on the OnChipMem and the memoryControl will fetch this one
  // new corresponding instrution in the intructiuon buffer
  std::map<AddrType, std::vector<Instruction *>> *DataInsMap;

  InsGen *insGenPointer;

  std::vector<AddrType> preAddr;

  uint32_t Level, Alpha, Beta, dnum;
  uint32_t MaxLevel;
  uint32_t batchCount;
  AddrManage *memMange;

  std::string baseName;

  std::map<std::string, std::vector<AddrType>> KeySwicthDataMap;
  std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>
      KeySwicthInsMap; // key->[level, batch, insGroup]

  std::vector<std::string> KeySwitchInsMapName;

public:
  KeySwitch(std::string labelName, uint32_t maxlevel, uint32_t level,
            uint32_t alpha,
            const std::vector<AddrType>
                &inputPolynomialAddress, // start addr for each level
            std::vector<AddrType> *pool,
            std::map<AddrType, std::vector<Instruction *>> *map, InsGen *insgen,
            AddrManage *memoryMange);

  std::pair<std::map<std::string,
                     std::vector<std::vector<std::vector<Instruction *>>>>,
            std::vector<std::string>>
  getInsMap();

  void ModUpINTT();
  void ModUpDecompFusionBConvStep1(uint32_t beta);
  void ModUpBConvStep2(uint32_t beta);

  void ModUpNTT(uint32_t beta);

  void InnerProduceOperation();

  void ModDown();

  void ModDownINTT();
  void ModDownBConvStep1();
  void ModDownBConvStep2();
  void ModDowNTT();
  void ModDownSub();

  ~KeySwitch();
};

class TensorCompute {
private:
  /* data */
  // All generated data address wil put into this pool
  // std::map<std::string, std::vector<AddrType>> *DataPool;
  std::vector<AddrType> *DataPool;

  // Generate the all data->intruction map, which will be used when
  // fetch one data on the OnChipMem and the memoryControl will fetch this one
  // new corresponding instrution in the intructiuon buffer
  std::map<AddrType, std::vector<Instruction *>> *DataInsMap;

  InsGen *insGenPointer;
  uint32_t currentLevel;

  uint32_t batchCount;
  AddrManage *memMange;

  std::string baseName;

  std::map<std::string, std::vector<AddrType>> TensorComputeDataMap;
  std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>
      TensorComputeInsMap; // key->[level, batch, insGroup]

  std::vector<std::string> TensorComputeInsMapName;

  std::vector<AddrType> ciph1_c0, ciph1_c1;
  std::vector<AddrType> ciph2_c0, ciph2_c1;

public:
  TensorCompute(std::string labelName, uint32_t level, Ciphertext *cipher1,
                Ciphertext *cipher2, std::vector<AddrType> *pool,
                std::map<AddrType, std::vector<Instruction *>> *map,
                InsGen *insgen, AddrManage *memoryMange);
  ~TensorCompute();

  void computeD0();
  void computeD1();
  void computeD2();

  std::pair<std::map<std::string,
                     std::vector<std::vector<std::vector<Instruction *>>>>,
            std::vector<std::string>>
  getInsMap() {
    return std::make_pair(TensorComputeInsMap, TensorComputeInsMapName);
  }
};

class Rescale {
private:
  // All generated data address wil put into this pool
  // std::map<std::string, std::vector<AddrType>> *DataPool;
  std::vector<AddrType> *DataPool;

  // Generate the all data->intruction map, which will be used when
  // fetch one data on the OnChipMem and the memoryControl will fetch this one
  // new corresponding instrution in the intructiuon buffer
  std::map<AddrType, std::vector<Instruction *>> *DataInsMap;

  std::vector<AddrType> preAddr;

  InsGen *insGenPointer;
  uint32_t currentLevel;

  uint32_t batchCount;
  AddrManage *memMange;

  std::string baseName;

  std::map<std::string, std::vector<AddrType>> RescaleDataMap;
  std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>
      RescaleInsMap; // key->[level, batch, insGroup]

  std::vector<std::string> RescaleInsMapName;

public:
  Rescale(std::string labelName, uint32_t level,

          const std::vector<AddrType>
              &inputPolynomialAddress, // start addr for each level
          std::vector<AddrType> *pool,
          std::map<AddrType, std::vector<Instruction *>> *map, InsGen *insgen,
          AddrManage *memoryMange);

  ~Rescale();

  void NTTOps();
  void SubOps();
  void MulOps();

  std::pair<std::map<std::string,
                     std::vector<std::vector<std::vector<Instruction *>>>>,
            std::vector<std::string>>
  getInsMap() {
    return std::make_pair(RescaleInsMap, RescaleInsMapName);
  }
};

class HMULT {
private:
  /* data */
  std::vector<AddrType> Datapool;
  std::map<AddrType, std::vector<Instruction *>> DataInsMap;
  std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>
      HMULTHaddInsMap;
  std::vector<std::string> HMULTHaddInsMapName;

  DataMap *datamap;
  InsGen *insgener;
  Driver *driver;
  AddrManage *addrManager;

  Ciphertext *c1;
  Ciphertext *c2;

  Arch *arch;

  std::vector<MemController *> memControlList;

public:
  HMULT(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
        uint32_t alpha, Config *cfg, Arch *_arch);

  bool simulate();
  ~HMULT();
};

class HADD {
private:
  /* data */
  std::vector<AddrType> Datapool;
  std::map<AddrType, std::vector<Instruction *>> DataInsMap;
  std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>
      HADDInsMap;
  std::vector<std::string> HADDInsMapName;

  DataMap *datamap;
  InsGen *insgener;
  Driver *driver;
  AddrManage *addrManager;

  Ciphertext *c1;
  Ciphertext *c2;

  Arch *arch;

  std::vector<MemController *> memControlList;

public:
  HADD(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
       uint32_t alpha, Config *cfg, Arch *_arch);

  bool simulate();
  ~HADD();
};

class HROTATE {
private:
  /* data */
  std::vector<AddrType> Datapool;
  std::map<AddrType, std::vector<Instruction *>> DataInsMap;

  DataMap *datamap;
  InsGen *insgener;
  Driver *driver;
  AddrManage *addrManager;

  Ciphertext *ciph;

  Arch *arch;

  std::vector<MemController *> memControlList;

  std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>
      AUTOInsMap; // key->[level, batch, insGroup]
  std::vector<std::string> AUTOInsMapName;

public:
  HROTATE(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
          uint32_t alpha, Config *cfg, Arch *_arch);

  bool simulate();
  ~HROTATE();
};

class PMULT {
private:
  /* data */
  std::vector<AddrType> Datapool;
  std::map<AddrType, std::vector<Instruction *>> DataInsMap;
  std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>
      PMULTInsMap;
  std::vector<std::string> PMULTInsMapName;

  DataMap *datamap;
  InsGen *insgener;
  Driver *driver;
  AddrManage *addrManager;

  Ciphertext *ctx;
  Plaintext *ptx;

  Arch *arch;

  std::vector<MemController *> memControlList;

public:
  PMULT(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
        uint32_t alpha, Config *cfg, Arch *_arch);

  bool simulate();
  ~PMULT();
};

class PADD {
private:
  /* data */
  std::vector<AddrType> Datapool;
  std::map<AddrType, std::vector<Instruction *>> DataInsMap;
  std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>
      PADDInsMap;
  std::vector<std::string> PADDInsMapName;

  DataMap *datamap;
  InsGen *insgener;
  Driver *driver;
  AddrManage *addrManager;

  Ciphertext *ctx;
  Plaintext *ptx;

  Arch *arch;

  std::vector<MemController *> memControlList;

public:
  PADD(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
       uint32_t alpha, Config *cfg, Arch *_arch);

  bool simulate();
  ~PADD();
};
#endif