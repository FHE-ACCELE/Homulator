#!/bin/bash

maxLevel=28
alpha=28
ops="padd"
config="../../config/config_4_N15.cfg"
cluster=$1
output_dir="../../outLogs/paraA/$cluster/$ops/${maxLevel}_${alpha}"

# Create the output directory if it doesn't exist
mkdir -p "$output_dir"

for ((i=maxLevel; i>=1; i--)); do
    output_file="$output_dir/${ops}_${maxLevel}_${alpha}_${i}.log"
    nohup ../../ForgeHomulator.run "$config" "$ops" "$maxLevel" "$i" "$alpha" "$cluster" >> "$output_file" 2>&1 &
done

echo "Completed start"
