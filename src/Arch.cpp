/**
 * Implementation of top archtechture
 */

#include "Arch.h"
#include "Config.h"

Arch::Arch(Config *cfg) {
  num_cluster = cfg->getValue("cluster");
  hasHPIPU = cfg->getValue("hasHPIPU") == 1;

  memlinestatistic = cfg->getValue("memlinestatistic") == 1;

  stat = new Statistic();

  completedIns = 0;
  cycle = 0;

  // resize memory for vectors
  IssuePortFromInsGenLayerOt.resize(num_cluster);
  IssuePortFromBCONV.resize(num_cluster);

  ewefetchflag.resize(num_cluster);
  ewepcholder.resize(num_cluster);
  eweDecodeHold.resize(num_cluster);
  eweDecodeflag.resize(num_cluster);
  eweIssueHold.resize(num_cluster);
  eweIssueflag.resize(num_cluster);

  autofetchflag.resize(num_cluster);
  autofetchholader.resize(num_cluster);
  autoDecodeHold.resize(num_cluster);
  autoDecodeflag.resize(num_cluster);
  autoIssueHold.resize(num_cluster);
  autoIssueflag.resize(num_cluster);

  nttfetchflag.resize(num_cluster);
  nttfetchholader.resize(num_cluster);
  nttDecodeHold.resize(num_cluster);
  nttDecodeflag.resize(num_cluster);
  nttIssueHold.resize(num_cluster);
  nttIssueflag.resize(num_cluster);

  bconvfetchflag.resize(num_cluster);
  bconvfetchholder.resize(num_cluster);
  bconvDecodeHold.resize(num_cluster);
  bconvDecodeflag.resize(num_cluster);
  bconvIssueHold.resize(num_cluster);
  bconvIssueflag.resize(num_cluster);

  CommitInstBCONV.resize(num_cluster);
  CommitInstBCONVOrder.resize(num_cluster);

  if (hasHPIPU) {
    IssuePortFromHPIP.resize(num_cluster);
    CommitInstHPIP.resize(num_cluster);
    CommitInstHPIPOrder.resize(num_cluster);

    hpipfetchflag.resize(num_cluster);
    hpipfetchholder.resize(num_cluster);
    hpipDecodeHold.resize(num_cluster);
    hpipDecodeflag.resize(num_cluster);
    hpipIssueHold.resize(num_cluster);
    hpipIssueflag.resize(num_cluster);
  }

  for (uint32_t c = 0; c < num_cluster; c++) {
    eweus.push_back(new EWE(cfg));

    uint32_t count =
        uint32_t(eweus[c]->getInputNum() / eweus[c]->getOutputNum());
    ewefetchflag[c].resize(count);
    ewepcholder[c].resize(count);
    eweDecodeHold[c].resize(count);
    eweDecodeflag[c].resize(count);
    eweIssueHold[c].resize(count);
    eweIssueflag[c].resize(count);

    bconvus.push_back(new BCONVU(cfg));

    // Initialize IssuePortFromBCONV
    int bconvHeight = bconvus[c]->getHigh();
    int bconvWidth = bconvus[c]->getWidth();

    IssuePortFromBCONV[c].resize(bconvHeight);
    CommitInstBCONV[c].resize(bconvHeight);
    CommitInstBCONVOrder[c].resize(bconvHeight);

    bconvfetchflag[c].resize(bconvHeight);
    bconvfetchholder[c].resize(bconvHeight);
    bconvDecodeHold[c].resize(bconvHeight);
    bconvDecodeflag[c].resize(bconvHeight);
    bconvIssueHold[c].resize(bconvHeight);
    bconvIssueflag[c].resize(bconvHeight);

    for (uint32_t h = 0; h < bconvHeight; h++) {
      IssuePortFromBCONV[c][h].resize(bconvWidth);
      CommitInstBCONV[c][h].resize(bconvWidth);
      CommitInstBCONVOrder[c][h].resize(bconvWidth);

      bconvfetchflag[c][h].resize(bconvWidth);
      bconvfetchholder[c][h].resize(bconvWidth);
      bconvDecodeHold[c][h].resize(bconvWidth);
      bconvDecodeflag[c][h].resize(bconvWidth);
      bconvIssueHold[c][h].resize(bconvWidth);
      bconvIssueflag[c][h].resize(bconvWidth);
    }

    nttus.push_back(new NTTU(cfg));
    autous.push_back(new AUTOU(cfg));

    if (hasHPIPU) {
      hpipus.push_back(new HPIP(cfg));

      // Initialize IssuePortFromHPIP
      int hpipHeight = hpipus[c]->getVecPECount();
      int hpipWidth = hpipus[c]->getMacCount();

      IssuePortFromHPIP[c].resize(hpipHeight);
      CommitInstHPIP[c].resize(hpipHeight);
      CommitInstHPIPOrder[c].resize(hpipHeight);

      hpipfetchflag[c].resize(hpipHeight);
      hpipfetchholder[c].resize(hpipHeight);
      hpipDecodeHold[c].resize(hpipHeight);
      hpipDecodeflag[c].resize(hpipHeight);
      hpipIssueHold[c].resize(hpipHeight);
      hpipIssueflag[c].resize(hpipHeight);

      for (uint32_t h = 0; h < hpipHeight; h++) {
        IssuePortFromHPIP[c][h].resize(hpipWidth);
        CommitInstHPIP[c][h].resize(hpipWidth);
        CommitInstHPIPOrder[c][h].resize(hpipWidth);
        hpipfetchflag[c][h].resize(hpipWidth);
        hpipfetchholder[c][h].resize(hpipWidth);
        hpipDecodeHold[c][h].resize(hpipWidth);
        hpipDecodeflag[c][h].resize(hpipWidth);
        hpipIssueHold[c][h].resize(hpipWidth);
        hpipIssueflag[c][h].resize(hpipWidth);
      }
    }

    // Setup IOs
    setupEWEIO(c);
    setupBCONVIO(c);
    setupNTTUIO(c);
    setupAUTOUIO(c);
    if (hasHPIPU) {
      setupHPIPIO(c);
    }
    // Todo: NEED getinformation form configuration
    chipMemControl.push_back(new MemController(cfg, this));
    chipMemControl[c]->setClusterId(c);
    chipMemControl[c]->setStat(stat);

    chipboard.push_back(new RecodeBoard());
  }

  for (uint32_t c = 0; c < num_cluster; c++) {
    chipMemControl[c]->setMemControlList(chipMemControl);
  }

  MemFetchAddr.resize(num_cluster);

  CommitInstEWE.resize(num_cluster);
  CommitInstAUTO.resize(num_cluster);
  CommitInstNTT.resize(num_cluster);
}

