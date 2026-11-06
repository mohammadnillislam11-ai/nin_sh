#!/bin/bash
set -e

echo "=== Packaging gemini-cli.sh In-Memory Self-Extracting Runner ==="

BIN_PATH="libs/arm64-v8a/gemini-cli"
OUT_PATH="libs/arm64-v8a/gemini-cli.sh"

if [ ! -f "$BIN_PATH" ]; then
    echo "[-] Binary $BIN_PATH not found!"
    exit 1
fi

cat << 'EOF' > "$OUT_PATH"
#!/bin/sh
# ==============================================================================
# NINMODS Official In-Memory External Engine (gemini-cli.sh)
# ==============================================================================

if [ "$(id -u)" -ne 0 ]; then
    echo "[*] Requesting superuser privileges..."
    exec su -c "sh \"$0\" \"$@\""
fi

RAM_DIR=""
if [ -d "/dev/shm" ] && [ -w "/dev/shm" ]; then
    RAM_DIR="/dev/shm"
elif [ -d "/dev" ] && [ -w "/dev" ]; then
    RAM_DIR="/dev"
else
    RAM_DIR="/data/local/tmp"
fi

RUN_ID="$(date +%s)_$RANDOM"
MEM_BIN="$RAM_DIR/.gemini_proc_$RUN_ID"

# Extract embedded ARM64 binary payload from footer
PAYLOAD_LINE=$(awk '/^__PAYLOAD_BEGIN__/ {print NR + 1; exit 0; }' "$0")
tail -n+$PAYLOAD_LINE "$0" | base64 -d > "$MEM_BIN" 2>/dev/null
chmod 755 "$MEM_BIN" 2>/dev/null

# Clean up temporary disk node on exit
trap 'rm -f "$MEM_BIN" 2>/dev/null' EXIT INT TERM HUP

# Execute directly into memory, maintaining interactive stdin/stdout
"$MEM_BIN" "$@"
EXIT_CODE=$?

rm -f "$MEM_BIN" 2>/dev/null
exit $EXIT_CODE
__PAYLOAD_BEGIN__
EOF

base64 "$BIN_PATH" >> "$OUT_PATH"
chmod 755 "$OUT_PATH"

echo "[+] gemini-cli.sh created successfully: $(ls -lh "$OUT_PATH")"
