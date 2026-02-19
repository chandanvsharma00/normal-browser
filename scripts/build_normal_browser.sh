#!/bin/bash
# Copyright 2024 Normal Browser Authors. All rights reserved.
# build_normal_browser.sh — Full build script for Normal Browser Android APK.
#
# Prerequisites:
#   - Ubuntu 22.04 or 24.04 (x86_64)
#   - At least 16GB RAM (32GB recommended)
#   - At least 130GB free disk (250GB recommended)
#   - depot_tools installed and in PATH
#   - Android SDK + NDK configured
#
# Usage:
#   ./build_normal_browser.sh [setup|patch|build|package|all]
#
#   setup   — Fetch Chromium source + install deps (first time only, ~50GB)
#   patch   — Apply Normal Browser patches to source tree
#   build   — Compile the APK (takes 4-12 hours depending on hardware)
#   package — Generate signed release APK
#   all     — Do everything (setup + patch + build + package)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
CHROMIUM_DIR="$HOME/chromium"
CHROMIUM_SRC="$CHROMIUM_DIR/src"

# Chrome version to build (update this with each release)
CHROME_VERSION_MAJOR=130
CHROME_VERSION_FULL="130.0.6723.58"

# Build configuration
BUILD_DIR="out/NormalBrowser"
TARGET_CPU="arm64"
IS_DEBUG=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# ====================================================================
# PHASE 1: Setup — Fetch Chromium source and dependencies
# ====================================================================
do_setup() {
  log_info "=== Phase 1: Setup ==="

  # Check system requirements
  local ram_gb
  ram_gb=$(free -g | awk '/Mem:/{print $2}')
  if [ "$ram_gb" -lt 14 ]; then
    log_warn "System has ${ram_gb}GB RAM. 16GB+ recommended for building."
    log_warn "Build may fail or swap heavily. Consider a cloud VM."
  fi

  local disk_gb
  disk_gb=$(df -BG "$HOME" | awk 'NR==2{print $4}' | tr -d 'G')
  if [ "$disk_gb" -lt 100 ]; then
    log_error "Only ${disk_gb}GB disk free. Need at least 130GB."
    exit 1
  fi

  # Install depot_tools if not present
  if ! command -v gclient &>/dev/null; then
    log_info "Installing depot_tools..."
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
        "$HOME/depot_tools"
    export PATH="$HOME/depot_tools:$PATH"
    echo 'export PATH="$HOME/depot_tools:$PATH"' >> ~/.bashrc
  fi

  # Fetch Chromium source
  if [ ! -d "$CHROMIUM_SRC" ]; then
    log_info "Fetching Chromium source (this takes 30-60 minutes)..."
    mkdir -p "$CHROMIUM_DIR"
    cd "$CHROMIUM_DIR"
    fetch --nohooks android
    cd src
    gclient sync
  else
    log_info "Chromium source already exists at $CHROMIUM_SRC"
    cd "$CHROMIUM_SRC"
    gclient sync
  fi

  # Install build dependencies
  log_info "Installing build dependencies..."
  cd "$CHROMIUM_SRC"
  ./build/install-build-deps-android.sh

  log_info "Setup complete!"
}

# ====================================================================
# PHASE 2: Patch — Apply Normal Browser modifications
# ====================================================================
do_patch() {
  log_info "=== Phase 2: Apply Patches ==="

  if [ ! -d "$CHROMIUM_SRC" ]; then
    log_error "Chromium source not found at $CHROMIUM_SRC"
    log_error "Run '$0 setup' first."
    exit 1
  fi

  # Run the patch application script
  bash "$SCRIPT_DIR/apply_patches.sh" "$CHROMIUM_SRC"

  log_info "Patches applied! Manual source edits still required."
  log_info "See patch files for inline instructions."
}

