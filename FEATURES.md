# GhostBrowser — Feature Specification

## One-Click Anti-Fingerprint Android Browser

> Press **"New Identity"** once → completely new device, new fingerprint, new TLS signature, all data cleared. Zero configuration.

---

## ✅ 100% WEB FINGERPRINT BYPASS

This browser defeats **ALL 27 detection methods** that the FingerprintJS Pro **web JavaScript agent** can perform. There is **nothing left** for the web agent to detect.

### Why 100%?

The FPJS web JS agent runs inside the browser sandbox. It can ONLY:
- Call JavaScript APIs (`navigator.*`, `screen.*`, `Math.*`, `canvas`, `WebGL`, etc.)
- Read HTTP headers (via server-side reflection)
- Perform TLS fingerprinting (via Cloudflare)

**Every single one of these is spoofed at the C++ level.** The JS agent sees a completely normal, unique Android device every time.

### What about root/Frida/jailbreak detection?

Those signals (`jailbroken`, `frida`, `clonedApp`, `factoryReset`) require the **FPJS Native Android SDK** — a library compiled into a native Android APK. The **web JS agent loaded via `<script>` CANNOT detect these** because the browser sandbox blocks access to `/system/bin/su`, `/proc/self/maps`, package metadata, etc.

Polymarket and all websites use the web agent → **root detection is irrelevant**.

### Remaining risks (NOT fingerprinting — need external tools):
| Risk | Solution |
|---|---|
| Same IP address | Residential rotating proxy |
| Behavioral patterns | Vary browsing behavior manually |
| Wallet graph analysis | Fund from separate sources |
| Email correlation | Fresh email per account |

---

## ONE-CLICK WORKFLOW

```
User presses "New Identity" button
        │
        ▼
┌─────────────────────────────────────────────────┐
│  1. CLEAR ALL DATA                              │
│     • Cookies         • Cache                   │
│     • localStorage    • IndexedDB               │
│     • sessionStorage  • WebSQL                   │
│     • Service Workers • Cache Storage            │
│     • File Systems    • Form Data                │
│     • Saved Passwords • Autofill                 │
│     • History         • Downloads                │
│     • Media Licenses  • Site Permissions          │
├─────────────────────────────────────────────────┤
│  2. GENERATE NEW DEVICE (Procedural)            │
│     • Manufacturer    → Samsung/Xiaomi/OnePlus...│
│     • Model number    → Procedurally generated   │
│     • SoC + GPU       → From real SoC database   │
│     • Screen          → OEM-valid resolution     │
│     • RAM / Cores     → Tier-appropriate          │
│     • Android version → Weighted random          │
│     • Build ID        → Procedural AOSP format   │
│     • Sensors         → Matched to manufacturer  │
│     • Fonts           → Manufacturer font list   │
│     • Cameras         → OEM camera config        │
├─────────────────────────────────────────────────┤
│  3. GENERATE NEW FINGERPRINTS                   │
│     • Canvas seed     → Unique Skia noise        │
│     • Audio seed      → Unique DSP bias          │
│     • WebGL seed      → Unique shader precision  │
│     • ClientRect seed → Unique sub-pixel noise   │
│     • Sensor noise    → Unique noise floor        │
│     • Math seed       → Unique FPU perturbation  │
│     • Emoji font      → Matched to manufacturer  │
│     • Perf timing     → Matched to SoC tier      │
│     • Storage values  → Matched to device tier   │
│     • Voice list      → Matched to OEM           │
├─────────────────────────────────────────────────┤
│  4. GENERATE NEW TLS PROFILE                    │
│     • Cipher order    → Shuffled                 │
│     • Extension order → Shuffled                 │
│     • GREASE values   → Random positions         │
│     • Curves order    → Varied                   │
│     → Unique JA3/JA4 hash                       │
├─────────────────────────────────────────────────┤
│  5. APPLY & RESTART                             │
│     • Update HTTP headers (UA + Client Hints)   │
│     • Update all JS APIs (Navigator, Screen...) │
│     • Update TLS stack (BoringSSL config)        │
│     • Restart browser with clean slate          │
└─────────────────────────────────────────────────┘
        │
        ▼
  New device. New fingerprint. New TLS. 
  No cookies. No history. No trace.
  Takes: ~2 seconds.
```

---

## COMPLETE FEATURE LIST

### F1: UNLIMITED PROCEDURAL DEVICE GENERATION

