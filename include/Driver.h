#ifndef _DRIVER_H
#define _DRIVER_H

#include "Basic.h"
#include "Config.h"
#include "Instruction.h"
#include "mem.h"

typedef std::vector<Instruction *> INSGROUP;

class Driver {
private:
  uint32_t cluster;
  std::vector<std::map<std::string, std::vector<INSGROUP>>>
      sentInsFIFO; // This var will strore the instruction list that will sent
                   // to the accelerator.

  uint32_t entryCountForFifoDram;

  std::map<AddrType, uint32_t> dispathTimes;

  // [cluser, list, entry]
  std::vector<std::vector<std::vector<AddrType>>> dispatchedAddrHolder;

  std::vector<std::vector<AddrType>>
      dataHolder; // [cluster, entry] each entry is equal ==
                  // entrySize it will sent else contain it

  uint32_t currentNTTClusterIndex;
  uint32_t currentAUTOClusterIndex;
  uint32_t currentBCONVClusterIndex;
  uint32_t currentEWEClusterIndex;
  uint32_t currentHPIPClusterIndex;

  DataMap *datamap;

  uint32_t bconvh, bconvw;
  uint32_t hpiph, hpipw;

  unsigned long long totalIns;

public:
  Driver(Config *cfg, DataMap *map) {
    cluster = cfg->getValue("cluster");
    sentInsFIFO.resize(cluster); // each cluster has its own fifo.
    dispatchedAddrHolder.resize(cluster);
    dataHolder.resize(cluster);

    currentNTTClusterIndex = 0;
    currentAUTOClusterIndex = 0;
    currentBCONVClusterIndex = 0;
    currentEWEClusterIndex = 0;
    currentHPIPClusterIndex = 0;

    entryCountForFifoDram = cfg->getValue("entryCount");

    bconvh = cfg->getValue("bconv_num_high");
    bconvw = cfg->getValue("bconv_num_width");

    hpiph = cfg->getValue("VecPECount");
    hpipw = cfg->getValue("MacCount");

    datamap = map;

    totalIns = 0;
  };

  /***
   * Input: [level, batch, insgroup]
   */
  void dispatchInstructions(const std::vector<std::vector<INSGROUP>> &map) {
    if (map.empty() || map[0].empty() || map[0][0].empty()) {
      throw std::runtime_error("Empty instruction map provided.");
    }

    std::string opName = map[0][0][0]->GetOpName();

    // std::cout<<"Debug: map size is "<<map.size()<<"\n";

    // Use a lambda that captures 'this' pointer
    auto genericDispatch = [this, &map](auto dispatchFunction) {
      for (uint32_t l = 0; l < map.size(); l++) {
        (this->*dispatchFunction)(map[l],
                                  l); // Correctly call the member function
      }
    };

    if (opName == "NTT" || opName == "INTT") {
      genericDispatch(&Driver::dispatchInsToNTTUnits);
    } else if (opName == "AUTO") {
      genericDispatch(&Driver::dispatchInsToAUTOUnits);
    } else if (opName == "MULT") {
      for (uint32_t l = 0; l < map.size(); l++) {
        dispatchInsToEWEUnits(map[l]);
      }
    } else if (opName == "BCONV_STEP2") {
      dispatchInsToBCONVUnits(map);
    } else if (opName == "IP") {
      dispatchInsToHPIPUnits(map);
    } else {
      std::cout << opName << "\n";
      throw std::runtime_error("This instruction generation error, as not "
                               "exist corresponding component.");
    }
  };

  void extractInputAddrFromInstruction(
      INSGROUP insg, uint32_t clusterId,
      std::vector<std::vector<AddrType>> &dispatchHolder) {
    for (auto &ins : insg) {
      auto count = ins->getinputCount();
      for (uint32_t i = 0; i < count; i++) {
        auto addr = ins->getOperand(i);

        // if(addr == 56861){
        //   std::cout<<"\n";
        //   ins->ShowIns();

        //   for(uint32_t m=0;m<count;m++){
        //     std::cout<<" "<<ins->getOperand(m)<<" ";
        //   }

        //   throw std::runtime_error("\nDebug for this error!\n");
        // }

        if (dispathTimes.find(addr) != dispathTimes.end()) {
          continue;
        }
        dispathTimes[addr] = 1;

        if (!datamap->isInOutputMap(addr)) {
          dataHolder[clusterId].push_back(addr);

          if (dataHolder[clusterId].size() > entryCountForFifoDram) {
            throw std::runtime_error(
                "Exceeded entry count for FIFO DRAM in cluster " +
                std::to_string(clusterId));
          }

          if (dataHolder[clusterId].size() == entryCountForFifoDram) {
            dispatchHolder.push_back(dataHolder[clusterId]);
            // std::cout<<"INSERT->"<<dataHolder[clusterId].size()<<"
            // "<<dataHolder[clusterId][0]<<"\n";
            dataHolder[clusterId].clear();
          } else {
            std::cout << entryCountForFifoDram << " "
                      << dataHolder[clusterId].size() << "\n";
            throw std::runtime_error("data not in map\n");
          }
        }
      }
    }
  };

