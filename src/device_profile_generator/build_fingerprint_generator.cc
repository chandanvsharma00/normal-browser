// Copyright 2024 Normal Browser Authors. All rights reserved.
// build_fingerprint_generator.cc — Build.FINGERPRINT, build IDs, etc.

#include "device_profile_generator/build_fingerprint_generator.h"

#include <cstdint>
#include <ctime>
#include <string>

#include "base/strings/stringprintf.h"

namespace normal_browser {

namespace {

uint64_t NextRand(uint64_t& state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

int RandRange(uint64_t& state, int lo, int hi) {
  return lo + static_cast<int>(NextRand(state) % (hi - lo + 1));
}

char RandHex(uint64_t& state) {
  const char kHex[] = "0123456789abcdef";
  return kHex[NextRand(state) % 16];
}

char RandUpperHex(uint64_t& state) {
  const char kHex[] = "0123456789ABCDEF";
  return kHex[NextRand(state) % 16];
}

char RandUpperChar(uint64_t& state) {
  return 'A' + static_cast<char>(NextRand(state) % 26);
}

int AndroidVersionToSdk(int ver) {
  switch (ver) {
    case 12: return 31;
    case 13: return 33;
    case 14: return 34;
    case 15: return 35;
    default: return 34;
  }
}

int PickAndroidVersion(uint64_t& state) {
  // Weighted: 14(50%), 13(30%), 12(15%), 15(5%)
  int roll = RandRange(state, 1, 100);
  if (roll <= 50) return 14;
  if (roll <= 80) return 13;
  if (roll <= 95) return 12;
  return 15;
}

}  // namespace

std::string GenerateBuildId(int android_version, uint64_t seed) {
  uint64_t s = seed;
  if (s == 0) s = 0xB01D1D01;

  // Real AOSP build ID format: {TAG}.{YYMMDD}.{SEQ}
  // Android 15: BP1A / BP2A
  // Android 14: UP1A / AP2A / UD1A / AD1A
  // Android 13: TQ3A / TP1A / TD1A
  // Android 12: SQ3A / SP1A / SD1A
  const char* prefixes_15[] = {"BP1A", "BP2A"};
  const char* prefixes_14[] = {"UP1A", "AP2A", "UD1A", "AD1A"};
  const char* prefixes_13[] = {"TQ3A", "TP1A", "TD1A"};
  const char* prefixes_12[] = {"SQ3A", "SP1A", "SD1A"};

  const char* prefix;
  switch (android_version) {
    case 15:
      prefix = prefixes_15[RandRange(s, 0, 1)];
      break;
    case 14:
      prefix = prefixes_14[RandRange(s, 0, 3)];
      break;
    case 13:
      prefix = prefixes_13[RandRange(s, 0, 2)];
      break;
    case 12:
      prefix = prefixes_12[RandRange(s, 0, 2)];
      break;
    default:
      prefix = prefixes_14[0];
      break;
  }

  // Date: recent (2024-2025 range)
  int year = RandRange(s, 24, 25);
  int month = RandRange(s, 1, 12);
  int day = RandRange(s, 1, 28);
  int seq = RandRange(s, 1, 99);

  return base::StringPrintf("%s.%02d%02d%02d.%03d",
                            prefix, year, month, day, seq);
}

std::string GenerateSecurityPatch(uint64_t seed) {
  uint64_t s = seed;
  if (s == 0) s = 0x5ECA1C40;

  // Generate a security patch date within the last 3 months.
  time_t now = time(nullptr);
  struct tm current_tm;
  gmtime_r(&now, &current_tm);

  int cur_year = current_tm.tm_year + 1900;
  int cur_month = current_tm.tm_mon + 1;  // 1-12

  // Pick 0, 1, or 2 months back from current month.
  int months_back = RandRange(s, 0, 2);
  int patch_month = cur_month - months_back;
  int patch_year = cur_year;
  if (patch_month <= 0) {
    patch_month += 12;
    patch_year -= 1;
  }

  // Security patches are always on 1st or 5th.
  int day = (RandRange(s, 0, 1) == 0) ? 1 : 5;
  return base::StringPrintf("%04d-%02d-%02d", patch_year, patch_month, day);
}

std::string GenerateAndroidId(uint64_t seed) {
  uint64_t s = seed;
  if (s == 0) s = 0xA0D201D0;
  std::string id;
  id.reserve(16);
  for (int i = 0; i < 16; ++i) {
    id += RandHex(s);
  }
  return id;
}

std::string GenerateSerialNumber(const std::string& manufacturer,
                                 uint64_t seed) {
  uint64_t s = seed;
  if (s == 0) s = 0x5E21A100;
  std::string sn;

  if (manufacturer == "Samsung") {
    // Samsung serials: R{2 alphanum}{1 alpha}{10 alphanum}
    // e.g. "R5CT12A3456789"
    sn += 'R';
    sn += static_cast<char>('0' + RandRange(s, 1, 9));
    sn += RandUpperChar(s);
    sn += RandUpperChar(s);
    for (int i = 0; i < 10; ++i)
      sn += RandUpperHex(s);
  } else if (manufacturer == "Google") {
    // Pixel: alphanumeric, 10 chars.
    for (int i = 0; i < 10; ++i)
      sn += RandUpperHex(s);
  } else {
    // Generic: 12 hex chars.
    for (int i = 0; i < 12; ++i)
      sn += RandUpperHex(s);
  }

  return sn;
}

BuildInfo GenerateBuildInfo(const std::string& manufacturer,
                            const std::string& model,
                            const std::string& device_codename,
                            uint64_t seed) {
  uint64_t s = seed;
  if (s == 0) s = 0xB01DF0E0;

  BuildInfo info;
  info.android_version = PickAndroidVersion(s);
  info.sdk_version = AndroidVersionToSdk(info.android_version);
  info.release = std::to_string(info.android_version);
  info.build_id = GenerateBuildId(info.android_version, NextRand(s));
  info.security_patch = GenerateSecurityPatch(NextRand(s));

  // Incremental: 8-char alphanumeric string (Samsung uses date+seq, others hex)
  if (manufacturer == "Samsung") {
    // e.g. "S936BXXU1AXL3"
    std::string upper_model = model;
    // Extract just the SM-X part and strip SM-
    std::string prefix;
    if (model.size() > 3 && model.substr(0, 3) == "SM-") {
      prefix = model.substr(3);
    } else {
      prefix = model;
    }
    // Samsung format: {MODEL}XXU{1}{tag}{month}{seq}
    // Tag increments: A,B,C...  Month: A-L  
    char tag = 'A' + static_cast<char>(RandRange(s, 0, 3));
    char month_ch = 'A' + static_cast<char>(RandRange(s, 0, 11));
    info.incremental = base::StringPrintf(
        "%sXXU1%c%c%c%d", prefix.c_str(), tag, 'X', month_ch,
        RandRange(s, 1, 9));
  } else {
    // Generic: 8 hex chars.
    info.incremental.reserve(8);
    for (int i = 0; i < 8; ++i)
      info.incremental += RandUpperHex(s);
  }

  info.display = info.build_id;

  // Build.FINGERPRINT format:
  // {brand}/{product}:{api_level}/{build_id}/{incremental}:user/release-keys
  std::string brand;
  std::string product;

  if (manufacturer == "Samsung") {
    brand = "samsung";
    // Samsung product: {device_codename}xx{region}1  e.g. s936bxxu1
    product = device_codename + "xx";
  } else if (manufacturer == "Xiaomi" || manufacturer == "Redmi") {
    brand = "Xiaomi";
    product = device_codename;
  } else if (manufacturer == "Google") {
    brand = "google";
    // Pixel uses device codenames like "raven", "husky"
    product = device_codename;
  } else if (manufacturer == "OnePlus") {
    brand = "OnePlus";
    product = device_codename;
  } else if (manufacturer == "Realme") {
    brand = "realme";
    product = device_codename;
  } else if (manufacturer == "Vivo" || manufacturer == "iQOO") {
    brand = "vivo";
    product = device_codename;
  } else if (manufacturer == "OPPO") {
    brand = "OPPO";
    product = device_codename;
  } else if (manufacturer == "Motorola") {
    brand = "motorola";
    product = device_codename;
  } else if (manufacturer == "Nothing") {
    brand = "Nothing";
    product = device_codename;
  } else if (manufacturer == "Tecno") {
    brand = "TECNO";
    product = device_codename;
  } else {
    brand = manufacturer;
    product = device_codename;
  }

  info.fingerprint = base::StringPrintf(
      "%s/%s:%d/%s/%s:user/release-keys",
      brand.c_str(), product.c_str(), info.android_version,
      info.build_id.c_str(), info.incremental.c_str());

  return info;
}

}  // namespace normal_browser
