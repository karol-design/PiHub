#!/bin/sh

##### ASCII Escape codes for changing colour of printed text #####

# Black        0;30     Dark Gray     1;30
# Red          0;31     Light Red     1;31
# Green        0;32     Light Green   1;32
# Brown/Orange 0;33     Yellow        1;33
# Blue         0;34     Light Blue    1;34
# Purple       0;35     Light Purple  1;35
# Cyan         0;36     Light Cyan    1;36
# Light Gray   0;37     White         1;37

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
GRAY='\033[0;37m'
NC='\033[0m' # No Color
BOLD='\033[1m' # Bold

# Log file
LOG_FILE="build/build_script.log"

# Function to print messages with color
print_msg() {
    local color="$1"
    local msg="$2"
    echo "${color}${msg}${NC}"
}

# Function to handle errors
handle_error() {
    print_msg "$RED" "Error: $1"
    exit 1
}

# Build script for PiHub
print_msg "$CYAN" "build.sh: Building PiHub [Config: Debug build; Unit Tests ON;]"

print_msg "$CYAN" "build.sh: Using CMake to update build files and compile unit tests and the app"
rm -rf build/* || handle_error "Failed to clean build directory"
cmake -B build -DUT=ON -DCMAKE_BUILD_TYPE=Debug >"$LOG_FILE" 2>&1 || handle_error "CMake configuration failed"

# Build the project and check for success or failure
if cmake --build build >>"$LOG_FILE" 2>&1; then
    print_msg "$GREEN" "build.sh: Successfully built piHub and test_piHub!"
else
    handle_error "Build failed! Check the log file for details: $LOG_FILE"
fi

# Execute all unit tests and capture the output
test_output=$(./build/tests/test_piHub 2>&1)
echo "$test_output" >>"$LOG_FILE"

# Check if all tests passed
if echo "$test_output" | grep -q '\[  FAILED  \]'; then
    print_msg "$RED" "build.sh: Some Unit Tests failed!"
    print_msg "$NC" "build.sh: Details of failed tests:"
    echo "$test_output" | grep -A 0 '\[  FAILED  \]' | while read -r line; do
        print_msg "$NC" "$line"
    done
    exit 1
else
    print_msg "$GREEN" "build.sh: All Unit Tests passed!"
    echo "$test_output" | grep -E '.+: Running [0-9]+ test\(s\)\.' | grep -v '\[  PASSED  \]' | while read -r line; do
        print_msg "$GRAY" "${line#\[==========\] }"
    done
fi

# Run the app if --run argument is provided
if [ "$1" = "--run" ]; then
    print_msg "$CYAN" "build.sh: Running PiHub..."
    print_msg "" ""
    ./build/src/piHub || handle_error "Failed to run PiHub"
fi