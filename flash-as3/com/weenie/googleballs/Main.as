package com.weenie.googleballs {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.TouchEvent;
	import flash.events.Event;
	import flash.utils.getTimer;
	
	public class Main extends MovieClip {
		private var lastFrameTime:Number = getTimer();
		private var pointCollection:PointCollection;

		public function Main() {
			var g:Vector.<Point> = new <Point>[new Point(202, 78, 0.0, 9, "#ed9d33"), new Point(348, 83, 0.0, 9, "#d44d61"), new Point(256, 69, 0.0, 9, "#4f7af2"), new Point(214, 59, 0.0, 9, "#ef9a1e"), new Point(265, 36, 0.0, 9, "#4976f3"), new Point(300, 78, 0.0, 9, "#269230"), new Point(294, 59, 0.0, 9, "#1f9e2c"), new Point(45, 88, 0.0, 9, "#1c48dd"), new Point(268, 52, 0.0, 9, "#2a56ea"), new Point(73, 83, 0.0, 9, "#3355d8"), new Point(294, 6, 0.0, 9, "#36b641"), new Point(235, 62, 0.0, 9, "#2e5def"), new Point(353, 42, 0.0, 8, "#d53747"), new Point(336, 52, 0.0, 8, "#eb676f"), new Point(208, 41, 0.0, 8, "#f9b125"), new Point(321, 70, 0.0, 8, "#de3646"), new Point(8, 60, 0.0, 8, "#2a59f0"), new Point(180, 81, 0.0, 8, "#eb9c31"), new Point(146, 65, 0.0, 8, "#c41731"), new Point(145, 49, 0.0, 8, "#d82038"), new Point(246, 34, 0.0, 8, "#5f8af8"), new Point(169, 69, 0.0, 8, "#efa11e"), new Point(273, 99, 0.0, 8, "#2e55e2"), new Point(248, 120, 0.0, 8, "#4167e4"), new Point(294, 41, 0.0, 8, "#0b991a"), new Point(267, 114, 0.0, 8, "#4869e3"), new Point(78, 67, 0.0, 8, "#3059e3"), new Point(294, 23, 0.0, 8, "#10a11d"), new Point(117, 83, 0.0, 8, "#cf4055"), new Point(137, 80, 0.0, 8, "#cd4359"), new Point(14, 71, 0.0, 8, "#2855ea"), new Point(331, 80, 0.0, 8, "#ca273c"), new Point(25, 82, 0.0, 8, "#2650e1"), new Point(233, 46, 0.0, 8, "#4a7bf9"), new Point(73, 13, 0.0, 8, "#3d65e7"), new Point(327, 35, 0.0, 6, "#f47875"), new Point(319, 46, 0.0, 6, "#f36764"), new Point(256, 81, 0.0, 6, "#1d4eeb"), new Point(244, 88, 0.0, 6, "#698bf1"), new Point(194, 32, 0.0, 6, "#fac652"), new Point(97, 56, 0.0, 6, "#ee5257"), new Point(105, 75, 0.0, 6, "#cf2a3f"), new Point(42, 4, 0.0, 6, "#5681f5"), new Point(10, 27, 0.0, 6, "#4577f6"), new Point(166, 55, 0.0, 6, "#f7b326"), new Point(266, 88, 0.0, 6, "#2b58e8"), new Point(178, 34, 0.0, 6, "#facb5e"), new Point(100, 65, 0.0, 6, "#e02e3d"), new Point(343, 32, 0.0, 6, "#f16d6f"), new Point(59, 5, 0.0, 6, "#507bf2"), new Point(27, 9, 0.0, 6, "#5683f7"), new Point(233, 116, 0.0, 6, "#3158e2"), new Point(123, 32, 0.0, 6, "#f0696c"), new Point(6, 38, 0.0, 6, "#3769f6"), new Point(63, 62, 0.0, 6, "#6084ef"), new Point(6, 49, 0.0, 6, "#2a5cf4"), new Point(108, 36, 0.0, 6, "#f4716e"), new Point(169, 43, 0.0, 6, "#f8c247"), new Point(137, 37, 0.0, 6, "#e74653"), new Point(318, 58, 0.0, 6, "#ec4147"), new Point(226, 100, 0.0, 5, "#4876f1"), new Point(101, 46, 0.0, 5, "#ef5c5c"), new Point(226, 108, 0.0, 5, "#2552ea"), new Point(17, 17, 0.0, 5, "#4779f7"), new Point(232, 93, 0.0, 5, "#4b78f1")];

			for (var i = 0; i < g.length; i++) {
				g[i].curPos.x = (stage.stageWidth/2 - 180) + g[i].curPos.x;
				g[i].curPos.y = (stage.stageHeight/2 - 65) + g[i].curPos.y;

				g[i].originalPos.x = (stage.stageWidth/2 - 180) + g[i].originalPos.x;
				g[i].originalPos.y = (stage.stageHeight/2 - 65) + g[i].originalPos.y;
			};

			pointCollection = new PointCollection();
			pointCollection.points = g;

			initEventListeners();
		}
		
		private function initEventListeners() {
			stage.addEventListener(Event.ENTER_FRAME, animate);
			stage.addEventListener(Event.RESIZE, updateStageDimensions);
			stage.addEventListener(MouseEvent.MOUSE_MOVE, onMove);

			stage.addEventListener(TouchEvent.TOUCH_MOVE, function(e:TouchEvent) {
				e.preventDefault();
				onTouchMove(e);
			});
			
			stage.addEventListener(TouchEvent.TOUCH_BEGIN, function(e:TouchEvent) {
				e.preventDefault();
			});
		}
		
		private function updateStageDimensions() {
	    	recenterPoints();
	    	draw();
		};
		
		private function recenterPoints() {
	    	if (!pointCollection) return;
	
	    	const offsetX = (stage.stageWidth / 2 - 180);
	    	const offsetY = (stage.stageHeight / 2 - 65);
	
	    	for (var i = 0; i < pointCollection.points.length; i++) {
	        	var point = pointCollection.points[i];
	
	        	const relX = point.originalPos.x - (stage.stageWidth / 2 - 180);
	        	const relY = point.originalPos.y - (stage.stageHeight / 2 - 65);
	
	        	point.originalPos.x = offsetX + relX;
	        	point.originalPos.y = offsetY + relY;
			
	        	point.curPos.x = point.originalPos.x;
	        	point.curPos.y = point.originalPos.y;
	    	}
		}
		
		private function onMove(e:MouseEvent) {
			pointCollection.mouseX = e.stageX;
			pointCollection.mouseY = e.stageY;
		};
	
		private function onTouchMove(e:TouchEvent) {
			pointCollection.mouseX = e.stageX;
			pointCollection.mouseY = e.stageY;
		};

		private function animate(e:Event) {
			var currentTime = getTimer();
			var deltaTime = (currentTime - lastFrameTime) / 1000; // Convert to seconds
			lastFrameTime = currentTime;

			// Clamp delta time to prevent large jumps (e.g., when tab becomes inactive)
			deltaTime = Math.min(deltaTime, 0.1);

			draw();
			update(deltaTime);
		};
		
		private function draw() {
			this.graphics.clear();
			if (pointCollection) pointCollection.draw(this.graphics);
		};

		private function update(dt:Number) {
			if (pointCollection) pointCollection.update(dt);
		};
	}
}
