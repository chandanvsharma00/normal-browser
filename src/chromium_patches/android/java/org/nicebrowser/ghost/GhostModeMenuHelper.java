// Copyright 2024 Normal Browser Authors. All rights reserved.
// GhostModeMenuHelper.java — Adds Ghost Mode options to Chrome's menu.
//
// LOCATION: chrome/android/java/src/org/nicebrowser/ghost/
//
// Adds to Chrome's 3-dot overflow menu:
//   - "Ghost Mode" toggle (on/off switch)
//   - "New Identity" button (regenerate everything)
//   - "Rotate Seeds" button (lighter rotation)
//   - Visual indicator when Ghost Mode is active

package org.nicebrowser.ghost;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.EditText;
import android.widget.Toast;

public class GhostModeMenuHelper {

    // Menu item IDs (use high IDs to avoid collision with Chrome's menu)
    public static final int MENU_GHOST_MODE_TOGGLE = 90001;
    public static final int MENU_NEW_IDENTITY = 90002;
    public static final int MENU_ROTATE_SEEDS = 90003;
    public static final int MENU_GHOST_INFO = 90004;
    public static final int MENU_SHARE_PROFILE = 90005;
    public static final int MENU_IMPORT_PROFILE = 90006;
    public static final int MENU_AUTO_ROTATE = 90007;

    /**
     * Add Ghost Mode items to Chrome's overflow menu.
     * Called from ChromeAppMenuPropertiesDelegate.prepareMenu()
     */
    public static void addGhostModeMenuItems(Menu menu, Context context) {
        GhostModeManager manager = GhostModeManager.getInstance(context);

        // Separator-like group
        menu.add(Menu.NONE, MENU_GHOST_MODE_TOGGLE, 1000,
                manager.isGhostModeActive() ?
                    "\uD83D\uDC7B Ghost Mode: ON" :
                    "\uD83D\uDC7B Ghost Mode: OFF")
            .setCheckable(true)
            .setChecked(manager.isGhostModeActive());

        if (manager.isGhostModeActive()) {
            menu.add(Menu.NONE, MENU_NEW_IDENTITY, 1001,
                    "\uD83D\uDD04 New Identity");

            menu.add(Menu.NONE, MENU_ROTATE_SEEDS, 1002,
                    "\uD83C\uDFB2 Rotate Fingerprint Seeds");

            menu.add(Menu.NONE, MENU_GHOST_INFO, 1003,
                    "\u2139\uFE0F Current Identity Info");

            menu.add(Menu.NONE, MENU_SHARE_PROFILE, 1004,
                    "\uD83D\uDCE4 Share Profile");

            menu.add(Menu.NONE, MENU_IMPORT_PROFILE, 1005,
                    "\uD83D\uDCE5 Import Profile");

            AutoRotateManager autoRotate = AutoRotateManager.getInstance(context);
            menu.add(Menu.NONE, MENU_AUTO_ROTATE, 1006,
                    autoRotate.isEnabled() ?
                        "\u23F0 Auto-Rotate: " + autoRotate.getIntervalMinutes() + "min" :
                        "\u23F0 Auto-Rotate: OFF")
                .setCheckable(true)
                .setChecked(autoRotate.isEnabled());
        }
    }

    /**
     * Handle Ghost Mode menu item clicks.
     * Called from ChromeAppMenuPropertiesDelegate.onMenuItemSelected()
     * @return true if the item was handled
     */
    public static boolean handleMenuItemClick(int itemId, Context context) {
        GhostModeManager manager = GhostModeManager.getInstance(context);

        switch (itemId) {
            case MENU_GHOST_MODE_TOGGLE:
                boolean newState = !manager.isGhostModeActive();
                manager.setGhostModeActive(newState);
                Toast.makeText(context,
                    newState ? "Ghost Mode enabled" : "Ghost Mode disabled",
                    Toast.LENGTH_SHORT).show();
                return true;

            case MENU_NEW_IDENTITY:
                showNewIdentityConfirmation(context, manager);
                return true;

            case MENU_ROTATE_SEEDS:
                manager.rotateSessionSeeds();
                Toast.makeText(context,
                    "Fingerprint seeds rotated",
                    Toast.LENGTH_SHORT).show();
                return true;

            case MENU_GHOST_INFO:
                showIdentityInfo(context);
                return true;

            case MENU_SHARE_PROFILE:
                shareProfile(context);
                return true;

            case MENU_IMPORT_PROFILE:
                showImportProfileDialog(context);
                return true;

            case MENU_AUTO_ROTATE:
                showAutoRotateDialog(context);
                return true;

            default:
                return false;
        }
    }