  void dispatchInsToNTTUnits(std::vector<INSGROUP> batchInsGroup,
                             uint32_t level) {

    currentNTTClusterIndex = level % cluster;

    for (auto &insg : batchInsGroup) {
      sentInsFIFO[currentNTTClusterIndex]["NTT"].push_back(insg);
      // std::cout<<"Debug:
      // "<<sentInsFIFO[currentNTTClusterIndex]["NTT"].size()<<"\n";
      try {
        extractInputAddrFromInstruction(
            insg, currentNTTClusterIndex,
            dispatchedAddrHolder[currentNTTClusterIndex]);
      } catch (const std::runtime_error &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        // Handle the error appropriately
      }
    }
  };

  void dispatchInsToAUTOUnits(std::vector<INSGROUP> batchInsGroup,
                              uint32_t level) {

    currentAUTOClusterIndex = level % cluster;

    for (auto &insg : batchInsGroup) {
      sentInsFIFO[currentAUTOClusterIndex]["AUTO"].push_back(insg);
      try {
        extractInputAddrFromInstruction(
            insg, currentAUTOClusterIndex,
            dispatchedAddrHolder[currentAUTOClusterIndex]);
      } catch (const std::runtime_error &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        // Handle the error appropriately
      }
    }
  };

  void dispatchInsToEWEUnits(std::vector<INSGROUP> batchInsGroup) {

    for (auto &insg : batchInsGroup) {
      sentInsFIFO[currentEWEClusterIndex]["EWE"].push_back(insg);
      try {
        extractInputAddrFromInstruction(
            insg, currentEWEClusterIndex,
            dispatchedAddrHolder[currentEWEClusterIndex]);
      } catch (const std::runtime_error &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        // Handle the error appropriately
      }
      currentEWEClusterIndex = (currentEWEClusterIndex + 1) % cluster;
    }
  };

  void dispatchInsToBCONVUnits(const std::vector<std::vector<INSGROUP>> &map) {
    // Calculate the width and height tile counts with ceiling division
    uint32_t wTileCount = static_cast<uint32_t>(
        std::ceil(static_cast<double>(map.size()) / bconvw));
    uint32_t hTileCount = static_cast<uint32_t>(
        std::ceil(static_cast<double>(map[0].size()) / bconvh));

    // Iterate over the grid of tiles
    for (uint32_t hTile = 0; hTile < hTileCount; hTile++) {
      for (uint32_t wTile = 0; wTile < wTileCount; wTile++) {
        // Process each instruction group within the tile
        for (uint32_t bconvHeightOffset = 0; bconvHeightOffset < bconvh;
             bconvHeightOffset++) {
          uint32_t executionBatch = hTile * bconvh + bconvHeightOffset;
          if (executionBatch < map[0].size()) {
            for (uint32_t bconvWidthOffset = 0; bconvWidthOffset < bconvw;
                 bconvWidthOffset++) {
              uint32_t executionLevel = wTile * bconvw + bconvWidthOffset;
              if (executionLevel < map.size()) {
                const auto &insGroup = map[executionLevel][executionBatch];
                sentInsFIFO[currentBCONVClusterIndex]["BCONV"].push_back(
                    insGroup);
                try {
                  extractInputAddrFromInstruction(
                      insGroup, currentBCONVClusterIndex,
                      dispatchedAddrHolder[currentBCONVClusterIndex]);
                } catch (const std::runtime_error &error) {
                  std::cerr << "Error: " << error.what() << std::endl;
                  // Handle the error appropriately
                }
              }
            }
          }
        }
      }
      currentBCONVClusterIndex = (currentBCONVClusterIndex + 1) % cluster;
    }
  };

