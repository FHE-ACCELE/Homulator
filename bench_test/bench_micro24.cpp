#include "Basic.h"
#include "Operation.h"
#include "Arch.h"

int main(int argc, char *argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <path>" << std::endl;
        return 1;
    }

    std::string path = argv[1];
    std::string ops = argv[2];

    // Create a configuration object (replace this with actual configuration
    // values)
    Config *config = new Config(path);
   

    uint32_t maxlevel = std::atoi(argv[3]);
    uint32_t currentlevel = std::atoi(argv[4]);
    uint32_t alpha = std::atoi(argv[5]);

    if (argc > 6){
        config->setValue("cluster", std::atoi(argv[6]));
    }

    Arch* arch = new Arch(config);
    
    if (ops == "hmult") {
        HMULT* hmult = new HMULT("test_hmult", maxlevel, currentlevel, alpha, config, arch);
        hmult->simulate();
    }
    else if (ops == "hrotate") {
        HROTATE* hrotate = new HROTATE("test_hrotate", maxlevel, currentlevel, alpha, config, arch);
        hrotate->simulate();
    }
    else if (ops == "hadd") {
        HADD* hadd = new HADD("test_hadd", maxlevel, currentlevel, alpha, config, arch);
        hadd->simulate();
    }
    else if (ops == "pmult") {
        PMULT* pmult = new PMULT("test_pmult", maxlevel, currentlevel, alpha, config, arch);
        pmult->simulate();
    }
    else if (ops == "padd") {
        PADD* padd = new PADD("test_ADD", maxlevel, currentlevel, alpha, config, arch);
        padd->simulate();
    }
    else {
        std::cout << "Error operation requirement, please double confirm!\n";
    }
}



