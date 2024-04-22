#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "Basic.h"
#include "Config.h"
#include "IO.h"
#include "Instruction.h"

/*
 * The basic implement of component, which is constructed the normal runtime of
 * one pipelined component.
 */
class Components {

private:
  /* data */
  uint32_t pipeline_delay;
  bool overlap;

  uint32_t used_pipeline;

  uint32_t exeCycles; // The execute cycles in this Component
  uint32_t stallCycles;

  std::string Name;

  std::vector<Instruction *> pipeline;

  IO *input; //单输入

  IO *output; // 单输出

  bool executed = false;

public:
  Components(std::string name, uint32_t delay, bool ovlap = true) {
    exeCycles = 0;
    stallCycles = 0;
    used_pipeline = 0;

    Name = name;
    pipeline_delay = delay;
    overlap = ovlap;

    pipeline.resize(delay);

    output = new IO();
  };
  Components(/* args */) {
    exeCycles = 0;
    used_pipeline = 0;
    stallCycles = 0;
  };

  void SetName(std::string name) { Name = name; };

  void SetDelay(uint32_t delay, bool ovlap = true) {
    pipeline_delay = delay;
    overlap = ovlap;
    pipeline.resize(pipeline_delay);

    output = new IO();
  };

  // Connection
  void SetInput(IO *in) { input = in; };

  IO *GetOutput() { return output; };

  void movePipeline() {
    if (used_pipeline > 0 &&
        pipeline[pipeline_delay - 1] ==
            nullptr) { // Only the finall pipeline register is
                       // empty, the pipeline can execution.
      std::rotate(std::begin(pipeline), std::end(pipeline) - 1,
                  std::end(pipeline));
      executed = true;
    }
  };

  uint32_t get_used_pipeline() { return used_pipeline; };

  void update() {
    executed = false;

    // pipeline output execution
    if (pipeline[pipeline_delay - 1] != nullptr && !output->GetSignal()) {
      output->SetIns(pipeline[pipeline_delay - 1]);
      used_pipeline -= 1;
      pipeline[pipeline_delay - 1] = nullptr;
    } else {
      stallCycles += 1;
    }

    movePipeline();

    // Check the state of this component.
    if ((overlap && used_pipeline == pipeline_delay) ||
        (!overlap && used_pipeline > 0)) {
      return;
    }

    if (pipeline[0] == nullptr) {
      Instruction *exe_ins = nullptr;
      if (input->GetSignal()) {
        exe_ins = input->GetIns();
        // exe_ins->ShowIns();
        input->CompleteFetch();
        pipeline[0] = exe_ins;
        used_pipeline += 1;
      }
    }
  };

  bool runningThisCycle() {
    auto flag = (input->GetSignal()) || (get_used_pipeline() > 0);

    return flag;
  };

  bool getExecuted() { return executed; };

  uint32_t getExeCycles() { return exeCycles; };

  uint32_t getStallCycles() { return stallCycles; };

  void getPipeline() {
    for (auto &ins : pipeline) {
      if (ins != nullptr)
        std::cout << ins->GetInsName() << " ";
      else
        std::cout << "nullptr"
                  << " ";
    }
    std::cout << std::endl;
  };

  std::string getName() { return Name; };

  void checkInputPorts() {
    std::cout << getName() << std::endl;
    if (input == nullptr) {
      std::cout << "Input ports not connection!\n";
    } else {
      std::cout << "Passing\n";
    }
  }
};

/***
 * This component executes multi instruction, and some instructions intergrate
 * into one intruction group. On instruction completed, and other instruction
 * also indicates completed.
 */
class EWE {
private:
  /* data */
  uint32_t num_mul, num_add;
  uint32_t delay_mul, delay_add;
  std::vector<Components *> mulList, addList;

  std::vector<IO *> inputList;
  std::vector<IO *> outputList;

  std::vector<IOFusion *> iofusionList;

  uint32_t stallCycles;

  std::string Name;

  unsigned long long exeCycles;

public:
  EWE(Config *cfg);

  // Connection
  uint32_t getInputNum();
  void setInput(std::vector<IO *> inputs);

  uint32_t getOutputNum();
  std::vector<IO *> getOutput();