    /**
     * Show confirmation dialog before generating new identity.
     * New Identity clears ALL data — user should be warned.
     */
    private static void showNewIdentityConfirmation(
            Context context, GhostModeManager manager) {
        new AlertDialog.Builder(context)
            .setTitle("New Identity")
            .setMessage(
                "This will:\n\n" +
                "• Generate a completely new device identity\n" +
                "• Clear ALL cookies, cache, and browsing data\n" +
                "• Reset all site permissions\n" +
                "• Close all tabs\n\n" +
                "You will appear as a brand new device to all websites.\n\n" +
                "Continue?")
            .setPositiveButton("New Identity", (dialog, which) -> {
                manager.generateNewIdentity();
                Toast.makeText(context,
                    "New identity generated! All data cleared.",
                    Toast.LENGTH_LONG).show();
                // TODO: Close all tabs and open a new one
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    /**
     * Show current identity info for debugging / user awareness.
     */
    private static void showIdentityInfo(Context context) {
        GhostModeManager manager = GhostModeManager.getInstance(context);
        // Get profile info from native side
        String info = getIdentityInfoString(context);

        new AlertDialog.Builder(context)
            .setTitle("Current Identity")
            .setMessage(info)
            .setPositiveButton("OK", null)
            .setNeutralButton("Copy", (dialog, which) -> {
                android.content.ClipboardManager clipboard =
                    (android.content.ClipboardManager)
                        context.getSystemService(Context.CLIPBOARD_SERVICE);
                android.content.ClipData clip =
                    android.content.ClipData.newPlainText("Identity Info", info);
                clipboard.setPrimaryClip(clip);
                Toast.makeText(context, "Copied", Toast.LENGTH_SHORT).show();
            })
            .show();
    }

    private static String getIdentityInfoString(Context context) {
        try {
            String json = GhostModeManagerJni.get().nativeGetProfileJson();
            if (json == null || json.isEmpty()) {
                return "Profile not yet generated.";
            }
            org.json.JSONObject obj = new org.json.JSONObject(json);

            StringBuilder sb = new StringBuilder();
            sb.append("Device: ").append(obj.optString("manufacturer", "?"))
              .append(" ").append(obj.optString("model", "?")).append("\n");
            sb.append("Model: ").append(obj.optString("marketing_name",
                      obj.optString("model", "?"))).append("\n");
            sb.append("Android: ").append(obj.optString("android_version", "?"))
              .append(" (API ").append(obj.optInt("sdk_int", 0)).append(")\n");
            sb.append("SoC: ").append(obj.optString("soc_model", "?")).append("\n");
            sb.append("GPU: ").append(obj.optString("gl_renderer", "?")).append("\n");
            sb.append("Screen: ").append(obj.optInt("screen_width", 0))
              .append("x").append(obj.optInt("screen_height", 0))
              .append(" @ ").append(obj.optDouble("density", 0))
              .append("x\n");
            sb.append("RAM: ").append(obj.optInt("total_ram_mb", 0))
              .append(" MB\n");
            sb.append("Storage: ").append(obj.optInt("total_storage_gb", 0))
              .append(" GB\n\n");

            // Build fingerprint
            sb.append("Build: ").append(obj.optString("build_fingerprint", "?"))
              .append("\n");
            sb.append("Security Patch: ").append(
                obj.optString("security_patch", "?")).append("\n");

            return sb.toString();
        } catch (Exception e) {
            return "Error reading profile: " + e.getMessage();
        }
    }

    // ================================================================
    // Share Profile — export current profile as JSON via Android share
    // ================================================================
    private static void shareProfile(Context context) {
        try {
            String json = GhostModeManagerJni.get().nativeGetProfileJson();
            if (json == null || json.isEmpty()) {
                Toast.makeText(context, "No profile to share",
                    Toast.LENGTH_SHORT).show();
                return;
            }

            Intent shareIntent = new Intent(Intent.ACTION_SEND);
            shareIntent.setType("application/json");
            shareIntent.putExtra(Intent.EXTRA_TEXT, json);
            shareIntent.putExtra(Intent.EXTRA_SUBJECT, "Normal Browser Profile");
            context.startActivity(
                Intent.createChooser(shareIntent, "Share Profile"));
        } catch (Exception e) {
            Toast.makeText(context, "Share failed: " + e.getMessage(),
                Toast.LENGTH_SHORT).show();
        }
    }

    // ================================================================
    // Import Profile — load a JSON profile from clipboard/paste
    // ================================================================
    private static void showImportProfileDialog(Context context) {
        EditText input = new EditText(context);
        input.setHint("Paste profile JSON here...");
        input.setMinLines(5);
        input.setMaxLines(10);

        new AlertDialog.Builder(context)
            .setTitle("Import Profile")
            .setMessage("Paste a previously exported profile JSON:")
            .setView(input)
            .setPositiveButton("Import", (dialog, which) -> {
                String json = input.getText().toString().trim();
                if (json.isEmpty()) {
                    Toast.makeText(context, "No JSON entered",
                        Toast.LENGTH_SHORT).show();
                    return;
                }
                importProfileFromJson(context, json);
            })
            .setNeutralButton("From Clipboard", (dialog, which) -> {
                android.content.ClipboardManager clipboard =
                    (android.content.ClipboardManager)
                        context.getSystemService(Context.CLIPBOARD_SERVICE);
                if (clipboard != null && clipboard.hasPrimaryClip()) {
                    CharSequence text = clipboard.getPrimaryClip()
                        .getItemAt(0).getText();
                    if (text != null) {
                        importProfileFromJson(context, text.toString().trim());
                        return;
                    }
                }
                Toast.makeText(context, "Clipboard empty",
                    Toast.LENGTH_SHORT).show();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private static void importProfileFromJson(Context context, String json) {
        try {
            // Validate it's valid JSON
            new org.json.JSONObject(json);

            // Store to SharedPreferences
            GhostModeManager manager = GhostModeManager.getInstance(context);
            manager.importProfileJson(json);

            Toast.makeText(context,
                "Profile imported! Restart browser for full effect.",
                Toast.LENGTH_LONG).show();
        } catch (org.json.JSONException e) {
            Toast.makeText(context,
                "Invalid JSON: " + e.getMessage(),
                Toast.LENGTH_LONG).show();
        }
    }

    // ================================================================
    // Auto-Rotate dialog — configure interval
    // ================================================================
    private static void showAutoRotateDialog(Context context) {
        AutoRotateManager autoRotate = AutoRotateManager.getInstance(context);
        String[] options = {"OFF", "5 minutes", "15 minutes",
                           "30 minutes", "60 minutes"};
        int[] intervals = {0, 5, 15, 30, 60};

        // Find current selection
        int currentIdx = 0;
        if (autoRotate.isEnabled()) {
            int current = autoRotate.getIntervalMinutes();
            for (int i = 1; i < intervals.length; i++) {
                if (intervals[i] == current) { currentIdx = i; break; }
            }
        }

        new AlertDialog.Builder(context)
            .setTitle("Auto-Rotate Identity")
            .setSingleChoiceItems(options, currentIdx, (dialog, which) -> {
                if (which == 0) {
                    autoRotate.setEnabled(false);
                    Toast.makeText(context, "Auto-rotate disabled",
                        Toast.LENGTH_SHORT).show();
                } else {
                    autoRotate.setIntervalMinutes(intervals[which]);
                    autoRotate.setEnabled(true);
                    Toast.makeText(context,
                        "Auto-rotate: every " + intervals[which] + " min",
                        Toast.LENGTH_SHORT).show();
                }
                dialog.dismiss();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }
}
