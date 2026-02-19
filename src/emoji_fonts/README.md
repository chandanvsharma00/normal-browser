# Emoji Font Assets

This directory should contain the emoji font files needed for realistic
device fingerprint spoofing. These binary font files must be sourced
separately and placed here before building.

## Required Files

| File | Size | Source | Used When |
|------|------|--------|-----------|
| `NotoColorEmoji.ttf` | ~10 MB | [Google Noto Fonts](https://github.com/googlefonts/noto-emoji) | Profile is any non-Samsung manufacturer |
| `SamsungColorEmoji.ttf` | ~12 MB | Extracted from Samsung firmware | Profile claims Samsung manufacturer |
| `NotoColorEmojiCompat.ttf` | ~10 MB | [AndroidX EmojiCompat](https://developer.android.com/develop/ui/views/text-and-emoji/emoji-compat) | Fallback for stock Android claim |

## How to Obtain

### NotoColorEmoji.ttf (Open Source)
```bash
# Download from Google's noto-emoji GitHub releases
wget https://github.com/googlefonts/noto-emoji/raw/main/fonts/NotoColorEmoji.ttf
```

### SamsungColorEmoji.ttf (Proprietary)
Extract from a Samsung Galaxy device:
```bash
adb pull /system/fonts/SamsungColorEmoji.ttf
```

### NotoColorEmojiCompat.ttf (Open Source)
```bash
# Available via AndroidX EmojiCompat library or from Google Maven
# Typically bundled via Gradle dependency:
# implementation "androidx.emoji2:emoji2-bundled:1.4.0"
```

## Build Integration

These files are copied into the APK's `assets/fonts/` directory during the
build. The font loading code in `font_spoofing.cc` (function
`GetEmojiFontPath()`) selects the correct font based on the active
device profile's manufacturer:

- **Samsung profile** → `SamsungColorEmoji.ttf`
  - Loaded from: `/data/data/com.nicebrowser.app/files/fonts/SamsungColorEmoji.ttf`
- **All other profiles** → `NotoColorEmoji.ttf`
  - Loaded from: `/system/fonts/NotoColorEmoji.ttf` (system default)

## GN Build Rule

Add to the browser target's `sources` or `data` in `BUILD.gn`:

```gn
copy("emoji_font_assets") {
  sources = [
    "//nicebrowser/emoji_fonts/NotoColorEmoji.ttf",
    "//nicebrowser/emoji_fonts/SamsungColorEmoji.ttf",
  ]
  outputs = [
    "$root_out_dir/assets/fonts/{{source_file_part}}",
  ]
}
```

## Note

Without these files:
- Emoji rendering will fall back to the system's default emoji font
- Samsung-spoofed profiles will NOT have Samsung emojis, creating a
  detectable inconsistency if the site checks emoji glyph rendering