void Arch::setupEWEIO(uint32_t c) {
  std::vector<IO *> ios = createIOs(4); // Assuming EWE always needs 4 IOs
  eweus[c]->setInput(ios);
  eweios.insert(eweios.end(), ios.begin(), ios.end());
}

void Arch::setupBCONVIO(uint32_t c) {
  std::vector<std::vector<IO *>> ionum_cluster;
  for (uint32_t h = 0; h < bconvus[c]->getHigh(); h++) {
    ionum_cluster.push_back(createIOs(bconvus[c]->getWidth()));
  }
  bconvus[c]->setInput(ionum_cluster);
  bconvIOs.insert(bconvIOs.end(), ionum_cluster);
}

void Arch::setupNTTUIO(uint32_t c) {
  IO *nttio = new IO();
  nttus[c]->setInput(nttio);
  nttuIOs.push_back(nttio);
}

void Arch::setupAUTOUIO(uint32_t c) {
  IO *autoio = new IO();
  autous[c]->setInput(autoio);
  autouIOs.push_back(autoio);
}

void Arch::setupHPIPIO(uint32_t c) {
  std::vector<std::vector<IO *>> ionum_cluster;
  for (uint32_t h = 0; h < hpipus[c]->getVecPECount(); h++) {
    ionum_cluster.push_back(createIOs(hpipus[c]->getMacCount()));
  }
  hpipus[c]->setInput(ionum_cluster);
  hpipIOs.insert(hpipIOs.end(), ionum_cluster);
}

std::vector<IO *> Arch::createIOs(uint32_t count) {
  std::vector<IO *> ios;
  ios.resize(count);
  for (uint32_t i = 0; i < count; ++i) {
    ios[i] = new IO(); // Consider using smart pointers here
  }
  return ios;
}

void Arch::updateUnitGroup(uint32_t c) {

  if (!CommitInstEWE[c].empty())
    eweus[c]->update();

  if (!CommitInstAUTO[c].empty())
    autous[c]->update();

  // if(!CommitInstBCONV)
  bconvus[c]->update();

  if (!CommitInstNTT[c].empty())
    nttus[c]->update();

  if (hasHPIPU) {
    hpipus[c]->update();
  }
}

void Arch::setInsPC(uint32_t start, std::vector<Instruction *> group) {
  for (auto &ins : group) {
    ins->setInsPC(start);
    start += 1;
  }
}

