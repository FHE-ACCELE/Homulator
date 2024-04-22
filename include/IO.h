#ifndef _IO_H_
#define _IO_H_

#include "Instruction.h"

class IO {
private:
  /* data */
  Instruction *ins;
  bool reday; // true indicates this port is used, and the instruction can be
              // executed. false indicates the port can sent next instruction.
public:
  IO() {
    ins = nullptr;
    reday = false;
  };

  // Output port need set instruction of completeing exection
  void SetIns(Instruction *ins_) {
    ins = ins_;
    reday = true;
  };

  // Input port need sent signal to previous component.
  void CompleteFetch() { reday = false; };

  bool GetSignal() { return reday; };

  Instruction *GetIns() { return ins; };
};

/***********************************
 * Two output merge into one port
 * The instruction need to re-commit
 * How to fuse the instruction?
 ************************************/
class IOFusion {
private:
  /* data */
  IO *in1;
  IO *in2;
  IO *out;

public:
  // IOFusion(){};
  IOFusion() {
    out = new IO();

    // return out;
  };

  IO *getPort(IO *i1, IO *i2) {
    in1 = i1;
    in2 = i2;
    return out;
  };

  // 两个指令二选一。
  // 根据指令组进行commit释放
  void update() {
    Instruction *exe_ins = nullptr;
    if (in1->GetSignal() && in2->GetSignal() && !out->GetSignal()) {
      exe_ins = in2->GetIns();
      in1->CompleteFetch();
      in2->CompleteFetch();
      out->SetIns(exe_ins);
      // std::cout<<"IOFUSION "<<exe_ins->GetInsName()<<"\n";
    }
  };
};

#endif