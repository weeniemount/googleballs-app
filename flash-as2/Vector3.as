class Vector3 {
	public var x:Number;
	public var y:Number;
	public var z:Number;

	public function Vector3(x:Number, y:Number, z:Number) {
		this.x = x;
		this.y = y;
		this.z = z;
	}
		
	public function set(newX:Number, newY:Number, newZ:Number) {
		this.x = newX;
		this.y = newY;
		this.z = newZ;
	}
		
	public function addX(dx:Number) { x += dx; }
	public function addY(dy:Number) { y += dy; }
	public function addZ(dz:Number) { z += dz; }
}