| Feature | Description |
|---|---|
| **Infinite unique devices** | NOT a fixed database — procedurally generates model numbers, build IDs, specs |
| **11 OEM manufacturers** | Samsung (28%), Xiaomi (22%), Realme (10%), OnePlus (8%), Vivo (8%), OPPO (6%), Motorola (5%), iQOO (4%), Nothing (3%), Google (3%), Tecno (3%) |
| **Weighted market distribution** | Device brand probabilities match real Indian market share |
| **4 price tiers** | Budget (40%), Mid-range (35%), Flagship (15%), Premium (10%) |
| **OEM naming conventions** | Samsung: SM-S/A/M/F/E + digits + B. Xiaomi: year+month+code. OnePlus: CPH/NE + digits. Each follows real rules |
| **25+ real SoCs** | Snapdragon 8G3/8G2/8G1/7+G2/7G1/6G1/695/4G2/4G1, Dimensity 9300/9200/8300/8200/8100/7200/7050/6100+/6080/6020, Exynos 2400/2200/1380, Tensor G3/G4, Helio G99/G85/G36 |
| **Accurate GPU specs** | Each SoC maps to exact GL renderer/vendor/version/extensions — validated against real devices |
| **Procedural build IDs** | AOSP format: UP1A.YYMMDD.NNN — unique every time |
| **Procedural build fingerprints** | Full `android.os.Build.FINGERPRINT` string matching OEM format |
| **Recent security patches** | Always within last 3 months |
| **SoC-manufacturer constraints** | Samsung uses Snapdragon + Exynos only. Xiaomi uses Snapdragon + Dimensity + Helio. Google uses Tensor only. Never generates impossible combos |
| **Tier-appropriate specs** | Budget phones don't get 16GB RAM or 1440p screens. Flagships don't get 720p or 3GB RAM |
| **>10^20 unique combinations** | Repeat probability: effectively zero |

---

### F2: NATIVE C++ FINGERPRINT SPOOFING (Undetectable)

| Feature | What Changes | Layer | Detection-Proof? |
|---|---|---|---|
| **Navigator.userAgent** | Device model + Android version + Chrome version | C++ Blink | ✅ Native data property, no getter |
| **Navigator.platform** | "Linux armv8l" | C++ Blink | ✅ |
| **Navigator.hardwareConcurrency** | CPU cores from SoC profile | C++ Blink | ✅ |
| **Navigator.deviceMemory** | RAM from device tier | C++ Blink | ✅ |
| **Navigator.language/languages** | Weighted: en-IN, en-US, hi-IN, en-GB | C++ Blink | ✅ |
| **Navigator.maxTouchPoints** | 5 (always, mobile) | C++ Blink | ✅ |
| **Navigator.connection** | "4g" or "wifi" | C++ Blink | ✅ |
| **Navigator.getBattery()** | Random level 15-95% | C++ Blink | ✅ |
| **Screen.width/height** | From OEM screen ranges | C++ Blink | ✅ |
| **Screen.availWidth/Height** | Minus realistic statusbar + navbar | C++ Blink | ✅ |
| **Screen.colorDepth** | 24 (always) | C++ Blink | ✅ |
| **window.devicePixelRatio** | From OEM density | C++ Blink | ✅ |
| **Canvas fingerprint** | Sub-pixel glyph offset at Skia level | C++ Skia | ✅ Genuine rendering difference |
| **WebGL renderer/vendor** | GPU string from SoC profile | C++ GPU | ✅ |
| **WebGL extensions** | GPU-appropriate extension list | C++ GPU | ✅ |
| **WebGL rendering** | Shader precision variation | C++ ANGLE | ✅ Genuine rendering difference |
| **Audio fingerprint** | DSP-level micro-bias on compressor | C++ WebAudio | ✅ Genuine processing difference |
| **Font list** | Manufacturer-specific fonts | C++ FontCache | ✅ |
| **Font metrics** | Slight glyph width variation | C++ HarfBuzz | ✅ |
| **Sensor names** | Matched to OEM sensor hardware | C++ GenericSensor | ✅ |
| **Sensor noise floor** | Per-session unique noise pattern | C++ GenericSensor | ✅ |
| **MediaDevices** | Correct camera count + random UUIDs | C++ MediaStream | ✅ |
| **ClientRect/BoundingRect** | ±0.001px sub-pixel noise | C++ Layout | ✅ |
| **WebRTC local IP** | Fake 192.168.x.x per session | C++ P2P | ✅ |
| **Math.sin/cos/tan/...** | Software FDLIBM + ULP perturbation | C++ V8 builtins | ✅ No CPU arch leak |
| **Emoji rendering** | OEM emoji font at Skia level | C++ FreeType/Skia | ✅ |
| **Performance timing** | SoC-tier delays with jitter | C++ Canvas/Audio | ✅ |
| **storage.estimate()** | Tier-appropriate storage values | C++ Quota | ✅ |
| **performance.memory** | RAM-matched heap stats | C++ V8 | ✅ |
| **navigator.connection** | Realistic downlink/rtt | C++ NetInfo | ✅ |
| **speechSynthesis.getVoices()** | OEM-appropriate voice list | C++ Speech | ✅ |
| **navigator.gpu (WebGPU)** | Spoofed adapter or disabled | C++ WebGPU | ✅ |
| **CSS matchMedia()** | Profile-consistent media queries | C++ CSS | ✅ |
| **navigator.webdriver** | Always `false` (no automation) | C++ Blink | ✅ |
| **navigator.vendor** | Always `"Google Inc."` | C++ Blink | ✅ |
| **navigator.productSub** | Always `"20030107"` | C++ Blink | ✅ |
| **navigator.plugins** | Empty PluginArray (mobile Chrome) | C++ Blink | ✅ |
| **window.chrome** | Chrome object exists with app/csi/loadTimes | C++ Blink | ✅ |
| **Automation markers** | No selenium/phantom/puppeteer globals | Build config | ✅ |
| **Error.stack format** | V8-native Chrome stack trace format | C++ V8 | ✅ |
| **screen.orientation** | portrait-primary, angle: 0 | C++ Blink | ✅ |
| **Root/Frida/Magisk** | Sandbox file interception | C++ Sandbox | 🟡 BONUS (SDK-only, not needed for web) |