void Arch::state() {
  for (uint32_t c = 0; c < num_cluster; c++) {
    std::cout << "EWE " << IssuePortFromInsGenLayerOt[c]["EWE"].size() << " "
              << CommitInstEWE[c].size() << "\n\n";

    std::cout << " " << IssuePortFromInsGenLayerOt[c]["EWE"][0].size() << "\n";

    auto ins = IssuePortFromInsGenLayerOt[c]["EWE"][0][0];
    ins->ShowIns();
    for (uint32_t i = 0; i < ins->getinputCount(); i++) {
      std::cout << " " << i << " " << ins->getOperand(i) << " | ";
    }
    std::cout << "\n";

    auto ewecount =
        uint32_t(eweus[c]->getInputNum() / eweus[c]->getOutputNum());
    for (uint32_t numIn = 0; numIn < ewecount; numIn++) {
      if (eweDecodeHold[c][numIn].size() > 0) {
        std::cout << "numIn " << numIn << " eweDecodeHold[c][numIn] "
                  << eweDecodeHold[c][numIn].size() << ".\n ";
        eweDecodeHold[c][numIn][0]->ShowIns();
      }

      if (eweIssueHold[c][numIn].size() > 0) {
        std::cout << "numIn " << numIn << " eweDecodeHold[c][numIn] "
                  << eweIssueHold[c][numIn].size() << ".\n";
        eweIssueHold[c][numIn][0]->ShowIns();
      }
    }

    std::cout << "NTT " << IssuePortFromInsGenLayerOt[c]["NTT"].size() << " "
              << CommitInstNTT[c].size() << "\n";

    if (IssuePortFromInsGenLayerOt[c]["NTT"].size() > 0) {
      IssuePortFromInsGenLayerOt[c]["NTT"][0][0]->ShowIns();
    }

    if (nttDecodeHold[c].size() > 0) {
      std::cout << "nttDecodeHold[c] " << nttDecodeHold[c].size() << ".\n ";
      nttDecodeHold[c][0]->ShowIns();
      std::cout << "nttIssueHold[c] " << nttIssueHold[c].size() << ".\n ";
      nttIssueHold[c][0]->ShowIns();
    }

    const int clusterHeight = bconvus[c]->getHigh();
    const int clusterWidth = bconvus[c]->getWidth();
    std::cout << "BCONV\n";

    for (uint32_t comInH = 0; comInH < clusterHeight; comInH++) {
      for (uint32_t comInW = 0; comInW < clusterWidth; comInW++) {
        std::cout << c << " " << comInH << " " << comInW << " "
                  << IssuePortFromBCONV[c][comInH][comInW].size() << " "
                  << CommitInstBCONV[c][comInH][comInW].size() << "\n";

        if (bconvIssueHold[c][comInH][comInW].size() > 0) {
          std::cout << c << " " << comInH << " " << comInW << " "
                    << bconvIssueHold[c][comInH][comInW].size() << " \n";
          bconvIssueHold[c][comInH][comInW][0]->ShowIns();
        }

        if (bconvDecodeHold[c][comInH][comInW].size() > 0) {
          std::cout << c << " " << comInH << " " << comInW << " "
                    << bconvDecodeHold[c][comInH][comInW].size() << "\n";
          bconvDecodeHold[c][comInH][comInW][0]->ShowIns();
        }
      }
    }
  }
}

bool Arch::ewecompleted() {
  bool completed = true;
  for (uint32_t c = 0; c < num_cluster; c++) {
    completed &= IssuePortFromInsGenLayerOt[c]["EWE"].empty();
    auto ewecount =
        uint32_t(eweus[c]->getInputNum() / eweus[c]->getOutputNum());
    for (uint32_t numIn = 0; numIn < ewecount; numIn++) {

      completed &= (eweDecodeHold[c][numIn].size() == 0);
      completed &= (eweIssueHold[c][numIn].size() == 0);
    }
    completed &= CommitInstEWE[c].empty();
  }

  return completed;
}

bool Arch::autocompleted() {
  bool completed = true;
  for (uint32_t c = 0; c < num_cluster; c++) {
    completed &= IssuePortFromInsGenLayerOt[c]["AUTO"].empty();

    // completed &= !(autoIssueflag[c] & !autoDecodeflag[c]);
    completed &= (autoDecodeHold[c].size() == 0);
    completed &= (autoIssueHold[c].size() == 0);

    completed &= CommitInstAUTO[c].empty();
  }
  return completed;
}

bool Arch::nttcompleted() {
  bool completed = true;
  for (uint32_t c = 0; c < num_cluster; c++) {
    completed &= IssuePortFromInsGenLayerOt[c]["NTT"].empty();

    completed &= (nttDecodeHold[c].size() == 0);
    completed &= (nttIssueHold[c].size() == 0);

    completed &= CommitInstNTT[c].empty();
  }
  return completed;
}

bool Arch::bconvcompleted() {
  bool completed = true;

  for (uint32_t c = 0; c < num_cluster; c++) {
    const int clusterHeight = bconvus[c]->getHigh();
    const int clusterWidth = bconvus[c]->getWidth();
    for (uint32_t comInH = 0; comInH < clusterHeight; comInH++) {
      for (uint32_t comInW = 0; comInW < clusterWidth; comInW++) {
        completed &= IssuePortFromBCONV[c][comInH][comInW].empty();

        completed &= (bconvIssueHold[c][comInH][comInW].size() == 0);
        completed &= (bconvDecodeHold[c][comInH][comInW].size() == 0);

        completed &= CommitInstBCONV[c][comInH][comInW].empty();
      }
    }
  }

  // if(completed){std::cout<<"bconv completed!\n";}
  return completed;
}

