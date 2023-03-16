#!/bin/sh

set -e
set -x

TEST_CMD="${1:-"./test"}"
MDIR="${2:-"`dirname ${0}`/.."}"
TDDIR="${MDIR}/test_data"

${TEST_CMD} ${TDDIR}/test.g722 test.raw.out
${TEST_CMD} --sln16k ${TDDIR}/test.g722 test.raw.16k.out
${TEST_CMD} --enc --sln16k --bend ${TDDIR}/pcminb.dat pcminb.g722.out
${TEST_CMD} --sln16k --bend pcminb.g722.out pcminb.raw.16k.out
${TEST_CMD} --enc test.raw.out test.g722.out
${TEST_CMD} --sln16k ${TDDIR}/fullscale.g722 fullscale.raw.out
sha256sum test.raw.out test.raw.16k.out pcminb.g722.out pcminb.raw.16k.out \
  test.g722.out fullscale.raw.out | diff ${TDDIR}/test.checksum -
