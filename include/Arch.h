/****
 * Arch.h
 *
 * Definition of the Arch class, constructing the top architecture.
 * This class designs each hardware component with its corresponding input list.
 * The list is a custom struct containing a ready bit for each component.
 ****/

#ifndef ARCH_H
#define ARCH_H
#include "Components.h"
#include "Staistics.h"
#include "mem.h"
#include "recodeboard.h"

class Arch {
private:
  uint32_t num_cluster; // Number of clusters in the architecture

  unsigned long long completedIns;

  unsigned long long cycle;

  bool hasHPIPU; // Flag to indicate the presence of HPIPU

  // Vectors of pointers to various components
  std::vector<EWE *> eweus;
  std::vector<AUTOU *> autous;
  std::vector<BCONVU *> bconvus;
  std::vector<NTTU *> nttus;
  std::vector<HPIP *> hpipus;

  // Input/Output interfaces
  std::vector<IO *> eweios; // EWE IO interfaces [cluster, x]
  std::vector<std::vector<std::vector<IO *>>>
      bconvIOs;               // BCONV IO interfaces [cluster, height, width]
  std::vector<IO *> nttuIOs;  // NTTU IO interfaces
  std::vector<IO *> autouIOs; // AUTOU IO interfaces
  std::vector<std::vector<std::vector<IO *>>>
      hpipIOs; // HPIP IO interfaces [cluster, height, width]

  // Issue Ports
  std::vector<std::map<std::string, std::vector<std::vector<Instruction *>>>>
      IssuePortFromInsGenLayerOt; // Issue ports for InsGenLayerOT [cluster,
                                  // name, group]
                                  //   std::vector<std::vector<

  std::vector<std::vector<std::vector<std::vector<std::vector<Instruction *>>>>>
      IssuePortFromBCONV;
  std::vector<std::vector<std::vector<std::vector<std::vector<Instruction *>>>>>
      IssuePortFromHPIP;

  // Commit Instructions
  std::vector<std::map<std::string, std::vector<Instruction *>>>
      CommitInstEWE; // Instructions committed for execution

  std::vector<std::map<std::string, std::vector<Instruction *>>>
      CommitInstAUTO; // Instructions committed for execution
  std::vector<std::map<std::string, std::vector<Instruction *>>>
      CommitInstNTT; // Instructions committed for execution

  std::vector<std::vector<
      std::vector<std::map<std::string, std::vector<Instruction *>>>>>
      CommitInstBCONV;
  std::vector<std::vector<std::vector<std::vector<std::string>>>>
      CommitInstBCONVOrder;

  std::vector<std::vector<
      std::vector<std::map<std::string, std::vector<Instruction *>>>>>
      CommitInstHPIP; // Instructions committed for execution
  std::vector<std::vector<std::vector<std::vector<std::string>>>>
      CommitInstHPIPOrder;

  uint32_t PointCounter; // Instruction PC

  // std::map<>

  // [c, inputNum]
  std::vector<std::vector<bool>> ewefetchflag;
  std::vector<std::vector<uint32_t>> ewepcholder;

  // [c, inputnum, insGroup]
  std::vector<std::vector<std::vector<Instruction *>>> eweDecodeHold;
  std::vector<std::vector<bool>> eweDecodeflag;

  // [c, inputnum, insGroup]
  std::vector<std::vector<std::vector<Instruction *>>> eweIssueHold;
  std::vector<std::vector<bool>> eweIssueflag;

  // [c]
  std::vector<bool> autofetchflag;
  std::vector<uint32_t> autofetchholader;

  //[c, insg]
  std::vector<std::vector<Instruction *>> autoDecodeHold;
  std::vector<bool> autoDecodeflag;

  //[c, insg]
  std::vector<std::vector<Instruction *>> autoIssueHold;
  std::vector<bool> autoIssueflag;

  // [c]
  std::vector<bool> nttfetchflag;
  std::vector<uint32_t> nttfetchholader;

  //[c, insg]
  std::vector<std::vector<Instruction *>> nttDecodeHold;
  std::vector<bool> nttDecodeflag;

  //[c, insg]
  std::vector<std::vector<Instruction *>> nttIssueHold;
  std::vector<bool> nttIssueflag;

  // [c, h, w]
  std::vector<std::vector<std::vector<bool>>> bconvfetchflag;
  std::vector<std::vector<std::vector<uint32_t>>> bconvfetchholder;

  // [c, h, w, insGroup]
  std::vector<std::vector<std::vector<std::vector<Instruction *>>>>
      bconvDecodeHold;
  std::vector<std::vector<std::vector<bool>>> bconvDecodeflag;

  // [c, h, w, insGroup]
  std::vector<std::vector<std::vector<std::vector<Instruction *>>>>
      bconvIssueHold;
  std::vector<std::vector<std::vector<bool>>> bconvIssueflag;

  // [c, h, w]
  std::vector<std::vector<std::vector<bool>>> hpipfetchflag;
  std::vector<std::vector<std::vector<uint32_t>>> hpipfetchholder;

  // [c, h, w, insGroup]
  std::vector<std::vector<std::vector<std::vector<Instruction *>>>>
      hpipDecodeHold;
  std::vector<std::vector<std::vector<bool>>> hpipDecodeflag;

  // [c, h, w, insGroup]
  std::vector<std::vector<std::vector<std::vector<Instruction *>>>>
      hpipIssueHold;
  std::vector<std::vector<std::vector<bool>>> hpipIssueflag;

