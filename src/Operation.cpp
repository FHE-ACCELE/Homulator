#include "Operation.h"
#include "Arch.h"
#include "Config.h"
#include "Context.h"
#include "Driver.h"

bool hasHPIP = false;

KeySwitch::KeySwitch(std::string labelName, uint32_t maxlevel, uint32_t level,
                     uint32_t alpha,
                     const std::vector<AddrType>
                         &inputPolynomialAddress, // start addr for each level
                     std::vector<AddrType> *pool,
                     std::map<AddrType, std::vector<Instruction *>> *map,
                     InsGen *insgen, AddrManage *memoryMange) {

  DataInsMap = map;
  DataPool = pool;
  MaxLevel = maxlevel;
  Level = level; // Currentlevel [0,...,Level]
  Alpha = alpha;
  Beta = std::ceil(static_cast<double>(Level) / Alpha);    // Currentlevel
  dnum = std::ceil(static_cast<double>(MaxLevel) / Alpha); // Currentlevel
  insGenPointer = insgen;
  batchCount = insGenPointer->getbatchCount();
  memMange = memoryMange;

  preAddr = inputPolynomialAddress;

  // std::cout << preAddr[0] << "\n";

  baseName = labelName + "_KeySwitch";

  // INTT operation
  ModUpINTT();

  // ModUp Bconv operation to execute, which conver each digit from alpha to
  // l;
  memMange->MallocMem("ModUpDecompOffset", 1);
  memMange->MallocMem("ModUpDecompOut", Level);
  for (uint32_t be = 0; be < Beta; be++) {
    ModUpDecompFusionBConvStep1(be);
    ModUpBConvStep2(be);
    ModUpNTT(be);
  }

  InnerProduceOperation();

  ModDownINTT();
  ModDownBConvStep1();
  ModDownBConvStep2();
  ModDowNTT();
  ModDownSub();
}

std::pair<
    std::map<std::string, std::vector<std::vector<std::vector<Instruction *>>>>,
    std::vector<std::string>>
KeySwitch::getInsMap() {
  return std::make_pair(KeySwicthInsMap, KeySwitchInsMapName);
}

void KeySwitch::ModUpINTT() {
  // Malloc the INTT Out
  memMange->MallocMem("ModUpINTTOut", Level);

  std::vector<std::vector<std::vector<Instruction *>>>
      INttOutIns; // [level, batch, InsGroup]

  // INTT operation, which convert the input limbs from evalutaion
  // represention to coefficient repersentation
  for (uint32_t l = 0; l < Level; l++) {
    const auto inputAddr = preAddr[l];      // batch
    auto ins = DataInsMap->find(inputAddr); // group
    std::vector<std::vector<Instruction *>> insList;

    if (ins != DataInsMap->end()) {
      for (uint32_t a = 0; a < batchCount; a++) {
        auto t = DataInsMap->find(inputAddr + a);
        insList.push_back(t->second);
      }
    }

    std::vector<std::vector<Instruction *>> *beforeInsDep =
        insList.empty() ? nullptr : &insList;

    if (beforeInsDep == nullptr) {
      throw std::runtime_error("Error! This dependece need exists!\n\n");
    }

    auto outAddr = memMange->getAddr("ModUpINTTOut")[l];

    auto inttInsList = insGenPointer->GenNTT(
        l, baseName + "_ModUp_INTT(" + std::to_string(l) + ")_", beforeInsDep,
        false, inputAddr, outAddr);

    INttOutIns.push_back(inttInsList);
  }

  KeySwicthInsMap["ModUp_INTT"] = INttOutIns;
  KeySwitchInsMapName.push_back("ModUp_INTT");
}

void KeySwitch::ModUpDecompFusionBConvStep1(uint32_t beta) {

  std::vector<std::vector<std::vector<Instruction *>>> alphaIns;

  uint32_t remainLevel = Level - beta * Alpha;
  uint32_t thisLevel;
  if (remainLevel > Alpha) {
    thisLevel = Alpha;
  } else {
    thisLevel = remainLevel;
  }
  // if()

  for (uint32_t a = 0; a < thisLevel; a++) {
    uint32_t currentLevel = beta * Alpha + a;

    if (currentLevel < Level) { // 0,1
      auto Step1InsList = insGenPointer->GenEWE(
          currentLevel,
          baseName + "_decompFusionBConvStep1_beta(" + std::to_string(beta) +
              ")_Level(" + std::to_string(a) + ")_",
          &KeySwicthInsMap["ModUp_INTT"][currentLevel], nullptr, nullptr,
          nullptr, memMange->getAddr("ModUpINTTOut")[currentLevel],
          memMange->getAddr("ModUpDecompOffset")[0], 0, 0,

          memMange->getAddr("ModUpDecompOut")[currentLevel]);
      alphaIns.push_back(Step1InsList);
    }
  }
  KeySwicthInsMap["ModUp_DecompOut" + std::to_string(beta) + ")"] = alphaIns;
  KeySwitchInsMapName.push_back("ModUp_DecompOut" + std::to_string(beta) + ")");
}

void KeySwitch::ModUpBConvStep2(uint32_t beta) {
  // if (Alpha * Level > batchCount) {
  //   throw std::runtime_error(
  //       "The Basic conversation map too big, not fit into DRAM. Please "
  //       "confirm or revise the memory allocation mechanism.\n");
  // }

  memMange->MallocMemOneBatch("BConvMap_(" + std::to_string(beta) + ")", 1);

  //

  std::vector<std::vector<std::vector<Instruction *>>>
      InsBconvGroup; //[level, batch, insgroup]

  uint32_t groupLevel = Level - beta * Alpha; // 1-0
  uint32_t outStart;
  if (groupLevel > Alpha) { // !1>1
    outStart = Alpha;
  } else {
    outStart = groupLevel; // 1
  }

  uint32_t inputLevel = outStart;                 // 1
  uint32_t outLevel = Level + Alpha - inputLevel; // 1+1-1-1 = 0
  memMange->MallocMem("BConvOut_(" + std::to_string(beta) + ")", outLevel);

  for (uint32_t ol = outStart; ol < (Level + Alpha); ol++) {

    // if(Beta == 1){
    //   inputLevel= Level;
    // }
    // else{
    //   inputLevel =
    //     beta == (Beta - 1) ? Level - (Beta - 1) * Alpha : Alpha;
    // }

    auto deps = KeySwicthInsMap["ModUp_DecompOut" + std::to_string(beta) + ")"];
    auto Step2InsList = insGenPointer->GenBCONV(
        ol, inputLevel, outLevel,
        baseName + "_BCONVStep2_beta(" + std::to_string(beta) + ")_", deps,
        memMange->getAddr("ModUpDecompOut"),
        memMange->getAddr("BConvMap_(" + std::to_string(beta) + ")")[0],
        memMange->getAddr("BConvOut_(" + std::to_string(beta) +
                          ")")[ol - outStart]);
    InsBconvGroup.push_back(Step2InsList);
  }

  auto key = "ModUp_BCONV_(" + std::to_string(beta) + ")";

  KeySwicthInsMap[key] = InsBconvGroup;
  KeySwitchInsMapName.push_back("ModUp_BCONV_(" + std::to_string(beta) + ")");
}

