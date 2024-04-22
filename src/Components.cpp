#include "Components.h"

/*****************
 * cfg: the configutation function
 * function: This function will perform the inital EWE
 *
 */
EWE::EWE(Config *cfg) {

  Name = "Element-wised-engine";

  stallCycles = 0;
  exeCycles = 0;

  // Get the configuration from config.txt
  num_mul = cfg->getValue("ewe_num_mul");
  num_add = cfg->getValue("ewe_num_add");

  delay_mul = cfg->getValue("ewe_mult_delay");
  delay_add = cfg->getValue("ewe_madd_delay");

  // the number of input ports is equally with the number of mul.
  inputList.reserve(num_mul);
  // the number of out ports is equally with the number of adder.
  outputList.resize(num_add);

  for (auto i = 0; i < num_mul; i++) {
    Components *mul =
        new Components(("ewe_mul_" + std::to_string(i)).c_str(), delay_mul);
    // The inputs of mul assign in the global setinputs function,

    mulList.push_back(mul);
  }

  for (auto i = 0; i < num_add; i++) {
    Components *add =
        new Components(("ewe_add_" + std::to_string(i)).c_str(), delay_add);
    outputList[i] = add->GetOutput();
    addList.push_back(add);
  }

  // Check if number of adders is twice the number of multipliers
  if (num_add * 2 != num_mul) {
    std::cerr << "EWE ERROR: Incompatible number of adders and multipliers!"
              << std::endl;
    exit(1); // Handle error appropriately
  }

  // Connection for each part
  for (auto i = 0; i < num_add; i++) {
    //
    IOFusion *fu = new IOFusion();
    addList[i]->SetInput(fu->getPort(mulList[2 * i]->GetOutput(),
                                     mulList[2 * i + 1]->GetOutput()));
    iofusionList.push_back(fu);
  }
}

uint32_t EWE::getInputNum() { return num_mul; }

// Connection for each mul component.
void EWE::setInput(std::vector<IO *> inputs) {

  for (auto i = 0; i < num_mul; i++) {
    if (inputs[i] == nullptr) {
      throw std::runtime_error("The input is nullptr, please to confirm!\n");
    }
    mulList[i]->SetInput(inputs[i]);
    inputList.push_back(inputs[i]);
  }
}

uint32_t EWE::getOutputNum() { return num_add; }

std::vector<IO *> EWE::getOutput() { return outputList; }

// uint32_t EWE::getExeCycles() {
//   // return exeCycles;
//   std::vector<uint32_t> exeList;

//   for (auto &mul : mulList) {
//     exeList.push_back(mul->getExeCycles());
//   }

//   for (auto &add : addList) {
//     exeList.push_back(add->getExeCycles());
//   }
//   auto maxElement = std::max_element(exeList.begin(), exeList.end());

//   return *maxElement;
// }

uint32_t EWE::getStallCycles() {
  std::vector<uint32_t> stallList;

  for (auto &add : addList) {
    stallList.push_back(add->getStallCycles());
  }
  auto maxElement = std::max_element(stallList.begin(), stallList.end());

  return *maxElement;
}

void EWE::getPipeline() {
  for (auto &mul : mulList) {
    std::cout << " " << mul->getName()
              << " \n"; // Assuming Components has a getName() method
    mul->getPipeline();
  }
  std::cout << std::endl;

  for (auto &add : addList) {
    std::cout << " " << add->getName()
              << " \n"; // Assuming Components has a getName() method
    add->getPipeline();
  }
  std::cout << std::endl;
}

std::string EWE::getName() { return Name; }

void EWE::update() {

  bool running = false;

  // Output is ok, it will change from adder components
  for (auto &add : addList) {
    if (add->runningThisCycle()) {
      add->update();
      if (add->getExecuted())
        running = true;
    }
  }

  for (auto &fu : iofusionList) {
    fu->update();
  }

  for (auto &mul : mulList) {
    if (mul->runningThisCycle()) {
      mul->update();
      if (mul->getExecuted())
        running = true;
    }
  }

  if (running) {
    exeCycles += 1;
  }
}

void EWE::checkInputPorts() {
  std::cout << getName() << std::endl;
  for (auto &in : inputList) {
    if (in == nullptr) {
      std::cout << "Input ports not connection!\n";
    } else {
      std::cout << "Passing\n";
    }
  }

  for (auto &mul : mulList) {
    mul->checkInputPorts();
  }

  for (auto &add : addList) {
    add->checkInputPorts();
  }
}

// AUTOU implement

