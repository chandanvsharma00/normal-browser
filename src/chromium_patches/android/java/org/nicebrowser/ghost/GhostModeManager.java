// Copyright 2024 Normal Browser Authors. All rights reserved.
// GhostModeManager.java — Android Java component for Ghost Mode UI.
//
// LOCATION: chrome/android/java/src/org/nicebrowser/ghost/
//
// Provides:
//   1. Ghost Mode toggle in Chrome menu
//   2. "New Identity" button to regenerate device profile
//   3. Complete data clearing on identity rotation
//   4. Profile persistence via SharedPreferences
//   5. WebRTC IP leak prevention toggle

package org.nicebrowser.ghost;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.webkit.CookieManager;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.browsing_data.BrowsingDataBridge;
import org.chromium.chrome.browser.browsing_data.BrowsingDataType;
import org.chromium.chrome.browser.browsing_data.TimePeriod;

@JNINamespace("normal_browser")
public class GhostModeManager {

    private static final String PREFS_NAME = "ghost_mode_prefs";
    private static final String KEY_GHOST_MODE_ACTIVE = "ghost_mode_active";
    private static final String KEY_DEVICE_PROFILE_JSON = "device_profile_json";
    private static final String KEY_PROFILE_SEED = "profile_seed";
    private static final String KEY_FIRST_LAUNCH = "first_launch_done";

    private static GhostModeManager sInstance;
    private final Context mContext;
    private final SharedPreferences mPrefs;
    private boolean mGhostModeActive;

