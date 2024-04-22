#!/bin/bash

maxLevel=28
alpha=28
ops="hmult"
config="../../config/config_4.cfg"
cluster=$1
output_dir="../../outLogs/motivation/$cluster/$ops/${maxLevel}_${alpha}"

# Create the output directory if it doesn't exist
mkdir -p "$output_dir"

for ((i=maxLevel; i>=2; i--)); do
    output_file="$output_dir/${ops}_${maxLevel}_${alpha}_${i}.log"
    nohup ../../ForgeHomulator.run "$config" "$ops" "$maxLevel" "$i" "$alpha" "$cluster" >> "$output_file" 2>&1 &
done

echo "Completed start"