  void dispatchInsToHPIPUnits(const std::vector<std::vector<INSGROUP>> &map) {
    // Calculate the width and height tile counts with ceiling division
    uint32_t wTileCount = static_cast<uint32_t>(
        std::ceil(static_cast<double>(map.size()) / hpipw));
    uint32_t hTileCount = static_cast<uint32_t>(
        std::ceil(static_cast<double>(map[0].size()) / hpiph));

    // Iterate over the grid of tiles
    for (uint32_t hTile = 0; hTile < hTileCount; hTile++) {
      for (uint32_t wTile = 0; wTile < wTileCount; wTile++) {
        // Process each instruction group within the tile
        for (uint32_t bconvHeightOffset = 0; bconvHeightOffset < hpiph;
             bconvHeightOffset++) {
          uint32_t executionBatch = hTile * hpiph + bconvHeightOffset;
          if (executionBatch < map[0].size()) {
            for (uint32_t bconvWidthOffset = 0; bconvWidthOffset < hpipw;
                 bconvWidthOffset++) {
              uint32_t executionLevel = wTile * hpipw + bconvWidthOffset;
              if (executionLevel < map.size()) {
                const auto &insGroup = map[executionLevel][executionBatch];
                sentInsFIFO[currentHPIPClusterIndex]["HPIP"].push_back(
                    insGroup);
                try {
                  extractInputAddrFromInstruction(
                      insGroup, currentHPIPClusterIndex,
                      dispatchedAddrHolder[currentHPIPClusterIndex]);
                } catch (const std::runtime_error &error) {
                  std::cerr << "Error: " << error.what() << std::endl;
                  // Handle the error appropriately
                }
              }
            }
          }
        }
      }
      currentHPIPClusterIndex = (currentHPIPClusterIndex + 1) % cluster;
    }
  };

  void IssueInsFromDramToChip(Arch *arch) {

    for (uint32_t c = 0; c < cluster; c++) {

      // Generic function to issue instructions and update FIFO
      auto issueInstructionAndUpdateFIFO = [this, c,
                                            arch](const std::string &type) {
        while (!sentInsFIFO[c][type].empty()) {
          arch->issueIns(c, type, sentInsFIFO[c][type].front());
          totalIns += sentInsFIFO[c][type].front().size();
          sentInsFIFO[c][type].erase(sentInsFIFO[c][type].begin());
        }
      };

      // Issue instructions for different components
      issueInstructionAndUpdateFIFO("EWE");
      issueInstructionAndUpdateFIFO("NTT");
      issueInstructionAndUpdateFIFO("AUTO");

      // Specific handling for BCONV and HPIP
      auto issueBCONVorHPIP = [this, c, arch](const std::string &type,
                                              uint32_t height, uint32_t width,
                                              bool flag) {
        while (!sentInsFIFO[c][type].empty()) {
          for (uint32_t h = 0; h < height; h++) {
            for (uint32_t w = 0; w < width; w++) {
              auto insg = sentInsFIFO[c][type][0];
              arch->issueIns(c, h, w, insg, flag);
              totalIns += insg.size();
            }
          }
          sentInsFIFO[c][type].erase(sentInsFIFO[c][type].begin());
        }
      };

      issueBCONVorHPIP("BCONV", bconvh, bconvw, false);
      issueBCONVorHPIP("HPIP", hpiph, hpipw, true);
    }
  };

  void IssueDataFromDramToChip(std::vector<MemController *> memC) {
    for (uint32_t c = 0; c < cluster;
         c++) { // scheme 1. put all memory to chip.
      // Check and send data group to on-chip memory
      auto &addrHolder = dispatchedAddrHolder[c];
      if (addrHolder.empty())
        continue;
      auto it = addrHolder.begin();
      if (!it->empty() && memC[c]->sentToOnChipMem(*it)) {
        // std::cout<<"===================\n\n\n";
        it = addrHolder.erase(it);
      }
    }
  }

  void shownData() {
    for (uint32_t c = 0; c < 1; c++) {
      // Check and send data group to on-chip memory
      auto &addrHolder = dispatchedAddrHolder[c];

      // if(addrHolder == nullptr) continue;

      if (addrHolder.empty()) {
        std::cout << "size is " << 0 << "\n\n";

        continue;
      }

      uint32_t size = addrHolder.size();
      std::cout << "size is " << size << "\n\n";
      // for(uint32_t a=0;a<size;a++){
      //   std::cout<<"c "<<c<<" "<<addrHolder[a][0]<<" ";
      // }
      std::cout << "\n";
    }
  };

  void checkvalue() {
    if (dispathTimes.find(50443) != dispathTimes.end()) {
      std::cout << "============================\n\n\n";
    }
  };

  unsigned long long getTotalIns() { return totalIns; };

  ~Driver();
};

#endif