#!/bin/bash
# apply_patches.sh — Wrapper that delegates to scripts/apply_patches.sh
#
# Usage:
#   cd ~/chromium/src
#   bash /path/to/normal-browser/apply_patches.sh
#
# Or:
#   bash /path/to/normal-browser/apply_patches.sh /path/to/chromium/src

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CHROMIUM_ROOT="${1:-$(pwd)}"

echo "============================================="
echo " Normal Browser — Full Patch Pipeline"
echo "============================================="
echo ""

# Step 1: Copy all new files
echo ">>> Running apply_patches.sh..."
bash "$SCRIPT_DIR/scripts/apply_patches.sh" "$CHROMIUM_ROOT"

echo ""

# Step 2: Apply hook points to existing Chromium files
echo ">>> Running apply_hook_points.sh..."
bash "$SCRIPT_DIR/scripts/apply_hook_points.sh" "$CHROMIUM_ROOT"

echo ""

# Step 3: Report BUILD.gn changes needed
echo ">>> Running apply_build_gn_patches.sh..."
bash "$SCRIPT_DIR/scripts/apply_build_gn_patches.sh" "$CHROMIUM_ROOT"

echo ""
echo "============================================="
echo " All patches applied!"
echo ""
echo " NEXT STEPS:"
echo "   1. Apply BUILD.gn edits listed above manually"
echo "   2. gn gen out/NormalBrowser --args='target_os=\"android\" target_cpu=\"arm64\" ...'"
echo "   3. autoninja -C out/NormalBrowser chrome_public_apk"
echo "============================================="
