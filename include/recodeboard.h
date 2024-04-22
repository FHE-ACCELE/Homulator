#ifndef RECOREBOARD
#define RECOREBOARD

#include "Basic.h"
#include "Instruction.h"

class RecodeBoard {
private:
  std::map<AddrType, Instruction *> board;

public:
  RecodeBoard() = default;

  void insert(Instruction *ins) {
    AddrType out = ins->getOperandOut();
    if (board.find(out) != board.end()) {
      // board[out]->ShowIns();
      // // auto inss = ;
      // ins->ShowIns();

      // BCONV instruction will insert same out address.
      // throw std::runtime_error("ERROR! Some instruction has written to this "
      //                          "address! WAW hazard detected!\n");

      board[out] = ins;
    } else {
      board[out] = ins;
    }
  };

  bool check(const std::vector<Instruction *> &insg) const {
    for (const auto &ins : insg) {
      AddrType op1 = ins->getOperand(1);
      if (board.find(op1) != board.end()) {
        return false;
      }

      AddrType op2 = ins->getOperand(2);
      if (board.find(op2) != board.end()) {
        return false;
      }
    }
    return true;
  };

  void retire(const Instruction *ins) {
    auto iter = board.find(ins->getOperandOut());
    if (iter == board.end()) {

      // For debugging

      // throw std::runtime_error(
      //     "Retire error: No instruction found with this address.");
    } else {
      board.erase(iter);
    }
  };
};

#endif