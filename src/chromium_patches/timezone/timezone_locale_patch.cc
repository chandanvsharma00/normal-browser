// Copyright 2024 Normal Browser Authors. All rights reserved.
// timezone_locale_patch.cc — Timezone and locale spoofing for V8 + ICU.
//
// FILES TO MODIFY:
//   third_party/icu/source/common/putil.cpp    → uprv_tzname()
//   v8/src/date/dateparser.cc                  → timezone offset
//   v8/src/objects/intl-objects.cc              → Intl resolved options
//   third_party/icu/source/common/uloc.cpp     → default locale
//   base/i18n/icu_util.cc                      → ICU initialization
//
// STRATEGY:
//   Lock timezone to "Asia/Kolkata" (UTC+5:30) for Indian market.
//   Lock locale to match profile's language setting.
//   Ensure consistency across ALL timezone/locale APIs:
//     - Date.prototype.getTimezoneOffset() → -330
//     - Intl.DateTimeFormat().resolvedOptions().timeZone → "Asia/Kolkata"
//     - Intl.DateTimeFormat().resolvedOptions().locale → "en-IN" (or profile)
//     - new Date().toString() → includes "IST" or "GMT+0530"
//     - performance.timeOrigin → reflects IST
//     - navigator.language → matches locale

#include "device_profile_generator/profile_store.h"

#include <cstdint>
#include <cstdlib>
#include <string>

// ICU headers for timezone/locale override.
#include "third_party/icu/source/common/unicode/uloc.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

namespace normal_browser {
namespace timezone {

// ====================================================================
// PATCH 1: ICU Default Timezone
// File: third_party/icu/source/common/putil.cpp → uprv_tzname()
// File: base/i18n/icu_util.cc → SetICUDefaultTimezone()
//
// Override ICU's timezone detection to always return "Asia/Kolkata".
// This affects ALL date/time formatting in the browser.
// ====================================================================
const char* GetSpoofedTimezone() {
  // Currently locked to India for the target market.
  // Future: could be per-profile if needed for other markets.
  return "Asia/Kolkata";
}

// ====================================================================
// PATCH 2: Date.prototype.getTimezoneOffset()
// File: v8/src/date/dateparser.cc or v8/src/builtins/builtins-date.cc
//
// getTimezoneOffset() returns negative of UTC offset in minutes.
// IST (UTC+5:30) → offset = -330
//
// ORIGINAL: V8 calls OS timezone APIs → reveals real timezone.
// PATCHED:  Always return -330 (IST).
// ====================================================================
int GetSpoofedTimezoneOffsetMinutes() {
  return -330;  // UTC+5:30 (IST)
}

// ====================================================================
// PATCH 3: Intl.DateTimeFormat resolvedOptions
// File: v8/src/objects/intl-objects.cc
//
// Intl.DateTimeFormat().resolvedOptions().timeZone → "Asia/Kolkata"
// Intl.DateTimeFormat().resolvedOptions().locale → from profile
//
// This is handled by ICU once we set the default timezone (PATCH 1).
// But we need to also ensure the locale matches.
// ====================================================================
std::string GetSpoofedLocale() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return "en-IN";

  // Use the primary language from the profile
  const auto& p = store->GetProfile();
  if (!p.languages.empty()) {
    return p.languages[0];  // e.g., "en-IN", "hi-IN", "en-US"
  }
  return "en-IN";
}

// ====================================================================
// PATCH 4: ICU Default Locale
// File: third_party/icu/source/common/uloc.cpp → uloc_getDefault()
// File: base/i18n/icu_util.cc
//
// Override ICU's default locale to match profile language.
// ====================================================================
const char* GetSpoofedICULocale() {
  // Map browser locale to ICU locale format
  // "en-IN" → "en_IN", "hi-IN" → "hi_IN"
  static thread_local std::string icu_locale;

  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    icu_locale = "en_IN";
    return icu_locale.c_str();
  }

  std::string locale = store->GetProfile().languages.empty() ?
      "en-IN" : store->GetProfile().languages[0];

  // Convert BCP-47 to ICU format: en-IN → en_IN
  icu_locale = locale;
  for (char& c : icu_locale) {
    if (c == '-') c = '_';
  }
  return icu_locale.c_str();
}

