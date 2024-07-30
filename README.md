# Homulator

Homulator is a cycle-accurate simulator tailored for Fully Homomorphic Encryption (FHE) accelerators. It offers detailed architectural runtime information during the execution of FHE applications, supporting extensive design space exploration across a variety of FHE parameters and hardware scales.

**Note:** This simulator is currently under review, and this version is specifically intended for use by reviewers.

# Building the Simulator

## Requirements
Ensure you have the following tools installed:
- Clang++
- make

## Build Instructions
Please note that this build process has been tested exclusively on the Linux platform.
```shell
git clone https://github.com/FHE-ACCELE/Homulator.git
cd Homulator
make -j
```

# Using the Simulator

## Testing the Simulator
To test the simulator, execute the following command with the appropriate parameters:
```shell
./Homulator.run <configfile> <operationName> <maxExecutionLevel> <currentLevel> <alpha>
```

For example, to perform a specific FHE operation:
```shell
./Homulator.run ./config/config_4.cfg hmult 45 35 15
```

This example runs the `hmult` operation with a configuration for a simulation at N=65536 (as specified in `config_4.cfg`), indicating a maximum execution level of 45, a current level of 35, and an alpha value of 15.

## Using with the benchmark
To evaluate the performance of the baseline accelerator, we provide several benchmark scripts in the [script directory](https://github.com/FHE-ACCELE/Homulator/tree/main/script). These scripts are designed to automate the testing process and provide consistent benchmarks across different configurations and operations. Use these scripts to systematically assess the simulator's performance and gather relevant metrics.

# Codebase
This section provides an overview of the primary components within the project's repository, structured to support the development and benchmark testing of Fully Homomorphic Encryption (FHE) applications. The codebase is organized into several directories, each containing specific types of files that contribute to the overall functionality of the FHE framework. Below, we detail the purpose and content of each directory, emphasizing the structure and roles of different components crucial for anyone working with or contributing to the project.

---

### Bench Test
This directory contains the source code for benchmark testing FHE applications:

- **`bench_micro24.cpp`**: Interface for benchmark testing of FHE applications as proposed in Micro-2024, designed to evaluate performance metrics.

### Config
This folder houses configuration files used for setting up simulation environments:

- **`config_4_N15.cfg`**: Configuration file for the SHARP framework, tailored for simulations with N=32768 and a 4-cluster setup.
- **`config_4.cfg`**: Similar to the above, but supports larger simulations with N=65536, also in a 4-cluster setup.

### Include
Contains all the header files for the framework, defining interfaces and basic data structures:

- **`Addr.h`**: Manages memory addressing schemes for the FHE data storage.
- **`Arch.h`**: Describes the architecture of the FHE accelerator, including its pipeline execution.
- **`Basic.h`**: Provides basic utility functions and macros used throughout the framework.
- **`Components.h`**: Describes functional components and the architectural pipeline for FHE operations.
- **`Config.h`**: Facilitates the retrieval of configurations from files.
- **`Context.h`**: Defines security contexts essential for maintaining FHE properties during operations.
- **`Driver.h`**: Top-level implementation of the FHE driver handling instruction mapping, dispatching, and other core functions.
- **`InsGen.h`**: Interface for generating FHE-specific instructions.
- **`Instruction.h`**: Base structure for defining instructions used in FHE computations.
- **`IO.h`**: Handles input/output operations, essential for interacting with external data securely.
- **`mem.h`**: Details the memory-related components critical for efficient FHE computation.
- **`Operation.h`**: Describes various FHE operations, allowing extensions as new cryptographic techniques are developed.
- **`recodeboard.h`**: Implements a recording board for debugging and monitoring the execution of FHE operations.
- **`Statistic.h`**: Interfaces for collecting and analyzing statistical data from FHE operations.

### Script
Includes scripts for running benchmarks and tests across different configurations:

- Directory content information is not provided but would typically include utility scripts for automating benchmark tests and setting up environments.

### Src
Contains the implementation files corresponding to the interfaces defined in the `include` directory. This ensures the functionality described is properly executed within the FHE framework.

# Adding Functionality
To add new functionality you would have to:

- Add any new functional units and their associated hardware and stat collection code to `components.h/cpp` and `Arch.h/.cpp`.
- Update config file `config/` with any new parameters.

## Contact

For questions or additional information regarding this project, please submit your inquiries as an issue in the repository's [Issues](https://github.com/FHE-ACCELE/Homulator/issues) section. We welcome your feedback and contributions!

