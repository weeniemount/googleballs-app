package com.weenie.googleballs;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.View;

public class BallsView extends View {
    private PointCollection collection;
    private Paint paint;
    private long lastTime = 0;
    private boolean initialized = false;

    private boolean darkMode = true;
    private boolean showFps = false;
    private int frameCount = 0;
    private long lastFpsTime = 0;
    private float currentFps = 0;

    public BallsView(Context context) {
        super(context);
        paint = new Paint();
        paint.setStyle(Paint.Style.FILL);
        paint.setAntiAlias(true);
        updateBackgroundColor();
    }

    public void setDarkMode(boolean darkMode) {
        this.darkMode = darkMode;
        updateBackgroundColor();
        invalidate();
    }

    public void setShowFps(boolean showFps) {
        this.showFps = showFps;
        invalidate();
    }

    private void updateBackgroundColor() {
        if (darkMode) {
            setBackgroundColor(Color.parseColor("#1a1a1a"));
        } else {
            setBackgroundColor(Color.WHITE);
        }
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        if (!initialized) {
            collection = new PointCollection();
            collection.start(w, h);
            initialized = true;
        } else {
            // Recenter
            float oldOffX = (oldw / 2.0f - 180);
            float oldOffY = (oldh / 2.0f - 65);
            float newOffX = (w / 2.0f - 180);
            float newOffY = (h / 2.0f - 65);
            float diffX = newOffX - oldOffX;
            float diffY = newOffY - oldOffY;

            for (Point p : collection.points) {
                p.originalPos.x += diffX;
                p.originalPos.y += diffY;
                // Reset to original position on resize, matching web behavior
                p.curPos.x = p.originalPos.x;
                p.curPos.y = p.originalPos.y;
            }
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (collection == null)
            return;

        long now = System.nanoTime();
        if (lastTime == 0)
            lastTime = now;

        float dt = (now - lastTime) / 1_000_000_000.0f;
        lastTime = now;

        // Cap dt to avoid explosion on lag
        if (dt > 0.1f)
            dt = 0.1f;

        collection.update(dt);
        collection.draw(canvas, paint);

        // FPS Counter logic
        if (showFps) {
            frameCount++;
            if (now - lastFpsTime >= 1_000_000_000) {
                currentFps = frameCount;
                frameCount = 0;
                lastFpsTime = now;
            }
            paint.setColor(darkMode ? Color.WHITE : Color.BLACK);
            paint.setTextSize(40);
            canvas.drawText("FPS: " + (int) currentFps, 20, 120, paint);
        }

        postInvalidateOnAnimation();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (collection == null)
            return false;

        float x = event.getX();
        float y = event.getY();

        collection.mousePos.set(x, y);

        // Keep updating as long as user is interacting, though loop is continuous
        // anyway
        // For web parity, mouse move updates specific target logic.
        // We set mousePos, update(dt) handles the rest.

        // Return true to continue consuming events (movement)
        return true;
    }
}