void KeySwitch::ModUpNTT(uint32_t beta) {

  memMange->MallocMem("NTTOut_beta(" + std::to_string(beta) + ")",
                      Level + Alpha);

  // Debug
  // return;

  // std::vector<std::vector<std::vector<Instruction *>>>
  //     NttOutIns; // [level, batch, InsGroup]

  std::vector<std::vector<std::vector<Instruction *>>>
      NttGroup; //[level, batch, insgroup]
  for (uint32_t l = 0; l < Level + Alpha; l++) {
    //
    std::vector<std::vector<Instruction *>> *inputDepIns;
    AddrType inputData;

    std::string key;

    // uint32_t flagLevel = (Beta == 1)? Level :Alpha;
    uint32_t groupLevel = Level - beta * Alpha;
    uint32_t flagLevel;
    if (groupLevel >= Alpha) {
      flagLevel = Alpha;
    } else {
      flagLevel = groupLevel;
    }

    if (l < flagLevel) { // Use the output from Decomp

      // Construct the key for the map
      // key = "0(" + std::to_string(beta) + ")";
      key = "ModUp_DecompOut" + std::to_string(beta) + ")";

      // Check if the key exists in the map
      auto it = KeySwicthInsMap.find(key);
      if (it != KeySwicthInsMap.end() && (l < it->second.size())) {
        // Key exists and l is within range, safe to access
        inputDepIns = &(it->second[l]);

      } else {
        // Handle the case where the key is not found or l is out of range
        // For example, set inputDepIns to a default value or handle the error
        if (it == KeySwicthInsMap.end()) {
          std::cout << KeySwicthInsMap.size() << "\n";
          std::cout << l << " \n";
        }
        std::cout << KeySwicthInsMap.size() << "\n";
        std::cout << l << " " << it->second.size() << " \n";
        std::cout << key << "\n\n";
        std::cout << "Map elements are :\n";
        for (const auto &pair : KeySwicthInsMap) {
          std::cout << pair.first << std::endl;
        }
        throw std::runtime_error("\nkey NOt in map\n");
      }

      // Similar approach for inputData using memMange->getAddr
      std::vector<AddrType> addrVec = memMange->getAddr("ModUpDecompOut");
      if (l < addrVec.size()) {
        inputData = addrVec[l];
      } else {

        // Handle the case where l is out of range
        // For example, set inputData to a default value or handle the error
      }
    } else { // Use the output from Bconv
      // Construct a different key for the map
      key = "ModUp_BCONV_(" + std::to_string(beta) + ")";

      // Check if the key exists in the map
      auto it = KeySwicthInsMap.find(key);
      if (it != KeySwicthInsMap.end()) {
        // Key exists and l - Alpha is within range, safe to access
        inputDepIns = &(it->second[l - flagLevel]);
      } else {
        throw std::runtime_error("key not find\n");
      }

      // Similar approach for inputData using memMange->getAddr
      std::vector<AddrType> addrVec =
          memMange->getAddr("BConvOut_(" + std::to_string(beta) + ")");
      if (l < addrVec.size()) {
        inputData = addrVec[l];
      } else {
        // Handle the case where l is out of range
        // For example, set inputData to a default value or handle the error
      }
    }

    auto nttInsList = insGenPointer->GenNTT(
        l,
        baseName + "_Modup_NTT_beta(" + std::to_string(beta) + ")_level(" +
            std::to_string(l) + ")_",
        inputDepIns, true, inputData,
        memMange->getAddr("NTTOut_beta(" + std::to_string(beta) + ")")[l]);
    NttGroup.push_back(nttInsList);
  }

  KeySwicthInsMap["ModUp_NTT_(" + std::to_string(beta) + ")"] = NttGroup;
  KeySwitchInsMapName.push_back("ModUp_NTT_(" + std::to_string(beta) + ")");
}

void KeySwitch::InnerProduceOperation() {
  // Allocate memory for each temporary output
  for (int k = 0; k < 2; k++) {
    memMange->MallocMem("InnerProduceOut_Key" + std::to_string(k),
                        Level + Alpha); // Final Output

    for (uint32_t be = 0; be < Beta; be++) {
      // Allocate memory for keys
      memMange->MallocMem("IP_Key" + std::to_string(k) + "_" +
                              std::to_string(be),
                          Level + Alpha);

      if (Beta != 1 && be <= Beta - 2) { // Allocate memory for temporary
                                         // outputs if not the last iteration
        memMange->MallocMem("InnerProduceOut_temp(" + std::to_string(be) +
                                ")_Key" + std::to_string(k),
                            Level + Alpha);
      }
    }

    if (Beta == 1) {
      std::vector<std::vector<std::vector<Instruction *>>>
          IPGroup; // [level, batch, insgroup]

      for (uint32_t ml = 0; ml < Level + Alpha; ml++) {
        auto deps1 =
            &KeySwicthInsMap["ModUp_NTT_(" + std::to_string(0) + ")"][ml];
        auto deps3 = nullptr;

        auto op1addr =
            memMange->getAddr("NTTOut_beta(" + std::to_string(0) + ")")[ml];

        auto op2addr = memMange->getAddr("IP_Key" + std::to_string(k) + "_" +
                                         std::to_string(0))[ml];

        auto op3addr = 0;
        auto op4addr = 0;

        std::string outKey = "InnerProduceOut_Key" + std::to_string(k);

        auto outaddr = memMange->getAddr(outKey)[ml];

        auto IPInsList = insGenPointer->GenEWE(
            ml,
            baseName + "_InnerProducOperation(" + std::to_string(0) +
                ")_Level(" + std::to_string(ml) + ")_Key(" + std::to_string(k) +
                ")",
            deps1, nullptr, deps3, nullptr, op1addr, op2addr, op3addr, op4addr,
            outaddr);

        IPGroup.push_back(IPInsList);
      }

      KeySwicthInsMap["InnerProOut_(" + std::to_string(0) + ")_Key" +
                      std::to_string(k)] = IPGroup;

      KeySwitchInsMapName.push_back("InnerProOut_(" + std::to_string(0) +
                                    ")_Key" + std::to_string(k));

    } else {
      // Beta - 1
      for (uint32_t be = 0; be < Beta - 1; be++) {
        std::vector<std::vector<std::vector<Instruction *>>>
            IPGroup; // [level, batch, insgroup]

        for (uint32_t ml = 0; ml < Level + Alpha; ml++) {
          auto deps1 =
              &KeySwicthInsMap["ModUp_NTT_(" +
                               std::to_string(be == 0 ? 0 : be + 1) + ")"][ml];
          auto deps3 =
              (be == 0)
                  ? &KeySwicthInsMap["ModUp_NTT_(" + std::to_string(1) + ")"]
                                    [ml]
                  : &KeySwicthInsMap["InnerProOut_(" + std::to_string(be - 1) +
                                     ")_Key" + std::to_string(k)][ml];

          auto op1addr = memMange->getAddr(
              "NTTOut_beta(" + std::to_string(be == 0 ? 0 : be + 1) + ")")[ml];

          auto op2addr =
              memMange->getAddr("IP_Key" + std::to_string(k) + "_" +
                                std::to_string(be == 0 ? 0 : be + 1))[ml];

          auto op3addr =
              (be == 0) ? memMange->getAddr("NTTOut_beta(" + std::to_string(1) +
                                            ")")[ml]
                        : memMange->getAddr("InnerProduceOut_temp(" +
                                            std::to_string(be - 1) + ")_Key" +
                                            std::to_string(k))[ml];
          auto op4addr = (be == 0)
                             ? memMange->getAddr("IP_Key" + std::to_string(k) +
                                                 "_" + std::to_string(1))[ml]
                             : 0;

          std::string outKey =
              (be <= Beta - 2) ? "InnerProduceOut_temp(" + std::to_string(be) +
                                     ")_Key" + std::to_string(k)
                               : "InnerProduceOut_Key" + std::to_string(k);

          auto outaddr = memMange->getAddr(outKey)[ml];

          auto IPInsList = insGenPointer->GenEWE(
              ml,
              baseName + "_InnerProducOperation(" + std::to_string(be) +
                  ")_Level(" + std::to_string(ml) + ")_Key(" +
                  std::to_string(k) + ")",
              deps1, nullptr, deps3, nullptr, op1addr, op2addr, op3addr,
              op4addr, outaddr);

          IPGroup.push_back(IPInsList);
        }

        KeySwicthInsMap["InnerProOut_(" + std::to_string(be) + ")_Key" +
                        std::to_string(k)] = IPGroup;

        KeySwitchInsMapName.push_back("InnerProOut_(" + std::to_string(be) +
                                      ")_Key" + std::to_string(k));
      }
    }
  }
}

