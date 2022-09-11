#!/bin/bash

set -e
set -u

if [ ! -e scripts/run_clang_format.sh ]; then
  echo "Must run in repository root directory."
  exit 1
fi

clang-format 
