#!/bin/bash
# Quick script to switch from CSV output to clean summary statistics

echo "Switching to clean summary statistics output..."

# 1. Update CMakeLists.txt to include analysis_task.c
echo "1. Adding analysis_task.c to CMakeLists.txt..."

# Check if already added
if grep -q "analysis_task.c" CMakeLists.txt; then
    echo "   Already added!"
else
    # Find the line with task_trace.c and add analysis_task.c after it
    sed -i '/src\/task_trace.c/a\    src\/analysis_task.c' CMakeLists.txt
    echo "   Added!"
fi

# 2. Update main.c to use task_analysis_summary
echo "2. Updating main.c to use task_analysis_summary..."
sed -i 's/task_trace_dump/task_analysis_summary/g' src/main.c
sed -i 's/"Dump"/"Analysis"/g' src/main.c
echo "   Updated!"

# 3. Rebuild
echo "3. Rebuilding..."
cd build
make -j4

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Next steps:"
    echo "  1. Flash build/tetris_freertos.uf2 to your Pico"
    echo "  2. Open PuTTY and connect to USB serial"
    echo "  3. Wait 10 seconds for clean statistics output"
    echo ""
else
    echo ""
    echo "✗ Build failed! Check errors above."
    echo ""
    exit 1
fi