bool Arch::hpipcompleted() {
  bool completed = true;

  for (uint32_t c = 0; c < num_cluster; c++) {
    const int clusterHeight = hpipus[c]->getVecPECount();
    const int clusterWidth = hpipus[c]->getMacCount();
    for (uint32_t comInH = 0; comInH < clusterHeight; comInH++) {
      for (uint32_t comInW = 0; comInW < clusterWidth; comInW++) {
        completed &= IssuePortFromHPIP[c][comInH][comInW].empty();

        // completed &= !(hpipIssueflag[c][comInH][comInW] &
        //                !hpipDecodeflag[c][comInH][comInW]);
        completed &= (hpipIssueHold[c][comInH][comInW].size() == 0);
        completed &= (hpipDecodeHold[c][comInH][comInW].size() == 0);

        completed &= CommitInstHPIP[c][comInH][comInW].empty();
      }
    }
  }
  // if(completed){std::cout<<"hpip completed!\n";}
  return completed;
}

void Arch::eweupdate(uint32_t c) {
  writebackEweInstructions(c);
  auto ewecount = uint32_t(eweus[c]->getInputNum() / eweus[c]->getOutputNum());
  for (uint32_t numIn = 0; numIn < ewecount; numIn++) {
    commitEweInstructions(c, numIn);
    issueEweInstructions(c, numIn);
    decodeEweInstructions(c, numIn);
    fetchEweInstructions(c, numIn);
  }
}

void Arch::fetchEweInstructions(uint32_t c, uint32_t numIn) {

  if (!ewefetchflag[c][numIn]) {

    uint32_t pc = getPC();
    ewepcholder[c][numIn] = pc;
    ewefetchflag[c][numIn] = true;
    increasePC(2); // each input has two instruction
  }
}

/***
 * 获取指令port的第一条指令
 * 如果该指令的所有数据全部reday的话，那么该指令可以完成decode
 * 并存储在相应的指令buffer中
 */
void Arch::decodeEweInstructions(uint32_t c, uint32_t numIn) {

  if (ewefetchflag[c][numIn] && !eweDecodeflag[c][numIn] &&
      !IssuePortFromInsGenLayerOt[c]["EWE"].empty()) {

    eweDecodeHold[c][numIn] = IssuePortFromInsGenLayerOt[c]["EWE"][0];

    if (chipMemControl[c]->checkMemReady(eweDecodeHold[c][numIn],
                                         "EWE_MEM_Stall_(" + std::to_string(c) +
                                             ")")) {
      setInsPC(ewepcholder[c][numIn], eweDecodeHold[c][numIn]);
      IssuePortFromInsGenLayerOt[c]["EWE"].erase(
          IssuePortFromInsGenLayerOt[c]["EWE"].begin());
      ewefetchflag[c][numIn] = false;
      eweDecodeflag[c][numIn] = true;
    } else {
      eweDecodeHold[c][numIn].clear();
    }
  }
}

// ISSUE and MEM
void Arch::issueEweInstructions(uint32_t c, uint32_t numIn) {

  if (eweDecodeflag[c][numIn] && !eweIssueflag[c][numIn]) {
    // check scoreboard and mem for all group instructions
    if (chipboard[c]->check(eweDecodeHold[c][numIn])) {
      eweIssueHold[c][numIn] = eweDecodeHold[c][numIn];
      eweDecodeHold[c][numIn].clear();
      eweIssueflag[c][numIn] = true;
      eweDecodeflag[c][numIn] = false;
    } else {
      // stat->increaseStat("EWE_RAW_Stall_("+std::to_string(c)+")");
    }
  }
}

void Arch::commitEweInstructions(uint32_t c, uint32_t numIn) {

  if (eweIssueflag[c][numIn]) {
    bool inFlag = true;
    for (uint32_t comIn = 0; comIn < 2; comIn++) {
      inFlag &= !eweios[4 * c + 2 * numIn + comIn]->GetSignal();
    }
    if (!inFlag) {
      return;
    }
    if (eweIssueHold[c][numIn].size() > 2) {
      throw std::runtime_error(
          "EWE instruction group contains too many instructions!\n");
    }
    for (uint32_t comIn = 0; comIn < 2; comIn++) {
      if (eweIssueHold[c][numIn].size() == 1) {
        eweios[4 * c + 2 * numIn + comIn]->SetIns(eweIssueHold[c][numIn][0]);
      } else {

        std::runtime_error("Error! The ewe intruction group should "
                           "contains one intruction, and each "
                           "instruction contains most four input!");
        eweios[4 * c + 2 * numIn + comIn]->SetIns(
            eweIssueHold[c][numIn][0]); // Modified The ewe instruction
                                        // contains four input, therefore it
                                        // only need push the first intruction
      }
    }

    chipMemControl[c]->accessFromUnits(eweIssueHold[c][numIn][0]);
    // Store the commit instruction
    chipboard[c]->insert(eweIssueHold[c][numIn][0]); // The final output for
                                                     // this instruction group!
    eweIssueflag[c][numIn] = false;
    CommitInstEWE[c][eweIssueHold[c][numIn][0]->GetInsName()] =
        eweIssueHold[c][numIn];
    eweIssueHold[c][numIn].clear();
  }
}

