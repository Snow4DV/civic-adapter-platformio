#!/bin/bash

# Check if argument is provided
if [ -z "$1" ]
  then
    echo "Please provide an argument (MASTER or SLAVE)"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p output

rm  output/*.hex

# Loop through directories in .pio/build
for dir in .pio/build/*/; do
    # Get board name from directory name
    board_name=$(basename "$dir")

    # Check if firmware.hex file exists
    if [ -f "$dir/firmware.hex" ]; then
        # Rename and move file based on argument provided
        if [ "$1" == "MASTER" ]; then
            mv "$dir/firmware.hex" "output/MASTER-${board_name}.hex"
            echo "Renamed and moved MASTER-${board_name}.hex"
        elif [ "$1" == "SLAVE" ]; then
            mv "$dir/firmware.hex" "output/SLAVE-${board_name}.hex"
            echo "Renamed and moved SLAVE-${board_name}.hex"
        else
            echo "Invalid argument. Please provide MASTER or SLAVE."
            exit 1
        fi
    else
        echo "firmware.hex not found in $board_name"
    fi
done


if [ -f "output/MASTER-ARDUINO_UNO.hex" ]; then
    cp "output/MASTER-ARDUINO_UNO.hex" "output/MASTER.hex"
    echo "Created MASTER.hex"
fi

if [ -f "output/SLAVE-ARDUINO_UNO.hex" ]; then
    cp "output/SLAVE-ARDUINO_UNO.hex" "output/SLAVE.hex"
    echo "Created SLAVE.hex"
fi