// ModDown
void KeySwitch::ModDownINTT() {
  for (uint32_t k = 0; k < 2; k++) {
    memMange->MallocMem("INTTOut_ModDown_Key(" + std::to_string(k) + ")",
                        Alpha);
    std::vector<std::vector<std::vector<Instruction *>>>
        NttGroup; //[level, batch, insgroup]
    for (uint32_t l = 0; l < Alpha; l++) {

      auto inputDepIns =
          &KeySwicthInsMap["InnerProOut_(" +
                           std::to_string(Beta == 1 ? 0 : Beta - 2) + ")_Key" +
                           std::to_string(k)][l];
      auto inputData =
          memMange->getAddr("InnerProduceOut_Key" + std::to_string(k))[l];
      auto outData = memMange->getAddr("INTTOut_ModDown_Key(" +
                                       std::to_string(k) + ")")[l];

      auto nttInsList =
          insGenPointer->GenNTT(l,
                                baseName + "ModDown_INTT(" + std::to_string(l) +
                                    ")_k(" + std::to_string(k) + ")",
                                inputDepIns, false, inputData, outData);
      NttGroup.push_back(nttInsList);
    }
    KeySwicthInsMap["ModDownINTTOut_Key(" + std::to_string(k) + ")"] = NttGroup;
    KeySwitchInsMapName.push_back("ModDownINTTOut_Key(" + std::to_string(k) +
                                  ")");
  }
}

void KeySwitch::ModDownBConvStep1() {

  memMange->MallocMem("ModDownBConvStep1_Ref", 2);

  for (uint32_t k = 0; k < 2; k++) {
    memMange->MallocMem("ModDownBConvStep1_Key(" + std::to_string(k) + ")",
                        Alpha);

    std::vector<std::vector<std::vector<Instruction *>>> alphaBConvIns;

    for (uint32_t l = 0; l < Alpha; l++) {
      auto deps1 = &KeySwicthInsMap["ModDownINTTOut_Key(" + std::to_string(k) +
                                    ")"][l]; // 上一轮的输出

      auto op1addr = memMange->getAddr("INTTOut_ModDown_Key(" +
                                       std::to_string(k) + ")")[l];
      auto op2addr = memMange->getAddr("ModDownBConvStep1_Ref")[k];
      auto op3addr = 0;
      auto op4addr = 0;

      auto outaddr = memMange->getAddr("ModDownBConvStep1_Key(" +
                                       std::to_string(k) + ")")[l];

      auto Step1InsList = insGenPointer->GenEWE(
          l,
          baseName + "_ModDownBConvStep1_Level(" + std::to_string(l) + +")_k(" +
              std::to_string(k) + ")",

          deps1, nullptr, nullptr, nullptr, op1addr, op2addr, op3addr,
          op4addr, // Two part is same, because none
                   // mac operation, only one
                   // computation
          outaddr);
      alphaBConvIns.push_back(Step1InsList);
    }
    KeySwicthInsMap["ModDownBConvStep1_Key(" + std::to_string(k) + ")"] =
        alphaBConvIns;
    KeySwitchInsMapName.push_back("ModDownBConvStep1_Key(" + std::to_string(k) +
                                  ")");
  }
}

void KeySwitch::ModDownBConvStep2() {
  memMange->MallocMemOneBatch("ModdownBConvMap", 1);

  //
  for (uint32_t k = 0; k < 2; k++) {

    memMange->MallocMem("ModdownBConvOut_Key" + std::to_string(k), Level);
    std::vector<std::vector<std::vector<Instruction *>>>
        InsBconvGroup; //[level, batch, insgroup]
    for (uint32_t ol = 0; ol < Level; ol++) {

      auto depsInsList =
          KeySwicthInsMap["ModDownBConvStep1_Key(" + std::to_string(k) + ")"];
      auto insAddrList =
          memMange->getAddr("ModDownBConvStep1_Key(" + std::to_string(k) + ")");
      auto convmapAddr = memMange->getAddr("ModdownBConvMap")[0];
      auto outaddr =
          memMange->getAddr("ModdownBConvOut_Key" + std::to_string(k))[ol];

      auto Step2InsList = insGenPointer->GenBCONV(
          ol, Alpha, Level,
          baseName + "_ModDownBConvStep2_" + "_k(" + std::to_string(k) + ")",
          depsInsList, insAddrList, convmapAddr, outaddr);
      InsBconvGroup.push_back(Step2InsList);
    }
    KeySwicthInsMap["ModDown_BCONV_Key(" + std::to_string(k) + ")"] =
        InsBconvGroup;
    KeySwitchInsMapName.push_back("ModDown_BCONV_Key(" + std::to_string(k) +
                                  ")");
  }
}

void KeySwitch::ModDowNTT() {
  for (uint32_t k = 0; k < 2; k++) {
    memMange->MallocMem("NTTOut_ModDown_Key(" + std::to_string(k) + ")", Level);
    std::vector<std::vector<std::vector<Instruction *>>>
        NttGroup; //[level, batch, insgroup]
    for (uint32_t l = 0; l < Level; l++) {

      auto inputDepIns =
          &KeySwicthInsMap["ModDown_BCONV_Key(" + std::to_string(k) + ")"][l];
      auto inputData =
          memMange->getAddr("ModdownBConvOut_Key" + std::to_string(k))[l];
      auto outData =
          memMange->getAddr("NTTOut_ModDown_Key(" + std::to_string(k) + ")")[l];

      auto nttInsList =
          insGenPointer->GenNTT(l,
                                baseName + "ModDown_NTT(" + std::to_string(l) +
                                    +")_k(" + std::to_string(k) + ")",
                                inputDepIns, false, inputData, outData);
      NttGroup.push_back(nttInsList);
    }
    KeySwicthInsMap["ModDownNTTOut_Key(" + std::to_string(k) + ")"] = NttGroup;
    KeySwitchInsMapName.push_back("ModDownNTTOut_Key(" + std::to_string(k) +
                                  ")");
  }
}

void KeySwitch::ModDownSub() {
  for (uint32_t k = 0; k < 2; k++) {
    std::vector<std::vector<std::vector<Instruction *>>> alphaIns;
    memMange->MallocMem("KeySwitchFinalOutput_Key(" + std::to_string(k) + ")",
                        Level);

    std::vector<std::vector<std::vector<Instruction *>>> alphaBConvIns;
    for (uint32_t l = 0; l < Level; l++) {

      auto deps1 = &KeySwicthInsMap["ModDownNTTOut_Key(" + std::to_string(k) +
                                    ")"][l]; // 上一轮的输出
      auto deps2 = &KeySwicthInsMap["InnerProOut_(" +
                                    std::to_string(Beta == 1 ? 0 : Beta - 2) +
                                    ")_Key" + std::to_string(k)][Alpha + l];

      auto op1addr =
          memMange->getAddr("NTTOut_ModDown_Key(" + std::to_string(k) + ")")[l];
      auto op2addr = memMange->getAddr("InnerProduceOut_Key" +
                                       std::to_string(k))[Alpha + l];
      auto op3addr = 0;
      auto op4addr = 0;

      auto outaddr = memMange->getAddr("KeySwitchFinalOutput_Key(" +
                                       std::to_string(k) + ")")[l];

      auto Step1InsList = insGenPointer->GenEWE(
          l,
          baseName + "_KeySwitchFinalOutput_Level(" + std::to_string(l) +
              +")_k(" + std::to_string(k) + ")",

          deps1, deps2, nullptr, nullptr, op1addr, op2addr, op3addr,
          op4addr, // Two part is same, because none
                   // mac operation, only one
                   // computation
          outaddr);
      alphaBConvIns.push_back(Step1InsList);
    }
    KeySwicthInsMap["KeySwitchFinalOutput_Key(" + std::to_string(k) + ")"] =
        alphaBConvIns;
    KeySwitchInsMapName.push_back("KeySwitchFinalOutput_Key(" +
                                  std::to_string(k) + ")");
  }
}

