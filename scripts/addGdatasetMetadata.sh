#!/bin/bash
# Script to add document metadata from the Gcatalog file using dclient.
# Usage: ./scripts/addGdatasetMetadata.sh <Gcatalog_file>

# Check if exactly one argument (the input file) is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <Gcatalog_file> "
    exit 1
fi

INPUT_FILE="$1"

# Check if input file exists before proceeding
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: File '$INPUT_FILE' not found."
    exit 1
fi

# Initialize a counter for processing documents
COUNT=0

SERVER="./bin/dserver Gdataset/ 500"
CLIENT="./bin/dclient"

# Boot up server
$SERVER > /dev/null 2>&1 & # Discards the server output for visual cleanliness
SERVER_PID=$!
sleep 2

# Read the input file line by line, using tab ('\t') as a delimiter
# The first line (header) is skipped
while IFS=$'\t' read -r filename title year authors; do
    COUNT=$((COUNT + 1))

    # Print document metadata being processed
    echo "------------------------"
    echo "Filename: $filename"
    echo "Title: $title"
    echo "Year: $year"
    echo "Authors: $authors"

    # Call the dclient program with extracted metadata
    $CLIENT -a "$title" "$authors" $year "$filename"

done < <(tail -n +2 "$INPUT_FILE")

# Stop the server
$CLIENT -f
wait $SERVER_PID

echo -e "\nAdded metadata for $COUNT files."