  uint32_t getStallCycles();

  void getPipeline();
  std::string getName();

  void update();

  void checkInputPorts();

  unsigned long long getExeCycles() { return exeCycles; };
};

/**
 * This implement based on the refernece: ARK
 * It consists 8 stage~(for log_{N} is 16), and each stage executes swapping
 * opeation.
 *
 */
class AUTOU {
private:
  /* data */
  uint32_t stages;
  uint32_t delay_auto;

  std::vector<Components *> autous;

  IO *input;
  IO *output;

  std::string Name;

  uint32_t stallCycles;

  unsigned long long exeCycles;

public:
  AUTOU(Config *cfg);

  // Connection
  virtual void setInput(IO *in);

  virtual IO *getOutput();

  uint32_t getStallCycles();

  virtual void getPipeline();
  std::string getName();

  virtual void update();

  virtual void checkInputPorts();

  void setName(std::string name) { Name = name; };

  unsigned long long getExeCycles() { return exeCycles; };
};

/**
 * 多个输入，num_width*num_high个输入
 * 多个输出，但是每个cycle应该只有num_high个输出
 * 指令分配和调度单应该检测该cycle，哪一个端口会输出
 */
class BCONVU {
private:
  /* data */
  // NOTE:!!!
  // This delay indicats the total delay of complete one mult and add
  // operation And the instruction still related with the length of N and
  // batch size Only the instruction group completed, the whole BCONV
  // operation indicates completed.
  uint32_t mac_delay; // mul+add+holdFIFO delay

  uint32_t fifo_delay;

  uint32_t num_width, num_high;

  std::vector<std::vector<Components *>> bconvArray;

  // std::vector<std::vector<IO*>> inputList;
  std::vector<std::vector<IO *>> outputList;

  std::string Name;

  uint32_t stallCycles;

  unsigned long long exeCycles;

public:
  BCONVU(Config *cfg);

  // Connection
  void setInput(std::vector<std::vector<IO *>> in);

  uint32_t getOutCount() { return num_high; };

  std::vector<std::vector<IO *>> getOutput();

  void update();

  void getPipeline();

  void checkInputPorts();

  uint32_t getStallCycles();

  std::string getName();

  uint32_t getHigh() { return num_high; };

  uint32_t getWidth() { return num_width; };

  unsigned long long getExeCycles() { return exeCycles; };
};

class NTTU {
private:
  /* data */
  uint32_t butterFly_delay;
  uint32_t phase1_step1_depth;
  uint32_t phase1_step2_depth;
  uint32_t intraTrans_delay; // lane 内的delay
  uint32_t interTrans_delay; // lane 间之间的delay
  uint32_t phase2_step1_depth;
  uint32_t phase2_step2_depth;

  uint32_t phase2_stall_delay;

  Components *phase1_step1;
  Components *phase1_step2;

  Components *intraTrans_phase1, *intraTrans_phase2;
  Components *interTrans;

  Components *stallCom;

  Components *phase2_step1;
  Components *phase2_step2;

  IO *output;

  std::string Name;

  unsigned long long exeCycles;

public:
  NTTU(Config *cfg);

  void update();

  void setInput(IO *in);

  IO *getOutput();

  void getPipeline();

  void checkInputPorts();

  uint32_t getStallCycles();

  std::string getName();

  unsigned long long getExeCycles() { return exeCycles; };
};

class HPIP {
private:
  /* data */
  uint32_t VecPECount;
  uint32_t MacCount;

  uint32_t MacDelay;

  uint32_t stallCycle;
  unsigned long long exeCycles;

  std::vector<std::vector<Components *>> VecPEArray;

  std::vector<std::vector<IO *>> VecPEArrayOutput;
  std::string Name;

public:
  HPIP(Config *cfg);

  void update();

  void setInput(std::vector<std::vector<IO *>> in);

  std::vector<std::vector<IO *>> getOutput();

  void getPipeline();

  void checkInputPorts();

  uint32_t getVecPECount() { return VecPECount; };

  uint32_t getMacCount() { return MacCount; };

  uint32_t getStallCycles();

  std::string getName();

  unsigned long long getExeCycles() { return exeCycles; };
};

#endif