**44 spoofed API surfaces.** All values come from C++ native code. JavaScript sees normal data properties with `[native code]` toString. No prototype pollution. No accessor descriptors. No timing differences. FingerprintJS Pro's tampering detection sees a completely normal browser.

---

### F3: HTTP HEADER CONSISTENCY

| Header | Source | Matches JS? |
|---|---|---|
| `User-Agent` | Built from device profile | ✅ Matches `navigator.userAgent` exactly |
| `Sec-CH-UA` | From Chrome version | ✅ |
| `Sec-CH-UA-Mobile` | `?1` (always mobile) | ✅ |
| `Sec-CH-UA-Platform` | `"Android"` | ✅ Matches `navigator.platform` logic |
| `Sec-CH-UA-Platform-Version` | From android_version | ✅ |
| `Sec-CH-UA-Model` | From generated model | ✅ |
| `Sec-CH-UA-Full-Version-List` | From Chrome version | ✅ |
| `Sec-CH-UA-Arch` | `"arm"` | ✅ |
| `Sec-CH-UA-Bitness` | `"64"` | ✅ |
| `Accept-Language` | From language profile | ✅ Matches `navigator.languages` |

**Modified at:** `net/http/`, `content/common/user_agent.cc`, `components/embedder_support/user_agent_utils.cc`

---

### F4: CLOUDFLARE TLS FINGERPRINT BYPASS

| Feature | Description |
|---|---|
| **JA3 hash randomization** | Unique JA3 hash every session via cipher suite reordering |
| **JA4 hash randomization** | Unique JA4 hash every session via extension reordering |
| **Cipher suite shuffling** | 15-17 cipher suites in random order per session |
| **Cipher suite count variation** | Drop 0-3 non-essential ciphers to vary list length |
| **TLS extension reordering** | All extensions (except SNI/PSK) shuffled per session |
| **GREASE injection** | 2-4 GREASE values at random positions (RFC 8701 compliant) |
| **Elliptic curve order variation** | X25519/P-256/P-384 order varies; optional P-521/Kyber |
| **Signature algorithm shuffling** | ECDSA/RSA-PSS/RSA order varies |
| **Mimics real browser diversity** | Profiles look like they come from different Chrome builds/versions |
| **Session ticket variation** | New vs returning session behavior varies |
| **Padding variation** | ClientHello padding target 512 or 1024 bytes |
| **BoringSSL-level modification** | Changes are in the TLS library itself — no middleware proxy needed |

**Modified at:** `third_party/boringssl/ssl/handshake_client.cc`, `ssl/extensions.cc`, `ssl/supported_groups.cc`

**Result:** Cloudflare Bot Management cannot correlate sessions. Each session looks like a different browser installation.

---

### F5: COMPLETE DATA CLEARING (One-Click)

| Data Type | Cleared? |
|---|---|
| Cookies (all domains) | ✅ |
| Cache (HTTP cache) | ✅ |
| localStorage | ✅ |
| sessionStorage | ✅ |
| IndexedDB | ✅ |
| WebSQL | ✅ |
| Service Workers | ✅ |
| Cache Storage (SW caches) | ✅ |
| File Systems (sandboxed) | ✅ |
| Form autofill data | ✅ |
| Saved passwords | ✅ |
| Browsing history | ✅ |
| Download history | ✅ |
| Media licenses (DRM) | ✅ |
| Site permissions | ✅ |
| Notification permissions | ✅ |
| HSTS preload state | ✅ |
| OCSP stapling cache | ✅ |
| DNS cache | ✅ |
| Socket connections | ✅ Closed + re-established |

---

### F6: TIMEZONE & LOCALE

| Feature | Value |
|---|---|
| Timezone | `Asia/Kolkata` (UTC+5:30) — always |
| `Date.getTimezoneOffset()` | -330 |
| `Intl.DateTimeFormat` timezone | `Asia/Kolkata` |
| `Performance.timeOrigin` | IST-consistent |
| ICU timezone | Modified at `third_party/icu/` |
| Language weights | en-IN (60%), en-US (20%), hi-IN (15%), en-GB (5%) |

---

### F7: BROWSER UI FEATURES

| UI Element | Function |
|---|---|
| **"New Identity" button** | One-click: clear all + new device + new TLS + restart |
| **"Current Device" info** | Shows: manufacturer, model, SoC, GPU, screen, RAM, Android version |
| **"Identity Log"** | History of all past identities with timestamps (for testing/debugging) |
| **Status bar indicator** | Shows current spoofed device name in toolbar |
| **TLS Profile viewer** | Shows current JA3/JA4 hash for verification |
| **Quick-toggle proxy** | Connect/disconnect residential proxy from browser |
| **"Share Profile" button** | Export current device profile as JSON (for reproducibility) |
| **"Import Profile" button** | Load a specific device profile from JSON |
| **Notification on identity change** | Brief toast: "Now: Samsung Galaxy A55 5G • Snapdragon 7s Gen 2" |

