#!/bin/sh

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

FORMAT_PATHS="
$ROOT_DIR/code/cgame/*.c
$ROOT_DIR/code/cgame/*.h
$ROOT_DIR/code/game/*.c
$ROOT_DIR/code/game/*.h
$ROOT_DIR/code/ui/*.c
$ROOT_DIR/code/ui/*.h
$ROOT_DIR/code/q3_ui/*.c
$ROOT_DIR/code/q3_ui/*.h
"
DRY_RUN=0

if ! command -v astyle >/dev/null 2>&1; then
  echo "Error: astyle not found" >&2
  exit 127
fi

if [ "$1" = "--dry" ]; then
  DRY_RUN=1
fi

NUMBER_FILES=0
NUMBER_ERRORS=0

astyle_job(){
  for SOME_FILE in "$@"; do

    # skip if no files
    if [ ! -f "$SOME_FILE" ]; then
      continue
    fi

    NUMBER_FILES=$((NUMBER_FILES + 1))

    # is formatted ?
    if [ "$DRY_RUN" -ne 0 ]; then
      RESULT=$(astyle --formatted --errors-to-stdout --options="$ROOT_DIR/tools/astyle.conf" --dry-run "$SOME_FILE" | grep -i formatted | wc -l)
    else
      RESULT=$(astyle --formatted --errors-to-stdout --options="$ROOT_DIR/tools/astyle.conf" "$SOME_FILE" | grep -i formatted | wc -l)
    fi

    # store and print error
    if [ "$RESULT" -ne 0 ]; then
      if [ "$DRY_RUN" -ne 0 ]; then
        echo File $SOME_FILE should be astyle formatted!
      fi
      NUMBER_ERRORS=$((NUMBER_ERRORS + 1))
    fi

  done
}


for CHECK_PATH in $FORMAT_PATHS
do
  astyle_job "$CHECK_PATH"
done


if [ "$NUMBER_FILES" -eq 0 ]; then
  echo "No matching source files found"
elif [ "$NUMBER_ERRORS" -ne 0 ] ; then
  if [ "$DRY_RUN" -ne 0 ]; then
    echo Run tools/format.sh to fix it
  else
    echo Checked files: $NUMBER_FILES
    echo Number of formatted files: $NUMBER_ERRORS
  fi
else
  echo Checked files: $NUMBER_FILES
  if [ "$DRY_RUN" -ne 0 ]; then
    echo All checked files are formatted
  else
    echo No files needed formatting
  fi
fi

exit $NUMBER_ERRORS

