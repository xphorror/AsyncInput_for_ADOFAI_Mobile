package com.fizzd.connectedworlds.editorport;

import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;

public class ExtraMenuUnityPlayerActivity extends com.unity3d.player.UnityPlayerActivity {
    private static final String TAG = "ADOFAI_ASYNC_INPUT";
    private static boolean asyncInputLoaded;

    static {
        try {
            System.loadLibrary("AsyncInput");
            asyncInputLoaded = true;
            Log.i(TAG, "loaded libAsyncInput.so");
        } catch (Throwable t) {
            asyncInputLoaded = false;
            Log.e(TAG, "failed to load libAsyncInput.so", t);
        }
    }

    private static native boolean nativeOnTouchEvent(MotionEvent event, int viewWidth, int viewHeight);
    private static native boolean nativeOnKeyEvent(KeyEvent event);
    private static native boolean nativeConfigureAsyncInputFilesDir(String path);
    private static native void nativeOnLifecycleReset();
    private static native void nativeOnLifecyclePause();
    private static native void nativeOnLifecycleResume();

    private static void configureAsyncInputFilesDir(ExtraMenuUnityPlayerActivity activity) {
        File filesDir = activity != null ? activity.getFilesDir() : null;
        String path = filesDir != null ? filesDir.getAbsolutePath() : null;
        if (path == null) {
            Log.e(TAG, "app files directory unavailable");
            return;
        }
        try {
            if (!nativeConfigureAsyncInputFilesDir(path)) {
                Log.e(TAG, "AsyncInput app files directory rejected");
            }
        } catch (Throwable t) {
            Log.e(TAG, "AsyncInput app files directory configuration failed", t);
        }
    }

    @Override
    protected void onCreate(android.os.Bundle savedInstanceState) {
        if (asyncInputLoaded) {
            configureAsyncInputFilesDir(this);
        }
        super.onCreate(savedInstanceState);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (asyncInputLoaded) {
            try {
                View view = getWindow().getDecorView();
                if (nativeOnTouchEvent(event, view.getWidth(), view.getHeight())) {
                    return true;
                }
            } catch (Throwable t) {
                Log.e(TAG, "nativeOnTouchEvent failed", t);
            }
        }
        return super.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (asyncInputLoaded) {
            try {
                if (nativeOnKeyEvent(event)) {
                    return true;
                }
            } catch (Throwable t) {
                Log.e(TAG, "nativeOnKeyEvent failed", t);
            }
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    protected void onPause() {
        if (asyncInputLoaded) {
            try {
                nativeOnLifecyclePause();
            } catch (Throwable t) {
                Log.e(TAG, "nativeOnLifecyclePause failed", t);
            }
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (asyncInputLoaded) {
            try {
                nativeOnLifecycleResume();
            } catch (Throwable t) {
                Log.e(TAG, "nativeOnLifecycleResume failed", t);
            }
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (asyncInputLoaded) {
            try {
                if (hasFocus) {
                    nativeOnLifecycleResume();
                } else {
                    nativeOnLifecyclePause();
                }
            } catch (Throwable t) {
                Log.e(TAG, "nativeOnLifecycle focus update failed", t);
            }
        }
        super.onWindowFocusChanged(hasFocus);
    }
}
