package com.weenie.googleballs {
	import flash.display.Graphics;

	public class Point {
		public var color:Color;
    	public var curPos:Vector3;
		public var friction:Number = 0.8;
		public var originalPos:Vector3;
		public var radius:Number;
		public var size:Number;
		public var springStrength:Number = 0.1;
		public var targetPos:Vector3;
		public var velocity:Vector3 = new Vector3(0.0, 0.0, 0.0);

		public function Point(x:Number, y:Number, z:Number, size:Number, color:String) {
			this.curPos = new Vector3(x, y, z);
			this.originalPos = new Vector3(x, y, z);
			this.targetPos = new Vector3(x, y, z);
			this.radius = size;
			this.size = size;
			this.color = new Color(color);
		}
		
		public function update(dt:Number) {
			// Target 30ms frame time (0.03s) for physics reference
			var targetFrameTime = 0.03;
			var timeScale = dt / targetFrameTime;

			var dx = this.targetPos.x - this.curPos.x;
			var ax = dx * this.springStrength * timeScale;
			this.velocity.x += ax;
			this.velocity.x *= Math.pow(this.friction, timeScale);
			this.curPos.x += this.velocity.x * timeScale;

			var dy = this.targetPos.y - this.curPos.y;
			var ay = dy * this.springStrength * timeScale;
			this.velocity.y += ay;
			this.velocity.y *= Math.pow(this.friction, timeScale);
			this.curPos.y += this.velocity.y * timeScale;

			var dox = this.originalPos.x - this.curPos.x;
			var doy = this.originalPos.y - this.curPos.y;
			var dd = (dox * dox) + (doy * doy);
			var d = Math.sqrt(dd);

			this.targetPos.z = d/100 + 1;
			var dz = this.targetPos.z - this.curPos.z;
			var az = dz * this.springStrength * timeScale;
			this.velocity.z += az;
			this.velocity.z *= Math.pow(this.friction, timeScale);
			this.curPos.z += this.velocity.z * timeScale;

			this.radius = this.size*this.curPos.z;
			if (this.radius < 1) this.radius = 1;
		};
		
		public function draw(graphics:Graphics) {
			graphics.beginFill(color.toUint());
			graphics.drawCircle(this.curPos.x, this.curPos.y, radius);
			graphics.endFill();
		}
	}
}