  // Memory Components
  std::vector<std::vector<uint32_t>> MemFetchAddr; // Memory fetch addresses
  std::vector<MemController *> chipMemControl;
  std::vector<RecodeBoard *> chipboard; // Check RAW problem

  DataMap *datamap;

  bool memlinestatistic;

  Statistic *stat;

public:
  Arch(Config *cfg); // Constructor

  uint32_t getPC() { return PointCounter; };
  void increasePC(uint32_t instSize) { PointCounter += instSize; };

  void setInsPC(uint32_t start, std::vector<Instruction *> group);

  void setDataMap(DataMap *map) {
    datamap = map;
    for (uint32_t c = 0; c < num_cluster; c++) {
      chipMemControl[c]->setGlobalDataMap(map);
    }
  };
  // Public methods for architecture operations
  void update();

  std::vector<IO *> createIOs(uint32_t count);
  void setupEWEIO(uint32_t c);
  void setupBCONVIO(uint32_t c);
  void setupAUTOUIO(uint32_t c);
  void setupNTTUIO(uint32_t c);
  void setupHPIPIO(uint32_t c);

  bool
  handleSignalOutput(uint32_t c,
                     std::map<std::string, std::vector<Instruction *>> &commit,
                     IO *out);

  bool handleMultiOutput(
      uint32_t c, std::map<std::string, std::vector<Instruction *>> &commitInst,
      std::vector<std::string> &commitorder, IO *out);

  void updateUnitGroup(uint32_t c);

  void eweupdate(uint32_t c);
  void fetchEweInstructions(uint32_t c, uint32_t numIn);
  void decodeEweInstructions(uint32_t c, uint32_t numIn);
  void issueEweInstructions(uint32_t c, uint32_t numIn);
  void commitEweInstructions(uint32_t c, uint32_t numIn);
  void writebackEweInstructions(uint32_t c);

  void autoupdate(uint32_t c);
  void fetchAutoInstructions(uint32_t c);
  void decodeAutoInstructions(uint32_t c);
  void issueAutoInstructions(uint32_t c);
  void commitAutoInstructions(uint32_t c);
  void writebackAutoInstructions(uint32_t c);

  void nttupdate(uint32_t c);
  void fetchNttInstructions(uint32_t c);
  void decodeNttInstructions(uint32_t c);
  void issueNttInstructions(uint32_t c);
  void commitNttInstructions(uint32_t c);
  void writebackNttInstructions(uint32_t c);

  void bconvupdate(uint32_t c);
  void fetchBconvInstructions(uint32_t c, uint32_t comInH, uint32_t comInW);
  void decodeBconvInstructions(uint32_t c, uint32_t comInH, uint32_t comInW);
  void issueBconvInstructions(uint32_t c, uint32_t comInH, uint32_t comInW);
  void commitBconvInstructions(uint32_t c, uint32_t comInH, uint32_t comInW);
  void writebackBconvInstructions(uint32_t c);

  void hpipupdate(uint32_t c);
  void fetchHpipInstructions(uint32_t c, uint32_t comInH, uint32_t comInW);
  void decodeHpipInstructions(uint32_t c, uint32_t comInH, uint32_t comInW);
  void issueHpipInstructions(uint32_t c, uint32_t comInH, uint32_t comInW);
  void commitHpipInstructions(uint32_t c, uint32_t comInH, uint32_t comInW);
  void writebackHpipInstructions(uint32_t c);

  bool ewecompleted();
  bool autocompleted();
  bool nttcompleted();
  bool bconvcompleted();
  bool hpipcompleted();

  void updateCommitInstructions(
      std::map<std::string, std::vector<Instruction *>> &commitInst,
      std::vector<std::string> &commitorder, const std::string &insName,
      Instruction *lastIns);

  void issueIns(uint32_t index, const std::string &name,
                std::vector<Instruction *> &insg);
  void issueIns(uint32_t index, uint32_t h, uint32_t w,
                std::vector<Instruction *> &insg, bool hpip);

  // Expose to instruction generator
  std::vector<MemController *> getMemController() { return chipMemControl; };

  unsigned long long getcompletedIns() { return completedIns; };

  void state();

  bool simulateComplete() {
    // if (!ewecompleted() || !autocompleted() || !nttcompleted() ||
    //     !bconvcompleted()) {
    //   return false;
    // }

    if (!ewecompleted()) {
      return false;
    }

    if (!nttcompleted()) {
      return false;
    }

    if (!bconvcompleted()) {
      return false;
    }

    // if (hasHPIPU && !hpipcompleted()) {
    //   return false;
    // }

    return true;
  };

  void shownMemState() {
    for (uint32_t c = 0; c < num_cluster; c++) {
      std::cout << c << " \n";
      chipMemControl[c]->shownState();
    }
  };

  unsigned long long getCycle() { return cycle; };

  void fetchComponentsExecute() {
    for (uint32_t c = 0; c < num_cluster; c++) {
      stat->setStat("EWE_(" + std::to_string(c) + ")",
                    eweus[c]->getExeCycles());
      stat->setStat("NTT_(" + std::to_string(c) + ")",
                    nttus[c]->getExeCycles());
      stat->setStat("AUTO_(" + std::to_string(c) + ")",
                    autous[c]->getExeCycles());
      stat->setStat("BCONV_(" + std::to_string(c) + ")",
                    bconvus[c]->getExeCycles());
      if (hasHPIPU) {
        stat->setStat("HPIP_(" + std::to_string(c) + ")",
                      hpipus[c]->getExeCycles());
      }
    }
  };

  bool MemLineStatic() { return memlinestatistic; };

  void shownStat();
};
#endif // ARCH_H
