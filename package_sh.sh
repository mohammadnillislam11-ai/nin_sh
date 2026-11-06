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

# Extract embedded ARM64 binary payload from footer without awk (pure Android toybox standard)
sed '1,/^__PAYLOAD_BEGIN__$/d' "$0" 2>/dev/null | base64 -d > "$MEM_BIN" 2>/dev/null

if [ ! -s "$MEM_BIN" ]; then
    tail -n +42 "$0" 2>/dev/null | base64 -d > "$MEM_BIN" 2>/dev/null
fi
if [ ! -s "$MEM_BIN" ]; then
    toybox sed '1,/^__PAYLOAD_BEGIN__$/d' "$0" 2>/dev/null | toybox base64 -d > "$MEM_BIN" 2>/dev/null
fi

if [ ! -s "$MEM_BIN" ]; then
    echo "[-] Failed to unpack in-memory payload."
    exit 1
fi

chmod 755 "$MEM_BIN" 2>/dev/null

# Discover APEX i18n & runtime paths across all Android 10-15 versions
APEX_DIRS=""
for d in /apex/com.android.i18n/lib64 /apex/com.android.runtime/lib64 /apex/com.android.art/lib64 /system/apex/com.android.i18n/lib64 /apex/com.android.i18n/lib /apex/com.android.runtime/lib /apex/com.android.art/lib; do
    if [ -d "$d" ]; then
        APEX_DIRS="$APEX_DIRS:$d"
    fi
done

# Ensure libicu.so is resolved if expected by system libraries
if [ ! -f "/system/lib64/libicu.so" ] && [ ! -f "/apex/com.android.i18n/lib64/libicu.so" ]; then
    ICU_SRC=$(ls /apex/com.android.i18n/lib64/libicuuc*.so /system/lib64/libicuuc*.so /apex/com.android.i18n/lib64/libicu*.so 2>/dev/null | head -n 1)
    if [ -n "$ICU_SRC" ] && [ -f "$ICU_SRC" ]; then
        ln -sf "$ICU_SRC" "$RAM_DIR/libicu.so" 2>/dev/null
        APEX_DIRS="$RAM_DIR:$APEX_DIRS"
    fi
fi

if [ -n "$APEX_DIRS" ]; then
    export LD_LIBRARY_PATH="$APEX_DIRS:/system/lib64:/vendor/lib64:/system/lib:/vendor/lib:$LD_LIBRARY_PATH"
fi

# Clean up temporary disk node on exit
trap 'rm -f "$MEM_BIN" "$RAM_DIR/libicu.so" 2>/dev/null' EXIT INT TERM HUP

# Execute directly into memory, maintaining interactive stdin/stdout
exec "$MEM_BIN" "$@"
__PAYLOAD_BEGIN__
EOF

base64 "$BIN_PATH" >> "$OUT_PATH"
chmod 755 "$OUT_PATH"

echo "[+] gemini-cli.sh created successfully: $(ls -lh "$OUT_PATH")"

# Compress into official distribution package gemini-cli.zip
ZIP_PATH="libs/arm64-v8a/gemini-cli.zip"
if command -v zip >/dev/null 2>&1; then
    (cd libs/arm64-v8a && zip -9 gemini-cli.zip gemini-cli.sh)
    echo "[+] gemini-cli.zip created successfully: $(ls -lh "$ZIP_PATH")"
fi
