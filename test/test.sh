#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Usage: $0 index-bin query-bin"
    exit 1
fi

$1 -cd testindex < ./testdata.json
$2 testindex < ./testqueries.txt
