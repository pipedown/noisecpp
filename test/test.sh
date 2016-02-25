#!/bin/bash
# ./test.sh ../build/index ../build/query

if [ $# -ne 2 ]; then
	echo "Usage: $0 index-binary query-binary"
    exit 1
fi

$1 -cd testindex < ../test/testdata.json
$2 testindex < ../test/testqueries.txt
