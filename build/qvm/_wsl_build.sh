#!/bin/bash
set -e
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:${PATH:-}"

# discover q3lcc if not in PATH
if ! command -v q3lcc >/dev/null 2>&1; then
  FOUND=$(find /opt /usr/local /root /home /mnt/c/git/OSP2-PBE -name q3lcc -type f 2>/dev/null | head -1)
  if [ -n "$FOUND" ]; then
    export PATH="$(dirname "$FOUND"):$PATH"
    echo "Using q3lcc at $FOUND"
  else
    echo "q3lcc not found"
    exit 127
  fi
fi

echo "q3lcc=$(command -v q3lcc)"
cd /mnt/c/git/OSP2-PBE/build/qvm

# smoke-test the two previously-failing units
mkdir -p vm/cgame
q3lcc -DQ3_VM -S -Wf-g -I../../code/game -DCGAME -I../../code/cgame -I../../code/cgame/chatWindow -I../../code/cgame/superhud -o vm/cgame/bg_slidemove.asm ../../code/game/bg_slidemove.c
echo "bg_slidemove OK"
q3lcc -DQ3_VM -S -Wf-g -I../../code/game -DCGAME -I../../code/cgame -I../../code/cgame/chatWindow -I../../code/cgame/superhud -o vm/cgame/bg_pmove.asm ../../code/game/bg_pmove.c
echo "bg_pmove OK"

# full rebuild like the user does
if command -v makecopy >/dev/null 2>&1; then
  make clean && makecopy
else
  make clean && make
fi
echo "BUILD_DONE"