// ====================================================================
// PATCH 5: Date.prototype.toString() timezone abbreviation
// File: v8/src/date/dateparser.cc
//
// Real output: "Mon Jan 15 2024 10:30:00 GMT+0530 (India Standard Time)"
// The timezone abbreviation comes from ICU. Once we set the timezone
// to "Asia/Kolkata", ICU will automatically provide "IST" / "GMT+0530".
// No additional patch needed IF timezone is correctly set.
// ====================================================================

// ====================================================================
// PATCH 6: performance.timeOrigin
// File: third_party/blink/renderer/core/timing/performance.cc
//
// performance.timeOrigin is in milliseconds since Unix epoch.
// This naturally reflects the system clock which includes timezone.
// No patch needed — timeOrigin is UTC-based, not timezone-dependent.
// However, the DISPLAY of time (via Date) must use correct timezone.
// ====================================================================

// ====================================================================
// PATCH 7: Intl locale for all Intl constructors
// File: v8/src/objects/intl-objects.cc
//
// The following Intl APIs all use ICU's default locale (set in PATCH 4):
//   - Intl.NumberFormat().resolvedOptions().locale
//   - Intl.DateTimeFormat().resolvedOptions().locale
//   - Intl.Collator().resolvedOptions().locale
//   - Intl.PluralRules().resolvedOptions().locale
//   - Intl.RelativeTimeFormat().resolvedOptions().locale
//   - Intl.ListFormat().resolvedOptions().locale
//   - Intl.Segmenter().resolvedOptions().locale
//
// All are covered by uloc_setDefault() in PATCH 4. No per-constructor
// patching is needed — V8 reads defaults from ICU.
//
// FPJS Pro specifically checks:
//   new Intl.Collator().resolvedOptions().locale  → must equal navigator.language
//   new Intl.NumberFormat("en-IN").format(100000)  → "1,00,000" (lakh grouping)
//   new Intl.DateTimeFormat().resolvedOptions().timeZone → "Asia/Kolkata"
//
// These are all consistent because:
//   1. ICU timezone set to "Asia/Kolkata" (PATCH 1)
//   2. ICU default locale set to profile's primary language (PATCH 4)
//   3. navigator.language reads from same profile (navigator_spoofing.cc)
// ====================================================================

// ====================================================================
// Integration: Call during browser startup (before any renderer starts)
// File: chrome/browser/chrome_browser_main.cc
//       → PreMainMessageLoopRun()
// ====================================================================
void ApplyTimezoneAndLocale() {
  // 1. Set ICU default timezone to Asia/Kolkata.
  icu::TimeZone* tz = icu::TimeZone::createTimeZone("Asia/Kolkata");
  icu::TimeZone::adoptDefault(tz);

  // 2. Set ICU default locale from profile.
  UErrorCode status = U_ZERO_ERROR;
  uloc_setDefault(GetSpoofedICULocale(), &status);

  // 3. Set the TZ environment variable (backup for C library).
  setenv("TZ", ":Asia/Kolkata", 1);
  tzset();

  // These must be called BEFORE any renderer process is spawned
  // so that V8's date code picks up the correct timezone.
}

// ====================================================================
// Consistency check: All timezone/locale surfaces
//
// API                                        Expected Value
// ────────────────────────────────────────────────────────────
// Date.prototype.getTimezoneOffset()          -330
// Intl.DateTimeFormat().resolvedOptions().tz   "Asia/Kolkata"
// Intl.DateTimeFormat().resolvedOptions().loc  "en-IN" (per profile)
// new Date().toString()                        ... GMT+0530 (IST)
// new Date().toLocaleString()                  ... (locale-formatted)
// Intl.NumberFormat().resolvedOptions().locale  "en-IN"
// navigator.language                           "en-IN"
// navigator.languages                          ["en-IN", "en", "hi"]
// Accept-Language header                       "en-IN,en;q=0.9,hi;q=0.8"
// performance.timeOrigin                       (UTC, no tz dep)
//
// All must be internally consistent.
// ====================================================================

}  // namespace timezone
}  // namespace normal_browser
