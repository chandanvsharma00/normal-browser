// Copyright 2024 Normal Browser Authors. All rights reserved.
// IdentityLogActivity.java — Shows history of all past identities.
//
// LOCATION: chrome/android/java/src/org/nicebrowser/ghost/
//
// Displays a scrollable list of previously used device identities,
// each with a timestamp. Useful for testing/debugging and verifying
// that identities are actually changing.

package org.nicebrowser.ghost;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * Activity showing a log of all past device identities.
 * Each entry shows: timestamp, device model, manufacturer, SoC, build ID.
 * Entries are stored in SharedPreferences as a JSON array.
 */
public class IdentityLogActivity extends Activity {

    private static final String PREFS_NAME = "identity_log_prefs";
    private static final String KEY_LOG_ENTRIES = "log_entries";
    private static final int MAX_LOG_ENTRIES = 100;

    private ListView mListView;
    private IdentityLogAdapter mAdapter;

    // ================================================================
    // Data model
    // ================================================================

    public static class LogEntry {
        public final long timestamp;
        public final String manufacturer;
        public final String model;
        public final String androidVersion;
        public final String socModel;
        public final String buildFingerprint;

        public LogEntry(long timestamp, String manufacturer, String model,
                       String androidVersion, String socModel,
                       String buildFingerprint) {
            this.timestamp = timestamp;
            this.manufacturer = manufacturer;
            this.model = model;
            this.androidVersion = androidVersion;
            this.socModel = socModel;
            this.buildFingerprint = buildFingerprint;
        }

        public org.json.JSONObject toJson() throws org.json.JSONException {
            org.json.JSONObject obj = new org.json.JSONObject();
            obj.put("timestamp", timestamp);
            obj.put("manufacturer", manufacturer);
            obj.put("model", model);
            obj.put("android_version", androidVersion);
            obj.put("soc_model", socModel);
            obj.put("build_fingerprint", buildFingerprint);
            return obj;
        }

        public static LogEntry fromJson(org.json.JSONObject obj)
                throws org.json.JSONException {
            return new LogEntry(
                obj.getLong("timestamp"),
                obj.getString("manufacturer"),
                obj.getString("model"),
                obj.getString("android_version"),
                obj.getString("soc_model"),
                obj.getString("build_fingerprint")
            );
        }
    }

    // ================================================================
    // Static API for logging new identities
    // ================================================================

    /**
     * Record a new identity in the log.
     * Call this from GhostModeManager when a new profile is generated.
     */
    public static void logIdentity(Context context, String profileJson) {
        try {
            org.json.JSONObject profile = new org.json.JSONObject(profileJson);
            LogEntry entry = new LogEntry(
                System.currentTimeMillis(),
                profile.optString("manufacturer", "Unknown"),
                profile.optString("model", "Unknown"),
                profile.optString("android_version", "?"),
                profile.optString("soc_model", "?"),
                profile.optString("build_fingerprint", "?")
            );

            SharedPreferences prefs = context.getSharedPreferences(
                PREFS_NAME, Context.MODE_PRIVATE);
            String existing = prefs.getString(KEY_LOG_ENTRIES, "[]");

            org.json.JSONArray arr = new org.json.JSONArray(existing);
            arr.put(entry.toJson());

            // Trim to max entries
            while (arr.length() > MAX_LOG_ENTRIES) {
                arr.remove(0);
            }

            prefs.edit().putString(KEY_LOG_ENTRIES, arr.toString()).apply();
        } catch (org.json.JSONException e) {
            // Silently fail — logging should never crash the app
        }
    }

    /**
     * Clear the identity log.
     */
    public static void clearLog(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(
            PREFS_NAME, Context.MODE_PRIVATE);
        prefs.edit().putString(KEY_LOG_ENTRIES, "[]").apply();
    }

    // ================================================================
    // Activity lifecycle
    // ================================================================

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Build UI programmatically (no XML layout needed)
        mListView = new ListView(this);
        mListView.setPadding(16, 16, 16, 16);
        setContentView(mListView);
        setTitle("Identity Log");

        loadEntries();
    }

    private void loadEntries() {
        SharedPreferences prefs = getSharedPreferences(
            PREFS_NAME, Context.MODE_PRIVATE);
        String json = prefs.getString(KEY_LOG_ENTRIES, "[]");

        List<LogEntry> entries = new ArrayList<>();
        try {
            org.json.JSONArray arr = new org.json.JSONArray(json);
            // Display newest first
            for (int i = arr.length() - 1; i >= 0; i--) {
                entries.add(LogEntry.fromJson(arr.getJSONObject(i)));
            }
        } catch (org.json.JSONException e) {
            Toast.makeText(this, "Error loading log", Toast.LENGTH_SHORT).show();
        }

        mAdapter = new IdentityLogAdapter(this, entries);
        mListView.setAdapter(mAdapter);

        if (entries.isEmpty()) {
            Toast.makeText(this, "No identity history yet",
                Toast.LENGTH_SHORT).show();
        }
    }

    // ================================================================
    // List adapter
    // ================================================================

    private static class IdentityLogAdapter extends ArrayAdapter<LogEntry> {
        private static final SimpleDateFormat DATE_FORMAT =
            new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US);

        public IdentityLogAdapter(Context context, List<LogEntry> entries) {
            super(context, 0, entries);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            LogEntry entry = getItem(position);
            if (convertView == null) {
                convertView = new TextView(getContext());
                ((TextView) convertView).setTextSize(13);
                convertView.setPadding(16, 12, 16, 12);
            }

            TextView tv = (TextView) convertView;
            String text = String.format(Locale.US,
                "#%d — %s\n%s %s | Android %s\nSoC: %s\n%s",
                getCount() - position,
                DATE_FORMAT.format(new Date(entry.timestamp)),
                entry.manufacturer, entry.model,
                entry.androidVersion,
                entry.socModel,
                entry.buildFingerprint
            );
            tv.setText(text);
            return convertView;
        }
    }
}
