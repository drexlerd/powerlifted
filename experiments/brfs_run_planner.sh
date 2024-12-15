#!/bin/bash

set -euo pipefail

if [[ $# != 3 ]]; then
    echo "usage: $(basename "$0") <planner_file> <domain_file> <problem_file>" 1>&2
    exit 2
fi

planner_exe=$1
domain_file=$PWD/$2
problem_file=$PWD/$3  # Assuming full path or relative to current dir

plan_file="plan"

# Check if the plan file already exists and prompt for removal
if [ -f "$plan_file" ]; then
    echo "Error: remove $plan_file" 1>&2
    exit 2
fi

# Ensure that strings like "CPU time limit exceeded" and "Killed" are in English.
export LANG=C

# Run planner
"$planner_exe" "-i" "$problem_file" "-s" "bfs"

# Run VAL
echo -e "\nRun VAL\n"

# After running the planner, check if the plan file was created
if [ -f "$plan_file" ]; then
    echo "Found plan file."

    # Check for 'Validate' or 'validate'
    if command -v Validate &>/dev/null; then
        val_binary="Validate"
    elif command -v validate &>/dev/null; then
        val_binary="validate"
    else
        echo "Error: Neither 'Validate' nor 'validate' command is found."
        exit 1
    fi

    "$val_binary" -v "$domain_file" "$problem_file" "$plan_file"
else
    echo "No plan file found."
    exit 99
fi