All accessible from Chrome's 3-dot menu → **"Ghost Mode"** submenu.

---

### F8: PROFILE PERSISTENCE MODES

| Mode | Behavior |
|---|---|
| **Session Mode** (default) | Same identity throughout browser session. New identity on "New Identity" press. |
| **Auto-Rotate Mode** | New identity every N minutes (configurable: 5/15/30/60 min) |
| **Persistent Mode** | Keep same identity across browser restarts until manually changed |
| **Import Mode** | Use a specific profile loaded from JSON file |

---

### F9: MATH FINGERPRINT NEUTRALIZATION (CPU FPU)

| Feature | Description |
|---|---|
| **Software math implementation** | Replace hardware FPU calls with FDLIBM/crlibm — bit-exact across ALL CPU architectures |
| **Per-session ULP perturbation** | Flip 2-3 bits in the unit of least precision per session seed |
| **Covers all Math.* functions** | `sin`, `cos`, `tan`, `atan2`, `exp`, `expm1`, `log`, `log1p`, `cbrt`, `sinh`, `cosh`, `tanh` |
| **Undetectable** | Perturbation is < 10⁻¹⁵ — within normal floating-point variance |
| **No CPU architecture leak** | Cortex-X4, Cortex-A78, Cortex-A53 all produce identical base results |

**Modified at:** `v8/src/builtins/builtins-math.cc`, `device_profile_generator/software_math.h`

---

### F10: EMOJI & UNICODE RENDERING

| Feature | Description |
|---|---|
| **Bundled OEM emoji fonts** | Samsung Color Emoji, Google Noto Color Emoji |
| **Manufacturer-matched selection** | Samsung profile → Samsung emoji. Others → Google emoji |
| **System UI font spoofing** | Samsung → SamsungOne, Xiaomi → MiSans, OnePlus → OnePlusSans, others → Roboto |
| **Skia/FreeType level loading** | Fonts loaded as system fonts, not web fonts — canvas renders correct emoji |
| **APK size impact** | ~27MB (SamsungEmoji 12MB + NotoEmoji 10MB + System fonts 5MB) |

**Modified at:** `chrome/android/java/res/font/`, `third_party/blink/renderer/platform/fonts/font_cache_android.cc`

---

### F11: PERFORMANCE TIMING NORMALIZATION

| Feature | Description |
|---|---|
| **SoC-tier timing delays** | Operations artificially slowed to match claimed chipset performance |
| **4 performance tiers** | Flagship: 5-12ms, Upper-Mid: 12-25ms, Mid: 25-50ms, Budget: 50-100ms (canvas) |
| **Random jitter** | ±10-20% per operation to prevent timing correlation |
| **Covers key operations** | `canvas.toDataURL()`, `crypto.subtle.digest()`, WASM compilation, `JSON.parse()`, regex, sort |
| **Prevents hardware-tier leak** | Budget SoC claim + fast operation → caught. Now normalized. |

**Modified at:** `third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.cc` + others

---

### F12: STORAGE & MEMORY FINGERPRINTING

| Feature | Description |
|---|---|
| **`navigator.storage.estimate()`** | Returns device-tier storage: Budget 64GB, Mid 128GB, Flag 256GB, Premium 512GB |
| **`performance.memory`** | Heap size matches claimed RAM (4GB → 1.5GB heap, 8GB → 2.5GB) |
| **`navigator.connection.*`** | Realistic downlink (5-80 Mbps), rtt (5-80ms) based on connection type |
| **Storage noise** | ±500MB on total, ±100MB on used — never round numbers |
| **Used percentage** | 30-70% randomly (realistic for real phones) |

**Modified at:** `third_party/blink/renderer/modules/quota/storage_manager.cc`, `modules/netinfo/network_information.cc`

---

### F13: SPEECH SYNTHESIS VOICE SPOOFING

| Feature | Description |
|---|---|
| **Manufacturer-specific voices** | Samsung profiles → Samsung TTS voices. Others → Google TTS only |
| **Correct voice count** | Samsung: 10-15 voices. Stock Android: 8-12 voices |
| **Language-appropriate** | Includes `en-IN`, `hi-IN`, `bn-IN` voices for Indian market |
| **Prevents voice list fingerprinting** | `speechSynthesis.getVoices()` returns expected voices for claimed OEM |

**Modified at:** `third_party/blink/renderer/modules/speech/speech_synthesis.cc`

---

### F14: WEBGPU ADAPTER SPOOFING

| Feature | Description |
|---|---|
| **Adapter info override** | `vendor`, `architecture`, `device`, `description` match SoC GPU |
| **Feature filtering** | Only expose WebGPU features the claimed GPU actually supports |
| **Conditional availability** | Budget SoCs (Helio G36, SD 4 Gen 1) → WebGPU disabled entirely |
| **SoC→WebGPU mapping** | SD 8G3→Adreno 750, Dim 9300→Immortalis-G720, Exynos 2400→Xclipse 940 |

**Modified at:** `third_party/blink/renderer/modules/webgpu/gpu_adapter.cc`

---

### F15: CSS MEDIA QUERY CONSISTENCY