TensorCompute::TensorCompute(
    std::string labelName, uint32_t level, Ciphertext *cipher1,
    Ciphertext *cipher2, std::vector<AddrType> *pool,
    std::map<AddrType, std::vector<Instruction *>> *map, InsGen *insgen,
    AddrManage *memoryMange) {
  DataInsMap = map;
  DataPool = pool;
  currentLevel = level;

  insGenPointer = insgen;
  batchCount = insGenPointer->getbatchCount();
  memMange = memoryMange;

  baseName = labelName + "_TensorCompute";

  ciph1_c0 = cipher1->getC0Addr();
  ciph1_c1 = cipher1->getC1Addr();

  ciph2_c0 = cipher2->getC0Addr();
  ciph2_c1 = cipher2->getC1Addr();

  /****
   * d0 = c00*c10;
   * d1 = c00*C11+C01*C10
   * d2 = c01*c11
   */

  computeD0();
  computeD1();
  computeD2();
}

void TensorCompute::computeD0() {

  // d0 = c00*c10;

  // Malloc the D0 Out
  memMange->MallocMem("TensorD0Out", currentLevel);

  std::vector<std::vector<std::vector<Instruction *>>>
      D0OutIns; // [level, batch, InsGroup]

  for (uint32_t l = 0; l < currentLevel; l++) {

    // NotSuppotr the continuous operation simulate
    auto deps1 = nullptr;
    auto deps2 = nullptr;
    auto deps3 = nullptr;
    auto deps4 = nullptr;

    //
    auto op1addr = ciph1_c0[l];
    auto op2addr = ciph2_c0[l];

    auto op3addr = 0;
    auto op4addr = 0;

    auto outaddr = memMange->getAddr("TensorD0Out")[l];

    auto d0InsList = insGenPointer->GenEWE(
        l, baseName + "_TensorCompute_D0_level(" + std::to_string(l) + ")",
        deps1, deps2, deps3, deps4, op1addr, op2addr, op3addr, op4addr,
        outaddr);

    D0OutIns.push_back(d0InsList);
  }

  TensorComputeInsMap["TensorCompute_INS_D0"] = D0OutIns;
  TensorComputeInsMapName.push_back("TensorCompute_INS_D0");
}

void TensorCompute::computeD1() {

  // d1 = c00*C11+C01*C10

  // Malloc the D1 Out
  memMange->MallocMem("TensorD1Out", currentLevel);

  std::vector<std::vector<std::vector<Instruction *>>>
      D1OutIns; // [level, batch, InsGroup]

  for (uint32_t l = 0; l < currentLevel; l++) {

    // NotSuppotr the continuous operation simulate
    auto deps1 = nullptr;
    auto deps2 = nullptr;
    auto deps3 = nullptr;
    auto deps4 = nullptr;

    //
    auto op1addr = ciph1_c0[l];
    auto op2addr = ciph2_c1[l];

    auto op3addr = ciph1_c1[l];
    auto op4addr = ciph2_c0[l];

    auto outaddr = memMange->getAddr("TensorD1Out")[l];

    auto d1InsList = insGenPointer->GenEWE(
        l, baseName + "_TensorCompute_D1_level(" + std::to_string(l) + ")",
        deps1, deps2, deps3, deps4, op1addr, op2addr, op3addr, op4addr,
        outaddr);

    D1OutIns.push_back(d1InsList);
  }

  TensorComputeInsMap["TensorCompute_INS_D1"] = D1OutIns;
  TensorComputeInsMapName.push_back("TensorCompute_INS_D1");
}

void TensorCompute::computeD2() {

  // d2 = c01*c11

  // Malloc the D2 Out
  memMange->MallocMem("TensorD2Out", currentLevel);

  std::vector<std::vector<std::vector<Instruction *>>>
      D2OutIns; // [level, batch, InsGroup]

  for (uint32_t l = 0; l < currentLevel; l++) {

    // NotSuppotr the continuous operation simulate
    auto deps1 = nullptr;
    auto deps2 = nullptr;
    auto deps3 = nullptr;
    auto deps4 = nullptr;

    //
    auto op1addr = ciph1_c1[l];
    auto op2addr = ciph2_c1[l];

    auto op3addr = 0;
    auto op4addr = 0;

    auto outaddr = memMange->getAddr("TensorD2Out")[l];

    auto d2InsList = insGenPointer->GenEWE(
        l, baseName + "_TensorCompute_D2_level(" + std::to_string(l) + ")",
        deps1, deps2, deps3, deps4, op1addr, op2addr, op3addr, op4addr,
        outaddr);

    D2OutIns.push_back(d2InsList);
  }

  TensorComputeInsMap["TensorCompute_INS_D2"] = D2OutIns;
  TensorComputeInsMapName.push_back("TensorCompute_INS_D2");
}

Rescale::Rescale(std::string labelName, uint32_t level,

                 const std::vector<AddrType>
                     &inputPolynomialAddress, // start addr for each level
                 std::vector<AddrType> *pool,
                 std::map<AddrType, std::vector<Instruction *>> *map,
                 InsGen *insgen, AddrManage *memoryMange) {

  DataInsMap = map;
  DataPool = pool;
  currentLevel = level; // Currentlevel [0,...,Level]

  insGenPointer = insgen;
  batchCount = insGenPointer->getbatchCount();
  memMange = memoryMange;

  preAddr = inputPolynomialAddress;

  baseName = labelName + "_Rescale";

  NTTOps();
  SubOps();
  MulOps();
}

void Rescale::NTTOps() {
  // Malloc the INTT Out
  memMange->MallocMem(baseName + "_ResINTTOut", 1);
  memMange->MallocMem(baseName + "_ResNTTOut", 1);

  std::vector<std::vector<std::vector<Instruction *>>>
      INttOutIns; // [level, batch, InsGroup]

  // INTT operation, which convert the input limbs from evalutaion
  // represention to coefficient repersentation
  for (uint32_t l = 0; l < 1; l++) {
    const auto inputAddr = preAddr[currentLevel - 1]; // batch
    auto ins = DataInsMap->find(inputAddr);           // group
    std::vector<std::vector<Instruction *>> insList;

    if (ins != DataInsMap->end()) {
      for (uint32_t a = 0; a < batchCount; a++) {
        auto t = DataInsMap->find(inputAddr + a);
        insList.push_back(t->second);
      }
    }

    std::vector<std::vector<Instruction *>> *beforeInsDep =
        insList.empty() ? nullptr : &insList;

    if (beforeInsDep == nullptr) {
      throw std::runtime_error("Error! This dependece need exists!\n\n");
    }

    auto outAddr = memMange->getAddr(baseName + "_ResINTTOut")[l];

    auto inttInsList = insGenPointer->GenNTT(
        l, baseName + "_Rescale_INTT(" + std::to_string(l) + ")_", beforeInsDep,
        false, inputAddr, outAddr);

    INttOutIns.push_back(inttInsList);
  }

  RescaleInsMap["Rescale_INTT"] = INttOutIns;
  RescaleInsMapName.push_back("Rescale_INTT");

  std::vector<std::vector<std::vector<Instruction *>>>
      NttGroup; //[level, batch, insgroup]

  for (uint32_t l = 0; l < 1; l++) {
    auto beforeDeps = &INttOutIns[l];

    auto inputAddr = memMange->getAddr(baseName + "_ResINTTOut")[l];

    auto nttInsList = insGenPointer->GenNTT(
        l, baseName + "_Rescale_NTT_level(" + std::to_string(l) + ")",
        beforeDeps, true, inputAddr,
        memMange->getAddr(baseName + "_ResNTTOut")[l]);
    NttGroup.push_back(nttInsList);
  }
  RescaleInsMap["Rescale_NTT"] = NttGroup;
  RescaleInsMapName.push_back("Rescale_NTT");
}

