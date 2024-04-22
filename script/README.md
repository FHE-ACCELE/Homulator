
# Benchmark

The Benchmark directory houses a collection of scripts designed to test various Fully Homomorphic Encryption (FHE) operations under different parameter sets. These benchmarks are crucial for evaluating the performance and efficiency of FHE operations on the accelerator.

## Available Operations
The benchmark scripts cover the following FHE operations:
- **HMULT**: Homomorphic multiplication
- **HROTATE**: Homomorphic rotation
- **HADD**: Homomorphic addition
- **PADD**: Plaintext addition
- **PMULT**: Plaintext multiplication

## Parameter Sets
Each operation can be tested across various parameter configurations to assess performance under different conditions. The scripts allow adjustments in the following parameters:

| Parameter Set | **N**  | **L** | **α** | **d_num** |
| ------------- | ------ | ----- | ----- | --------- |
| A             | 2^15   | 28    | 28    | 1         |
| B             | 2^16   | 45    | 15    | 3         |
| C             | 2^16   | 24    | 6     | 4         |
| D             | 2^16   | 26    | 9     | 3         |


Here's a nicely formatted Markdown version of your instructions for testing benchmarks using shell scripts:

---

## Testing for Benchmarks

To test the benchmarks, navigate to the appropriate directory and run the provided scripts. You can execute all operations or test a specific operation on a defined number of clusters.

### Running All Operations
To run all available operations, follow these steps:
```shell
cd paraA
bash run.sh # This script runs all operations
```
Outputs from all operation will also be stored in the `../outLogs` directory.

### Testing a Specific Operation
To specifically test the HADD operation on 4 clusters:
```shell
cd paraA
bash micro24_A_hadd.sh 4 # Testing HADD operation on 4 clusters
```
Outputs from this operation will also be stored in the `../outLogs` directory.



## Contact

For questions or additional information regarding this benchmarks, please submit your inquiries as an issue in the repository's [Issues](<GitHub_Issue_Link>) section. We welcome your feedback and contributions!

