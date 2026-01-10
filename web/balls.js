document.addEventListener('DOMContentLoaded', function() {
	var canvas = document.getElementById("c");
	var canvasHeight;
	var canvasWidth;
	var ctx;
	var lastFrameTime = 0;
	var fps30Mode = false;
	var animationId = null;
	var originalMode = false;

	var pointCollection;

	// original variables
	var u = 200, v = -200, w = -200, x, y, z, A = 0, C = 0, E = 0, F = 35, G, H = [], I, J;

	function loadSettings() {
		try {
			const darkBg = localStorage.getItem('googleballs_darkBg') === 'true';
			const fps30 = localStorage.getItem('googleballs_fps30') === 'true';
			const useOriginal = localStorage.getItem('googleballs_original') === 'true';

			document.getElementById('darkBg').checked = darkBg;
			document.getElementById('fps30').checked = fps30;
			document.getElementById('originalMode').checked = useOriginal;

			if (darkBg) {
				document.body.classList.add('dark-bg');
			}

			updateThemeColor(darkBg);

			fps30Mode = fps30;
			originalMode = useOriginal;
		} catch (e) {
			console.log('Could not load settings from localStorage');
		}
	}

	function saveSettings() {
		try {
			localStorage.setItem('googleballs_darkBg', document.getElementById('darkBg').checked);
			localStorage.setItem('googleballs_fps30', document.getElementById('fps30').checked);
			localStorage.setItem('googleballs_original', document.getElementById('originalMode').checked);
		} catch (e) {
			console.log('Could not save settings to localStorage');
		}
	}

	function updateThemeColor(isDark) {
		let metaTheme = document.querySelector('meta[name="theme-color"]');
		if (isDark) {
			if (!metaTheme) {
				metaTheme = document.createElement('meta');
				metaTheme.name = 'theme-color';
				document.head.appendChild(metaTheme);
			}
			metaTheme.content = '#1a1a1a';
		} else {
			if (metaTheme) {
				metaTheme.remove();
			}
		}
	}

	function isPWA() {
		return window.matchMedia('(display-mode: standalone)').matches ||
			window.navigator.standalone === true;
	}

	function init() {
		loadSettings();
		updateCanvasDimensions();

		var g = [new Point(202, 78, 0.0, 9, "#ed9d33"), new Point(348, 83, 0.0, 9, "#d44d61"), new Point(256, 69, 0.0, 9, "#4f7af2"), new Point(214, 59, 0.0, 9, "#ef9a1e"), new Point(265, 36, 0.0, 9, "#4976f3"), new Point(300, 78, 0.0, 9, "#269230"), new Point(294, 59, 0.0, 9, "#1f9e2c"), new Point(45, 88, 0.0, 9, "#1c48dd"), new Point(268, 52, 0.0, 9, "#2a56ea"), new Point(73, 83, 0.0, 9, "#3355d8"), new Point(294, 6, 0.0, 9, "#36b641"), new Point(235, 62, 0.0, 9, "#2e5def"), new Point(353, 42, 0.0, 8, "#d53747"), new Point(336, 52, 0.0, 8, "#eb676f"), new Point(208, 41, 0.0, 8, "#f9b125"), new Point(321, 70, 0.0, 8, "#de3646"), new Point(8, 60, 0.0, 8, "#2a59f0"), new Point(180, 81, 0.0, 8, "#eb9c31"), new Point(146, 65, 0.0, 8, "#c41731"), new Point(145, 49, 0.0, 8, "#d82038"), new Point(246, 34, 0.0, 8, "#5f8af8"), new Point(169, 69, 0.0, 8, "#efa11e"), new Point(273, 99, 0.0, 8, "#2e55e2"), new Point(248, 120, 0.0, 8, "#4167e4"), new Point(294, 41, 0.0, 8, "#0b991a"), new Point(267, 114, 0.0, 8, "#4869e3"), new Point(78, 67, 0.0, 8, "#3059e3"), new Point(294, 23, 0.0, 8, "#10a11d"), new Point(117, 83, 0.0, 8, "#cf4055"), new Point(137, 80, 0.0, 8, "#cd4359"), new Point(14, 71, 0.0, 8, "#2855ea"), new Point(331, 80, 0.0, 8, "#ca273c"), new Point(25, 82, 0.0, 8, "#2650e1"), new Point(233, 46, 0.0, 8, "#4a7bf9"), new Point(73, 13, 0.0, 8, "#3d65e7"), new Point(327, 35, 0.0, 6, "#f47875"), new Point(319, 46, 0.0, 6, "#f36764"), new Point(256, 81, 0.0, 6, "#1d4eeb"), new Point(244, 88, 0.0, 6, "#698bf1"), new Point(194, 32, 0.0, 6, "#fac652"), new Point(97, 56, 0.0, 6, "#ee5257"), new Point(105, 75, 0.0, 6, "#cf2a3f"), new Point(42, 4, 0.0, 6, "#5681f5"), new Point(10, 27, 0.0, 6, "#4577f6"), new Point(166, 55, 0.0, 6, "#f7b326"), new Point(266, 88, 0.0, 6, "#2b58e8"), new Point(178, 34, 0.0, 6, "#facb5e"), new Point(100, 65, 0.0, 6, "#e02e3d"), new Point(343, 32, 0.0, 6, "#f16d6f"), new Point(59, 5, 0.0, 6, "#507bf2"), new Point(27, 9, 0.0, 6, "#5683f7"), new Point(233, 116, 0.0, 6, "#3158e2"), new Point(123, 32, 0.0, 6, "#f0696c"), new Point(6, 38, 0.0, 6, "#3769f6"), new Point(63, 62, 0.0, 6, "#6084ef"), new Point(6, 49, 0.0, 6, "#2a5cf4"), new Point(108, 36, 0.0, 6, "#f4716e"), new Point(169, 43, 0.0, 6, "#f8c247"), new Point(137, 37, 0.0, 6, "#e74653"), new Point(318, 58, 0.0, 6, "#ec4147"), new Point(226, 100, 0.0, 5, "#4876f1"), new Point(101, 46, 0.0, 5, "#ef5c5c"), new Point(226, 108, 0.0, 5, "#2552ea"), new Point(17, 17, 0.0, 5, "#4779f7"), new Point(232, 93, 0.0, 5, "#4b78f1")];

		gLength = g.length;
		for (var i = 0; i < gLength; i++) {
			g[i].curPos.x = (canvasWidth/2 - 180) + g[i].curPos.x;
			g[i].curPos.y = (canvasHeight/2 - 65) + g[i].curPos.y;

			g[i].originalPos.x = (canvasWidth/2 - 180) + g[i].originalPos.x;
			g[i].originalPos.y = (canvasHeight/2 - 65) + g[i].originalPos.y;
		};

		pointCollection = new PointCollection();
		pointCollection.points = g;

		initOriginalMode();
		initEventListeners();

		if (isPWA()) {
            document.querySelectorAll('.page-links a').forEach(link => {
				link.remove();
			});
        }

		if (originalMode) {
			document.body.classList.add('original-mode');
			startOriginalAnimation();
		} else {
			startAnimation();
		}

	};

	document.addEventListener('keydown', function(e) {
		if (e.key === 'h' || e.key === 'H') {
			const controls = document.querySelector('.controls');
			const links = document.querySelectorAll('.page-links');

			if (isPWA()) {
				if (controls.style.display === 'none') {
					controls.style.display = 'flex';
				} else {
					controls.style.display = 'none';
				}
			} else {
				if (controls.style.display === 'none') {
					controls.style.display = 'flex';
					links.forEach(link => link.style.display = 'block');
				} else {
					controls.style.display = 'none';
					links.forEach(link => link.style.display = 'none');
				}
			}
		}
	});

	function initEventListeners() {
		window.addEventListener('resize', updateCanvasDimensions);
		window.addEventListener('mousemove', onMove);

		canvas.ontouchmove = function(e) {
			e.preventDefault();
			onTouchMove(e);
		};

		canvas.ontouchstart = function(e) {
			e.preventDefault();
		};

		document.getElementById('darkBg').addEventListener('change', function(e) {
			if (e.target.checked) {
				document.body.classList.add('dark-bg');
			} else {
				document.body.classList.remove('dark-bg');
			}
			updateThemeColor(e.target.checked);
			saveSettings();
		});

		document.getElementById('fps30').addEventListener('change', function(e) {
			fps30Mode = e.target.checked;
			if (animationId) {
				cancelAnimationFrame(animationId);
				animationId = null;
			}
			if (G) {
				clearTimeout(G);
				G = null;
			}
			lastFrameTime = 0;
			originalLastFrame = 0;
			if (originalMode) {
				startOriginalAnimation();
			} else {
				startAnimation();
			}
			saveSettings();
		});

		document.getElementById('originalMode').addEventListener('change', function(e) {
			originalMode = e.target.checked;

			// de ball
			if (animationId) {
				cancelAnimationFrame(animationId);
				animationId = null;
			}
			if (G) {
				clearTimeout(G);
				G = null;
			}

			lastFrameTime = 0;
			originalLastFrame = 0;

			if (originalMode) {
				document.body.classList.add('original-mode');
				startOriginalAnimation();
			} else {
				document.body.classList.remove('original-mode');
				startAnimation();
			}
			saveSettings();
		});
	};

	// the original ball
	function initOriginalMode() {
		I = document.getElementById('original-container');
		J = false; // circle

		var K = function() {
			return [window.screenLeft || window.screenX || 0, window.screenTop || window.screenY || 0, document.body.clientWidth || 0];
		};

		var L = function(a, d, b, j) {
			this.x = this.F = a;
			this.y = this.G = d;
			this.r = this.m = b;
			this.d = 100 * (Math.random() - 0.5);
			this.g = 100 * (Math.random() - 0.5);
			this.o = 3 + Math.random() * 98;
			this.H = 0.1 + Math.random() * 0.4;
			this.h = 0;
			this.i = 1;
			this.n = false;
			this.a = document.createElement("div");
			a = this.a.style;
			a.position = "absolute";
			a.zIndex = "-1";
			j = "#" + j;
			this.a.className = "original-circle";
			a.backgroundColor = j;
			I.appendChild(this.a);
		};

		L.prototype = {
			remove: function() {
				this.a && I.removeChild(this.a);
				this.a = null;
			},
			update: function() {
				this.x += this.d;
				this.y += this.g;
				this.d = Math.min(50, Math.max(-50, (this.d + (A + E) / this.m) * 0.92));
				this.g = Math.min(50, Math.max(-50, (this.g + C / this.m) * 0.92));
				var a = v - this.x, d = w - this.y, b = Math.sqrt(a * a + d * d);
				a /= b;
				d /= b;
				b < u ? (this.d -= a * this.o, this.g -= d * this.o, this.h += (0.005 - this.h) * 0.4, this.i = Math.max(0, this.i * 0.9 - 0.01), this.d *= 1 - this.i, this.g *= 1 - this.i) : (this.h += (this.H - this.h) * 0.005, this.i = Math.min(1, this.i + 0.03));
				a = this.F - this.x;
				d = this.G - this.y;
				b = Math.sqrt(a * a + d * d);
				this.d += a * this.h;
				this.g += d * this.h;
				this.r = this.m + b / 8;
				this.n = b < 0.3 && this.d < 0.3 && this.g < 0.3;
				if (!this.n) {
					a = this.a.style;
					a.width = a.height = this.r * 2 + "px";
					a.left = this.x + "px";
					a.top = this.y + "px";
				}
			}
		};

		var M = function(a, d, b, j) {
			return new L(a, d, b, j);
		};

		window.initOriginalParticles = function() {
			H = [M(202, 78, 9, "ed9d33"), M(348, 83, 9, "d44d61"), M(256, 69, 9, "4f7af2"), M(214, 59, 9, "ef9a1e"), M(265, 36, 9, "4976f3"), M(300, 78, 9, "269230"), M(294, 59, 9, "1f9e2c"), M(45, 88, 9, "1c48dd"), M(268, 52, 9, "2a56ea"), M(73, 83, 9, "3355d8"), M(294, 6, 9, "36b641"), M(235, 62, 9, "2e5def"), M(353, 42, 8, "d53747"), M(336, 52, 8, "eb676f"), M(208, 41, 8, "f9b125"), M(321, 70, 8, "de3646"), M(8, 60, 8, "2a59f0"), M(180, 81, 8, "eb9c31"), M(146, 65, 8, "c41731"), M(145, 49, 8, "d82038"), M(246, 34, 8, "5f8af8"), M(169, 69, 8, "efa11e"), M(273, 99, 8, "2e55e2"), M(248, 120, 8, "4167e4"), M(294, 41, 8, "0b991a"), M(267, 114, 8, "4869e3"), M(78, 67, 8, "3059e3"), M(294, 23, 8, "10a11d"), M(117, 83, 8, "cf4055"), M(137, 80, 8, "cd4359"), M(14, 71, 8, "2855ea"), M(331, 80, 8, "ca273c"), M(25, 82, 8, "2650e1"), M(233, 46, 8, "4a7bf9"), M(73, 13, 8, "3d65e7"), M(327, 35, 6, "f47875"), M(319, 46, 6, "f36764"), M(256, 81, 6, "1d4eeb"), M(244, 88, 6, "698bf1"), M(194, 32, 6, "fac652"), M(97, 56, 6, "ee5257"), M(105, 75, 6, "cf2a3f"), M(42, 4, 6, "5681f5"), M(10, 27, 6, "4577f6"), M(166, 55, 6, "f7b326"), M(266, 88, 6, "2b58e8"), M(178, 34, 6, "facb5e"), M(100, 65, 6, "e02e3d"), M(343, 32, 6, "f16d6f"), M(59, 5, 6, "507bf2"), M(27, 9, 6, "5683f7"), M(233, 116, 6, "3158e2"), M(123, 32, 6, "f0696c"), M(6, 38, 6, "3769f6"), M(63, 62, 6, "6084ef"), M(6, 49, 6, "2a5cf4"), M(108, 36, 6, "f4716e"), M(169, 43, 6, "f8c247"), M(137, 37, 6, "e74653"), M(318, 58, 6, "ec4147"), M(226, 100, 5, "4876f1"), M(101, 46, 5, "ef5c5c"), M(226, 108, 5, "2552ea"), M(17, 17, 5, "4779f7"), M(232, 93, 5, "4b78f1")];

			var a = K();
			x = a[0];
			y = a[1];
			z = a[2];
		};

		window.updateOriginal = function() {
			var a = K(), d = a[0], b = a[1], a = a[2];
			A = d - x;
			C = b - y;
			E = a - z;
			x = d;
			y = b;
			z = a;
			u = Math.max(0, u - 2);
			d = true;
			for (b = 0; a = H[b++];)
				a.update(), d &= a.n;
			F = d ? 250 : 35;
		};

		window.onOriginalMove = function(e) {
			u = 200;
			var rect = I.getBoundingClientRect();
			v = e.clientX - rect.left;
			w = e.clientY - rect.top;
		};

		document.addEventListener('mousemove', window.onOriginalMove);
	}

	function startOriginalAnimation() {
		// de ball
		while (I.firstChild) {
			I.removeChild(I.firstChild);
		}
		H = [];

		window.initOriginalParticles();
		originalLastFrame = performance.now();
		if (fps30Mode) {
			animateOriginal30fps();
		} else {
			animateOriginalRAF();
		}
	}

	function animateOriginal30fps() {
		window.updateOriginal();
		G = setTimeout(animateOriginal30fps, 1000 / 30);
	}

	function animateOriginalRAF() {
		window.updateOriginal();
		animationId = requestAnimationFrame(animateOriginalRAF);
	}

	function startAnimation() {
		if (fps30Mode) {
			animate30fps();
		} else {
			animateRAF(performance.now());
		}
	}

	function animate30fps() {
		var timestamp = performance.now();
		if (!lastFrameTime) lastFrameTime = timestamp;
		var deltaTime = (timestamp - lastFrameTime) / 1000;
		lastFrameTime = timestamp;

		deltaTime = Math.min(deltaTime, 0.1);

		draw();
		update(deltaTime);

		setTimeout(animate30fps, 1000 / 30);
	}

	function animateRAF(timestamp) {
		if (!lastFrameTime) lastFrameTime = timestamp;
		var deltaTime = (timestamp - lastFrameTime) / 1000;
		lastFrameTime = timestamp;

		deltaTime = Math.min(deltaTime, 0.1);

		draw();
		update(deltaTime);

		animationId = requestAnimationFrame(animateRAF);
	}

	function updateCanvasDimensions() {
	    var dpr = window.devicePixelRatio || 1;
	    var logicalWidth = window.innerWidth;
	    var logicalHeight = window.innerHeight;

	    canvas.width = logicalWidth * dpr;
	    canvas.height = logicalHeight * dpr;
	    canvas.style.width = logicalWidth + 'px';
	    canvas.style.height = logicalHeight + 'px';

	    canvasWidth = logicalWidth;
	    canvasHeight = logicalHeight;

	    if (canvas.getContext) {
	        ctx = canvas.getContext('2d');
	        ctx.scale(dpr, dpr);
	    }

	    recenterPoints();
	    draw();
	};

	function recenterPoints() {
	    if (!pointCollection) return;

	    let offsetX = (canvasWidth / 2 - 180);
	    let offsetY = (canvasHeight / 2 - 65);

	    for (let i = 0; i < pointCollection.points.length; i++) {
	        let point = pointCollection.points[i];

	        let relX = point.originalPos.x - (canvasWidth / 2 - 180);
	        let relY = point.originalPos.y - (canvasHeight / 2 - 65);

	        point.originalPos.x = offsetX + relX;
	        point.originalPos.y = offsetY + relY;

	        point.curPos.x = point.originalPos.x;
	        point.curPos.y = point.originalPos.y;
	    }
	}

	function onMove(e) {
		if (pointCollection) pointCollection.mousePos.set(e.pageX, e.pageY);
	};

	function onTouchMove(e) {
		if (pointCollection) pointCollection.mousePos.set(e.targetTouches[0].pageX, e.targetTouches[0].pageY);
	};


	function draw() {
		if (!ctx) {
			return;
		};

		ctx.clearRect(0, 0, canvasWidth, canvasHeight);

		if (pointCollection) pointCollection.draw();
	};

	function update(dt) {
		if (pointCollection) pointCollection.update(dt);
	};

	function Vector(x, y, z) {
		this.x = x;
		this.y = y;
		this.z = z;

		this.addX = function(x) {
			this.x += x;
		};

		this.addY = function(y) {
			this.y += y;
		};

		this.addZ = function(z) {
			this.z += z;
		};

		this.set = function(x, y, z) {
			this.x = x;
			this.y = y;
			this.z = z;
		};
	};

	function PointCollection() {
		this.mousePos = new Vector(0, 0);
		this.points = new Array();

		this.newPoint = function(x, y, z) {
			var point = new Point(x, y, z);
			this.points.push(point);
			return point;
		};

		this.update = function(dt) {
			var pointsLength = this.points.length;

			for (var i = 0; i < pointsLength; i++) {
				var point = this.points[i];

				if (point == null)
					continue;

				var dx = this.mousePos.x - point.curPos.x;
				var dy = this.mousePos.y - point.curPos.y;
				var dd = (dx * dx) + (dy * dy);
				var d = Math.sqrt(dd);

				if (d < 150) {
					point.targetPos.x = (this.mousePos.x < point.curPos.x) ? point.curPos.x - dx : point.curPos.x - dx;
					point.targetPos.y = (this.mousePos.y < point.curPos.y) ? point.curPos.y - dy : point.curPos.y - dy;
				} else {
					point.targetPos.x = point.originalPos.x;
					point.targetPos.y = point.originalPos.y;
				};

				point.update(dt);
			};
		};

		this.draw = function() {
			var pointsLength = this.points.length;
			for (var i = 0; i < pointsLength; i++) {
				var point = this.points[i];

				if (point == null)
					continue;

				point.draw();
			};
		};
	};

	function Point(x, y, z, size, colour) {
		this.colour = colour;
		this.curPos = new Vector(x, y, z);
		this.friction = 0.8;
		this.originalPos = new Vector(x, y, z);
		this.radius = size;
		this.size = size;
		this.springStrength = 0.1;
		this.targetPos = new Vector(x, y, z);
		this.velocity = new Vector(0.0, 0.0, 0.0);

		this.update = function(dt) {
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

		this.draw = function() {
			ctx.fillStyle = this.colour;
			ctx.beginPath();
			ctx.arc(this.curPos.x, this.curPos.y, this.radius, 0, Math.PI*2, true);
			ctx.fill();
		};
	};

	init();

	if ('serviceWorker' in navigator) {
		navigator.serviceWorker.register('/sw.js')
			.then(() => console.log('Service Worker registered!'))
			.catch(err => console.error('SW registration failed:', err));
	}
});