AUTOU::AUTOU(Config *cfg) {
  Name = "Automoriphism Unit";

  stallCycles = 0;
  exeCycles = 0;

  stages = cfg->getValue("auto_stages");
  delay_auto = cfg->getValue("auto_delay");
  // std::cout<<stages<<" "<<delay_auto<<"\n";

  for (auto i = 0; i < stages; i++) {
    autous.push_back(new Components(("auto_stage_" + std::to_string(i)).c_str(),
                                    delay_auto));
    // Connection
    if (i > 0) {
      autous[i]->SetInput(autous[i - 1]->GetOutput());
    }
  }

  input = nullptr;
  output = autous[stages - 1]->GetOutput();
}

void AUTOU::setInput(IO *in) {
  input = in;
  autous[0]->SetInput(in);
}

IO *AUTOU::getOutput() { return output; }

// uint32_t AUTOU::getExeCycles() {
//   // return exeCycles;
//   std::vector<uint32_t> exeList;

//   for (auto &autou : autous) {
//     exeList.push_back(autou->getExeCycles());
//   }
//   auto maxElement = std::max_element(exeList.begin(), exeList.end());

//   return *maxElement;
// }

uint32_t AUTOU::getStallCycles() {
  std::vector<uint32_t> stallList;

  for (auto &autou : autous) {
    stallList.push_back(autou->getStallCycles());
  }
  auto maxElement = std::max_element(stallList.begin(), stallList.end());

  return *maxElement;
}

void AUTOU::getPipeline() {
  for (auto &autou : autous) {
    std::cout << " " << autou->getName()
              << " \n"; // Assuming Components has a getName() method
    autou->getPipeline();
  }
  std::cout << std::endl;
}

std::string AUTOU::getName() { return Name; }

// This component only need update sub-components is ok.
void AUTOU::update() {

  bool running = false;

  for (auto it = autous.rbegin(); it != autous.rend(); ++it) {
    if ((*it)->runningThisCycle()) {
      (*it)->update();
      if ((*it)->getExecuted())
        running = true;
    }
  }

  if (running) {
    exeCycles += 1;
  }
}

void AUTOU::checkInputPorts() {
  std::cout << getName() << std::endl;
  if (input == nullptr) {
    std::cout << "Input ports not connection!\n";
  } else {
    std::cout << "Passing\n";
  }

  for (auto &autou : autous) {
    autou->checkInputPorts();
  }
}

BCONVU::BCONVU(Config *cfg) {
  Name = "BConv Unit";

  mac_delay = cfg->getValue("bconv_mac_delay");

  fifo_delay = cfg->getValue("bconv_fifo_delay");

  num_high = cfg->getValue("bconv_num_high");
  num_width = cfg->getValue("bconv_num_width");

  for (auto i = 0; i < num_high; i++) {
    std::vector<Components *> bconv_lane;
    std::vector<IO *> outs;
    for (auto j = 0; j < num_width; j++) {
      Components *mac = new Components(
          (Name + "_mac_" + std::to_string(i) + "_" + std::to_string(j))
              .c_str(),
          mac_delay + j * fifo_delay);
      bconv_lane.push_back(mac);
      outs.push_back(mac->GetOutput());
    }
    bconvArray.push_back(bconv_lane);
    // inputList.push_back()
    outputList.push_back(outs);
  }

  exeCycles = 0;
}

void BCONVU::setInput(std::vector<std::vector<IO *>> in) {
  for (auto i = 0; i < num_high; i++) {
    for (auto j = 0; j < num_width; j++) {
      bconvArray[i][j]->SetInput(in[i][j]);
    }
  }
}

std::vector<std::vector<IO *>> BCONVU::getOutput() { return outputList; }

void BCONVU::update() {

  bool running = false;

  for (auto &lane : bconvArray) {
    for (auto &mac : lane) {
      if (mac->runningThisCycle()) {
        mac->update();
        if (mac->getExecuted())
          running = true;
      }
    }
  }

  if (running) {
    exeCycles += 1;
  }
}

void BCONVU::getPipeline() {
  for (auto i = 0; i < num_high; i++) {
    for (auto j = 0; j < num_width; j++) {
      std::cout << " " << bconvArray[i][j]->getName()
                << " \n"; // Assuming Components has a getName() method
      bconvArray[i][j]->getPipeline();
    }
  }
  std::cout << std::endl;
}

std::string BCONVU::getName() { return Name; }

void BCONVU::checkInputPorts() {

  for (auto i = 0; i < num_high; i++) {
    for (auto j = 0; j < num_width; j++) {
      bconvArray[i][j]->checkInputPorts();
    }
  }
}

uint32_t BCONVU::getStallCycles() {
  std::vector<uint32_t> stallList;

  // for(auto& autou: autous){
  //     stallList.push_back(autou->getStallCycles());
  // }
  for (auto i = 0; i < num_high; i++) {
    for (auto j = 0; j < num_width; j++) {
      stallList.push_back(bconvArray[i][j]->getStallCycles());
    }
  }
  auto maxElement = std::max_element(stallList.begin(), stallList.end());

  return *maxElement;
}

