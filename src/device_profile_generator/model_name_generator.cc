// Copyright 2024 Normal Browser Authors. All rights reserved.
// model_name_generator.cc — Per-OEM model naming rules implementation.

#include "device_profile_generator/model_name_generator.h"

#include <cstdint>
#include <string>

#include "base/strings/stringprintf.h"

namespace normal_browser {

namespace {

// Simple seeded xorshift64 for deterministic generation from seed.
uint64_t NextRand(uint64_t& state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

int RandRange(uint64_t& state, int lo, int hi) {
  return lo + static_cast<int>(NextRand(state) % (hi - lo + 1));
}

char RandUpperChar(uint64_t& state) {
  return 'A' + static_cast<char>(NextRand(state) % 26);
}

char RandAlphaNum(uint64_t& state) {
  const char kChars[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  return kChars[NextRand(state) % 36];
}

bool RandBool(uint64_t& state) {
  return (NextRand(state) & 1) == 0;
}

// ============================================
// Samsung: SM-{series}{3-4 digits}{region}
// - S=flagship, A=mid, M=online, F=flipkart, E=economy
// - 9xx=2024, 8xx=2023, 7xx=2022
// ============================================
GeneratedModel GenerateSamsung(int tier, uint64_t& s) {
  GeneratedModel m;
  char series;
  std::string name_prefix;
  int base_num;

  switch (tier) {
    case 0:
      series = 'S';
      base_num = RandRange(s, 910, 968);
      if (RandBool(s))
        name_prefix = "Galaxy S" + std::to_string(RandRange(s, 23, 26));
      else
        name_prefix = "Galaxy S" + std::to_string(RandRange(s, 23, 26)) +
                      (RandBool(s) ? " Ultra" : "+");
      break;
    case 1:
      series = 'A';
      base_num = RandRange(s, 536, 578);
      name_prefix = "Galaxy A" + std::to_string(RandRange(s, 54, 56));
      break;
    case 2:
      series = RandBool(s) ? 'M' : 'A';
      base_num = RandRange(s, 346, 398);
      if (series == 'M')
        name_prefix = "Galaxy M" + std::to_string(RandRange(s, 34, 36));
      else
        name_prefix = "Galaxy A" + std::to_string(RandRange(s, 34, 36));
      break;
    case 3:
    default:
      series = RandBool(s) ? 'F' : 'E';
      base_num = RandRange(s, 146, 198);
      if (series == 'F')
        name_prefix = "Galaxy F" + std::to_string(RandRange(s, 14, 16));
      else
        name_prefix = "Galaxy A" + std::to_string(RandRange(s, 14, 16));
      break;
  }

  m.model_number =
      base::StringPrintf("SM-%c%dB", series, base_num);
  m.marketing_name = name_prefix;
  // Device codename is lowercase from model digits.
  m.device_codename =
      base::StringPrintf("%c%db", series + 32, base_num);
  return m;
}

// ============================================
// Xiaomi/Redmi: {2yr}{2mo}{model}{region}
// e.g. "24031PN0DC" for 2024 March
// ============================================
GeneratedModel GenerateXiaomi(int tier, uint64_t& s) {
  GeneratedModel m;
  int year = RandRange(s, 23, 25);
  int month = RandRange(s, 1, 12);

  m.model_number = base::StringPrintf(
      "%02d%02d%d%c%c%02d%c", year, month, RandRange(s, 1, 9),
      RandUpperChar(s), RandUpperChar(s), RandRange(s, 0, 99),
      "GIBY"[RandRange(s, 0, 3)]);

  switch (tier) {
    case 0:
      m.marketing_name =
          base::StringPrintf("Xiaomi %d%s",
                             RandRange(s, 13, 15),
                             RandBool(s) ? " Ultra" : " Pro");
      break;
    case 1:
      m.marketing_name =
          base::StringPrintf("Xiaomi %dT Pro", RandRange(s, 12, 14));
      break;
    case 2:
      m.marketing_name = base::StringPrintf(
          "Redmi Note %d%s", RandRange(s, 13, 15),
          RandBool(s) ? " Pro+ 5G" : " Pro 5G");
      break;
    case 3:
    default:
      m.marketing_name = base::StringPrintf(
          "Redmi %d%c", RandRange(s, 13, 15),
          RandBool(s) ? 'A' : 'C');
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// Realme: RMX{4 digits}
// ============================================
GeneratedModel GenerateRealme(int tier, uint64_t& s) {
  GeneratedModel m;
  m.model_number =
      base::StringPrintf("RMX%04d", RandRange(s, 3600, 3999));

  switch (tier) {
    case 0:
      m.marketing_name =
          base::StringPrintf("Realme GT %d Pro+", RandRange(s, 5, 7));
      break;
    case 1:
      m.marketing_name =
          base::StringPrintf("Realme GT %d", RandRange(s, 5, 7));
      break;
    case 2:
      m.marketing_name =
          base::StringPrintf("Realme %d Pro+ 5G", RandRange(s, 12, 14));
      break;
    case 3:
    default:
      m.marketing_name =
          base::StringPrintf("Realme Narzo %dA", RandRange(s, 60, 80));
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// OnePlus: CPH{4} or NE{4}
// ============================================
GeneratedModel GenerateOnePlus(int tier, uint64_t& s) {
  GeneratedModel m;
  if (RandBool(s))
    m.model_number =
        base::StringPrintf("CPH%04d", RandRange(s, 2400, 2999));
  else
    m.model_number =
        base::StringPrintf("NE%04d", RandRange(s, 2400, 2999));

  switch (tier) {
    case 0:
      m.marketing_name =
          base::StringPrintf("OnePlus %d Pro", RandRange(s, 12, 14));
      break;
    case 1:
      m.marketing_name =
          base::StringPrintf("OnePlus %d", RandRange(s, 12, 14));
      break;
    case 2:
      m.marketing_name =
          base::StringPrintf("OnePlus Nord CE%d 5G", RandRange(s, 3, 5));
      break;
    case 3:
    default:
      m.marketing_name =
          base::StringPrintf("OnePlus Nord N%d", RandRange(s, 30, 40));
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// Vivo: V{4 digits}
// ============================================
GeneratedModel GenerateVivo(int tier, uint64_t& s) {
  GeneratedModel m;
  m.model_number =
      base::StringPrintf("V%04d", RandRange(s, 2200, 2599));

  switch (tier) {
    case 0:
      m.marketing_name =
          base::StringPrintf("Vivo X%d Pro+", RandRange(s, 100, 120));
      break;
    case 1:
      m.marketing_name =
          base::StringPrintf("Vivo X%d Pro", RandRange(s, 80, 100));
      break;
    case 2:
      m.marketing_name =
          base::StringPrintf("Vivo V%d Pro", RandRange(s, 30, 35));
      break;
    case 3:
    default:
      m.marketing_name =
          base::StringPrintf("Vivo Y%d 5G", RandRange(s, 100, 200));
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// OPPO: CPH{4 digits}
// ============================================
GeneratedModel GenerateOPPO(int tier, uint64_t& s) {
  GeneratedModel m;
  m.model_number =
      base::StringPrintf("CPH%04d", RandRange(s, 2400, 2799));

  switch (tier) {
    case 0:
      m.marketing_name =
          base::StringPrintf("OPPO Find X%d Pro", RandRange(s, 7, 9));
      break;
    case 1:
      m.marketing_name =
          base::StringPrintf("OPPO Reno%d Pro+", RandRange(s, 11, 13));
      break;
    case 2:
      m.marketing_name =
          base::StringPrintf("OPPO Reno%d", RandRange(s, 11, 13));
      break;
    case 3:
    default:
      m.marketing_name =
          base::StringPrintf("OPPO A%d 5G", RandRange(s, 57, 80));
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// Motorola: XT{4 digits}-{1 digit}
// ============================================
GeneratedModel GenerateMotorola(int tier, uint64_t& s) {
  GeneratedModel m;
  m.model_number = base::StringPrintf("XT%04d-%d",
                                       RandRange(s, 2300, 2499),
                                       RandRange(s, 1, 4));

  switch (tier) {
    case 0:
      m.marketing_name =
          base::StringPrintf("Motorola Edge %d Ultra",
                             RandRange(s, 50, 55));
      break;
    case 1:
      m.marketing_name =
          base::StringPrintf("Motorola Edge %d+", RandRange(s, 50, 55));
      break;
    case 2:
      m.marketing_name =
          base::StringPrintf("Moto G%d Power", RandRange(s, 84, 95));
      break;
    case 3:
    default:
      m.marketing_name =
          base::StringPrintf("Moto G%d 5G", RandRange(s, 34, 45));
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// iQOO: (Vivo sub-brand)
// ============================================
GeneratedModel GenerateIQOO(int tier, uint64_t& s) {
  GeneratedModel m;
  m.model_number =
      base::StringPrintf("I%04d", RandRange(s, 2200, 2599));

  switch (tier) {
    case 0:
      m.marketing_name =
          base::StringPrintf("iQOO %d Legend", RandRange(s, 12, 14));
      break;
    case 1:
      m.marketing_name =
          base::StringPrintf("iQOO %d Pro", RandRange(s, 12, 14));
      break;
    case 2:
      m.marketing_name =
          base::StringPrintf("iQOO Z%d 5G", RandRange(s, 7, 9));
      break;
    case 3:
    default:
      m.marketing_name =
          base::StringPrintf("iQOO Z%de", RandRange(s, 7, 9));
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// Nothing: A{3 digits}
// ============================================
GeneratedModel GenerateNothing(int tier, uint64_t& s) {
  GeneratedModel m;
  m.model_number =
      base::StringPrintf("A%03d", RandRange(s, 60, 120));

  switch (tier) {
    case 0:
    case 1:
      m.marketing_name = base::StringPrintf(
          "Nothing Phone (%d)", RandRange(s, 2, 4));
      break;
    case 2:
      m.marketing_name = base::StringPrintf(
          "Nothing Phone (%da)", RandRange(s, 2, 4));
      break;
    case 3:
    default:
      m.marketing_name = "Nothing Phone (1)";
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// Google Pixel: G{4 alphanumeric}
// ============================================
GeneratedModel GeneratePixel(int tier, uint64_t& s) {
  GeneratedModel m;
  m.model_number = base::StringPrintf(
      "G%c%c%c%c", RandAlphaNum(s), RandAlphaNum(s),
      RandAlphaNum(s), RandAlphaNum(s));

  switch (tier) {
    case 0:
      m.marketing_name =
          base::StringPrintf("Pixel %d Pro", RandRange(s, 8, 10));
      break;
    case 1:
      m.marketing_name =
          base::StringPrintf("Pixel %d", RandRange(s, 8, 10));
      break;
    case 2:
      m.marketing_name =
          base::StringPrintf("Pixel %da", RandRange(s, 8, 10));
      break;
    case 3:
    default:
      m.marketing_name =
          base::StringPrintf("Pixel %da", RandRange(s, 6, 8));
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

// ============================================
// Tecno: various format
// ============================================
GeneratedModel GenerateTecno(int tier, uint64_t& s) {
  GeneratedModel m;
  m.model_number = base::StringPrintf(
      "%c%c%c%d%c", RandUpperChar(s), RandUpperChar(s),
      RandUpperChar(s), RandRange(s, 1, 9), RandUpperChar(s));

  switch (tier) {
    case 0:
    case 1:
      m.marketing_name =
          base::StringPrintf("Tecno Phantom X%d", RandRange(s, 2, 4));
      break;
    case 2:
      m.marketing_name =
          base::StringPrintf("Tecno Camon %d Pro", RandRange(s, 20, 25));
      break;
    case 3:
    default:
      m.marketing_name =
          base::StringPrintf("Tecno Spark %d", RandRange(s, 10, 20));
      break;
  }

  m.device_codename = m.model_number;
  return m;
}

}  // namespace

GeneratedModel GenerateModelName(Manufacturer manufacturer,
                                 int tier,
                                 uint64_t seed) {
  uint64_t s = seed;
  if (s == 0) s = 0xDEADBEEFCAFEull;  // avoid zero seed

  switch (manufacturer) {
    case Manufacturer::kSamsung:
      return GenerateSamsung(tier, s);
    case Manufacturer::kXiaomi:
      return GenerateXiaomi(tier, s);
    case Manufacturer::kRealme:
      return GenerateRealme(tier, s);
    case Manufacturer::kOnePlus:
      return GenerateOnePlus(tier, s);
    case Manufacturer::kVivo:
      return GenerateVivo(tier, s);
    case Manufacturer::kOPPO:
      return GenerateOPPO(tier, s);
    case Manufacturer::kMotorola:
      return GenerateMotorola(tier, s);
    case Manufacturer::kIQOO:
      return GenerateIQOO(tier, s);
    case Manufacturer::kNothing:
      return GenerateNothing(tier, s);
    case Manufacturer::kGoogle:
      return GeneratePixel(tier, s);
    case Manufacturer::kTecno:
      return GenerateTecno(tier, s);
  }

  return GenerateSamsung(tier, s);
}

}  // namespace normal_browser