void Arch::writebackEweInstructions(uint32_t c) {
  // Handle EWE unit outputs
  auto eweOutputs = eweus[c]->getOutput();
  for (auto &out : eweOutputs) {
    if (out->GetSignal()) {
      // std::cout<<"Debug: compleated\n";
      // out->GetIns()->ShowIns();
      if (!handleSignalOutput(c, CommitInstEWE[c], out)) {
        stat->increaseStat("EWE_OutMem_Stall_(" + std::to_string(c) + ")");
      }
    }
  }
}

void Arch::autoupdate(uint32_t c) {
  writebackAutoInstructions(c);
  commitAutoInstructions(c);
  issueAutoInstructions(c);
  decodeAutoInstructions(c);
  fetchAutoInstructions(c);
}
void Arch::fetchAutoInstructions(uint32_t c) {

  if (!autofetchflag[c]) {
    uint32_t pc = getPC();
    autofetchholader[c] = pc;
    autofetchflag[c] = true;
    increasePC(1); // Only one instruction for the auto operation
  }
}

void Arch::decodeAutoInstructions(uint32_t c) {
  if (autofetchflag[c] && !autoDecodeflag[c] &&
      !IssuePortFromInsGenLayerOt[c]["AUTO"].empty()) {
    autoDecodeHold[c] = IssuePortFromInsGenLayerOt[c]["AUTO"][0];
    if (chipMemControl[c]->checkMemReady(
            autoDecodeHold[c], "AUTO_MEM_Stall_(" + std::to_string(c) + ")")) {
      IssuePortFromInsGenLayerOt[c]["AUTO"].erase(
          IssuePortFromInsGenLayerOt[c]["AUTO"].begin());
      autofetchflag[c] = false;
      autoDecodeflag[c] = true;
    }
  }
}
void Arch::issueAutoInstructions(uint32_t c) {
  if (autoDecodeflag[c] && !autoIssueflag[c]) {
    // check scoreboard and mem for all group instructions
    if (chipboard[c]->check(autoDecodeHold[c])) {
      autoIssueHold[c] = autoDecodeHold[c];
      autoIssueflag[c] = true;
      autoDecodeflag[c] = false;
    } else {
      // stat->increaseStat("AUTO_RAW_Stall_("+std::to_string(c)+")");
    }
  }
}

void Arch::commitAutoInstructions(uint32_t c) {
  if (autoIssueflag[c]) {

    bool inFlag = !autouIOs[c]->GetSignal();
    if (!inFlag) {
      return;
    }
    autouIOs[c]->SetIns(autoIssueHold[c][0]);
    chipMemControl[c]->accessFromUnits(autoIssueHold[c][0]);

    // Store the commit instruction
    chipboard[c]->insert(autoIssueHold[c][0]);
    CommitInstAUTO[c][autoIssueHold[c][0]->GetInsName()] = autoIssueHold[c];
    autoIssueflag[c] = false;
  }
}
void Arch::writebackAutoInstructions(uint32_t c) {
  // Handle AUTO unit outputs
  auto autoOuts = autous[c]->getOutput();
  if (autoOuts->GetSignal()) {
    // Handle AUTO unit output when it has a signal
    if (!handleSignalOutput(c, CommitInstAUTO[c], autoOuts)) {
      stat->increaseStat("AUTO_OutMem_Stall_(" + std::to_string(c) + ")");
    }
  }
}

// NTT
void Arch::nttupdate(uint32_t c) {
  writebackNttInstructions(c);
  commitNttInstructions(c);
  issueNttInstructions(c);
  decodeNttInstructions(c);
  fetchNttInstructions(c);
}
void Arch::fetchNttInstructions(uint32_t c) {
  // std::cout<<"Size is "<<IssuePortFromInsGenLayerOt[c]["NTT"].size()<<"\n";

  if (!nttfetchflag[c]) {
    uint32_t pc = getPC();
    nttfetchholader[c] = pc;
    nttfetchflag[c] = true;
    increasePC(1); // Only one instruction for the ntt operation
  }
}

void Arch::decodeNttInstructions(uint32_t c) {
  if (nttfetchflag[c] && !nttDecodeflag[c] &&
      !IssuePortFromInsGenLayerOt[c]["NTT"].empty()) {
    nttDecodeHold[c] = IssuePortFromInsGenLayerOt[c]["NTT"][0];
    // std::cout<<"decode ->"<<nttDecodeHold[c].size()<<"\n";
    if (chipMemControl[c]->checkMemReady(
            nttDecodeHold[c], "NTT_MEM_Stall_(" + std::to_string(c) + ")")) {
      IssuePortFromInsGenLayerOt[c]["NTT"].erase(
          IssuePortFromInsGenLayerOt[c]["NTT"].begin());
      nttfetchflag[c] = false;
      nttDecodeflag[c] = true;
    } else {
      nttDecodeHold[c].clear();
    }
  }
}
void Arch::issueNttInstructions(uint32_t c) {
  if (nttDecodeflag[c] && !nttIssueflag[c]) {
    // check scoreboard and mem for all group instructions
    // std::cout<<nttDecodeHold[c].size()<<"\n";
    if (chipboard[c]->check(nttDecodeHold[c])) {
      nttIssueHold[c] = nttDecodeHold[c];
      nttDecodeHold[c].clear();
      // std::cout<<nttDecodeHold[c].size()<<"\n";
      nttIssueflag[c] = true;
      nttDecodeflag[c] = false;
    } else {
      // stat->increaseStat("NTT_RAW_Stall");
    }
  }
}

