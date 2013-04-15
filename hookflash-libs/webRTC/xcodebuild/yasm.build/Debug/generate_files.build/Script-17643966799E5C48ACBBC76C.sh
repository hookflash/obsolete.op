#!/bin/sh
JOB_COUNT="$(/usr/sbin/sysctl -n hw.ncpu)"
if [ "${JOB_COUNT}" -gt 4 ]; then
  JOB_COUNT=4
fi
exec "${DEVELOPER_BIN_DIR}/make" -f "${PROJECT_FILE_PATH}/generate_files_generate_gperf.make" -j "${JOB_COUNT}"
exit 1

