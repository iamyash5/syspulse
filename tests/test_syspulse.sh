#!/bin/bash
# test_syspulse.sh
# Integration test for SysPulse: starts the daemon, exercises the CLI,
# and checks that status/threshold commands behave as expected.
#
# Run from the repo root:  bash tests/test_syspulse.sh

set -e

DAEMON_BIN="daemon/syspulsed"
CLI_BIN="cli/syspulsectl"
PASS=0
FAIL=0

check() {
    local desc="$1"
    local result="$2"
    if [ "$result" -eq 0 ]; then
        echo "  [PASS] $desc"
        PASS=$((PASS+1))
    else
        echo "  [FAIL] $desc"
        FAIL=$((FAIL+1))
    fi
}

echo "=== SysPulse Integration Test ==="

# --- Build check ---
echo "[1] Checking binaries exist..."
if [ ! -x "$DAEMON_BIN" ]; then
    echo "Daemon binary not found at $DAEMON_BIN. Build it first:"
    echo "  cd daemon && g++ -std=c++17 -o syspulsed syspulsed.cpp -lpthread"
    exit 1
fi
if [ ! -x "$CLI_BIN" ]; then
    echo "CLI binary not found at $CLI_BIN. Build it first:"
    echo "  cd cli && g++ -std=c++17 -o syspulsectl syspulsectl.cpp"
    exit 1
fi
check "Binaries present" 0

# --- Start daemon ---
echo "[2] Starting daemon..."
rm -f /tmp/syspulse.sock
"$DAEMON_BIN" > /tmp/syspulsed_test.log 2>&1 &
DAEMON_PID=$!
sleep 2 # allow daemon to bind socket and take first CPU sample

if ! kill -0 "$DAEMON_PID" 2>/dev/null; then
    echo "Daemon failed to start. Log:"
    cat /tmp/syspulsed_test.log
    exit 1
fi
check "Daemon process running" 0

# --- Socket file exists ---
echo "[3] Checking socket file..."
if [ -S /tmp/syspulse.sock ]; then
    check "Socket file created at /tmp/syspulse.sock" 0
else
    check "Socket file created at /tmp/syspulse.sock" 1
fi

# --- STATUS command ---
echo "[4] Testing STATUS command..."
STATUS_OUTPUT=$("$CLI_BIN" status)
echo "  Output: $STATUS_OUTPUT"
if echo "$STATUS_OUTPUT" | grep -q "cpu=" && echo "$STATUS_OUTPUT" | grep -q "mem="; then
    check "STATUS returns cpu and mem fields" 0
else
    check "STATUS returns cpu and mem fields" 1
fi

# --- SET_THRESHOLD command ---
echo "[5] Testing SET_THRESHOLD command..."
SET_OUTPUT=$("$CLI_BIN" set-threshold 50)
echo "  Output: $SET_OUTPUT"
if echo "$SET_OUTPUT" | grep -q "OK"; then
    check "SET_THRESHOLD accepted" 0
else
    check "SET_THRESHOLD accepted" 1
fi

STATUS_AFTER=$("$CLI_BIN" status)
echo "  Status after update: $STATUS_AFTER"
if echo "$STATUS_AFTER" | grep -q "threshold=50"; then
    check "Threshold actually updated to 50" 0
else
    check "Threshold actually updated to 50" 1
fi

# --- Load test: generate CPU load and confirm alert flips ---
echo "[6] Testing alert under CPU load (this takes a few seconds)..."
"$CLI_BIN" set-threshold 1 > /dev/null   # very low threshold, easy to cross

yes > /dev/null &
LOAD_PID=$!
sleep 2

LOAD_STATUS=$("$CLI_BIN" status)
echo "  Status under load: $LOAD_STATUS"

kill "$LOAD_PID" 2>/dev/null || true
wait "$LOAD_PID" 2>/dev/null || true

if echo "$LOAD_STATUS" | grep -q "alert=1"; then
    check "Alert becomes active when CPU exceeds threshold" 0
else
    check "Alert becomes active when CPU exceeds threshold" 1
fi

# --- Daemon shutdown ---
echo "[7] Testing clean shutdown..."
kill -SIGINT "$DAEMON_PID"
sleep 1

if kill -0 "$DAEMON_PID" 2>/dev/null; then
    check "Daemon exits on SIGINT" 1
    kill -9 "$DAEMON_PID" 2>/dev/null || true
else
    check "Daemon exits on SIGINT" 0
fi

if [ ! -S /tmp/syspulse.sock ]; then
    check "Socket file removed after shutdown" 0
else
    check "Socket file removed after shutdown" 1
fi

# --- Summary ---
echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
if [ "$FAIL" -eq 0 ]; then
    exit 0
else
    exit 1
fi