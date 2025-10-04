using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Controls;

namespace Google_Balls
{
    public class PointCollection
    {
        private Canvas canvas;
        public Vector MousePos { get; set; }
        public List<Point> Points { get; set; }

        public PointCollection(Canvas parentCanvas)
        {
            canvas = parentCanvas;
            MousePos = new Vector(0, 0);
            Points = new List<Point>();
        }

        public Point NewPoint(double x, double y, double z, double size, string colour)
        {
            var point = new Point(x, y, z, size, colour);
            point.InitializeVisual(canvas);
            Points.Add(point);
            return point;
        }

        public void SetMousePosition(double x, double y)
        {
            MousePos.Set(x, y, 0);
        }

        public void Update()
        {
            int pointsLength = Points.Count;

            for (int i = 0; i < pointsLength; i++)
            {
                var point = Points[i];
                if (point == null) continue;

                double dx = MousePos.X - point.CurPos.X;
                double dy = MousePos.Y - point.CurPos.Y;
                double dd = (dx * dx) + (dy * dy);
                double d = Math.Sqrt(dd);

                if (d < 150)
                {
                    point.TargetPos.X = point.CurPos.X - dx;
                    point.TargetPos.Y = point.CurPos.Y - dy;
                }
                else
                {
                    point.TargetPos.X = point.OriginalPos.X;
                    point.TargetPos.Y = point.OriginalPos.Y;
                }

                point.Update();
            }
        }

        public void InitializeVisuals()
        {
            foreach (var point in Points)
            {
                point.InitializeVisual(canvas);
            }
        }

        public void ClearCanvas()
        {
            foreach (var point in Points)
            {
                point.RemoveFromCanvas();
            }
        }
    }
}