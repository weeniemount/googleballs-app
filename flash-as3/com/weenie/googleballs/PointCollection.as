package com.weenie.googleballs {
	import flash.display.Graphics;

	public class PointCollection {
		public var mouseX:Number = 0;
		public var mouseY:Number = 0;
		public var points:Vector.<Point>;

		public function PointCollection() {
			this.points = new Vector.<Point>();
		}
		
		public function newPoint(x:Number, y:Number, z:Number, size:Number, color:String):Point {
			var point = new Point(x, y, z, size, color);
			this.points.push(point);
			return point;
		}
		
		public function update(dt:Number) {
			var pointsLength = this.points.length;

			for (var i = 0; i < pointsLength; i++) {
				var point = this.points[i];

				if (point == null)
					continue;

				var dx = this.mouseX - point.curPos.x;
				var dy = this.mouseY - point.curPos.y;
				var dd = (dx * dx) + (dy * dy);
				var d = Math.sqrt(dd);

				if (d < 150) {
					point.targetPos.x = point.curPos.x - dx;
					point.targetPos.y = point.curPos.y - dy;
				} else {
					point.targetPos.x = point.originalPos.x;
					point.targetPos.y = point.originalPos.y;
				};

				point.update(dt);
			};
		}
		
		public function draw(graphics: Graphics) {
			var pointsLength = this.points.length;
			for (var i = 0; i < pointsLength; i++) {
				var point = this.points[i];
				
				if (point == null)
					continue;

				point.draw(graphics);
			};
		}
	}
}