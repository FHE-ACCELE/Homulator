#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "Basic.h"

enum ins_ops {
  NTT,
  INTT,
  MULT,
  MADD,
  MSUB,
  BCONV_STEP1,
  BCONV_STEP2,
  AUTO,
  DS,
  PRNG,
  IP,
  FETCH_RF,
  STORE_RF
};

extern std::vector<std::string> ins_ops_name;

/*
 */
class Instruction {
private:
  /* data */
  uint32_t level_id; //
  uint32_t batch_id;
  ins_ops ops; // ins_ops
  AddrType pc;

  // Not used
  AddrType data_address;

  std::string Name; // Instruction Name

  std::string OpName; // Operation Name

  uint32_t inputCount;

  std::vector<Instruction *> depsInsList;
  std::vector<bool> operandReadyList;
  std::vector<bool> operandFormatList; // ture -> ntt format
  std::vector<AddrType> operandList;

  AddrType OutputOperand;
  bool OutputOperandFormat;

public:
  Instruction(ins_ops ops_, uint32_t level, uint32_t batch, uint32_t data = 0) {
    ops = ops_;
    level_id = level;
    batch_id = batch;
    data_address = data;
    depsInsList.reserve(2);
  };

  Instruction(std::string name, ins_ops ops_, uint32_t level, uint32_t batch,
              uint32_t data = 0) {
    ops = ops_;
    Name = name;
    level_id = level;
    batch_id = batch;
    data_address = data;
    depsInsList.reserve(2);
  };

  Instruction(){};

  Instruction(std::string name, ins_ops ops_, uint32_t levelId,
              uint32_t batchId, uint32_t op1Address, uint32_t opOutAddress) {
    Name = name;
    ops = ops_;
    level_id = levelId;
    batch_id = batchId;
    inputCount = 1;

    operandList.push_back(op1Address);
    OutputOperand = opOutAddress;
    operandReadyList.push_back(false);
    operandFormatList.push_back(false);

    depsInsList.reserve(2);
  };

  Instruction(std::string name, ins_ops ops_, uint32_t levelId,
              uint32_t batchId, uint32_t op1Address, uint32_t op2Address,
              uint32_t op3Address, uint32_t op4Address, uint32_t opOutAddress) {
    Name = name;
    ops = ops_;
    level_id = levelId;
    batch_id = batchId;
    inputCount = 4;

    operandList.insert(operandList.end(),
                       {op1Address, op2Address, op3Address, op4Address});
    OutputOperand = opOutAddress;
    operandReadyList.insert(operandReadyList.end(),
                            {false, false, false, false});
    operandFormatList.insert(operandFormatList.end(),
                             {false, false, false, false});

    depsInsList.reserve(2);
  };

  Instruction(std::string name, ins_ops ops_, uint32_t levelId,
              uint32_t batchId, uint32_t op1Address, uint32_t op2Address,
              uint32_t opOutAddress) {
    Name = name;
    ops = ops_;
    level_id = levelId;
    batch_id = batchId;
    inputCount = 2;

    operandList.insert(operandList.end(), {op1Address, op2Address});
    OutputOperand = opOutAddress;
    operandReadyList.insert(operandReadyList.end(), {false, false});
    operandFormatList.insert(operandFormatList.end(), {false, false});
    depsInsList.reserve(2);
  };

  // void SetOpName(std::string name) { OpName = name; };

  uint32_t getinputCount() { return inputCount; };

  std::string GetOpName() { return ins_ops_name[ops]; };

  void SetLevel(uint32_t level) { level_id = level; };

  void SetBatch(uint32_t batch) { batch_id = batch; };

  void SetOps(ins_ops ops_) { ops = ops_; };

  void SetData(uint32_t data) { data_address = data; };

  uint32_t getLevel() { return level_id; };

  uint32_t getBatch() { return batch_id; };

  std::string GetInsName() {
    return Name + " Operation(" + ins_ops_name[ops] + ")_level(" +
           std::to_string(level_id) + ")_batchId(" + std::to_string(batch_id) +
           ")";
  };

  void ShowIns() { std::cout << GetInsName() << std::endl; }

  bool isInputReady() {
    auto iter =
        std::find(operandReadyList.begin(), operandReadyList.end(), false);
    return iter == operandReadyList.end();
  };

  bool isStateOperandReady(uint32_t index) { return operandReadyList[index]; };

  AddrType getOperand(uint32_t index) const { return operandList[index]; };

  AddrType getOperandOut() const { return OutputOperand; }

  void setOperandReady(uint32_t index) { operandReadyList[index] = true; };

  bool getOperandFormat(uint32_t index) { return operandFormatList[index]; };

  bool getOperandOutFormat() { return OutputOperandFormat; };

  void setInsPC(AddrType addr) { pc = addr; };

  void setDepIns(Instruction *ins, uint32_t index) {
    depsInsList[index] = ins;
  };

  Instruction *getDepIns(uint32_t index) { return depsInsList[index]; };

  void setNttForOpOut() { OutputOperandFormat = true; };

  void setINttForOpOut() { OutputOperandFormat = false; };
  // Set format for a specific operand (1-indexed for convenience)
  void setNttForOperand(uint32_t operandIndex) {
    operandFormatList[operandIndex] = true;
  }

  // Reset format for a specific operand (1-indexed for convenience)
  void setINttForOperand(uint32_t operandIndex) {
    operandFormatList[operandIndex] = false;
  }
};

#endif