void Rescale::SubOps() {

  // std::vector<std::vector<std::vector<Instruction *>>> subIns;
  memMange->MallocMem(baseName + "_Rescale_SubOut", currentLevel - 1);

  std::vector<std::vector<std::vector<Instruction *>>> subIns;
  for (uint32_t l = 0; l < currentLevel - 1; l++) {
    const auto inputAddr = preAddr[l];      // batch
    auto ins = DataInsMap->find(inputAddr); // group
    std::vector<std::vector<Instruction *>> insList;

    if (ins != DataInsMap->end()) {
      for (uint32_t a = 0; a < batchCount; a++) {
        auto t = DataInsMap->find(inputAddr + a);
        insList.push_back(t->second);
      }
    }

    std::vector<std::vector<Instruction *>> *beforeInsDep =
        insList.empty() ? nullptr : &insList;

    if (beforeInsDep == nullptr) {
      throw std::runtime_error("Error! This dependece need exists!\n\n");
    }
    // else{
    //   insList[0][0]->ShowIns();
    // }

    auto deps1 = &RescaleInsMap["Rescale_NTT"][0]; // 上一轮的输出
    auto deps2 = beforeInsDep;

    auto op1addr = memMange->getAddr(baseName + "_ResNTTOut")[0];
    auto op2addr = inputAddr;

    auto op3addr = 0;
    auto op4addr = 0;

    auto outaddr = memMange->getAddr(baseName + "_Rescale_SubOut")[l];
    auto InsList = insGenPointer->GenEWE(
        l, baseName + "_Rescale_Sub_Level(" + std::to_string(l) + ")",

        deps1, deps2, nullptr, nullptr, op1addr, op2addr, op3addr,
        op4addr, // Two part is same, because none
                 // mac operation, only one
                 // computation
        outaddr);

    subIns.push_back(InsList);
  }

  RescaleInsMap["Rescale_SUB"] = subIns;
  RescaleInsMapName.push_back("Rescale_SUB");
}

void Rescale::MulOps() {

  memMange->MallocMem(baseName + "_Rescale_MulOut", currentLevel - 1);
  memMange->MallocMem(baseName + "_Rescale_Mul_Offset", currentLevel - 1);

  std::vector<std::vector<std::vector<Instruction *>>> mulIns;
  for (uint32_t l = 0; l < currentLevel - 1; l++) {

    auto deps1 = &RescaleInsMap["Rescale_SUB"][l]; // 上一轮的输出
    auto deps2 = nullptr;

    auto op1addr = memMange->getAddr(baseName + "_Rescale_SubOut")[l];
    auto op2addr = memMange->getAddr(baseName + "_Rescale_Mul_Offset")[l];

    auto op3addr = 0;
    auto op4addr = 0;

    auto outaddr = memMange->getAddr(baseName + "_Rescale_MulOut")[l];
    auto InsList = insGenPointer->GenEWE(
        l, baseName + "_Rescale_Mul_Level(" + std::to_string(l) + ")",

        deps1, deps2, nullptr, nullptr, op1addr, op2addr, op3addr,
        op4addr, // Two part is same, because none
                 // mac operation, only one
                 // computation
        outaddr);

    mulIns.push_back(InsList);
  }

  RescaleInsMap["Rescale_Mul"] = mulIns;
  RescaleInsMapName.push_back("Rescale_Mul");
}

HMULT::HMULT(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
             uint32_t alpha, Config *cfg, Arch *_arch) {

  datamap = new DataMap();
  insgener = new InsGen(cfg);
  driver = new Driver(cfg, datamap);

  arch = _arch;

  uint32_t batchSize = cfg->getValue("batchSize");

  // std::cout<<"BAT "<< batchSize<<"\n";

  insgener->setGlobalDatapPoll(&Datapool);
  insgener->setGlobalDataInsMap(&DataInsMap);
  insgener->setGlobalDataMap(datamap);

  arch->setDataMap(datamap);
  memControlList = arch->getMemController();

  Datapool.push_back(BASEADDRESS);

  c1 = new Ciphertext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  c2 = new Ciphertext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  addrManager = new AddrManage(Datapool[Datapool.size() - 1] + 1, batchSize);
  addrManager->setGlobalDatapPoll(&Datapool);

  TensorCompute *tcm =
      new TensorCompute(labelName, currentLevel, c1, c2, &Datapool, &DataInsMap,
                        insgener, addrManager);
  auto tcmMap = tcm->getInsMap();

  for (auto &key : tcmMap.second) {
    auto map = tcmMap.first[key];
    driver->dispatchInstructions(map);
  }
  // tensorcompute

  auto depsAddr =
      addrManager->getAddr("TensorD0Out"); // the start address for each level
  KeySwitch *ksw =
      new KeySwitch(labelName, maxLevel, currentLevel, alpha, depsAddr,
                    &Datapool, &DataInsMap, insgener, addrManager);

  auto kswMap = ksw->getInsMap();

  for (auto &key : kswMap.second) {
    auto map = kswMap.first[key];
    driver->dispatchInstructions(map);
  }
  // ksw

  std::vector<AddrType> c0 =
      addrManager->getAddr("KeySwitchFinalOutput_Key(0)");
  std::vector<AddrType> c1 =
      addrManager->getAddr("KeySwitchFinalOutput_Key(1)");

  std::vector<AddrType> d0 = addrManager->getAddr("TensorD2Out");
  std::vector<AddrType> d1 = addrManager->getAddr("TensorD1Out");

  for (uint32_t k = 0; k < 2; k++) {
    std::vector<std::vector<std::vector<Instruction *>>> HMULTHaddInsOut;
    addrManager->MallocMem("HMULTHaddOutput(" + std::to_string(k) + ")",
                           currentLevel);
    for (uint32_t l = 0; l < currentLevel; l++) {

      auto op1addr = k == 0 ? c0[l] : c1[l];
      auto op2addr = 0;
      auto op3addr = k == 0 ? d1[l] : d0[l];
      auto op4addr = 0;

      auto outaddr =
          addrManager->getAddr("HMULTHaddOutput(" + std::to_string(k) + ")")[l];

      auto InsList =
          insgener->GenEWE(l,
                           labelName + "_HMULTHadd_Level(" + std::to_string(l) +
                               +")_k(" + std::to_string(k) + ")",
                           nullptr, nullptr, nullptr, nullptr, op1addr, op2addr,
                           op3addr, op4addr, outaddr);
      HMULTHaddInsOut.push_back(InsList);
    }
    HMULTHaddInsMap["HMULT_Hadd_Key(" + std::to_string(k) + ")"] =
        HMULTHaddInsOut;
    HMULTHaddInsMapName.push_back("HMULT_Hadd_Key(" + std::to_string(k) + ")");
  }

  for (auto &key : HMULTHaddInsMapName) {
    auto map = HMULTHaddInsMap[key];
    driver->dispatchInstructions(map);
  }
  // rescale

  for (uint32_t k = 0; k < 2; k++) {
    auto depsAddr =
        addrManager->getAddr("HMULTHaddOutput(" + std::to_string(k) +
                             ")"); // the start address for each level
    Rescale *res =
        new Rescale(labelName + "_" + std::to_string(k), currentLevel, depsAddr,
                    &Datapool, &DataInsMap, insgener, addrManager);

    auto resMap = res->getInsMap();

    for (auto &key : resMap.second) {
      auto map = resMap.first[key];
      driver->dispatchInstructions(map);
    }
  }
}