void Arch::commitNttInstructions(uint32_t c) {
  if (nttIssueflag[c]) {
    bool inFlag = !nttuIOs[c]->GetSignal();
    if (!inFlag) {
      return;
    }
    nttuIOs[c]->SetIns(nttIssueHold[c][0]);
    chipMemControl[c]->accessFromUnits(nttIssueHold[c][0]);
    // Store the commit instruction
    chipboard[c]->insert(nttIssueHold[c][0]);
    // std::cout << "Debug: " << nttIssueHold[c][0]->GetInsName() <<
    // std::endl;
    CommitInstNTT[c][nttIssueHold[c][0]->GetInsName()] = nttIssueHold[c];
    nttIssueHold[c].clear();
    nttIssueflag[c] = false;
  }
}
void Arch::writebackNttInstructions(uint32_t c) {
  // Handle NTT unit outputs
  auto nttOuts = nttus[c]->getOutput();
  if (nttOuts->GetSignal()) {
    // Handle NTT unit output when it has a signal
    if (!handleSignalOutput(c, CommitInstNTT[c], nttOuts)) {
      stat->increaseStat("NTT_OutMem_Stall_(" + std::to_string(c) + ")");
    }
  }
}

void Arch::bconvupdate(uint32_t c) {
  const int clusterHeight = bconvus[c]->getHigh();
  const int clusterWidth = bconvus[c]->getWidth();

  writebackBconvInstructions(c);

  for (uint32_t comInH = 0; comInH < clusterHeight; comInH++) {
    for (uint32_t comInW = 0; comInW < clusterWidth; comInW++) {
      commitBconvInstructions(c, comInH, comInW);
      issueBconvInstructions(c, comInH, comInW);
      decodeBconvInstructions(c, comInH, comInW);
      fetchBconvInstructions(c, comInH, comInW);
    }
  }
}
void Arch::fetchBconvInstructions(uint32_t c, uint32_t comInH,
                                  uint32_t comInW) {
  if (!bconvfetchflag[c][comInH][comInW]) {
    uint32_t pc = getPC();
    bconvfetchholder[c][comInH][comInW] = pc;
    bconvfetchflag[c][comInH][comInW] = true;
    // Total intrcutions for one bconv unint
    if (IssuePortFromBCONV[c][comInH][comInW].size() > 0)
      increasePC(IssuePortFromBCONV[c][comInH][comInW][0].size());
    else {
      increasePC(0);
    }
  }
}

void Arch::decodeBconvInstructions(uint32_t c, uint32_t comInH,
                                   uint32_t comInW) {
  if (bconvfetchflag[c][comInH][comInW] &&
      !bconvDecodeflag[c][comInH][comInW] &&
      !IssuePortFromBCONV[c][comInH][comInW].empty()) {

    bconvDecodeHold[c][comInH][comInW] =
        IssuePortFromBCONV[c][comInH][comInW][0];

    // std::cout<<"Size is
    // "<<bconvDecodeHold[c][comInH][comInW][0]->getinputCount()<<"\n";

    if (chipMemControl[c]->checkMemReady(bconvDecodeHold[c][comInH][comInW],
                                         "BCONV_MEM_Stall_(" +
                                             std::to_string(c) + ")")) {
      IssuePortFromBCONV[c][comInH][comInW].erase(
          IssuePortFromBCONV[c][comInH][comInW].begin());

      bconvfetchflag[c][comInH][comInW] = false;
      bconvDecodeflag[c][comInH][comInW] = true;
    } else {
      bconvDecodeHold[c][comInH][comInW].clear();
    }
  }
}

void Arch::issueBconvInstructions(uint32_t c, uint32_t comInH,
                                  uint32_t comInW) {
  if (bconvDecodeflag[c][comInH][comInW] &&
      !bconvIssueflag[c][comInH][comInW]) {
    auto lastIns =
        bconvDecodeHold[c][comInH][comInW]
                       [bconvDecodeHold[c][comInH][comInW].size() - 1];

    // check scoreboard and mem for one instruction, as each mac unit only
    // can recive one instruction

    if (chipboard[c]->check({lastIns})) {

      bconvIssueHold[c][comInH][comInW] = {lastIns};
      // ! all ins commit can next issue
      bconvIssueflag[c][comInH][comInW] = true;
      bconvDecodeHold[c][comInH][comInW].pop_back();

      // bconvDecodeHold[c][comInH][comInW].erase(bconvDecodeHold[c][comInH][comInW].end()-1);

      if (bconvDecodeHold[c][comInH][comInW]
              .empty()) { //如果所有的指令都可以issue了，则可以decode下一组指令了
        bconvDecodeflag[c][comInH][comInW] = false;
        bconvDecodeHold[c][comInH][comInW].clear();
      }
    } else {
      // stat->increaseStat("BCONV_RAW_Stall");
    }
  }
}