# ====================================================================
# PHASE 3: Build — Compile Chrome for Android
# ====================================================================
do_build() {
  log_info "=== Phase 3: Build ==="

  if [ ! -d "$CHROMIUM_SRC" ]; then
    log_error "Chromium source not found. Run '$0 setup' first."
    exit 1
  fi

  cd "$CHROMIUM_SRC"

  # Generate build configuration
  log_info "Generating build configuration..."
  gn gen "$BUILD_DIR" --args="
    target_os = \"android\"
    target_cpu = \"$TARGET_CPU\"
    is_debug = $IS_DEBUG
    is_component_build = false
    is_official_build = true
    chrome_pgo_phase = 0
    enable_nacl = false
    symbol_level = 0
    blink_symbol_level = 0
    v8_symbol_level = 0
    enable_resource_allowlist_generation = false
    dfn_gen = false
    use_thin_lto = true
    android_channel = \"stable\"
    android_default_version_name = \"$CHROME_VERSION_FULL\"
    android_default_version_code = \"${CHROME_VERSION_MAJOR}00\"

    # Normal Browser: Include our custom modules
    # These are added to the build via our BUILD.gn files
  "

  # Determine CPU count for parallel build
  local jobs
  jobs=$(nproc)
  # Use n-2 cores to keep system responsive
  if [ "$jobs" -gt 4 ]; then
    jobs=$((jobs - 2))
  fi

  log_info "Building with $jobs parallel jobs..."
  log_info "This will take 4-12 hours depending on hardware."
  log_info "Build started at: $(date)"

  # Build the APK
  autoninja -C "$BUILD_DIR" -j "$jobs" chrome_public_apk

  log_info "Build completed at: $(date)"
  log_info "APK: $CHROMIUM_SRC/$BUILD_DIR/apks/ChromePublic.apk"
}

# ====================================================================
# PHASE 4: Package — Sign and finalize the APK
# ====================================================================
do_package() {
  log_info "=== Phase 4: Package ==="

  APK_PATH="$CHROMIUM_SRC/$BUILD_DIR/apks/ChromePublic.apk"
  OUTPUT_DIR="$PROJECT_DIR/output"
  mkdir -p "$OUTPUT_DIR"

  if [ ! -f "$APK_PATH" ]; then
    log_error "APK not found at $APK_PATH. Run '$0 build' first."
    exit 1
  fi

  # Generate keystore if not exists
  KEYSTORE="$PROJECT_DIR/normal_browser.keystore"
  if [ ! -f "$KEYSTORE" ]; then
    log_info "Generating signing keystore..."
    keytool -genkey -v \
        -keystore "$KEYSTORE" \
        -alias normal_browser \
        -keyalg RSA \
        -keysize 2048 \
        -validity 10000 \
        -storepass normalbrowser2024 \
        -keypass normalbrowser2024 \
        -dname "CN=Normal Browser, OU=Development, O=NiceBrowser, L=India, ST=India, C=IN"
  fi

  # Zipalign
  log_info "Zipaligning APK..."
  local ALIGNED_APK="$OUTPUT_DIR/NormalBrowser-aligned.apk"
  zipalign -f -v 4 "$APK_PATH" "$ALIGNED_APK"

  # Sign with apksigner
  log_info "Signing APK..."
  local SIGNED_APK="$OUTPUT_DIR/NormalBrowser-v${CHROME_VERSION_FULL}.apk"
  apksigner sign \
      --ks "$KEYSTORE" \
      --ks-key-alias normal_browser \
      --ks-pass pass:normalbrowser2024 \
      --key-pass pass:normalbrowser2024 \
      --out "$SIGNED_APK" \
      "$ALIGNED_APK"

  # Verify signature
  apksigner verify "$SIGNED_APK"

  # Clean up
  rm -f "$ALIGNED_APK"

  # Print APK info
  local apk_size
  apk_size=$(du -h "$SIGNED_APK" | cut -f1)

  log_info "=== Build Complete! ==="
  log_info "APK: $SIGNED_APK"
  log_info "Size: $apk_size"
  log_info "Version: $CHROME_VERSION_FULL"
  log_info ""
  log_info "Install on device:"
  log_info "  adb install $SIGNED_APK"
}

# ====================================================================
# Main
# ====================================================================
case "${1:-all}" in
  setup)
    do_setup
    ;;
  patch)
    do_patch
    ;;
  build)
    do_build
    ;;
  package)
    do_package
    ;;
  all)
    do_setup
    do_patch
    do_build
    do_package
    ;;
  *)
    echo "Usage: $0 [setup|patch|build|package|all]"
    exit 1
    ;;
esac
