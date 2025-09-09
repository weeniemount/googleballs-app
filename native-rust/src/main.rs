use macroquad::prelude::*;

#[derive(Clone, Debug)]
struct Vector3 {
    x: f32,
    y: f32,
    z: f32,
}

impl Vector3 {
    fn new(x: f32, y: f32, z: f32) -> Self {
        Self { x, y, z }
    }

    fn set(&mut self, x: f32, y: f32, z: f32) {
        self.x = x;
        self.y = y;
        self.z = z;
    }
}

struct Point {
    colour: Color,
    cur_pos: Vector3,
    friction: f32,
    original_pos: Vector3,
    radius: f32,
    size: f32,
    spring_strength: f32,
    target_pos: Vector3,
    velocity: Vector3,
}

impl Point {
    fn new(x: f32, y: f32, z: f32, size: f32, color_hex: &str) -> Self {
        let color = Self::hex_to_color(color_hex);
        Self {
            colour: color,
            cur_pos: Vector3::new(x, y, z),
            friction: 0.8,
            original_pos: Vector3::new(x, y, z),
            radius: size,
            size,
            spring_strength: 0.1,
            target_pos: Vector3::new(x, y, z),
            velocity: Vector3::new(0.0, 0.0, 0.0),
        }
    }

    fn hex_to_color(hex: &str) -> Color {
        let hex = hex.trim_start_matches('#');
        if hex.len() != 6 {
            return WHITE;
        }

        let r = u8::from_str_radix(&hex[0..2], 16).unwrap_or(255) as f32 / 255.0;
        let g = u8::from_str_radix(&hex[2..4], 16).unwrap_or(255) as f32 / 255.0;
        let b = u8::from_str_radix(&hex[4..6], 16).unwrap_or(255) as f32 / 255.0;

        Color::new(r, g, b, 1.0)
    }

    fn update(&mut self) {
        // X axis physics
        let dx = self.target_pos.x - self.cur_pos.x;
        let ax = dx * self.spring_strength;
        self.velocity.x += ax;
        self.velocity.x *= self.friction;
        self.cur_pos.x += self.velocity.x;

        // Y axis physics
        let dy = self.target_pos.y - self.cur_pos.y;
        let ay = dy * self.spring_strength;
        self.velocity.y += ay;
        self.velocity.y *= self.friction;
        self.cur_pos.y += self.velocity.y;

        // Z axis physics (for size scaling)
        let dox = self.original_pos.x - self.cur_pos.x;
        let doy = self.original_pos.y - self.cur_pos.y;
        let dd = (dox * dox) + (doy * doy);
        let d = dd.sqrt();

        self.target_pos.z = d / 100.0 + 1.0;
        let dz = self.target_pos.z - self.cur_pos.z;
        let az = dz * self.spring_strength;
        self.velocity.z += az;
        self.velocity.z *= self.friction;
        self.cur_pos.z += self.velocity.z;

        self.radius = self.size * self.cur_pos.z;
        if self.radius < 1.0 {
            self.radius = 1.0;
        }
    }

    fn draw(&self) {
        // Draw filled circle
        draw_circle(self.cur_pos.x, self.cur_pos.y, self.radius, self.colour);
        // Only draw outline if radius is large enough to avoid 1px white spots
        if self.radius > 2.5 {
            draw_circle_lines(self.cur_pos.x, self.cur_pos.y, self.radius, 2.0, self.colour);
        }
    }
}

struct PointCollection {
    mouse_pos: Vector3,
    points: Vec<Point>,
}

impl PointCollection {
    fn new() -> Self {
        Self {
            mouse_pos: Vector3::new(0.0, 0.0, 0.0),
            points: Vec::new(),
        }
    }

