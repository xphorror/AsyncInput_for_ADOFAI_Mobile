package com.fizzd.connectedworlds.editorport;

import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

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
    private static native void nativeOnLifecycleReset();
    private static native void nativeOnLifecyclePause();
    private static native void nativeOnLifecycleResume();

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
