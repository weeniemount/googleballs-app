package com.weenie.googleballs;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;

public class Point {
    public int color;
    public Vector curPos;
    public float friction = 0.8f;
    public Vector originalPos;
    public float radius;
    public float size;
    public float springStrength = 0.1f;
    public Vector targetPos;
    public Vector velocity;

    public Point(float x, float y, float z, float size, String colorHex) {
        this.color = Color.parseColor(colorHex);
        this.curPos = new Vector(x, y, z);
        this.originalPos = new Vector(x, y, z);
        this.radius = size;
        this.size = size;
        this.targetPos = new Vector(x, y, z);
        this.velocity = new Vector(0.0f, 0.0f, 0.0f);
    }

    public void update(float dt) {
        float targetFrameTime = 0.03f;
        float timeScale = dt / targetFrameTime;

        float dx = this.targetPos.x - this.curPos.x;
        float ax = dx * this.springStrength * timeScale;
        this.velocity.x += ax;
        this.velocity.x *= (float) Math.pow(this.friction, timeScale);
        this.curPos.x += this.velocity.x * timeScale;

        float dy = this.targetPos.y - this.curPos.y;
        float ay = dy * this.springStrength * timeScale;
        this.velocity.y += ay;
        this.velocity.y *= (float) Math.pow(this.friction, timeScale);
        this.curPos.y += this.velocity.y * timeScale;

        float dox = this.originalPos.x - this.curPos.x;
        float doy = this.originalPos.y - this.curPos.y;
        float dd = (dox * dox) + (doy * doy);
        float d = (float) Math.sqrt(dd);

        this.targetPos.z = d / 100.0f + 1.0f;
        float dz = this.targetPos.z - this.curPos.z;
        float az = dz * this.springStrength * timeScale;
        this.velocity.z += az;
        this.velocity.z *= (float) Math.pow(this.friction, timeScale);
        this.curPos.z += this.velocity.z * timeScale;

        this.radius = this.size * this.curPos.z;
        if (this.radius < 1.0f) {
            this.radius = 1.0f;
        }
    }

    public void draw(Canvas canvas, Paint paint) {
        paint.setColor(this.color);
        canvas.drawCircle(this.curPos.x, this.curPos.y, this.radius, paint);
    }
}