    fn update(&mut self) {
        for point in &mut self.points {
            let dx = self.mouse_pos.x - point.cur_pos.x;
            let dy = self.mouse_pos.y - point.cur_pos.y;
            let dd = (dx * dx) + (dy * dy);
            let d = dd.sqrt();

            if d < 150.0 {
                point.target_pos.x = point.cur_pos.x - dx;
                point.target_pos.y = point.cur_pos.y - dy;
            } else {
                point.target_pos.x = point.original_pos.x;
                point.target_pos.y = point.original_pos.y;
            }

            point.update();
        }
    }

    fn draw(&self) {
        for point in &self.points {
            point.draw();
        }
    }

    fn recenter_points(&mut self, canvas_width: f32, canvas_height: f32) {
        let offset_x = canvas_width / 2.0 - 180.0;
        let offset_y = canvas_height / 2.0 - 65.0;

        for point in &mut self.points {
            // Calculate relative position from the old center
            let rel_x = point.original_pos.x - offset_x;
            let rel_y = point.original_pos.y - offset_y;

            // Update to new center
            point.original_pos.x = offset_x + rel_x;
            point.original_pos.y = offset_y + rel_y;

            point.cur_pos.x = point.original_pos.x;
            point.cur_pos.y = point.original_pos.y;
        }
    }
}

