using System;
using System.Collections.Generic;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Core;

namespace Google_Balls
{
    public sealed partial class MainPage : Page
    {
        private PointCollection pointCollection;
        private DispatcherTimer animationTimer;
        private const double DT = 0.1;
        private bool isAnimating = false;
        private int frameCounter = 0;

        public MainPage()
        {
            this.InitializeComponent();
            this.Loaded += MainPage_Loaded;
        }

        private void MainPage_Loaded(object sender, RoutedEventArgs e)
        {
            InitializeParticles();
            StartAnimation();
        }

        private void InitializeParticles()
        {
            var particles = new List<Point>
            {
                new Point(202, 78, 0.0, 9, "#ed9d33"), new Point(348, 83, 0.0, 9, "#d44d61"),
                new Point(256, 69, 0.0, 9, "#4f7af2"), new Point(214, 59, 0.0, 9, "#ef9a1e"),
                new Point(265, 36, 0.0, 9, "#4976f3"), new Point(300, 78, 0.0, 9, "#269230"),
                new Point(294, 59, 0.0, 9, "#1f9e2c"), new Point(45, 88, 0.0, 9, "#1c48dd"),
                new Point(268, 52, 0.0, 9, "#2a56ea"), new Point(73, 83, 0.0, 9, "#3355d8"),
                new Point(294, 6, 0.0, 9, "#36b641"), new Point(235, 62, 0.0, 9, "#2e5def"),
                new Point(353, 42, 0.0, 8, "#d53747"), new Point(336, 52, 0.0, 8, "#eb676f"),
                new Point(208, 41, 0.0, 8, "#f9b125"), new Point(321, 70, 0.0, 8, "#de3646"),
                new Point(8, 60, 0.0, 8, "#2a59f0"), new Point(180, 81, 0.0, 8, "#eb9c31"),
                new Point(146, 65, 0.0, 8, "#c41731"), new Point(145, 49, 0.0, 8, "#d82038"),
                new Point(246, 34, 0.0, 8, "#5f8af8"), new Point(169, 69, 0.0, 8, "#efa11e"),
                new Point(273, 99, 0.0, 8, "#2e55e2"), new Point(248, 120, 0.0, 8, "#4167e4"),
                new Point(294, 41, 0.0, 8, "#0b991a"), new Point(267, 114, 0.0, 8, "#4869e3"),
                new Point(78, 67, 0.0, 8, "#3059e3"), new Point(294, 23, 0.0, 8, "#10a11d"),
                new Point(117, 83, 0.0, 8, "#cf4055"), new Point(137, 80, 0.0, 8, "#cd4359"),
                new Point(14, 71, 0.0, 8, "#2855ea"), new Point(331, 80, 0.0, 8, "#ca273c"),
                new Point(25, 82, 0.0, 8, "#2650e1"), new Point(233, 46, 0.0, 8, "#4a7bf9"),
                new Point(73, 13, 0.0, 8, "#3d65e7"), new Point(327, 35, 0.0, 6, "#f47875"),
                new Point(319, 46, 0.0, 6, "#f36764"), new Point(256, 81, 0.0, 6, "#1d4eeb"),
                new Point(244, 88, 0.0, 6, "#698bf1"), new Point(194, 32, 0.0, 6, "#fac652"),
                new Point(97, 56, 0.0, 6, "#ee5257"), new Point(105, 75, 0.0, 6, "#cf2a3f"),
                new Point(42, 4, 0.0, 6, "#5681f5"), new Point(10, 27, 0.0, 6, "#4577f6"),
                new Point(166, 55, 0.0, 6, "#f7b326"), new Point(266, 88, 0.0, 6, "#2b58e8"),
                new Point(178, 34, 0.0, 6, "#facb5e"), new Point(100, 65, 0.0, 6, "#e02e3d"),
                new Point(343, 32, 0.0, 6, "#f16d6f"), new Point(59, 5, 0.0, 6, "#507bf2"),
                new Point(27, 9, 0.0, 6, "#5683f7"), new Point(233, 116, 0.0, 6, "#3158e2"),
                new Point(123, 32, 0.0, 6, "#f0696c"), new Point(6, 38, 0.0, 6, "#3769f6"),
                new Point(63, 62, 0.0, 6, "#6084ef"), new Point(6, 49, 0.0, 6, "#2a5cf4"),
                new Point(108, 36, 0.0, 6, "#f4716e"), new Point(169, 43, 0.0, 6, "#f8c247"),
                new Point(137, 37, 0.0, 6, "#e74653"), new Point(318, 58, 0.0, 6, "#ec4147"),
                new Point(226, 100, 0.0, 5, "#4876f1"), new Point(101, 46, 0.0, 5, "#ef5c5c"),
                new Point(226, 108, 0.0, 5, "#2552ea"), new Point(17, 17, 0.0, 5, "#4779f7"),
                new Point(232, 93, 0.0, 5, "#4b78f1")
            };

            pointCollection = new PointCollection(ParticleCanvas);
            pointCollection.Points = particles;
            pointCollection.InitializeVisuals();
            PositionParticles();
        }

        private void PositionParticles()
        {
            if (pointCollection?.Points == null) return;

            double centerX = ParticleCanvas.ActualWidth / 2;
            double centerY = ParticleCanvas.ActualHeight / 2;
            double offsetX = centerX - 180;
            double offsetY = centerY - 65;

            foreach (var point in pointCollection.Points)
            {
                point.CurPos.X = offsetX + point.CurPos.X;
                point.CurPos.Y = offsetY + point.CurPos.Y;
                point.OriginalPos.X = offsetX + point.OriginalPos.X;
                point.OriginalPos.Y = offsetY + point.OriginalPos.Y;
            }
        }

        private void StartAnimation()
        {
            CompositionTarget.Rendering += CompositionTarget_Rendering;
            isAnimating = true;
        }

        private void CompositionTarget_Rendering(object sender, object e)
        {
            if (isAnimating && pointCollection != null)
            {
                frameCounter++;
                if (frameCounter % 2 == 0)
                {
                    pointCollection.Update();
                }
            }
        }

        private void ParticleCanvas_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
            var position = e.GetCurrentPoint(ParticleCanvas).Position;
            pointCollection?.SetMousePosition(position.X, position.Y);
        }

        private void ParticleCanvas_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            ParticleCanvas.CapturePointer(e.Pointer);
        }

        private void ParticleCanvas_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            RecenterParticles();
        }

        private void ParticleCanvas_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            var position = e.GetCurrentPoint(ParticleCanvas).Position;
            pointCollection?.SetMousePosition(position.X, position.Y);
        }

        private void ParticleCanvas_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            pointCollection?.SetMousePosition(-1000, -1000);
        }

        private void RecenterParticles()
        {
            if (pointCollection?.Points == null) return;

            double centerX = ParticleCanvas.ActualWidth / 2;
            double centerY = ParticleCanvas.ActualHeight / 2;
            double newOffsetX = centerX - 180;
            double newOffsetY = centerY - 65;

            foreach (var point in pointCollection.Points)
            {
                double relX = point.OriginalPos.X - (centerX - 180);
                double relY = point.OriginalPos.Y - (centerY - 65);

                point.OriginalPos.X = newOffsetX + relX;
                point.OriginalPos.Y = newOffsetY + relY;
                point.CurPos.X = point.OriginalPos.X;
                point.CurPos.Y = point.OriginalPos.Y;

                point.UpdateVisual();
            }
        }
    }
}