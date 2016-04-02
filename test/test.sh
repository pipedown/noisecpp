#!/bin/bash
# ./test.sh ../build/index ../build/query
# ./test.sh ../build/index ../build/query testdata.json testqueries.txt

if [ $# -eq 2 ]; then
	DATA=../test/testdata.json
	QUERIES=../test/testqueries.txt
elif [ $# -eq 4 ]; then
	DATA=$3
	QUERIES=$4

else
	echo "Usage: $0 index-binary query-binary"
	echo "or"
	echo "Usage: $0 index-binary query-binary data.json queries.txt"
	exit
fi

OUT=$QUERIES-out.txt
$1 -cd testindex < $DATA
$2 -t testindex < $QUERIES > $OUT

DIFFOUT=$(diff $QUERIES $OUT)
if [[ $DIFFOUT ]]; then
	echo ==TEST FAILED==
	echo $DIFFOUT
fi
