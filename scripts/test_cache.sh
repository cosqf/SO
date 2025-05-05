#!/bin/bash
# Script to test cache efficiency with different cache sizes.
# Assumes ./scripts/addGdatasetMetadata.sh <Gcatalog_file> was run first.
# Usage: ./scripts/test_cache.sh

# Checks command usage
if [ "$#" -ne 0 ]; then
    echo "Usage:   $0"
    exit 1
fi

CLIENT="./bin/dclient"
SERVER="./bin/dserver"

CACHE_SIZES="0 100 200 300" # "0 10 50 100 200 500" - Cache sizes to test with - can be changed
ACCESS_PAT="hotspot" # "random" | "hotspot" - Acess pattern options to generate IDs to lookup

DOCUMENTS=1500  # Number of documents indexed for test, used to generate IDs to lookup - can be changed
REPETITIONS=100000 # Number of operations for each test - can be changed

echo "Starting cache tests:"
echo ""

for size in $CACHE_SIZES; do
    # Temporary file for the output
    TMP_OUTPUT=$(mktemp)

    # Boot up server with specified cache size
    $SERVER Gdataset $size &> "$TMP_OUTPUT" & # Redirects server output to a temporary file for later analysis
    SERVER_PID=$!
    sleep 2
    
    # Initialize counters
    TOTAL_HITS=0
    TOTAL_MISSES=0
    TOTAL_TIME=0

    echo "Testing cache size $size with $ACCESS_PAT access"
    echo "Performing $REPETITIONS lookups..."

    for (( i=1; i<=REPETITIONS; i++ )); do
        # Generate document ID based on pattern
        case $ACCESS_PAT in
            ("random")
                doc_id=$(( RANDOM % DOCUMENTS + 1 ))
                ;;
            ("hotspot")
                # 20% of documents get 80% of accesses
                if (( RANDOM % 5 == 0 )); then
                    doc_id=$(( RANDOM % (DOCUMENTS/5) + 1 ))
                else
                    doc_id=$(( RANDOM % (DOCUMENTS*4/5) + DOCUMENTS/5 + 1 ))
                fi
                ;;
        esac

        start_time=$(date +%s%N)
        $CLIENT -c $doc_id > /dev/null 2>&1 # Discards the client output for visual cleanliness
        end_time=$(date +%s%N)

        elapsed=$(( (end_time - start_time) / 1000000 ))
        TOTAL_TIME=$(( TOTAL_TIME + elapsed ))
    done

    # Analyze cache hits/misses
    TOTAL_HITS=$(grep -c "Cache hit" "$TMP_OUTPUT")
    TOTAL_MISSES=$(grep -c "Cache miss" "$TMP_OUTPUT")

    # Clean up the temp file
    rm -f "$TMP_OUTPUT"

    # Results
    avg_time=$(echo "scale=1; $TOTAL_TIME / $REPETITIONS" | bc)
    total_time=$(echo "scale=1; $TOTAL_TIME / 1000" | bc)
    hit_ratio=$(echo "scale=1; $TOTAL_HITS * 100 / $REPETITIONS" | bc)
    miss_ratio=$(echo "scale=1; $TOTAL_MISSES * 100 / $REPETITIONS" | bc)

    echo -e "\nResults for cache size $size:"
    echo "---------------------"
    echo "Total operations:    $REPETITIONS"
    echo "Cache hits:          $TOTAL_HITS ($hit_ratio%)"
    echo "Cache misses:        $TOTAL_MISSES ($miss_ratio%)"
    echo "Average lookup time: $avg_time ms"
    echo "Total test duration: $total_time s"
    
    # Stop the server
    $CLIENT -f
    wait $SERVER_PID
    
    echo ""
done

echo "Tests finished."