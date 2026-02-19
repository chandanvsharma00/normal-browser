// Copyright 2024 Normal Browser Authors. All rights reserved.
// AutoRotateManager.java — Automatic identity rotation on a timer.
//
// LOCATION: chrome/android/java/src/org/nicebrowser/ghost/
//
// Provides configurable auto-rotation: every N minutes (5/15/30/60),
// the browser automatically generates a new device identity and clears
// session data. This prevents long-lived fingerprint correlation.

package org.nicebrowser.ghost;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

public class AutoRotateManager {

    private static final String TAG = "AutoRotate";
    private static final String PREFS_NAME = "ghost_mode_prefs";
    private static final String KEY_AUTO_ROTATE_ENABLED = "auto_rotate_enabled";
    private static final String KEY_AUTO_ROTATE_INTERVAL_MIN = "auto_rotate_interval_min";

    // Supported intervals in minutes.
    public static final int INTERVAL_5_MIN  = 5;
    public static final int INTERVAL_15_MIN = 15;
    public static final int INTERVAL_30_MIN = 30;
    public static final int INTERVAL_60_MIN = 60;

    private static AutoRotateManager sInstance;

    private final Context mContext;
    private final SharedPreferences mPrefs;
    private final Handler mHandler;
    private boolean mEnabled;
    private int mIntervalMinutes;
    private boolean mTimerRunning;

    // Listener interface for UI updates when auto-rotation fires.
    public interface OnAutoRotateListener {
        void onAutoRotate();
    }

    private OnAutoRotateListener mListener;

    public static AutoRotateManager getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new AutoRotateManager(context.getApplicationContext());
        }
        return sInstance;
    }

    private AutoRotateManager(Context context) {
        mContext = context;
        mPrefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        mHandler = new Handler(Looper.getMainLooper());
        mEnabled = mPrefs.getBoolean(KEY_AUTO_ROTATE_ENABLED, false);
        mIntervalMinutes = mPrefs.getInt(KEY_AUTO_ROTATE_INTERVAL_MIN, INTERVAL_30_MIN);
    }

    // ================================================================
    // Public API
    // ================================================================

    public void setListener(OnAutoRotateListener listener) {
        mListener = listener;
    }

    public boolean isEnabled() {
        return mEnabled;
    }

    public int getIntervalMinutes() {
        return mIntervalMinutes;
    }

    /**
     * Enable or disable auto-rotation.
     * When enabled, starts a repeating timer that generates a new identity.
     */
    public void setEnabled(boolean enabled) {
        mEnabled = enabled;
        mPrefs.edit().putBoolean(KEY_AUTO_ROTATE_ENABLED, enabled).apply();
        if (enabled) {
            startTimer();
        } else {
            stopTimer();
        }
        Log.i(TAG, "Auto-rotate " + (enabled ? "enabled" : "disabled")
                + " (interval=" + mIntervalMinutes + "min)");
    }

    /**
     * Set the rotation interval in minutes.
     * @param minutes One of INTERVAL_5_MIN, INTERVAL_15_MIN,
     *                INTERVAL_30_MIN, INTERVAL_60_MIN.
     */
    public void setIntervalMinutes(int minutes) {
        if (minutes != INTERVAL_5_MIN && minutes != INTERVAL_15_MIN
                && minutes != INTERVAL_30_MIN && minutes != INTERVAL_60_MIN) {
            Log.w(TAG, "Invalid interval: " + minutes + ". Using 30.");
            minutes = INTERVAL_30_MIN;
        }
        mIntervalMinutes = minutes;
        mPrefs.edit().putInt(KEY_AUTO_ROTATE_INTERVAL_MIN, minutes).apply();

        // Restart timer with new interval if currently running.
        if (mEnabled && mTimerRunning) {
            stopTimer();
            startTimer();
        }
    }

    /**
     * Call from Activity.onResume() to restart timer if enabled.
     */
    public void onResume() {
        if (mEnabled && !mTimerRunning) {
            startTimer();
        }
    }

    /**
     * Call from Activity.onPause() to pause the timer (save battery).
     */
    public void onPause() {
        if (mTimerRunning) {
            stopTimer();
        }
    }

    // ================================================================
    // Timer implementation
    // ================================================================

    private final Runnable mRotateRunnable = new Runnable() {
        @Override
        public void run() {
            if (!mEnabled) return;

            Log.i(TAG, "Auto-rotating identity (interval="
                    + mIntervalMinutes + "min)");

            // Delegate to GhostModeManager for the actual rotation.
            GhostModeManager ghost = GhostModeManager.getInstance(mContext);
            ghost.generateNewIdentity();

            // Notify listener (e.g., UI refresh).
            if (mListener != null) {
                mListener.onAutoRotate();
            }

            // Schedule next rotation.
            mHandler.postDelayed(this, mIntervalMinutes * 60L * 1000L);
        }
    };

    private void startTimer() {
        if (mTimerRunning) return;
        mTimerRunning = true;
        long delayMs = mIntervalMinutes * 60L * 1000L;
        mHandler.postDelayed(mRotateRunnable, delayMs);
        Log.d(TAG, "Timer started. Next rotation in " + mIntervalMinutes + " min.");
    }

    private void stopTimer() {
        mTimerRunning = false;
        mHandler.removeCallbacks(mRotateRunnable);
        Log.d(TAG, "Timer stopped.");
    }
}