// uint32_t BCONVU::getExeCycles() {
//   std::vector<uint32_t> stallList;

//   // for(auto& autou: autous){
//   //     stallList.push_back(autou->getStallCycles());
//   // }
//   for (auto i = 0; i < num_high; i++) {
//     for (auto j = 0; j < num_width; j++) {
//       stallList.push_back(bconvArray[i][j]->getExeCycles());
//     }
//   }
//   auto maxElement = std::max_element(stallList.begin(), stallList.end());

//   return *maxElement;
// }

NTTU::NTTU(Config *cfg) {

  Name = "NTTU Components";

  butterFly_delay = cfg->getValue("butterfly_delay");

  phase1_step1_depth = cfg->getValue("phase1_step1_depth");
  phase1_step2_depth = cfg->getValue("phase1_step2_depth");

  phase2_step1_depth = cfg->getValue("phase2_step1_depth");
  phase2_step2_depth = cfg->getValue("phase2_step2_depth");

  intraTrans_delay = cfg->getValue("intraTrans_delay");
  interTrans_delay = cfg->getValue("interTrans_delay");

  phase2_stall_delay = cfg->getValue("ntt_stall_delay");

  phase1_step1 = new Components(Name + "pahse1_step1",
                                butterFly_delay * phase1_step1_depth);
  phase1_step2 = new Components(Name + "pahse1_step2",
                                butterFly_delay * phase1_step2_depth);

  phase2_step1 = new Components(Name + "pahse2_step1",
                                butterFly_delay * phase2_step1_depth);
  phase2_step2 = new Components(Name + "pahse2_step2",
                                butterFly_delay * phase2_step2_depth);

  intraTrans_phase1 =
      new Components(Name + "pahse1_intraTrans", intraTrans_delay);
  intraTrans_phase2 =
      new Components(Name + "pahse2_intraTrans", intraTrans_delay);
  interTrans = new Components(Name + "interTrans_delay", interTrans_delay);

  if (phase2_stall_delay > 0)
    stallCom = new Components(Name + "ntt_stall", phase2_stall_delay, false);

  // Connection
  intraTrans_phase1->SetInput(phase1_step1->GetOutput());
  phase1_step2->SetInput(intraTrans_phase1->GetOutput());
  interTrans->SetInput(phase1_step2->GetOutput());
  if (phase2_stall_delay > 0) {
    stallCom->SetInput(interTrans->GetOutput());
    phase2_step1->SetInput(stallCom->GetOutput());
  } else {
    phase2_step1->SetInput(interTrans->GetOutput());
  }

  intraTrans_phase2->SetInput(phase2_step1->GetOutput());
  phase2_step2->SetInput(intraTrans_phase2->GetOutput());

  // connecte the output port of NTTU
  output = phase2_step2->GetOutput();

  //

  exeCycles = 0;
}

void NTTU::setInput(IO *in) { phase1_step1->SetInput(in); }

IO *NTTU::getOutput() { return output; }

void NTTU::getPipeline() {
  phase1_step1->getPipeline();
  phase1_step2->getPipeline();

  phase2_step1->getPipeline();
  phase2_step2->getPipeline();

  intraTrans_phase1->getPipeline();
  intraTrans_phase2->getPipeline();
  interTrans->getPipeline();

  if (phase2_stall_delay > 0)
    stallCom->getPipeline();
}

void NTTU::checkInputPorts() {
  phase1_step1->checkInputPorts();
  phase1_step2->checkInputPorts();

  phase2_step1->checkInputPorts();
  phase2_step2->checkInputPorts();

  intraTrans_phase1->checkInputPorts();
  intraTrans_phase2->checkInputPorts();
  interTrans->checkInputPorts();

  if (phase2_stall_delay > 0)
    stallCom->checkInputPorts();
}

// uint32_t NTTU::getExeCycles() {
//   std::vector<uint32_t> exeList;

//   exeList.push_back(phase1_step1->getExeCycles());
//   exeList.push_back(phase1_step2->getExeCycles());
//   exeList.push_back(phase2_step1->getExeCycles());
//   exeList.push_back(phase2_step2->getExeCycles());
//   exeList.push_back(intraTrans_phase1->getExeCycles());
//   exeList.push_back(intraTrans_phase2->getExeCycles());
//   exeList.push_back(interTrans->getExeCycles());

//   auto maxElement = std::max_element(exeList.begin(), exeList.end());

//   return *maxElement;
// }