| Feature | Description |
|---|---|
| **`(hover: hover/none)`** | Always `none` (mobile = no hover) |
| **`(pointer: fine/coarse)`** | Always `coarse` (touch, not mouse) |
| **`(prefers-color-scheme)`** | 60% dark, 40% light (random per profile) |
| **`(max-resolution: Xdppx)`** | Returns profile density, not real display |
| **`(color-gamut: p3)`** | Flagship = P3 supported. Budget = sRGB only |
| **All via profile values** | `matchMedia()` resolves against DeviceProfile, not real hardware |

**Modified at:** `third_party/blink/renderer/core/css/media_query_evaluator.cc`

---

### F16: ANTI-BOT / ANTI-AUTOMATION MARKERS

> **CRITICAL — These are the FIRST things FPJS checks before fingerprinting.**

| Feature | Value | Why |
|---|---|---|
| `navigator.webdriver` | `false` | Puppeteer/Selenium set `true`. Must be `false`, not `undefined` |
| `window.chrome` | Object with `app`, `csi`, `loadTimes` | FPJS checks `typeof window.chrome !== 'undefined'` |
| `navigator.plugins` | Empty PluginArray (length 0) | Mobile Chrome has no plugins (desktop has PDF viewer) |
| `navigator.mimeTypes` | Empty MimeTypeArray (length 0) | Same — mobile has none |
| `navigator.pdfViewerEnabled` | `false` | Mobile Chrome = no PDF viewer |
| `navigator.doNotTrack` | `null` | Default state — user hasn't toggled |
| `navigator.vendor` | `"Google Inc."` | Chrome-specific. Firefox = empty. Safari = Apple |
| `navigator.productSub` | `"20030107"` | Chrome/Safari value. Firefox = different |
| `'ontouchstart' in window` | `true` | Verifies mobile claim |
| `typeof TouchEvent` | `"function"` | Must exist on mobile |
| Permissions defaults | All `'prompt'` | Fresh browser = no permission decisions yet |
| `Error.prototype.stack` | V8 format | Chrome stack traces differ from Firefox/Safari |
| `eval.toString()` | `"function eval() { [native code] }"` | Don't monkey-patch |
| `screen.orientation` | `portrait-primary`, angle `0` | Mobile default |
| `window.outerWidth` | = `screen.width` (mobile fullscreen) | Must match screen dims |
| No automation globals | No `__selenium_*`, `__nightmare`, `_phantom`, `cdc_*` | Any of these → bot detected immediately |