void Arch::commitBconvInstructions(uint32_t c, uint32_t comInH,
                                   uint32_t comInW) {

  if (bconvIssueflag[c][comInH][comInW]) {

    bool inFlag = !bconvIOs[c][comInH][comInW]->GetSignal();
    if (!inFlag) {
      return;
    }

    auto lastIns = bconvIssueHold[c][comInH][comInW][0];

    // Set instruction in IO and update commit instructions
    bconvIOs[c][comInH][comInW]->SetIns(lastIns);
    auto insName = bconvDecodeHold[c][comInH][comInW][0]->GetInsName();

    updateCommitInstructions(CommitInstBCONV[c][comInH][comInW],
                             CommitInstBCONVOrder[c][comInH][comInW], insName,
                             lastIns);

    if (!bconvDecodeflag[c][comInH][comInW]) {
      chipboard[c]->insert(lastIns);
      chipMemControl[c]->accessFromUnits(lastIns);
    }

    bconvIssueflag[c][comInH][comInW] = false;
    bconvIssueHold[c][comInH][comInW].clear();
  }
}
void Arch::writebackBconvInstructions(uint32_t c) {
  // Handle BCONV unit outputs
  auto bconvOutputs = bconvus[c]->getOutput();
  for (auto h = 0; h < bconvus[c]->getOutCount(); h++) {
    for (auto w = 0; w < bconvOutputs[h].size(); w++) {
      auto out = bconvOutputs[h][w];
      if (out->GetSignal()) {
        if (handleMultiOutput(c, CommitInstBCONV[c][h][w],
                              CommitInstBCONVOrder[c][h][w], out)) {
          stat->increaseStat("BCONV_OutMem_Stall_(" + std::to_string(c) + ")");
        }
      }
    }
  }
}

void Arch::hpipupdate(uint32_t c) {
  const int clusterHeight = hpipus[c]->getVecPECount();
  const int clusterWidth = hpipus[c]->getMacCount();

  writebackHpipInstructions(c);

  for (uint32_t comInH = 0; comInH < clusterHeight; comInH++) {
    for (uint32_t comInW = 0; comInW < clusterWidth; comInW++) {
      commitHpipInstructions(c, comInH, comInW);
      issueHpipInstructions(c, comInH, comInW);
      decodeHpipInstructions(c, comInH, comInW);
      fetchHpipInstructions(c, comInH, comInW);
    }
  }
}
void Arch::fetchHpipInstructions(uint32_t c, uint32_t comInH, uint32_t comInW) {
  if (!hpipfetchflag[c][comInH][comInW]) {
    uint32_t pc = getPC();
    hpipfetchholder[c][comInH][comInW] = pc;
    hpipfetchflag[c][comInH][comInW] = true;
    // Total intrcutions for one hpip unint
    if (IssuePortFromHPIP[c][comInH][comInW].size() > 0)
      increasePC(IssuePortFromHPIP[c][comInH][comInW][0].size());
    else {
      increasePC(0);
    }
  }
}

void Arch::decodeHpipInstructions(uint32_t c, uint32_t comInH,
                                  uint32_t comInW) {
  if (hpipfetchflag[c][comInH][comInW] && !hpipDecodeflag[c][comInH][comInW] &&
      !IssuePortFromHPIP[c][comInH][comInW].empty()) {
    hpipDecodeHold[c][comInH][comInW] = IssuePortFromHPIP[c][comInH][comInW][0];
    if (chipMemControl[c]->checkMemReady(hpipDecodeHold[c][comInH][comInW],
                                         "HPIP_MEM_Stall_(" +
                                             std::to_string(c) + ")")) {
      IssuePortFromHPIP[c][comInH][comInW].erase(
          IssuePortFromHPIP[c][comInH][comInW].begin());
      hpipfetchflag[c][comInH][comInW] = false;
      hpipDecodeflag[c][comInH][comInW] = true;
    }
  }
}

void Arch::issueHpipInstructions(uint32_t c, uint32_t comInH, uint32_t comInW) {
  if (hpipDecodeflag[c][comInH][comInW] && !hpipIssueflag[c][comInH][comInW]) {
    auto lastIns = hpipDecodeHold[c][comInH][comInW].back();
    // check scoreboard and mem for one instruction, as each mac unit only
    // can recive one instruction
    if (chipboard[c]->check({lastIns})) {
      hpipIssueHold[c][comInH][comInW] = {lastIns};
      // hpipIssueHold[c][comInH][comInW] =
      // std::vector<Instruction*>{lastIns};
      hpipDecodeHold[c][comInH][comInW].pop_back();
      // ! all ins commit can next issue
      hpipIssueflag[c][comInH][comInW] = true;
      if (hpipDecodeHold[c][comInH][comInW]
              .empty()) { //如果所有的指令都可以issue了，则可以decode下一组指令了
        hpipDecodeflag[c][comInH][comInW] = false;
      }
    } else {
      // stat->increaseStat("HPIP_RAW_Stall");
    }
  }
}