bool HMULT::simulate() {
  // MemControl fetch data and issue the corresponsding instructions
  driver->IssueInsFromDramToChip(arch);

  auto TotalIns = driver->getTotalIns();

  // cycle-level
  unsigned long long cycle = 0;

  unsigned long long exeInsCycle = 0;

  std::cout << "\n\nWelcome! Start simulating HMULT!\n\n";

  // Output current time
  time_t currentTime = time(0);

  // Convert current time to string representation
  char *timeString = ctime(&currentTime);
  std::cout << "Start time: " << timeString << std::endl;

  time_t periodTime = time(0);
  while (!arch->simulateComplete()) {
    // MemControl fetches data and issues the corresponding instructions
    driver->IssueDataFromDramToChip(memControlList);

    arch->update();

    cycle = arch->getCycle();
    if (cycle % 2000 == 0) {

      auto exeins = arch->getcompletedIns() - exeInsCycle;

      if (exeins == 0) {
        std::cout << "We have executed " << exeins
                  << " instruction(s) in this period!\n";
        arch->state();

        std::cout << "\n";
        driver->shownData();
        // std::cout << "Memory state\n";
        // arch->shownMemState();
        break;
      }

      std::cout << "\nFHE-Sim running " << cycle << " cycles!\n";
      std::cout << "We have executed " << arch->getcompletedIns()
                << " instructions!\n";
      auto remainIns = TotalIns - arch->getcompletedIns();
      std::cout << "Remaining " << remainIns << " instructions!\n";
      std::cout << "We have executed " << exeins
                << " instruction(s) in this period!\n";

      time_t nowTime = time(0);
      auto exetime = nowTime - periodTime;
      auto speed = static_cast<double>(exeins) / exetime;
      auto remainTime = static_cast<double>(remainIns) / speed;
      std::cout << "Estimated time remaining " << remainTime / 60
                << " minutes\n";
      periodTime = nowTime;

      exeInsCycle = arch->getcompletedIns();
    }
  }

  time_t currentTime2 = time(0);

  // Convert current time to string representation
  char *timeString2 = ctime(&currentTime2);

  std::cout << "\n\nCompleted Simulate!\n";
  std::cout << "FHE-Sim Total simulated\t" << cycle << " cycles!\n\n";

  std::cout << "End time: " << timeString2 << std::endl;

  std::cout << "The simulator total cost\t"
            << static_cast<double>(currentTime2 - currentTime) / 60
            << " Minutes!\n";

  // arch->state();
  // std::cout << "mem state\n";
  // arch->shownMemState();
  // std::cout<<"\n";
  // driver->shownData();

  arch->shownStat();

  return true;
}

HADD::HADD(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
           uint32_t alpha, Config *cfg, Arch *_arch) {
  datamap = new DataMap();
  insgener = new InsGen(cfg);
  driver = new Driver(cfg, datamap);

  arch = _arch;

  uint32_t batchSize = cfg->getValue("batchSize");

  insgener->setGlobalDatapPoll(&Datapool);
  insgener->setGlobalDataInsMap(&DataInsMap);
  insgener->setGlobalDataMap(datamap);

  arch->setDataMap(datamap);
  memControlList = arch->getMemController();

  Datapool.push_back(BASEADDRESS);

  c1 = new Ciphertext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  c2 = new Ciphertext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  addrManager = new AddrManage(Datapool[Datapool.size() - 1] + 1, batchSize);
  addrManager->setGlobalDatapPoll(&Datapool);

  std::vector<AddrType> ciph1_c0 = c1->getC0Addr();
  std::vector<AddrType> ciph1_c1 = c1->getC1Addr();

  std::vector<AddrType> ciph2_c0 = c2->getC0Addr();
  std::vector<AddrType> ciph2_c1 = c2->getC1Addr();

  for (uint32_t k = 0; k < 2; k++) {
    std::vector<std::vector<std::vector<Instruction *>>> HADDInsOut;
    addrManager->MallocMem("HADDOutput(" + std::to_string(k) + ")",
                           currentLevel);
    for (uint32_t l = 0; l < currentLevel; l++) {

      auto op1addr = k == 0 ? ciph1_c0[l] : ciph1_c1[l];
      auto op2addr = 0;
      auto op3addr = k == 0 ? ciph2_c0[l] : ciph2_c1[l];
      auto op4addr = 0;

      auto outaddr =
          addrManager->getAddr("HADDOutput(" + std::to_string(k) + ")")[l];

      auto InsList =
          insgener->GenEWE(l,
                           labelName + "_HADD_Level(" + std::to_string(l) +
                               +")_k(" + std::to_string(k) + ")",
                           nullptr, nullptr, nullptr, nullptr, op1addr, op2addr,
                           op3addr, op4addr, outaddr);
      HADDInsOut.push_back(InsList);
    }
    HADDInsMap["HADD_Key(" + std::to_string(k) + ")"] = HADDInsOut;
    HADDInsMapName.push_back("HADD_Key(" + std::to_string(k) + ")");
  }

  for (auto &key : HADDInsMapName) {
    auto map = HADDInsMap[key];
    driver->dispatchInstructions(map);
  }
}

bool HADD::simulate() {
  // MemControl fetch data and issue the corresponsding instructions
  driver->IssueInsFromDramToChip(arch);

  auto TotalIns = driver->getTotalIns();

  // cycle-level
  unsigned long long cycle = 0;

  unsigned long long exeInsCycle = 0;

  std::cout << "\n\nWelcome! Start simulating HADD!\n\n";

  // Output current time
  time_t currentTime = time(0);

  // Convert current time to string representation
  char *timeString = ctime(&currentTime);
  std::cout << "Start time: " << timeString << std::endl;

  time_t periodTime = time(0);
  while (!arch->simulateComplete()) {
    // MemControl fetches data and issues the corresponding instructions
    driver->IssueDataFromDramToChip(memControlList);

    arch->update();

    cycle = arch->getCycle();
    if (cycle % 2000 == 0) {

      auto exeins = arch->getcompletedIns() - exeInsCycle;

      if (exeins == 0) {
        std::cout << "We have executed " << exeins
                  << " instruction(s) in this period!\n";
        arch->state();

        std::cout << "\n";
        driver->shownData();
        // std::cout << "Memory state\n";
        // arch->shownMemState();
        break;
      }

      // std::cout << "ARCH State\n\n";
      // arch->state();
      // driver->shownData();

      std::cout << "\nFHE-Sim running " << cycle << " cycles!\n";
      std::cout << "We have executed " << arch->getcompletedIns()
                << " instructions!\n";
      auto remainIns = TotalIns - arch->getcompletedIns();
      std::cout << "Remaining " << remainIns << " instructions!\n";
      std::cout << "We have executed " << exeins
                << " instruction(s) in this period!\n";

      time_t nowTime = time(0);
      auto exetime = nowTime - periodTime;
      auto speed = static_cast<double>(exeins) / exetime;
      auto remainTime = static_cast<double>(remainIns) / speed;
      std::cout << "Estimated time remaining " << remainTime / 60
                << " minutes\n";
      periodTime = nowTime;

      exeInsCycle = arch->getcompletedIns();
    }
  }

  time_t currentTime2 = time(0);

  // Convert current time to string representation
  char *timeString2 = ctime(&currentTime2);

  std::cout << "\n\nCompleted Simulate!\n";
  std::cout << "FHE-Sim Total simulated\t" << cycle << " cycles!\n\n";

  std::cout << "End time: " << timeString2 << std::endl;

  std::cout << "The simulator total cost\t"
            << static_cast<double>(currentTime2 - currentTime) / 60
            << " Minutes!\n";

  // arch->state();
  // std::cout << "mem state\n";
  // arch->shownMemState();
  // std::cout<<"\n";
  // driver->shownData();

  arch->shownStat();

  return true;
}

