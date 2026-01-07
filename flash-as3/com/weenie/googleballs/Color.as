package com.weenie.googleballs {
	public class Color {
		public var r:uint = 0;
		public var g:uint = 0;
		public var b:uint = 0;

		public function Color(hex:String) {
			if (hex.charAt(0) == '#') {
				const value = parseInt(hex.substr(1), 16);
				this.r = (value >> 16) & 0xFF;
				this.g = (value >> 8) & 0xFF;
				this.b = value & 0xFF;
			}
		}
		
		public function toUint():uint {
			return ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
		}
	}
}