void Arch::commitHpipInstructions(uint32_t c, uint32_t comInH,
                                  uint32_t comInW) {
  if (hpipIssueflag[c][comInH][comInW] &&
      hpipIOs[c][comInH][comInW]->GetSignal()) {
    auto lastIns = hpipIssueHold[c][comInH][comInW][0];
    // Set instruction in IO and update commit instructions
    hpipIOs[c][comInH][comInW]->SetIns(lastIns);
    auto insName = hpipDecodeHold[c][comInH][comInW][0]->GetInsName();

    updateCommitInstructions(CommitInstHPIP[c][comInH][comInW],
                             CommitInstHPIPOrder[c][comInH][comInW], insName,
                             lastIns);
    chipMemControl[c]->accessFromUnits(lastIns);
    chipboard[c]->insert(lastIns);
    hpipIssueflag[c][comInH][comInW] = false;
  }
}
void Arch::writebackHpipInstructions(uint32_t c) {
  // Handle HPIP unit outputs
  auto hpipOutputs = hpipus[c]->getOutput();
  for (auto h = 0; h < hpipus[c]->getVecPECount(); h++) {
    for (auto w = 0; w < hpipOutputs[h].size(); w++) {
      auto out = hpipOutputs[h][w];
      if (out->GetSignal()) {
        if (!handleMultiOutput(c, CommitInstHPIP[c][h][w],
                               CommitInstHPIPOrder[c][h][w], out)) {
          stat->increaseStat("HPIP_OutMem_Stall_(" + std::to_string(c) + ")");
        }
      }
    }
  }
}

void Arch::updateCommitInstructions(
    std::map<std::string, std::vector<Instruction *>> &commitInst,
    std::vector<std::string> &commitorder, const std::string &insName,
    Instruction *lastIns) {
  if (commitInst.find(insName) == commitInst.end()) {
    commitInst[insName] = {lastIns};

    commitorder.push_back(insName);
  } else {
    commitInst[insName].push_back(lastIns);
  }
}

void Arch::update() {
  // update components states
  for (auto c = 0; c < num_cluster; c++) {
    updateUnitGroup(c); // Components update
    // pipeline update
    eweupdate(c);
    autoupdate(c);
    nttupdate(c);
    bconvupdate(c);
    if (hasHPIPU) {
      hpipupdate(c);
    }
    // MemControl update
    chipMemControl[c]->update();
  }

  cycle += 1;
}

bool Arch::handleMultiOutput(
    uint32_t c, std::map<std::string, std::vector<Instruction *>> &commitInst,
    std::vector<std::string> &commitOrder, IO *out) {
  auto ins = out->GetIns();

  auto pointerName = commitOrder[0];

  auto &group = commitInst[pointerName];

  if (group[0]->GetInsName() != ins->GetInsName()) {
    throw std::runtime_error("Computation Out of Order for HPIP, causing "
                             "computational error!\n");
  }

  bool retFlag = false;

  if (chipMemControl[c]->insert2MemFromUnits(ins->getOperandOut())) {
    out->CompleteFetch();

    // Todo: instruction completed, the out needs to be stored in on-chip cache.
    chipboard[c]->retire(ins);

    // Remove the first element because it has been executed
    group.erase(group.begin());

    completedIns += 1;
    retFlag = true;
  }

  if (group.empty()) {
    commitInst.erase(commitInst.find(pointerName));
    commitOrder.erase(commitOrder.begin());
  }

  return retFlag;
}

bool Arch::handleSignalOutput(
    uint32_t c, std::map<std::string, std::vector<Instruction *>> &commit,
    IO *out) {
  auto ins = out->GetIns();
  auto insName = ins->GetInsName();

  // Debug: Display completed instruction
  // std::cout << "Completed Instruction: ";
  // ins->ShowIns();

  auto iter = commit.find(insName);
  if (iter == commit.end()) {
    std::cout << "Completed Instruction: ";
    ins->ShowIns();
    throw std::runtime_error("Instruction not committed? Need to confirm!\n");
  }

  //

  if (chipMemControl[c]->insert2MemFromUnits(ins->getOperandOut())) {
    // Todo: instruction completed, the out needs to be stored in on-chip cache.
    chipboard[c]->retire(ins);
    out->CompleteFetch();
    commit.erase(iter);
    completedIns += 1;

    return true;
  } else {
    return false;
  }
}

void Arch::issueIns(uint32_t index, const std::string &name,
                    std::vector<Instruction *> &insg) {
  // Assuming IssuePortFromInsGenLayerOt is declared and accessible
  // Check for valid index and name if necessary

  // Add the group of instructions to the specified issue port
  IssuePortFromInsGenLayerOt[index][name].push_back(
      insg); // Use std::move if insg is not needed afterwards
}

void Arch::issueIns(uint32_t index, uint32_t h, uint32_t w,
                    std::vector<Instruction *> &insg, bool hpip) {
  if (hpip) {
    IssuePortFromHPIP[index][h][w].push_back(insg);
  } else {
    IssuePortFromBCONV[index][h][w].push_back(insg);
  }
}

void Arch::shownStat() {
  fetchComponentsExecute();
  stat->showStat();
}