HROTATE::HROTATE(std::string labelName, uint32_t maxLevel,
                 uint32_t currentLevel, uint32_t alpha, Config *cfg,
                 Arch *_arch) {

  datamap = new DataMap();
  insgener = new InsGen(cfg);
  driver = new Driver(cfg, datamap);

  arch = _arch;

  uint32_t batchSize = cfg->getValue("batchSize");

  // std::cout<<"BAT "<< batchSize<<"\n";

  insgener->setGlobalDatapPoll(&Datapool);
  insgener->setGlobalDataInsMap(&DataInsMap);
  insgener->setGlobalDataMap(datamap);

  arch->setDataMap(datamap);
  memControlList = arch->getMemController();

  Datapool.push_back(BASEADDRESS);

  ciph = new Ciphertext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  std::vector<AddrType> ciph_c0 = ciph->getC0Addr();
  std::vector<AddrType> ciph_c1 = ciph->getC1Addr();

  addrManager = new AddrManage(Datapool[Datapool.size() - 1] + 1, batchSize);
  addrManager->setGlobalDatapPoll(&Datapool);

  for (uint32_t k = 0; k < 2; k++) {
    std::vector<std::vector<std::vector<Instruction *>>> AUTOInsOut;
    addrManager->MallocMem("AUTOOutput(" + std::to_string(k) + ")",
                           currentLevel);
    auto opaddr = k == 0 ? ciph_c0 : ciph_c1;
    for (uint32_t l = 0; l < currentLevel; l++) {
      auto outaddr =
          addrManager->getAddr("AUTOOutput(" + std::to_string(k) + ")")[l];
      auto InsList =
          insgener->GenAUTO(l,
                            labelName + "_HADD_Level(" + std::to_string(l) +
                                +")_k(" + std::to_string(k) + ")",
                            nullptr, opaddr[l], outaddr);
      AUTOInsOut.push_back(InsList);
    }
    AUTOInsMap["AUTO_Key(" + std::to_string(k) + ")"] = AUTOInsOut;
    AUTOInsMapName.push_back("AUTO_Key(" + std::to_string(k) + ")");
  }

  for (auto &key : AUTOInsMapName) {
    auto map = AUTOInsMap[key];
    driver->dispatchInstructions(map);
  }

  auto depsAddr =
      addrManager->getAddr("AUTOOutput(0)"); // the start address for each level
  KeySwitch *ksw =
      new KeySwitch(labelName, maxLevel, currentLevel, alpha, depsAddr,
                    &Datapool, &DataInsMap, insgener, addrManager);

  auto kswMap = ksw->getInsMap();

  for (auto &key : kswMap.second) {
    auto map = kswMap.first[key];
    driver->dispatchInstructions(map);
  }

  std::vector<std::vector<std::vector<Instruction *>>> HROTATEHaddInsOut;
  addrManager->MallocMem("HROTATEOutput(1)", currentLevel);

  for (uint32_t l = 0; l < currentLevel; l++) {

    auto op1addr = addrManager->getAddr("KeySwitchFinalOutput_Key(1)")[l];
    auto op3addr = addrManager->getAddr("AUTOOutput(1)")[l];
    auto op2addr = 0, op4addr = 0;
    auto outaddr = addrManager->getAddr("HROTATEOutput(1)")[l];

    auto deps1 = kswMap.first["KeySwitchFinalOutput_Key(1)"][l];
    auto deps2 = AUTOInsMap["AUTO_Key(1)"][l];
    auto InsList = insgener->GenEWE(
        l, labelName + "_HROTATE_HADD_Level(" + std::to_string(l) + ")_k(1)",
        &deps1, nullptr, &deps2, nullptr, op1addr, op2addr, op3addr, op4addr,
        outaddr);
    HROTATEHaddInsOut.push_back(InsList);
  }
  driver->dispatchInstructions(HROTATEHaddInsOut);
}

bool HROTATE::simulate() {
  // MemControl fetch data and issue the corresponsding instructions
  driver->IssueInsFromDramToChip(arch);

  auto TotalIns = driver->getTotalIns();

  // cycle-level
  unsigned long long cycle = 0;

  unsigned long long exeInsCycle = 0;

  std::cout << "\n\nWelcome! Start simulating HROTATE!\n\n";

  // Output current time
  time_t currentTime = time(0);

  // Convert current time to string representation
  char *timeString = ctime(&currentTime);
  std::cout << "Start time: " << timeString << std::endl;

  time_t periodTime = time(0);
  while (!arch->simulateComplete()) {
    // MemControl fetches data and issues the corresponding instructions
    driver->IssueDataFromDramToChip(memControlList);

    arch->update();

    cycle = arch->getCycle();
    if (cycle % 2000 == 0) {

      auto exeins = arch->getcompletedIns() - exeInsCycle;

      if (exeins == 0) {
        std::cout << "We have executed " << exeins
                  << " instruction(s) in this period!\n";
        arch->state();

        std::cout << "\n";
        driver->shownData();
        // std::cout << "Memory state\n";
        // arch->shownMemState();
        break;
      }

      // std::cout << "ARCH State\n\n";
      // arch->state();
      // driver->shownData();

      std::cout << "\nFHE-Sim running " << cycle << " cycles!\n";
      std::cout << "We have executed " << arch->getcompletedIns()
                << " instructions!\n";
      auto remainIns = TotalIns - arch->getcompletedIns();
      std::cout << "Remaining " << remainIns << " instructions!\n";
      std::cout << "We have executed " << exeins
                << " instruction(s) in this period!\n";

      time_t nowTime = time(0);
      auto exetime = nowTime - periodTime;
      auto speed = static_cast<double>(exeins) / exetime;
      auto remainTime = static_cast<double>(remainIns) / speed;
      std::cout << "Estimated time remaining " << remainTime / 60
                << " minutes\n";
      periodTime = nowTime;

      exeInsCycle = arch->getcompletedIns();
    }
  }

  time_t currentTime2 = time(0);

  // Convert current time to string representation
  char *timeString2 = ctime(&currentTime2);

  std::cout << "\n\nCompleted Simulate!\n";
  std::cout << "FHE-Sim Total simulated\t" << cycle << " cycles!\n\n";

  std::cout << "End time: " << timeString2 << std::endl;

  std::cout << "The simulator total cost\t"
            << static_cast<double>(currentTime2 - currentTime) / 60
            << " Minutes!\n";

  // arch->state();
  // std::cout << "mem state\n";
  // arch->shownMemState();
  // std::cout<<"\n";
  // driver->shownData();

  arch->shownStat();

  return true;
}

PMULT::PMULT(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
             uint32_t alpha, Config *cfg, Arch *_arch) {

  datamap = new DataMap();
  insgener = new InsGen(cfg);
  driver = new Driver(cfg, datamap);

  arch = _arch;

  uint32_t batchSize = cfg->getValue("batchSize");

  // std::cout<<"BAT "<< batchSize<<"\n";

  insgener->setGlobalDatapPoll(&Datapool);
  insgener->setGlobalDataInsMap(&DataInsMap);
  insgener->setGlobalDataMap(datamap);

  arch->setDataMap(datamap);
  memControlList = arch->getMemController();

  Datapool.push_back(BASEADDRESS);

  ctx = new Ciphertext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  ptx = new Plaintext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  addrManager = new AddrManage(Datapool[Datapool.size() - 1] + 1, batchSize);
  addrManager->setGlobalDatapPoll(&Datapool);

  auto c0 = ctx->getC0Addr();
  auto c1 = ctx->getC1Addr();

  auto p0 = ptx->getC0Addr();

  for (uint32_t k = 0; k < 2; k++) {
    std::vector<std::vector<std::vector<Instruction *>>> PMULTOutIns;
    addrManager->MallocMem("HMult" + std::to_string(k) + "Out", currentLevel);
    for (uint32_t l = 0; l < currentLevel; l++) {

      // NotSuppotr the continuous operation simulate
      auto deps1 = nullptr;
      auto deps2 = nullptr;
      auto deps3 = nullptr;
      auto deps4 = nullptr;

      //
      auto op1addr = k == 0 ? c0[l] : c1[l];
      auto op2addr = p0[l];
      auto op3addr = 0;
      auto op4addr = 0;

      auto outaddr =
          addrManager->getAddr("HMult" + std::to_string(k) + "Out")[l];
      auto HMULTInsList =
          insgener->GenEWE(l,
                           labelName + "_HMULT_level(" + std::to_string(l) +
                               ")_Key(" + std::to_string(k) + ")",
                           deps1, deps2, deps3, deps4, op1addr, op2addr,
                           op3addr, op4addr, outaddr);

      PMULTOutIns.push_back(HMULTInsList);
    }

    PMULTInsMap["PMULT_Key(" + std::to_string(k) + ")"] = PMULTOutIns;
    PMULTInsMapName.push_back("PMULT_Key(" + std::to_string(k) + ")");
  }
  for (auto &key : PMULTInsMapName) {
    auto map = PMULTInsMap[key];
    driver->dispatchInstructions(map);
  }
}

