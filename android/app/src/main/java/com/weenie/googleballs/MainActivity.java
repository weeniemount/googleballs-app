package com.weenie.googleballs;

import android.app.Activity;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

public class MainActivity extends Activity {
    private BallsView ballsView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Main Layout
        FrameLayout frameLayout = new FrameLayout(this);

        // Balls View (Background)
        ballsView = new BallsView(this);
        frameLayout.addView(ballsView);

        // Controls Layout (Foreground)
        LinearLayout controlsLayout = new LinearLayout(this);
        controlsLayout.setOrientation(LinearLayout.HORIZONTAL);
        controlsLayout.setGravity(Gravity.CENTER);

        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT);
        params.gravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;
        params.topMargin = 100;

        controlsLayout.setLayoutParams(params);

        CheckBox darkModeCheck = new CheckBox(this);
        darkModeCheck.setText("dark");
        darkModeCheck.setTextColor(0xFF888888); // Gray text
        darkModeCheck.setChecked(true);
        darkModeCheck.setOnCheckedChangeListener((buttonView, isChecked) -> {
            ballsView.setDarkMode(isChecked);
        });

        CheckBox fpsCheck = new CheckBox(this);
        fpsCheck.setText("show fps");
        fpsCheck.setTextColor(0xFF888888);
        fpsCheck.setChecked(false);
        fpsCheck.setOnCheckedChangeListener((buttonView, isChecked) -> {
            ballsView.setShowFps(isChecked);
        });

        controlsLayout.addView(darkModeCheck);
        controlsLayout.addView(fpsCheck);

        frameLayout.addView(controlsLayout);

        setContentView(frameLayout);
    }
}