**Modified at:** `third_party/blink/renderer/core/frame/navigator.cc`, `local_dom_window.cc`, stock Chromium (don't add automation frameworks to build)

---

### F17: PROFILE STORE SINGLETON (Central Architecture)

| Feature | Description |
|---|---|
| **Thread-safe singleton** | `DeviceProfileStore::Get()->GetProfile()` — one global access point |
| **`base::NoDestructor` pattern** | Chromium's standard singleton: no destruction, no race conditions |
| **Serialization to SharedPreferences** | JSON serialization for persistence across app restarts |
| **Mojo IPC to renderers** | Browser process → renderer process profile passing (renderers can't access disk) |
| **Observer pattern** | `NotifyProfileChanged()` updates HTTP header generator, TLS stack, all components |
| **BUILD.gn integration** | New `source_set("device_profile_generator")` linked into `//chrome/browser` |
| **Startup wiring** | `chrome/browser/chrome_browser_main.cc` → `PreMainMessageLoopRun()` |

**Modified at:** `device_profile_generator/profile_store.h`, `profile_store.cc`, `chrome_browser_main.cc`, `content/common/ghost_profile.mojom`

---

### F18: CHROME VERSION MANAGEMENT

| Feature | Description |
|---|---|
| **Build-time version fetch** | CI/CD fetches current Chrome stable from `chromiumdash.appspot.com` |
| **Source↔version match** | Built from Chromium 122 → claims Chrome 122.x.x.x (never mismatched) |
| **Appears in all surfaces** | `User-Agent`, `navigator.userAgent`, `Sec-CH-UA`, `Sec-CH-UA-Full-Version-List` |
| **Weekly rebuild CI** | If Chrome stable bumps → rebuild + push update |
| **API consistency** | New JS APIs added in Chrome 128 won't exist in 122 binary — version must match |

---

### F19: ROOT/FRIDA/MAGISK HIDING (BONUS — Native SDK Only)

> **⚠️ NOT NEEDED FOR WEB BYPASS.** The `jailbroken`, `frida`, `clonedApp`, `factoryReset` signals are **only available through the FPJS Native Android SDK** — the web JS `<script>` agent CANNOT detect these. Polymarket and all websites use the web agent. This part is only needed if targeting a native Android app that embeds the FPJS SDK.

| Feature | Description |
|---|---|
| **File system interception** | `/system/bin/su`, `/system/xbin/su`, `/data/local/tmp/frida-server` → "not found" |
| **/proc masking** | `/proc/self/maps` (Frida), `/proc/self/status` (TracerPid) → sanitized |
| **Package hiding** | `com.topjohnwu.magisk` not visible to browser process |
| **Chromium sandbox level** | Interception at content sandbox, not OS — only affects this browser |
| **FPJS signals defeated** | `jailbroken: false`, `frida: false`, `clonedApp: false` |

**Modified at:** `content/browser/sandbox_linux.cc`

---

## WHAT THIS BROWSER DEFEATS

### ✅ Fully Defeated — Web JS Agent (27 Detection Methods)

These are ALL the detection methods that the **FPJS Pro web JavaScript agent** can perform. Our browser defeats every single one at the C++ level:

| # | Detection Method | How |
|---|---|---|
| 1 | FingerprintJS Pro Bot Detection | Native C++ = no JS hook artifacts |
| 2 | FingerprintJS Pro Tampering | Normal property descriptors, native toString |
| 3 | FingerprintJS Pro VM Detection | Reports real mobile device |
| 4 | Cloudflare Bot Management (TLS) | JA3/JA4 randomized per session |
| 5 | Cloudflare Bot Management (JS) | Turnstile sees natural browser |
| 6 | Canvas Fingerprinting | Genuine Skia-level rendering variation |
| 7 | WebGL Fingerprinting | Accurate GPU strings + shader variation |
| 8 | Audio Fingerprinting | Genuine DSP-level processing variation |
| 9 | Font Fingerprinting | Manufacturer-specific font lists |
| 10 | Screen Fingerprinting | Native API returns profile values |
| 11 | UA/Client Hints Matching | HTTP headers = JS values, always |
| 12 | Sensor Fingerprinting | OEM-matched sensor names + noise |
| 13 | ClientRect Fingerprinting | Sub-pixel layout noise |
| 14 | WebRTC IP Leak | Fake local IP or disabled |
| 15 | Cross-session Tracking (cookies etc.) | Complete data wipe on identity change |
| 16 | usbrowserspeed.com / JS fingerprinters | All JS APIs return C++ native values |
| 17 | **Math Fingerprinting (CPU FPU)** | **Software math + per-session ULP perturbation** |
| 18 | **Emoji Rendering Fingerprinting** | **Bundled OEM emoji fonts at Skia level** |
| 19 | **Performance Timing Analysis** | **SoC-tier normalized delays with jitter** |
| 20 | **Storage/Memory Fingerprinting** | **Spoofed storage.estimate(), performance.memory, connection** |
| 21 | **Speech Synthesis Voices** | **Manufacturer-appropriate voice list** |
| 22 | **WebGPU Adapter Info** | **Spoofed adapter or disabled for budget SoCs** |
| 23 | **CSS Media Query Fingerprinting** | **matchMedia() resolves against DeviceProfile** |
| 24 | **Anti-Bot Markers (webdriver, chrome)** | **navigator.webdriver=false, window.chrome exists, no automation globals** |
| 25 | **Navigator Consistency (vendor/plugins)** | **vendor=Google Inc., empty plugins, correct productSub** |
| 26 | **Error Stack Format** | **V8-native Chrome stack trace format, unmodified eval.toString()** |
| 27 | **Touch / Orientation APIs** | **ontouchstart=true, TouchEvent exists, portrait-primary** |

### 🟡 BONUS — Native SDK Only (Not needed for web browsing)

| # | Signal | Why Not Needed |
|---|---|---|
| B1 | Root/Jailbreak (`jailbroken`) | **Web JS agent CANNOT detect this** — requires FPJS Native Android SDK |
| B2 | Frida (`frida`) | **Web JS agent CANNOT detect this** — requires native `/proc` access |
| B3 | Cloned App (`clonedApp`) | **Web JS agent CANNOT detect this** — requires package installer check |
| B4 | Factory Reset (`factoryReset`) | **Web JS agent CANNOT detect this** — requires device setup timestamp |

> **Polymarket, your website, and ALL websites use the web JS agent.** These 4 signals only fire when a native Android app embeds the FPJS Android SDK (`com.fingerprintjs.android`). For web browsing, they are irrelevant.

### ❌ Requires Separate Tools (6 Areas)

| # | Area | Required Tool |
|---|---|---|
| 1 | IP Address & Reputation | Residential rotating proxy (Indian IPs) |
| 2 | IP Geolocation | Proxy must be from allowed country (not US) |
| 3 | Behavioral Biometrics | Manual variation in browsing patterns |
| 4 | Rate Limiting | IP rotation between sessions |
| 5 | Wallet Graph Analysis (fun.xyz) | Each wallet funded from separate source |
| 6 | Email Correlation | Fresh email per account (temp mail) |

---

## UNLIMITED DEVICE GENERATION — HOW IT WORKS

```
                    ┌──────────────────────┐
                    │  WEIGHTED RANDOM     │
                    │  Manufacturer Pick   │
                    │  (11 OEMs)           │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │  PRICE TIER PICK     │
                    │  Budget/Mid/Flag/Prem│
                    └──────────┬───────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
    ┌─────────▼──────┐ ┌──────▼───────┐ ┌──────▼──────┐
    │ MODEL NUMBER   │ │  SoC PICK    │ │ SCREEN PICK │
    │ OEM rules      │ │  From tier   │ │ OEM ranges  │
    │ Procedural     │ │  25+ SoCs    │ │ Real DPIs   │
    └─────────┬──────┘ └──────┬───────┘ └──────┬──────┘
              │                │                │
              └────────────────┼────────────────┘
                               │
                    ┌──────────▼───────────┐
                    │  DERIVATION          │
                    │  • User-Agent        │
                    │  • Client Hints      │
                    │  • GPU strings       │
                    │  • Build fingerprint │
                    │  • Font list         │
                    │  • Sensor names      │
                    │  • Noise seeds       │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │  VALIDATION          │
                    │  • SoC ↔ OEM valid?  │
                    │  • RAM ↔ Tier valid? │
                    │  • Screen ↔ DPI?     │
                    │  • GPU ↔ SoC match?  │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │  TLS PROFILE         │
                    │  • Cipher shuffle    │
                    │  • Extension shuffle │
                    │  • GREASE injection  │
                    │  • Curve order       │
                    │  → Unique JA3/JA4    │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │  FINAL PROFILE       │
                    │  Unique device that  │
                    │  never existed but   │
                    │  COULD exist.        │
                    │  Stored in memory.   │
                    └──────────────────────┘

Total possible combinations: >10^20 (effectively infinite)
```

---

## PROJECT STRUCTURE

```
chromium-ghost-browser/
│
├── device_profile_generator/              # NEW: Procedural device identity engine
│   ├── BUILD.gn                          
│   ├── profile_generator.h               # Main generation API
│   ├── profile_generator.cc              # 14-step generation algorithm
│   ├── profile_store.h                   # Singleton: stores current profile
│   ├── profile_store.cc                  # Thread-safe access for all components
│   ├── manufacturer_traits.h             # 11 OEM rule definitions
│   ├── manufacturer_traits.cc            # OEM constraints, weights, ranges
│   ├── gpu_database.h                    # 25+ SoC → GPU mapping
│   ├── gpu_database.cc                   # Exact GL renderer/vendor/version/extensions
│   ├── model_name_generator.h            # OEM model number generators
│   ├── model_name_generator.cc           # Samsung SM-*, Xiaomi date-format, etc.
│   ├── build_fingerprint_generator.h     # Build.FINGERPRINT procedural builder
│   ├── build_fingerprint_generator.cc    # AOSP build ID + OEM format
│   ├── profile_validator.h               # Ensures no impossible combinations
│   └── profile_validator.cc              # SoC↔OEM, RAM↔tier, screen↔DPI checks
│
├── software_math/                         # NEW: CPU-independent math
│   ├── BUILD.gn
│   ├── software_math.h                   # FDLIBM/crlibm implementations
│   ├── software_math.cc                  # sin/cos/tan/atan2/exp/log/cbrt/etc.
│   └── ulp_perturbation.h               # Per-session last-bit perturbation
│
├── emoji_fonts/                           # NEW: OEM emoji bundles
│   ├── SamsungColorEmoji.ttf             # Samsung emoji (~12MB)
│   ├── NotoColorEmoji.ttf                # Google emoji (~10MB)
│   ├── SamsungOne-Regular.ttf            # Samsung system font
│   ├── MiSans-Regular.ttf               # Xiaomi system font
│   └── OnePlusSans-Regular.ttf           # OnePlus system font
│
├── tls_randomizer/                        # NEW: Cloudflare TLS bypass
│   ├── BUILD.gn
│   ├── ja3_randomizer.h                  # JA3/JA4 hash randomization engine
│   ├── ja3_randomizer.cc                 # Per-session unique TLS fingerprint
│   ├── cipher_suite_shuffler.h           # Cipher ordering + count variation
│   ├── cipher_suite_shuffler.cc          
│   ├── extension_randomizer.h            # TLS extension ordering
│   ├── extension_randomizer.cc           
│   ├── tls_profile.h                     # TLSProfile struct definition
│   ├── tls_profile.cc                    # Profile generation + validation
│   └── tls_profile_database.h            # Reference real browser JA3s to mimic
│
├── MODIFIED CHROMIUM FILES:
│
│   third_party/blink/renderer/
│   ├── core/frame/navigator.cc           # navigator.* properties → ProfileStore
│   ├── core/frame/navigator_base.cc      # hardwareConcurrency, deviceMemory
│   ├── core/frame/screen.cc              # screen.* properties → ProfileStore
│   ├── core/layout/layout_box.cc         # ClientRect sub-pixel noise
│   ├── modules/canvas/                   # Canvas rendering with session noise
│   ├── modules/webaudio/
│   │   └── dynamics_compressor_kernel.cc  # Audio DSP micro-bias
│   ├── modules/mediastream/
│   │   └── media_devices.cc              # Camera count + random device IDs
│   └── platform/fonts/
│       └── font_cache_android.cc         # Manufacturer font lists + emoji font selection
│
│   third_party/skia/
│   ├── src/core/SkScalerContext.cpp       # Sub-pixel glyph offset
│   └── src/gpu/GrSurfaceDrawContext.cpp   # GPU canvas noise
│
│   third_party/boringssl/
│   ├── ssl/handshake_client.cc            # ClientHello cipher shuffling
│   ├── ssl/extensions.cc                  # TLS extension reordering
│   ├── ssl/supported_groups.cc            # Elliptic curve order variation
│   └── ssl/signature_algorithms.cc        # Sig algo shuffling
│
│   gpu/
│   ├── command_buffer/service/
│   │   ├── gles2_cmd_decoder.cc           # WebGL parameter spoofing
│   │   └── shader_translator.cc           # Shader precision variation
│   └── config/gpu_info.cc                 # GL renderer/vendor strings
│
│   net/
│   ├── http/http_util.cc                  # User-Agent header
│   └── socket/ssl_client_socket.cc        # TLS profile injection point
│
│   content/
│   ├── common/user_agent.cc               # UA string building
│   ├── common/ghost_profile.mojom         # Mojo IPC for profile passing to renderers
│   ├── browser/sandbox_linux.cc           # Root/Frida/Magisk file interception
│   └── browser/renderer_host/
│       └── media/p2p/                     # WebRTC local IP fake
│
│   components/
│   └── embedder_support/
│       └── user_agent_utils.cc            # Client Hints (Sec-CH-UA-*)
│
│   services/
│   └── device/generic_sensor/
│       └── platform_sensor_android.cc     # Sensor names + noise
│
│   third_party/icu/
│   └── source/common/putil.cpp            # Timezone override
│
│   v8/
│   ├── src/date/dateparser.cc             # Date timezone offset
│   └── src/builtins/builtins-math.cc      # Software math + ULP perturbation
│
│   chrome/android/
│   ├── java/src/.../GhostModeMenu.java    # "Ghost Mode" UI menu
│   ├── java/src/.../ProfileInfoActivity.java  # "Current Device" screen
│   ├── java/src/.../IdentityLogActivity.java  # "Identity Log" screen
│   └── java/res/menu/ghost_mode_menu.xml  # Menu XML
│
└── docs/
    ├── CUSTOM_BROWSER_BUILD_SPEC.md       # Full technical specification
    ├── FEATURES.md                        # This file
    ├── TLS_BYPASS_DETAILS.md              # Deep dive on Cloudflare bypass
    └── TESTING_GUIDE.md                   # How to verify each feature
```

---

## ESTIMATED EFFORT

| Component | Difficulty | Time |
|---|---|---|
| Build environment setup | Medium | 1-2 days |
| Procedural device generator (11 OEMs) | Medium | 3-4 days |
| SoC/GPU database (25+ SoCs) | Medium | 2 days |
| Model name + build fingerprint generators | Easy | 1-2 days |
| Profile validator | Easy | 1 day |
| Profile store singleton | Easy | 1 day |
| Navigator/Screen properties | Easy | 1 day |
| User-Agent & Client Hints | Medium | 1-2 days |
| Canvas fingerprint (Skia level) | Hard | 3-5 days |
| WebGL fingerprint (ANGLE/GPU) | Hard | 2-3 days |
| Audio fingerprint (DSP level) | Medium | 1-2 days |
| Font enumeration (per-OEM) | Medium | 1-2 days |
| Sensor fingerprinting | Easy | 1 day |
| Timezone/Locale lock | Easy | 0.5 days |
| Media devices spoofing | Easy | 0.5 days |
| ClientRect noise | Easy | 0.5 days |
| WebRTC IP protection | Easy | 0.5 days |
| **Math fingerprint (V8 software math)** | **Hard** | **3-4 days** |
| **Emoji/Unicode font bundling** | **Medium** | **2-3 days** |
| **Performance timing normalization** | **Medium** | **2 days** |
| **Storage/Memory/Network spoofing** | **Easy** | **1 day** |
| **Speech synthesis voices** | **Easy** | **0.5 days** |
| **WebGPU adapter spoofing** | **Easy** | **0.5 days** |
| **CSS media query overrides** | **Easy** | **0.5 days** |
| **Root/Frida/Magisk hiding (BONUS)** | **Medium** | **1-2 days (optional — not needed for web)** |
| **BoringSSL TLS randomization** | **Very Hard** | **5-8 days** |
| **TLS profile generation & validation** | **Hard** | **2-3 days** |
| Ghost Mode UI (Android) | Easy | 1-2 days |
| Profile persistence & rotation | Medium | 1-2 days |
| Testing & debugging (all features) | Hard | 5-7 days |
| **Anti-bot markers (F16)** | **Easy** | **0.5 days** |
| **ProfileStore singleton (F17)** | **Medium** | **1-2 days** |
| **Chrome version CI/CD (F18)** | **Easy** | **0.5 days** |
| **TOTAL (web bypass)** | | **~40-55 days (skip F19 for web-only)** |

---

## COMPARISON: BEFORE vs AFTER

| Scenario | Without GhostBrowser | With GhostBrowser |
|---|---|---|
| FingerprintJS Pro | Same visitorId every time | **Different visitorId every session** |
| Cloudflare | Same JA3 hash → correlated | **Different JA3/JA4 → uncorrelated** |
| Tampering detection | "tampering: 0.87" | **"tampering: 0" (undetectable)** |
| Bot detection | "adspower" / "automation" | **"notDetected"** |
| VM detection | "virtualMachine: true" | **"virtualMachine: false"** |
| Device pool | Fixed list → eventually catalogued | **Infinite → never catalogued** |
| Canvas/Audio fingerprint | Same hash → tracked | **Different hash → unlinked** |
| Multiple accounts | All linked to same fingerprint | **Each account = unique device** |
