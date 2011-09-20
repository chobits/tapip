#!/bin/bash
# script for memory leak test
# -v verbose model
sudo valgrind -q --leak-check=full --show-reachable=yes ./tapip
