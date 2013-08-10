#!/bin/bash

TIME_FORMAT="%e seconds"

FILENAME=`tempfile`
BENCH_DIR=`dirname $0`

EXPECTED_OUTPUT="${BENCH_DIR}/mandelbrot.out"
BF_EXECUTABLE="${BENCH_DIR}/../bf"
BENCH_SOURCE="${BENCH_DIR}/mandelbrot.bf"

/usr/bin/time -f "${TIME_FORMAT}" ${BF_EXECUTABLE} ${BENCH_SOURCE} > ${FILENAME}
diff -u ${FILENAME} ${EXPECTED_OUTPUT} > /dev/null

if [[ "$?" != "0" ]]; then
    echo "incorrect output (see ${FILENAME})"
    exit 1
fi

exit 0
