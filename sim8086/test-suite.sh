#!/bin/bash

# Runs tests on sim8086 using listings from the Performance-Aware Programming course
# on Computer, Enhance (https://github.com/cmuratori/computer_enhance).

# Requirements:
# - 'bin/sim8086' (relative to this sctipt)
# - All Performance-Aware Programming listings included in the test suite placed in 'listings/' (relative to this sctipt)
# - 'nasm' accessible from PATH variable
# - All platform metrics and timings printouts be disabled in the sim8086 output

PASSED=0
FAILED=0

echo
echo "Running sim8068 test suite:"
echo "---------------------------"

# Test function
TestListing() {
    LISTING=$1
    echo "Testing '$LISTING':"
    echo
    bin/sim8086_debug "$LISTING" > output.asm
    nasm output.asm -o output
    if cmp -s "$LISTING" output; then
        ((PASSED++))
        echo "PASSED"
    else
        ((FAILED++))
        echo
        echo "                   FAILED"
    fi
    echo "------------------------"
}

# Run tests
TestListing listings/listing_0037_single_register_mov
TestListing listings/listing_0038_many_register_mov
TestListing listings/listing_0039_more_movs
TestListing listings/listing_0040_challenge_movs
TestListing listings/listing_0041_add_sub_cmp_jnz
TestListing listings/listing_0043_immediate_movs
TestListing listings/listing_0044_register_movs
TestListing listings/listing_0046_add_sub_cmp
TestListing listings/listing_0048_ip_register
TestListing listings/listing_0049_conditional_jumps
TestListing listings/listing_0051_memory_mov
TestListing listings/listing_0052_memory_add_loop
TestListing listings/listing_0053_add_loop_challenge
TestListing listings/listing_0054_draw_rectangle
TestListing listings/listing_0056_estimating_cycles

# Output results
echo
echo "PASSED: $PASSED"
echo "FAILED: $FAILED"

# Clean up
rm -f output.asm output
