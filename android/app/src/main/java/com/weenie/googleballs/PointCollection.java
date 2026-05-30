package com.weenie.googleballs;

import android.graphics.Canvas;
import android.graphics.Paint;
import java.util.ArrayList;
import java.util.List;

public class PointCollection {
    public Vector mousePos = new Vector(0, 0);
    public List<Point> points = new ArrayList<>();

    public PointCollection() {
        // Initialize points with data from original web version
        points.add(new Point(202, 78, 0, 9, "#ed9d33"));
        points.add(new Point(348, 83, 0, 9, "#d44d61"));
        points.add(new Point(256, 69, 0, 9, "#4f7af2"));
        points.add(new Point(214, 59, 0, 9, "#ef9a1e"));
        points.add(new Point(265, 36, 0, 9, "#4976f3"));
        points.add(new Point(300, 78, 0, 9, "#269230"));
        points.add(new Point(294, 59, 0, 9, "#1f9e2c"));
        points.add(new Point(45, 88, 0, 9, "#1c48dd"));
        points.add(new Point(268, 52, 0, 9, "#2a56ea"));
        points.add(new Point(73, 83, 0, 9, "#3355d8"));
        points.add(new Point(294, 6, 0, 9, "#36b641"));
        points.add(new Point(235, 62, 0, 9, "#2e5def"));
        points.add(new Point(353, 42, 0, 8, "#d53747"));
        points.add(new Point(336, 52, 0, 8, "#eb676f"));
        points.add(new Point(208, 41, 0, 8, "#f9b125"));
        points.add(new Point(321, 70, 0, 8, "#de3646"));
        points.add(new Point(8, 60, 0, 8, "#2a59f0"));
        points.add(new Point(180, 81, 0, 8, "#eb9c31"));
        points.add(new Point(146, 65, 0, 8, "#c41731"));
        points.add(new Point(145, 49, 0, 8, "#d82038"));
        points.add(new Point(246, 34, 0, 8, "#5f8af8"));
        points.add(new Point(169, 69, 0, 8, "#efa11e"));
        points.add(new Point(273, 99, 0, 8, "#2e55e2"));
        points.add(new Point(248, 120, 0, 8, "#4167e4"));
        points.add(new Point(294, 41, 0, 8, "#0b991a"));
        points.add(new Point(267, 114, 0, 8, "#4869e3"));
        points.add(new Point(78, 67, 0, 8, "#3059e3"));
        points.add(new Point(294, 23, 0, 8, "#10a11d"));
        points.add(new Point(117, 83, 0, 8, "#cf4055"));
        points.add(new Point(137, 80, 0, 8, "#cd4359"));
        points.add(new Point(14, 71, 0, 8, "#2855ea"));
        points.add(new Point(331, 80, 0, 8, "#ca273c"));
        points.add(new Point(25, 82, 0, 8, "#2650e1"));
        points.add(new Point(233, 46, 0, 8, "#4a7bf9"));
        points.add(new Point(73, 13, 0, 8, "#3d65e7"));
        points.add(new Point(327, 35, 0, 6, "#f47875"));
        points.add(new Point(319, 46, 0, 6, "#f36764"));
        points.add(new Point(256, 81, 0, 6, "#1d4eeb"));
        points.add(new Point(244, 88, 0, 6, "#698bf1"));
        points.add(new Point(194, 32, 0, 6, "#fac652"));
        points.add(new Point(97, 56, 0, 6, "#ee5257"));
        points.add(new Point(105, 75, 0, 6, "#cf2a3f"));
        points.add(new Point(42, 4, 0, 6, "#5681f5"));
        points.add(new Point(10, 27, 0, 6, "#4577f6"));
        points.add(new Point(166, 55, 0, 6, "#f7b326"));
        points.add(new Point(266, 88, 0, 6, "#2b58e8"));
        points.add(new Point(178, 34, 0, 6, "#facb5e"));
        points.add(new Point(100, 65, 0, 6, "#e02e3d"));
        points.add(new Point(343, 32, 0, 6, "#f16d6f"));
        points.add(new Point(59, 5, 0, 6, "#507bf2"));
        points.add(new Point(27, 9, 0, 6, "#5683f7"));
        points.add(new Point(233, 116, 0, 6, "#3158e2"));
        points.add(new Point(123, 32, 0, 6, "#f0696c"));
        points.add(new Point(6, 38, 0, 6, "#3769f6"));
        points.add(new Point(63, 62, 0, 6, "#6084ef"));
        points.add(new Point(6, 49, 0, 6, "#2a5cf4"));
        points.add(new Point(108, 36, 0, 6, "#f4716e"));
        points.add(new Point(169, 43, 0, 6, "#f8c247"));
        points.add(new Point(137, 37, 0, 6, "#e74653"));
        points.add(new Point(318, 58, 0, 6, "#ec4147"));
        points.add(new Point(226, 100, 0, 5, "#4876f1"));
        points.add(new Point(101, 46, 0, 5, "#ef5c5c"));
        points.add(new Point(226, 108, 0, 5, "#2552ea"));
        points.add(new Point(17, 17, 0, 5, "#4779f7"));
        points.add(new Point(232, 93, 0, 5, "#4b78f1"));
    }

    public void start(float canvasWidth, float canvasHeight) {
        // Initial center
        float offsetX = (canvasWidth / 2 - 180);
        float offsetY = (canvasHeight / 2 - 65);

        for (Point p : points) {
            p.curPos.x += offsetX;
            p.curPos.y += offsetY;
            p.originalPos.x += offsetX;
            p.originalPos.y += offsetY;
        }
    }

    public void update(float dt) {
        for (Point point : points) {
            if (point == null)
                continue;

            float dx = this.mousePos.x - point.curPos.x;
            float dy = this.mousePos.y - point.curPos.y;
            float dd = (dx * dx) + (dy * dy);
            float d = (float) Math.sqrt(dd);

            if (d < 150) {
                // Determine direction to move
                if (this.mousePos.x < point.curPos.x) {
                    point.targetPos.x = point.curPos.x - dx;
                } else {
                    point.targetPos.x = point.curPos.x - dx;
                }

                if (this.mousePos.y < point.curPos.y) {
                    point.targetPos.y = point.curPos.y - dy;
                } else {
                    point.targetPos.y = point.curPos.y - dy;
                }
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }

            point.update(dt);
        }
    }

    public void draw(Canvas canvas, Paint paint) {
        for (Point point : points) {
            if (point != null) {
                point.draw(canvas, paint);
            }
        }
    }
}