fn create_google_balls(canvas_width: f32, canvas_height: f32) -> PointCollection {
    let mut collection = PointCollection::new();

    // Original Google logo ball data
    let ball_data = vec![
        (202.0, 78.0, 0.0, 9.0, "#ed9d33"),
        (348.0, 83.0, 0.0, 9.0, "#d44d61"),
        (256.0, 69.0, 0.0, 9.0, "#4f7af2"),
        (214.0, 59.0, 0.0, 9.0, "#ef9a1e"),
        (265.0, 36.0, 0.0, 9.0, "#4976f3"),
        (300.0, 78.0, 0.0, 9.0, "#269230"),
        (294.0, 59.0, 0.0, 9.0, "#1f9e2c"),
        (45.0, 88.0, 0.0, 9.0, "#1c48dd"),
        (268.0, 52.0, 0.0, 9.0, "#2a56ea"),
        (73.0, 83.0, 0.0, 9.0, "#3355d8"),
        (294.0, 6.0, 0.0, 9.0, "#36b641"),
        (235.0, 62.0, 0.0, 9.0, "#2e5def"),
        (353.0, 42.0, 0.0, 8.0, "#d53747"),
        (336.0, 52.0, 0.0, 8.0, "#eb676f"),
        (208.0, 41.0, 0.0, 8.0, "#f9b125"),
        (321.0, 70.0, 0.0, 8.0, "#de3646"),
        (8.0, 60.0, 0.0, 8.0, "#2a59f0"),
        (180.0, 81.0, 0.0, 8.0, "#eb9c31"),
        (146.0, 65.0, 0.0, 8.0, "#c41731"),
        (145.0, 49.0, 0.0, 8.0, "#d82038"),
        (246.0, 34.0, 0.0, 8.0, "#5f8af8"),
        (169.0, 69.0, 0.0, 8.0, "#efa11e"),
        (273.0, 99.0, 0.0, 8.0, "#2e55e2"),
        (248.0, 120.0, 0.0, 8.0, "#4167e4"),
        (294.0, 41.0, 0.0, 8.0, "#0b991a"),
        (267.0, 114.0, 0.0, 8.0, "#4869e3"),
        (78.0, 67.0, 0.0, 8.0, "#3059e3"),
        (294.0, 23.0, 0.0, 8.0, "#10a11d"),
        (117.0, 83.0, 0.0, 8.0, "#cf4055"),
        (137.0, 80.0, 0.0, 8.0, "#cd4359"),
        (14.0, 71.0, 0.0, 8.0, "#2855ea"),
        (331.0, 80.0, 0.0, 8.0, "#ca273c"),
        (25.0, 82.0, 0.0, 8.0, "#2650e1"),
        (233.0, 46.0, 0.0, 8.0, "#4a7bf9"),
        (73.0, 13.0, 0.0, 8.0, "#3d65e7"),
        (327.0, 35.0, 0.0, 6.0, "#f47875"),
        (319.0, 46.0, 0.0, 6.0, "#f36764"),
        (256.0, 81.0, 0.0, 6.0, "#1d4eeb"),
        (244.0, 88.0, 0.0, 6.0, "#698bf1"),
        (194.0, 32.0, 0.0, 6.0, "#fac652"),
        (97.0, 56.0, 0.0, 6.0, "#ee5257"),
        (105.0, 75.0, 0.0, 6.0, "#cf2a3f"),
        (42.0, 4.0, 0.0, 6.0, "#5681f5"),
        (10.0, 27.0, 0.0, 6.0, "#4577f6"),
        (166.0, 55.0, 0.0, 6.0, "#f7b326"),
        (266.0, 88.0, 0.0, 6.0, "#2b58e8"),
        (178.0, 34.0, 0.0, 6.0, "#facb5e"),
        (100.0, 65.0, 0.0, 6.0, "#e02e3d"),
        (343.0, 32.0, 0.0, 6.0, "#f16d6f"),
        (59.0, 5.0, 0.0, 6.0, "#507bf2"),
        (27.0, 9.0, 0.0, 6.0, "#5683f7"),
        (233.0, 116.0, 0.0, 6.0, "#3158e2"),
        (123.0, 32.0, 0.0, 6.0, "#f0696c"),
        (6.0, 38.0, 0.0, 6.0, "#3769f6"),
        (63.0, 62.0, 0.0, 6.0, "#6084ef"),
        (6.0, 49.0, 0.0, 6.0, "#2a5cf4"),
        (108.0, 36.0, 0.0, 6.0, "#f4716e"),
        (169.0, 43.0, 0.0, 6.0, "#f8c247"),
        (137.0, 37.0, 0.0, 6.0, "#e74653"),
        (318.0, 58.0, 0.0, 6.0, "#ec4147"),
        (226.0, 100.0, 0.0, 5.0, "#4876f1"),
        (101.0, 46.0, 0.0, 5.0, "#ef5c5c"),
        (226.0, 108.0, 0.0, 5.0, "#2552ea"),
        (17.0, 17.0, 0.0, 5.0, "#4779f7"),
        (232.0, 93.0, 0.0, 5.0, "#4b78f1"),
    ];

    for (x, y, z, size, color) in ball_data {
        let mut point = Point::new(
            (canvas_width / 2.0 - 180.0) + x,
            (canvas_height / 2.0 - 65.0) + y,
            z,
            size,
            color,
        );

        point.original_pos.x = (canvas_width / 2.0 - 180.0) + x;
        point.original_pos.y = (canvas_height / 2.0 - 65.0) + y;

        collection.points.push(point);
    }

    collection
}

fn window_conf() -> Conf {
    Conf {
        window_title: "Google Balls".to_owned(),
        window_width: 800,
        window_height: 600,
        window_resizable: true,
        // Include icon data directly in the binary
        icon: Some(miniquad::conf::Icon {
            small: *include_bytes!("../images/icon-16.rgba"),
            medium: *include_bytes!("../images/icon-32.rgba"),
            big: *include_bytes!("../images/icon-64.rgba"),
        }),
        ..Default::default()
    }
}

#[macroquad::main(window_conf)]
async fn main() {
    let mut point_collection = create_google_balls(screen_width(), screen_height());
    let mut last_screen_size = (screen_width(), screen_height());

    loop {
        // Handle window resize
        let current_screen_size = (screen_width(), screen_height());
        if current_screen_size != last_screen_size {
            point_collection.recenter_points(screen_width(), screen_height());
            last_screen_size = current_screen_size;
        }

        // Update mouse position
        let (mouse_x, mouse_y) = mouse_position();
        point_collection.mouse_pos.set(mouse_x, mouse_y, 0.0);

        // Clear screen
        clear_background(WHITE);

        // Update and draw points
        point_collection.update();
        point_collection.draw();

        next_frame().await
    }
}
