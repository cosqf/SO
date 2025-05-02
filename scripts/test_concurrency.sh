#!/bin/bash
# Script to test the concurrency search improvements.
# Assumes ./addGdatasetMetadata.sh Gcatalog.tsv was run first.
# Usage: ./tests/test_concurrency.sh

# Checks command usage
if [ "$#" -ne 0 ]; then
    echo "Usage:   $0"
    exit 1
fi

CLIENT="./bin/dclient"
SERVER="./bin/dserver Gdataset 500"

KEYWORD="law"
NUM_PROCS="1 2 4 6 8" # "1 2 4 6 8"
REPETITIONS=10 # Number of operations for test

echo "Starting concurrency tests with keyword $KEYWORD:"

# Boot up server
$SERVER > /dev/null 2>&1 &
SERVER_PID=$!
sleep 2

for nump in $NUM_PROCS; do    
    TOTAL_TIME=0

    echo -e "\nTesting with $nump process(es)"
    echo "Performing $REPETITIONS searches..."
    
    for (( i=1; i<=$REPETITIONS; i++ )); do
        start_time=$(date +%s%N)
        $CLIENT -s "$KEYWORD" $nump > /dev/null 2>&1
        end_time=$(date +%s%N)
        
        elapsed=$(( (end_time - start_time) / 1000000 ))
        TOTAL_TIME=$(( TOTAL_TIME + elapsed ))
        
        echo "  Execution $i duration: $elapsed ms"
    done

    # Calculate results
    avg_time=$(echo "scale=1; $TOTAL_TIME / $REPETITIONS" | bc)
    total_time=$(echo "scale=1; $TOTAL_TIME / 1000" | bc)

    echo -e "\nResults for $nump process(es):"
    echo "---------------------"
    echo "Total operations:    $REPETITIONS"
    echo "Average lookup time: $avg_time ms"
    echo "Total test duration: $total_time s"
    echo ""

done

# Stop the server
$CLIENT -f
wait $SERVER_PID

echo ""
echo "Tests finished."