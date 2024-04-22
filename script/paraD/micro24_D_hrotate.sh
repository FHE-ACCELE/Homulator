#!/bin/bash

maxLevel=26
alpha=9
ops="hrotate"
config="../../config/config_4.cfg"
cluster=$1
output_dir="../../outLogs/paraD/$cluster/$ops/${maxLevel}_${alpha}"

# Create the output directory if it doesn't exist
mkdir -p "$output_dir"

for ((i=maxLevel; i>=1; i--)); do
    output_file="$output_dir/${ops}_${maxLevel}_${alpha}_${i}.log"
    nohup ../../ForgeHomulator.run "$config" "$ops" "$maxLevel" "$i" "$alpha" "$cluster" >> "$output_file" 2>&1 &
done

echo "Completed start"
