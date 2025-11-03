#!/bin/bash
cd /Users/jinwoowang/project/gogo-boot/mystation
echo "Running tests..."
pio test -e native -v 2>&1 | tee test_output.log
echo ""
echo "=== Test Summary ==="
grep -E "(PASS|FAIL)" test_output.log | tail -30
echo ""
echo "=== Failed Tests ==="
grep "FAIL" test_output.log