    // Singleton
    public static GhostModeManager getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new GhostModeManager(context.getApplicationContext());
        }
        return sInstance;
    }

    private GhostModeManager(Context context) {
        mContext = context;
        mPrefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        mGhostModeActive = mPrefs.getBoolean(KEY_GHOST_MODE_ACTIVE, true);

        // First launch: generate initial profile
        if (!mPrefs.getBoolean(KEY_FIRST_LAUNCH, false)) {
            generateNewIdentity();
            mPrefs.edit().putBoolean(KEY_FIRST_LAUNCH, true).apply();
        }
    }

    // ================================================================
    // Ghost Mode Toggle
    // ================================================================

    public boolean isGhostModeActive() {
        return mGhostModeActive;
    }

    public void setGhostModeActive(boolean active) {
        mGhostModeActive = active;
        mPrefs.edit().putBoolean(KEY_GHOST_MODE_ACTIVE, active).apply();
        GhostModeManagerJni.get().nativeSetGhostModeActive(active);
    }

    // ================================================================
    // New Identity — Complete rotation
    // ================================================================

    public void generateNewIdentity() {
        // Step 1: Clear ALL browsing data
        clearAllBrowsingData();

        // Step 2: Generate new device profile (C++ side)
        GhostModeManagerJni.get().nativeGenerateNewProfile();

        // Step 3: Get the new profile JSON and persist it
        String profileJson = GhostModeManagerJni.get().nativeGetProfileJson();
        mPrefs.edit().putString(KEY_DEVICE_PROFILE_JSON, profileJson).apply();

        // Step 3b: Log the new identity for debugging/history
        IdentityLogActivity.logIdentity(mContext, profileJson);

        // Step 4: Rotate TLS session (new JA3/JA4 hash)
        // This happens automatically on new profile generation

        // Step 5: Notify all open tabs to reload with new identity
        GhostModeManagerJni.get().nativeNotifyProfileChanged();
    }

    // ================================================================
    // Import Profile from JSON
    // ================================================================

    public void importProfileJson(String json) {
        mPrefs.edit().putString(KEY_DEVICE_PROFILE_JSON, json).apply();
        GhostModeManagerJni.get().nativeImportProfileJson(json);
        GhostModeManagerJni.get().nativeNotifyProfileChanged();
    }

    // ================================================================
    // Rotate Session Seeds Only (lighter than full identity change)
    // Keeps the same device but changes canvas/audio/math fingerprints
    // ================================================================

    public void rotateSessionSeeds() {
        GhostModeManagerJni.get().nativeRotateSessionSeeds();
        String profileJson = GhostModeManagerJni.get().nativeGetProfileJson();
        mPrefs.edit().putString(KEY_DEVICE_PROFILE_JSON, profileJson).apply();
    }

    // ================================================================
    // Complete Data Clearing
    // ================================================================

    private void clearAllBrowsingData() {
        // Clear via Chromium's BrowsingDataBridge
        // This clears: cookies, cache, history, passwords, autofill,
        //              site settings, hosted app data, service workers

        // Clear cookies immediately via CookieManager
        CookieManager cookieManager = CookieManager.getInstance();
        cookieManager.removeAllCookies(null);
        cookieManager.flush();

        // Clear all browsing data types
        int[] dataTypes = new int[] {
            BrowsingDataType.HISTORY,
            BrowsingDataType.CACHE,
            BrowsingDataType.COOKIES,
            BrowsingDataType.PASSWORDS,
            BrowsingDataType.FORM_DATA,
            BrowsingDataType.SITE_SETTINGS,
            BrowsingDataType.DOWNLOADS,
            BrowsingDataType.MEDIA_LICENSES,
        };

        // Clear from all time
        BrowsingDataBridge.getInstance().clearBrowsingData(
            null,  // callback
            Profile.getLastUsedRegularProfile(),
            dataTypes,
            TimePeriod.ALL_TIME
        );

        // Clear WebView data
        android.webkit.WebStorage.getInstance().deleteAllData();

        // Clear localStorage, IndexedDB, WebSQL
        clearWebStorageData();

        // Clear Cache2 and HTTP cache
        clearHttpCache();

        // Clear service workers
        clearServiceWorkers();
    }

    private void clearWebStorageData() {
        // Delete the app's WebView storage directory
        java.io.File webStorageDir = new java.io.File(
            mContext.getDir("webview", Context.MODE_PRIVATE).getPath());
        deleteRecursive(webStorageDir);

        // Also clear via native
        GhostModeManagerJni.get().nativeClearWebStorage();
    }

    private void clearHttpCache() {
        // Delete cache directory
        java.io.File cacheDir = mContext.getCacheDir();
        deleteRecursive(cacheDir);

        // Also clear via native
        GhostModeManagerJni.get().nativeClearHttpCache();
    }

    private void clearServiceWorkers() {
        GhostModeManagerJni.get().nativeClearServiceWorkers();
    }

    private static void deleteRecursive(java.io.File fileOrDirectory) {
        if (fileOrDirectory == null || !fileOrDirectory.exists()) return;
        if (fileOrDirectory.isDirectory()) {
            java.io.File[] children = fileOrDirectory.listFiles();
            if (children != null) {
                for (java.io.File child : children) {
                    deleteRecursive(child);
                }
            }
        }
        fileOrDirectory.delete();
    }

    // ================================================================
    // Profile persistence — load on startup
    // ================================================================

    @CalledByNative
    public static String getStoredProfileJson(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(
            PREFS_NAME, Context.MODE_PRIVATE);
        return prefs.getString(KEY_DEVICE_PROFILE_JSON, "");
    }

    @CalledByNative
    public static boolean hasStoredProfile(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(
            PREFS_NAME, Context.MODE_PRIVATE);
        return prefs.contains(KEY_DEVICE_PROFILE_JSON);
    }

    // ================================================================
    // JNI Bridge to C++ DeviceProfileStore
    // ================================================================

    @NativeMethods
    interface Natives {
        void nativeSetGhostModeActive(boolean active);
        void nativeGenerateNewProfile();
        String nativeGetProfileJson();
        void nativeNotifyProfileChanged();
        void nativeRotateSessionSeeds();
        void nativeImportProfileJson(String json);
        void nativeClearWebStorage();
        void nativeClearHttpCache();
        void nativeClearServiceWorkers();
    }
}