uint32_t NTTU::getStallCycles() {
  std::vector<uint32_t> stallList;

  stallList.push_back(phase1_step1->getStallCycles());
  stallList.push_back(phase1_step2->getStallCycles());
  stallList.push_back(phase2_step1->getStallCycles());
  stallList.push_back(phase2_step2->getStallCycles());
  stallList.push_back(intraTrans_phase1->getStallCycles());
  stallList.push_back(intraTrans_phase2->getStallCycles());
  stallList.push_back(interTrans->getStallCycles());

  auto maxElement = std::max_element(stallList.begin(), stallList.end());

  return *maxElement + phase2_stall_delay;
}

void NTTU::update() {

  bool running = false;

  if (phase2_step2->runningThisCycle()) {
    phase2_step2->update();
    if (phase2_step2->getExecuted()) {
      running = true;
    }
  }

  if (intraTrans_phase2->runningThisCycle()) {
    intraTrans_phase2->update();
    if (intraTrans_phase2->getExecuted()) {
      running = true;
    }
  }

  if (phase2_step1->runningThisCycle()) {
    phase2_step1->update();
    if (phase2_step1->getExecuted()) {
      running = true;
    }
  }

  if (phase2_stall_delay > 0) {
    if (stallCom->runningThisCycle()) {
      stallCom->update();
      if (stallCom->getExecuted()) {
        running = false;
      }
    }
  }

  if (interTrans->runningThisCycle()) {
    interTrans->update();
    if (interTrans->getExecuted()) {
      running = true;
    }
  }

  if (phase1_step2->runningThisCycle()) {
    phase1_step2->update();
    if (phase1_step2->getExecuted()) {
      running = true;
    }
  }

  if (intraTrans_phase1->runningThisCycle()) {
    intraTrans_phase1->update();
    if (intraTrans_phase1->getExecuted()) {
      running = true;
    }
  }

  if (phase1_step1->runningThisCycle()) {
    phase1_step1->update();
    if (phase1_step1->getExecuted()) {
      running = true;
    }
  }

  if (running) {
    exeCycles += 1;
  }
}

HPIP::HPIP(Config *cfg) {
  Name = "HP-IP Unit"; // Proposed in Tayi

  stallCycle = 0;
  exeCycles = 0;

  VecPECount = cfg->getValue("VecPECount");
  MacCount = cfg->getValue("MacCount");
  MacDelay = cfg->getValue("MacDelay");

  for (auto v = 0; v < VecPECount; v++) {
    std::vector<Components *> MacList;
    std::vector<IO *> MacOut;
    for (auto m = 0; m < MacCount; m++) {
      Components *mac = new Components("hpip_mac_pe(" + std::to_string(v) +
                                           ")_mac(" + std::to_string(m) + ")",
                                       MacDelay);

      MacList.push_back(mac);
      MacOut.push_back(mac->GetOutput());
    }
    VecPEArray.push_back(MacList);
    VecPEArrayOutput.push_back(MacOut);
  }
}

void HPIP::setInput(std::vector<std::vector<IO *>> in) {
  for (auto v = 0; v < VecPECount; v++) {
    for (auto m = 0; m < MacCount; m++) {
      VecPEArray[v][m]->SetInput(in[v][m]);
    }
  }
}

std::vector<std::vector<IO *>> HPIP::getOutput() { return VecPEArrayOutput; }

void HPIP::getPipeline() {
  for (auto &maclist : VecPEArray) {
    for (auto &mac : maclist) {
      mac->getPipeline();
    }
  }
}

void HPIP::checkInputPorts() {
  for (auto &maclist : VecPEArray) {
    for (auto &mac : maclist) {
      mac->checkInputPorts();
    }
  }
}

// uint32_t HPIP::getExeCycles() {
//   std::vector<uint32_t> exeList;

//   for (auto &maclist : VecPEArray) {
//     for (auto &mac : maclist) {
//       exeList.push_back(mac->getExeCycles());
//     }
//   }
//   auto maxElement = std::max_element(exeList.begin(), exeList.end());

//   return *maxElement;
// }

uint32_t HPIP::getStallCycles() {
  std::vector<uint32_t> stallList;

  for (auto &maclist : VecPEArray) {
    for (auto &mac : maclist) {
      stallList.push_back(mac->getExeCycles());
    }
  }
  auto maxElement = std::max_element(stallList.begin(), stallList.end());

  return *maxElement;
}

std::string HPIP::getName() { return Name; }

void HPIP::update() {

  bool running = false;

  for (auto &maclist : VecPEArray) {
    for (auto &mac : maclist) {
      if (mac->runningThisCycle()) {
        mac->update();
        if (mac->getExecuted()) {
          running = true;
        }
      }
    }
  }
  if (running) {
    exeCycles += 1;
  }
}