bool PMULT::simulate() {
  // MemControl fetch data and issue the corresponsding instructions
  driver->IssueInsFromDramToChip(arch);

  auto TotalIns = driver->getTotalIns();

  // cycle-level
  unsigned long long cycle = 0;

  unsigned long long exeInsCycle = 0;

  std::cout << "\n\nWelcome! Start simulating PMULT!\n\n";

  // Output current time
  time_t currentTime = time(0);

  // Convert current time to string representation
  char *timeString = ctime(&currentTime);
  std::cout << "Start time: " << timeString << std::endl;

  time_t periodTime = time(0);
  while (!arch->simulateComplete()) {
    // MemControl fetches data and issues the corresponding instructions
    driver->IssueDataFromDramToChip(memControlList);

    arch->update();

    cycle = arch->getCycle();
    if (cycle % 2000 == 0) {

      auto exeins = arch->getcompletedIns() - exeInsCycle;

      if (exeins == 0) {
        std::cout << "We have executed " << exeins
                  << " instruction(s) in this period!\n";
        arch->state();

        std::cout << "\n";
        driver->shownData();
        // std::cout << "Memory state\n";
        // arch->shownMemState();
        break;
      }

      // std::cout << "ARCH State\n\n";
      // arch->state();
      // driver->shownData();

      std::cout << "\nFHE-Sim running " << cycle << " cycles!\n";
      std::cout << "We have executed " << arch->getcompletedIns()
                << " instructions!\n";
      auto remainIns = TotalIns - arch->getcompletedIns();
      std::cout << "Remaining " << remainIns << " instructions!\n";
      std::cout << "We have executed " << exeins
                << " instruction(s) in this period!\n";

      time_t nowTime = time(0);
      auto exetime = nowTime - periodTime;
      auto speed = static_cast<double>(exeins) / exetime;
      auto remainTime = static_cast<double>(remainIns) / speed;
      std::cout << "Estimated time remaining " << remainTime / 60
                << " minutes\n";
      periodTime = nowTime;

      exeInsCycle = arch->getcompletedIns();
    }
  }

  time_t currentTime2 = time(0);

  // Convert current time to string representation
  char *timeString2 = ctime(&currentTime2);

  std::cout << "\n\nCompleted Simulate!\n";
  std::cout << "FHE-Sim Total simulated\t" << cycle << " cycles!\n\n";

  std::cout << "End time: " << timeString2 << std::endl;

  std::cout << "The simulator total cost\t"
            << static_cast<double>(currentTime2 - currentTime) / 60
            << " Minutes!\n";

  // arch->state();
  // std::cout << "mem state\n";
  // arch->shownMemState();
  // std::cout<<"\n";
  // driver->shownData();

  arch->shownStat();

  return true;
}

PADD::PADD(std::string labelName, uint32_t maxLevel, uint32_t currentLevel,
           uint32_t alpha, Config *cfg, Arch *_arch) {

  datamap = new DataMap();
  insgener = new InsGen(cfg);
  driver = new Driver(cfg, datamap);

  arch = _arch;

  uint32_t batchSize = cfg->getValue("batchSize");

  insgener->setGlobalDatapPoll(&Datapool);
  insgener->setGlobalDataInsMap(&DataInsMap);
  insgener->setGlobalDataMap(datamap);

  arch->setDataMap(datamap);
  memControlList = arch->getMemController();

  Datapool.push_back(BASEADDRESS);

  ctx = new Ciphertext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  ptx = new Plaintext(currentLevel, cfg->getValue("N"), Datapool, batchSize);

  addrManager = new AddrManage(Datapool[Datapool.size() - 1] + 1, batchSize);
  addrManager->setGlobalDatapPoll(&Datapool);

  auto c0 = ctx->getC0Addr();
  auto c1 = ctx->getC1Addr();

  auto p0 = ptx->getC0Addr();

  for (uint32_t k = 0; k < 2; k++) {
    std::vector<std::vector<std::vector<Instruction *>>> PADDInsOut;
    addrManager->MallocMem("PADDOutput(" + std::to_string(k) + ")",
                           currentLevel);
    for (uint32_t l = 0; l < currentLevel; l++) {

      auto op1addr = k == 0 ? c0[l] : c1[l];
      auto op2addr = 0;
      auto op3addr = p0[l];
      auto op4addr = 0;

      auto outaddr =
          addrManager->getAddr("PADDOutput(" + std::to_string(k) + ")")[l];

      auto InsList =
          insgener->GenEWE(l,
                           labelName + "_PADD_Level(" + std::to_string(l) +
                               +")_k(" + std::to_string(k) + ")",
                           nullptr, nullptr, nullptr, nullptr, op1addr, op2addr,
                           op3addr, op4addr, outaddr);
      PADDInsOut.push_back(InsList);
    }
    PADDInsMap["PADD_Key(" + std::to_string(k) + ")"] = PADDInsOut;
    PADDInsMapName.push_back("PADD_Key(" + std::to_string(k) + ")");
  }

  for (auto &key : PADDInsMapName) {
    auto map = PADDInsMap[key];
    driver->dispatchInstructions(map);
  }
}

bool PADD::simulate() {
  // MemControl fetch data and issue the corresponsding instructions
  driver->IssueInsFromDramToChip(arch);

  auto TotalIns = driver->getTotalIns();

  // cycle-level
  unsigned long long cycle = 0;

  unsigned long long exeInsCycle = 0;

  std::cout << "\n\nWelcome! Start simulating PADD!\n\n";

  // Output current time
  time_t currentTime = time(0);

  // Convert current time to string representation
  char *timeString = ctime(&currentTime);
  std::cout << "Start time: " << timeString << std::endl;

  time_t periodTime = time(0);
  while (!arch->simulateComplete()) {
    // MemControl fetches data and issues the corresponding instructions
    driver->IssueDataFromDramToChip(memControlList);

    arch->update();

    cycle = arch->getCycle();
    if (cycle % 2000 == 0) {

      auto exeins = arch->getcompletedIns() - exeInsCycle;

      if (exeins == 0) {
        std::cout << "We have executed " << exeins
                  << " instruction(s) in this period!\n";
        arch->state();

        std::cout << "\n";
        driver->shownData();
        // std::cout << "Memory state\n";
        // arch->shownMemState();
        break;
      }

      // std::cout << "ARCH State\n\n";
      // arch->state();
      // driver->shownData();

      std::cout << "\nFHE-Sim running " << cycle << " cycles!\n";
      std::cout << "We have executed " << arch->getcompletedIns()
                << " instructions!\n";
      auto remainIns = TotalIns - arch->getcompletedIns();
      std::cout << "Remaining " << remainIns << " instructions!\n";
      std::cout << "We have executed " << exeins
                << " instruction(s) in this period!\n";

      time_t nowTime = time(0);
      auto exetime = nowTime - periodTime;
      auto speed = static_cast<double>(exeins) / exetime;
      auto remainTime = static_cast<double>(remainIns) / speed;
      std::cout << "Estimated time remaining " << remainTime / 60
                << " minutes\n";
      periodTime = nowTime;

      exeInsCycle = arch->getcompletedIns();
    }
  }

  time_t currentTime2 = time(0);

  // Convert current time to string representation
  char *timeString2 = ctime(&currentTime2);

  std::cout << "\n\nCompleted Simulate!\n";
  std::cout << "FHE-Sim Total simulated\t" << cycle << " cycles!\n\n";

  std::cout << "End time: " << timeString2 << std::endl;

  std::cout << "The simulator total cost\t"
            << static_cast<double>(currentTime2 - currentTime) / 60
            << " Minutes!\n";

  // arch->state();
  // std::cout << "mem state\n";
  // arch->shownMemState();
  // std::cout<<"\n";
  // driver->shownData();

  arch->shownStat();

  return true;
}