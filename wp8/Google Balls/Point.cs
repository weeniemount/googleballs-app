using System;
using Windows.UI;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

namespace Google_Balls
{
    public class Point
    {
        private Ellipse visualElement;
        private Canvas parentCanvas;

        public string Colour { get; }
        public Vector CurPos { get; set; }
        public double Friction { get; set; } = 0.8;
        public Vector OriginalPos { get; set; }
        public double Radius { get; set; }
        public double Size { get; set; }
        public double SpringStrength { get; set; } = 0.1;
        public Vector TargetPos { get; set; }
        public Vector Velocity { get; set; }

        public Point(double x, double y, double z, double size, string colour)
        {
            Colour = colour;
            Size = size;
            Radius = size;
            CurPos = new Vector(x, y, z);
            OriginalPos = new Vector(x, y, z);
            TargetPos = new Vector(x, y, z);
            Velocity = new Vector(0.0, 0.0, 0.0);
        }

        public void InitializeVisual(Canvas canvas)
        {
            parentCanvas = canvas;
            visualElement = new Ellipse
            {
                Fill = new SolidColorBrush(ColorFromHex(Colour)),
                Width = Radius * 2,
                Height = Radius * 2
            };

            Canvas.SetLeft(visualElement, CurPos.X - Radius);
            Canvas.SetTop(visualElement, CurPos.Y - Radius);

            canvas.Children.Add(visualElement);
        }

        public void Update()
        {
            double dx = TargetPos.X - CurPos.X;
            double ax = dx * SpringStrength;
            Velocity.X += ax;
            Velocity.X *= Friction;
            CurPos.X += Velocity.X;

            double dy = TargetPos.Y - CurPos.Y;
            double ay = dy * SpringStrength;
            Velocity.Y += ay;
            Velocity.Y *= Friction;
            CurPos.Y += Velocity.Y;

            double dox = OriginalPos.X - CurPos.X;
            double doy = OriginalPos.Y - CurPos.Y;
            double dd = (dox * dox) + (doy * doy);
            double d = Math.Sqrt(dd);

            TargetPos.Z = d / 100 + 1;
            double dz = TargetPos.Z - CurPos.Z;
            double az = dz * SpringStrength;
            Velocity.Z += az;
            Velocity.Z *= Friction;
            CurPos.Z += Velocity.Z;

            Radius = Size * CurPos.Z;
            if (Radius < 1) Radius = 1;

            UpdateVisual();
        }

        public void UpdateVisual()
        {
            if (visualElement != null)
            {
                double newWidth = Radius * 2;
                double newHeight = Radius * 2;
                double newLeft = CurPos.X - Radius;
                double newTop = CurPos.Y - Radius;

                if (Math.Abs(visualElement.Width - newWidth) > 0.1 ||
                    Math.Abs(visualElement.Height - newHeight) > 0.1)
                {
                    visualElement.Width = newWidth;
                    visualElement.Height = newHeight;
                }

                if (Math.Abs(Canvas.GetLeft(visualElement) - newLeft) > 0.1 ||
                    Math.Abs(Canvas.GetTop(visualElement) - newTop) > 0.1)
                {
                    Canvas.SetLeft(visualElement, newLeft);
                    Canvas.SetTop(visualElement, newTop);
                }
            }
        }

        private Color ColorFromHex(string hex)
        {
            hex = hex.Replace("#", "");
            byte r = Convert.ToByte(hex.Substring(0, 2), 16);
            byte g = Convert.ToByte(hex.Substring(2, 2), 16);
            byte b = Convert.ToByte(hex.Substring(4, 2), 16);
            return Color.FromArgb(255, r, g, b);
        }

        public void RemoveFromCanvas()
        {
            if (visualElement != null && parentCanvas != null)
            {
                parentCanvas.Children.Remove(visualElement);
            }
        }
    }
}