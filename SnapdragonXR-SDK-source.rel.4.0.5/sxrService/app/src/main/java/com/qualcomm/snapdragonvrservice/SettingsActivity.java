//-----------------------------------------------------------------------------
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
//-----------------------------------------------------------------------------

package com.qualcomm.snapdragonvrservice;


import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ServiceInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.app.ActionBar;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;

import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.preference.PreferenceFragmentCompat;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;

import java.util.List;

public class SettingsActivity extends AppCompatActivity {

    static final int TAB_INDEX_CONTROLLER_SETTINGS = 0;
    static final int TAB_INDEX_CONTROLLER_DEBUG = 1;
    static final int TAB_INDEX_SVR_CONFIG = 2;
    static final int TAB_COUNT  = 3;

    private SectionsPagerAdapter mSectionsPagerAdapter;
    /**
     * The {@link ViewPager} that will host the section contents.
     */
    private ViewPager mViewPager;
    /**
     * A preference value change listener that updates the preference's summary
     * to reflect its new value.
     */
    private static Preference.OnPreferenceChangeListener sBindPreferenceSummaryToValueListener = new Preference.OnPreferenceChangeListener() {
        @Override
        public boolean onPreferenceChange(Preference preference, Object value) {
            String stringValue = value.toString();

            if (preference instanceof ListPreference) {
                // For list preferences, look up the correct display value in
                // the preference's 'entries' list.
                ListPreference listPreference = (ListPreference) preference;
                int index = listPreference.findIndexOfValue(stringValue);

                // Set the summary to reflect the new value.
                preference.setSummary(
                        index >= 0
                                ? listPreference.getEntries()[index]
                                : null);

            } else {
                // For all other preferences, set the summary to the value's
                // simple string representation.
                preference.setSummary(stringValue);
            }
            return true;
        }
    };

    /**
     * Helper method to determine if the device has an extra-large screen. For
     * example, 10" tablets are extra-large.
     */
    private static boolean isXLargeTablet(Context context) {
        return (context.getResources().getConfiguration().screenLayout
                & Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_XLARGE;
    }

    /**
     * Binds a preference's summary to its value. More specifically, when the
     * preference's value is changed, its summary (line of text below the
     * preference title) is updated to reflect the value. The summary is also
     * immediately updated upon calling this method. The exact display format is
     * dependent on the type of preference.
     *
     * @see #sBindPreferenceSummaryToValueListener
     */

    private static void bindPreferenceSummaryToValue(Preference preference) {
        // Set the listener to watch for value changes.
        preference.setOnPreferenceChangeListener(sBindPreferenceSummaryToValueListener);

        // Trigger the listener immediately with the preference's
        // current value.
        sBindPreferenceSummaryToValueListener.onPreferenceChange(preference,
                PreferenceManager
                        .getDefaultSharedPreferences(preference.getContext())
                        .getString(preference.getKey(), ""));
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSectionsPagerAdapter = new SectionsPagerAdapter(getSupportFragmentManager());

        mViewPager = (ViewPager) findViewById(R.id.container);
        mViewPager.setAdapter(mSectionsPagerAdapter);

        TabLayout tabLayout = (TabLayout) findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(mViewPager);

    }

    /**
     * Set up the {@link android.app.ActionBar}, if the API is available.
     */
    private void setupActionBar() {
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    /**
     * This method stops fragment injection in malicious applications.
     * Make sure to deny any unknown fragments here.
     */
    protected boolean isValidFragment(String fragmentName) {
        return PreferenceFragment.class.getName().equals(fragmentName)
                || ControllerPreferenceFragment.class.getName().equals(fragmentName);
    }

    /**
     * A {@link FragmentPagerAdapter} that returns a fragment corresponding to
     * one of the sections/tabs/pages.
     */
    public class SectionsPagerAdapter extends FragmentPagerAdapter {

        public SectionsPagerAdapter(FragmentManager fm) {
            super(fm);
        }

        @Override
        public Fragment getItem(int position) {

            if (position == TAB_INDEX_CONTROLLER_SETTINGS) {
                return new ControllerPreferenceFragment();
            } else if (position == TAB_INDEX_CONTROLLER_DEBUG) {
                return new ControllerDebugFragment();
            } else if (position == TAB_INDEX_SVR_CONFIG) {
                return new SvrConfigFragment();
            }
            return new Fragment();
        }

        @Override
        public int getCount() {
            return TAB_COUNT;
        }

        @Override
        public CharSequence getPageTitle(int position) {
            switch (position) {
                case TAB_INDEX_CONTROLLER_SETTINGS:
                    return getString(R.string.page_title_controller_settings);
                case TAB_INDEX_CONTROLLER_DEBUG:
                    return getString(R.string.page_title_controller_debug);
                case TAB_INDEX_SVR_CONFIG:
                    return getString(R.string.page_title_svr_config);
            }
            return "";
        }
    }

    public static class ControllerPreferenceFragment extends PreferenceFragmentCompat
            implements LoadAppsAsyncTask.Listener {
        ListPreference listPreference;

        // ------------------------------------------------------------------------------------------
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
        }

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            // Load the preferences from an XML resource
            setPreferencesFromResource(R.xml.pref_controller, rootKey);
            listPreference = (ListPreference) findPreference(PREF_CONTROLLER_PROVIDER);
            new LoadAppsAsyncTask(this).execute(getActivity());
            bindPreferenceSummaryToValue(findPreference(PREF_CONTROLLER_PROVIDER));
            bindPreferenceSummaryToValue(findPreference(PREF_CONTROLLER_RINGBUFFER_SIZE));
        }

        //------------------------------------------------------------------------------------------
        @Override
        public void onUpdate(Bundle appInfo) {

        }

        // ------------------------------------------------------------------------------------------
        @Override
        public void onComplete(List<ServiceInfo> listOfApps) {
            CharSequence keys[] = new CharSequence[1 + listOfApps.size()];
            CharSequence values[] = new CharSequence[1 + listOfApps.size()];
            keys[0] = "None";
            values[0] = "None";

            if ((listOfApps.size() > 0)) {
                for (int i = 0; i < listOfApps.size(); i++) {
                    keys[i + 1] = listOfApps.get(i).loadLabel(getActivity().getPackageManager());
                    values[i + 1] = listOfApps.get(i).packageName + "#" + listOfApps.get(i).name;
                }
            }

            listPreference.setEntries(keys);
            listPreference.setEntryValues(values);
            SharedPreferences sharedPrefs = PreferenceManager
                    .getDefaultSharedPreferences(getContext());
            String value = sharedPrefs.getString(SettingsActivity.PREF_CONTROLLER_PROVIDER, null);
            if (value != null && values != null) {
                int index = -1;
                for (int i = 0; i < values.length; i++) {
                    if (values[i].toString().equals(value)) {
                        index = i;
                        break;
                    }
                }
                if (index != -1) {
                    listPreference.setSummary(keys[index]);
                }
            }

        }
    }

    public static final String PREF_CONTROLLER_PROVIDER = "ControllerProvider";
    public static final String PREF_CONTROLLER_RINGBUFFER_SIZE = "RingBufferSize";
}
