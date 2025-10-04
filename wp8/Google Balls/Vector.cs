namespace Google_Balls
{
    public class Vector
    {
        public double X { get; set; }
        public double Y { get; set; }
        public double Z { get; set; }

        public Vector(double x = 0, double y = 0, double z = 0)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public void AddX(double x)
        {
            X += x;
        }

        public void AddY(double y)
        {
            Y += y;
        }

        public void AddZ(double z)
        {
            Z += z;
        }

        public void Set(double x, double y, double z)
        {
            X = x;
            Y = y;
            Z = z;
        }
    }
}