#!/bin/bash
# Script to test the concurrency search improvements.
# Usage: ./scripts/test_concurrency.sh
# Will reset the server and any indexed files

# Checks command usage
if [ "$#" -ne 0 ]; then
    echo "Usage:   $0"
    exit 1
fi

CLIENT="./bin/dclient"
SERVER="./bin/dserver Gdataset 500"

KEYWORD="law"           # Keyword - can be changed
NUM_PROCS="1 2 3 4 5 6 7 8 9 10"   # Number of processes to test with - can be changed
REPETITIONS=100          # Number of operations for each test - can be changed

# Prepares server for testing - indexes files for test
make clean > /dev/null 2>&1
make > /dev/null 2>&1
./scripts/addGdatasetMetadata.sh Gcatalog.tsv > /dev/null 2>&1

echo "Starting concurrency tests with keyword $KEYWORD:"

# Boot up server
$SERVER > /dev/null 2>&1 & # Discards the server output for visual cleanliness
SERVER_PID=$!
sleep 2

for nump in $NUM_PROCS; do    
    TOTAL_TIME=0

    echo -e "\nTesting with $nump process(es)"
    echo "Performing $REPETITIONS searches..."
    
    for (( i=1; i<=$REPETITIONS; i++ )); do
        start_time=$(date +%s%N)
        $CLIENT -s "$KEYWORD" $nump > /dev/null 2>&1 # Discards the client output for visual cleanliness
        end_time=$(date +%s%N)
        
        elapsed=$(( (end_time - start_time) / 1000000 ))
        TOTAL_TIME=$(( TOTAL_TIME + elapsed ))
    done

    # Results
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

# Resets server
make clean > /dev/null 2>&1
make > /dev/null 2>&1

echo ""
echo "Tests finished."