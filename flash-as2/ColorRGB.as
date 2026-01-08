class ColorRGB {
	public var r:Number = 0;
	public var g:Number = 0;
	public var b:Number = 0;

	public function ColorRGB(hex:String) {
		if (hex.charAt(0) == '#') {
			var value = parseInt(hex.substr(1), 16);
			this.r = (value >> 16) & 0xFF;
			this.g = (value >> 8) & 0xFF;
			this.b = value & 0xFF;
		}
	}

	public function toNumber():Number {
		return ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
	}
}
