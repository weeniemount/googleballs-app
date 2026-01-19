/// Google Balls adapted for touchbar display
/// Reacts to touch input

#[derive(Clone, Debug)]
pub struct Vector2 {
    pub x: f32,
    pub y: f32,
}

impl Vector2 {
    pub fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
}

#[derive(Clone)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

impl Color {
    pub fn from_hex(hex: &str) -> Self {
        let hex = hex.trim_start_matches('#');
        if hex.len() != 6 {
            return Self { r: 255, g: 255, b: 255 };
        }
        let r = u8::from_str_radix(&hex[0..2], 16).unwrap_or(255);
        let g = u8::from_str_radix(&hex[2..4], 16).unwrap_or(255);
        let b = u8::from_str_radix(&hex[4..6], 16).unwrap_or(255);
        Self { r, g, b }
    }
}

pub struct Ball {
    pub color: Color,
    pub pos: Vector2,
    pub original_pos: Vector2,
    pub velocity: Vector2,
    pub radius: f32,
    pub base_radius: f32,
    pub target_pos: Vector2,
}

impl Ball {
    pub fn new(x: f32, y: f32, radius: f32, color_hex: &str) -> Self {
        Self {
            color: Color::from_hex(color_hex),
            pos: Vector2::new(x, y),
            original_pos: Vector2::new(x, y),
            velocity: Vector2::new(0.0, 0.0),
            target_pos: Vector2::new(x, y),
            radius,
            base_radius: radius,
        }
    }

    pub fn update(&mut self) {
        let dx = self.target_pos.x - self.pos.x;
        let ax = dx * 0.1;
        self.velocity.x += ax;
        self.velocity.x *= 0.8;
        self.pos.x += self.velocity.x;

        let dy = self.target_pos.y - self.pos.y;
        let ay = dy * 0.1;
        self.velocity.y += ay;
        self.velocity.y *= 0.8;
        self.pos.y += self.velocity.y;

        let dox = self.original_pos.x - self.pos.x;
        let doy = self.original_pos.y - self.pos.y;
        let d = (dox * dox + doy * doy).sqrt();
        
        let target_scale = d / 100.0 + 1.0;
        self.radius = self.base_radius * target_scale;
        if self.radius < 1.0 {
            self.radius = 1.0;
        }
    }
}

pub struct GoogleBalls {
    pub balls: Vec<Ball>,
    pub touch_pos: Option<Vector2>,
    pub disp_width: u16,
    pub disp_height: u16,
    pub dark_mode: bool,
    pub switch_area: (f32, f32),
}

impl GoogleBalls {
    pub fn new(disp_width: u16, disp_height: u16) -> Self {
        let ball_data = vec![
            (202.0, 78.0, 9.0, "#ed9d33"),
            (348.0, 83.0, 9.0, "#d44d61"),
            (256.0, 69.0, 9.0, "#4f7af2"),
            (214.0, 59.0, 9.0, "#ef9a1e"),
            (265.0, 36.0, 9.0, "#4976f3"),
            (300.0, 78.0, 9.0, "#269230"),
            (294.0, 59.0, 9.0, "#1f9e2c"),
            (45.0, 88.0, 9.0, "#1c48dd"),
            (268.0, 52.0, 9.0, "#2a56ea"),
            (73.0, 83.0, 9.0, "#3355d8"),
            (294.0, 6.0, 9.0, "#36b641"),
            (235.0, 62.0, 9.0, "#2e5def"),
            (353.0, 42.0, 8.0, "#d53747"),
            (336.0, 52.0, 8.0, "#eb676f"),
            (208.0, 41.0, 8.0, "#f9b125"),
            (321.0, 70.0, 8.0, "#de3646"),
            (8.0, 60.0, 8.0, "#2a59f0"),
            (180.0, 81.0, 8.0, "#eb9c31"),
            (146.0, 65.0, 8.0, "#c41731"),
            (145.0, 49.0, 8.0, "#d82038"),
            (246.0, 34.0, 8.0, "#5f8af8"),
            (169.0, 69.0, 8.0, "#efa11e"),
            (273.0, 99.0, 8.0, "#2e55e2"),
            (248.0, 120.0, 8.0, "#4167e4"),
            (294.0, 41.0, 8.0, "#0b991a"),
            (267.0, 114.0, 8.0, "#4869e3"),
            (78.0, 67.0, 8.0, "#3059e3"),
            (294.0, 23.0, 8.0, "#10a11d"),
            (117.0, 83.0, 8.0, "#cf4055"),
            (137.0, 80.0, 8.0, "#cd4359"),
            (14.0, 71.0, 8.0, "#2855ea"),
            (331.0, 80.0, 8.0, "#ca273c"),
            (25.0, 82.0, 8.0, "#2650e1"),
            (233.0, 46.0, 8.0, "#4a7bf9"),
            (73.0, 13.0, 8.0, "#3d65e7"),
            (327.0, 35.0, 6.0, "#f47875"),
            (319.0, 46.0, 6.0, "#f36764"),
            (256.0, 81.0, 6.0, "#1d4eeb"),
            (244.0, 88.0, 6.0, "#698bf1"),
            (194.0, 32.0, 6.0, "#fac652"),
            (97.0, 56.0, 6.0, "#ee5257"),
            (105.0, 75.0, 6.0, "#cf2a3f"),
            (42.0, 4.0, 6.0, "#5681f5"),
            (10.0, 27.0, 6.0, "#4577f6"),
            (166.0, 55.0, 6.0, "#f7b326"),
            (266.0, 88.0, 6.0, "#2b58e8"),
            (178.0, 34.0, 6.0, "#facb5e"),
            (100.0, 65.0, 6.0, "#e02e3d"),
            (343.0, 32.0, 6.0, "#f16d6f"),
            (59.0, 5.0, 6.0, "#507bf2"),
            (27.0, 9.0, 6.0, "#5683f7"),
            (233.0, 116.0, 6.0, "#3158e2"),
            (123.0, 32.0, 6.0, "#f0696c"),
            (6.0, 38.0, 6.0, "#3769f6"),
            (63.0, 62.0, 6.0, "#6084ef"),
            (6.0, 49.0, 6.0, "#2a5cf4"),
            (108.0, 36.0, 6.0, "#f4716e"),
            (169.0, 43.0, 6.0, "#f8c247"),
            (137.0, 37.0, 6.0, "#e74653"),
            (318.0, 58.0, 6.0, "#ec4147"),
            (226.0, 100.0, 5.0, "#4876f1"),
            (101.0, 46.0, 5.0, "#ef5c5c"),
            (226.0, 108.0, 5.0, "#2552ea"),
            (17.0, 17.0, 5.0, "#4779f7"),
            (232.0, 93.0, 5.0, "#4b78f1"),
        ];

        let original_width = 360.0;
        let original_height = 130.0;
        
        let scale = 50.0 / original_height;
        let scaled_width = original_width * scale;
        
        let offset_x = (disp_height as f32 - scaled_width) / 2.0;
        let offset_y = 7.0;

        let mut balls = Vec::new();
        for (ox, oy, size, color) in ball_data {
            let fb_x = (disp_width as f32) - (offset_y + (oy * scale));
            let fb_y = offset_x + (ox * scale);
            let new_size = size * scale;
            
            balls.push(Ball::new(fb_x, fb_y, new_size, color));
        }

        Self {
            balls,
            touch_pos: None,
            disp_width,
            disp_height,
            dark_mode: false,
            switch_area: (120.0, 0.0),
        }
    }

    pub fn set_touch_raw(&mut self, raw_x: f32, raw_y: f32) {
        let fb_x = (self.disp_width as f32) - raw_y;
        let fb_y = raw_x;
        self.touch_pos = Some(Vector2::new(fb_x, fb_y));
    }

    pub fn handle_touch_start(&mut self) {
        if let Some(ref touch) = self.touch_pos {
            if touch.y < self.switch_area.0 {
                self.dark_mode = !self.dark_mode;
            }
        }
    }

    pub fn clear_touch(&mut self) {
        self.touch_pos = None;
    }

    pub fn update(&mut self) {
        const INTERACTION_RADIUS: f32 = 40.0;

        for ball in &mut self.balls {
            if let Some(ref touch) = self.touch_pos {
                if touch.y < self.switch_area.0 {
                     ball.target_pos.x = ball.original_pos.x;
                     ball.target_pos.y = ball.original_pos.y;
                     continue;
                }

                let dx = touch.x - ball.pos.x;
                let dy = touch.y - ball.pos.y;
                let dist = (dx * dx + dy * dy).sqrt();

                if dist < INTERACTION_RADIUS {
                    ball.target_pos.x = ball.pos.x - dx;
                    ball.target_pos.y = ball.pos.y - dy;
                } else {
                    ball.target_pos.x = ball.original_pos.x;
                    ball.target_pos.y = ball.original_pos.y;
                }
            } else {
                ball.target_pos.x = ball.original_pos.x;
                ball.target_pos.y = ball.original_pos.y;
            }

            ball.update();
        }
    }
}

pub fn draw_circle(
    buffer: &mut [u8],
    pitch: u32,
    cx: f32,
    cy: f32,
    radius: f32,
    color: &Color,
    buf_width: u16,
    buf_height: u16,
) {
    let min_x = ((cx - radius - 1.0).floor() as i32).max(0) as u16;
    let max_x = ((cx + radius + 1.0).ceil() as i32).min(buf_width as i32 - 1) as u16;
    let min_y = ((cy - radius - 1.0).floor() as i32).max(0) as u16;
    let max_y = ((cy + radius + 1.0).ceil() as i32).min(buf_height as i32 - 1) as u16;

    for py in min_y..=max_y {
        for px in min_x..=max_x {
            let dx = px as f32 - cx;
            let dy = py as f32 - cy;
            let dist_sq = dx * dx + dy * dy;
            
            if dist_sq > (radius + 1.0) * (radius + 1.0) {
                continue;
            }

            let dist = dist_sq.sqrt();
            let delta = dist - radius;

            if delta <= -0.5 {
                let offset = (py as u32 * pitch + px as u32 * 4) as usize;
                if offset + 4 <= buffer.len() {
                    buffer[offset] = color.b;
                    buffer[offset + 1] = color.g;
                    buffer[offset + 2] = color.r;
                    buffer[offset + 3] = 0;
                }
            } else if delta < 0.5 {
                let alpha = 0.5 - delta;
                
                let offset = (py as u32 * pitch + px as u32 * 4) as usize;
                if offset + 4 <= buffer.len() {
                    let bg_b = buffer[offset] as f32;
                    let bg_g = buffer[offset + 1] as f32;
                    let bg_r = buffer[offset + 2] as f32;

                    let new_b = (color.b as f32 * alpha + bg_b * (1.0 - alpha)) as u8;
                    let new_g = (color.g as f32 * alpha + bg_g * (1.0 - alpha)) as u8;
                    let new_r = (color.r as f32 * alpha + bg_r * (1.0 - alpha)) as u8;

                    buffer[offset] = new_b;
                    buffer[offset + 1] = new_g;
                    buffer[offset + 2] = new_r;
                }
            }
        }
    }
}

pub fn draw_switch(
    buffer: &mut [u8],
    pitch: u32,
    dark_mode: bool,
    buf_width: u16,
    buf_height: u16,
) {
    let center_y: f32 = 60.0;
    let center_x: f32 = 30.0;
    let width: f32 = 50.0;
    let height: f32 = 26.0;
    let radius: f32 = height / 2.0;
    
    let pill_color = if dark_mode {
        Color { r: 60, g: 60, b: 60 }
    } else {
        Color { r: 200, g: 200, b: 200 }
    };
    
    let toggle_color = if dark_mode {
        Color { r: 255, g: 255, b: 255 }
    } else {
        Color { r: 20, g: 20, b: 20 }
    };

    let rect_min_x = (center_x - radius).floor() as i32;
    let rect_max_x = (center_x + radius).ceil() as i32;
    let rect_min_y = (center_y - width / 2.0).floor() as i32;
    let rect_max_y = (center_y + width / 2.0).ceil() as i32;
    
    for py in (rect_min_y - 2)..(rect_max_y + 2) {
        for px in (rect_min_x - 2)..(rect_max_x + 2) {
             if px < 0 || px >= buf_width as i32 || py < 0 || py >= buf_height as i32 {
                continue;
            }

            let cy_top = center_y - (width / 2.0) + radius;
            let cy_bot = center_y + (width / 2.0) - radius;
            
            let px_f = px as f32;
            let py_f = py as f32;
            
            let cx = center_x;
            
            let mut dist = 0.0;
            let mut inside = false;
            
            if py_f < cy_top {
                dist = ((px_f - cx).powi(2) + (py_f - cy_top).powi(2)).sqrt();
                inside = true;
            } else if py_f > cy_bot {
                dist = ((px_f - cx).powi(2) + (py_f - cy_bot).powi(2)).sqrt();
                inside = true;
            } else {
                dist = (px_f - cx).abs();
                inside = true;
            }
            
            if inside {
                 let delta = dist - radius;
                 if delta <= -0.5 {
                     let offset = (py as u32 * pitch + px as u32 * 4) as usize;
                     if offset + 4 <= buffer.len() {
                        buffer[offset] = pill_color.b;
                        buffer[offset + 1] = pill_color.g;
                        buffer[offset + 2] = pill_color.r;
                     }
                 } else if delta < 0.5 {
                     let alpha = 0.5 - delta;
                     let offset = (py as u32 * pitch + px as u32 * 4) as usize;
                     if offset + 4 <= buffer.len() {
                        let bg_b = buffer[offset] as f32;
                        let bg_g = buffer[offset + 1] as f32;
                        let bg_r = buffer[offset + 2] as f32;

                        let new_b = (pill_color.b as f32 * alpha + bg_b * (1.0 - alpha)) as u8;
                        let new_g = (pill_color.g as f32 * alpha + bg_g * (1.0 - alpha)) as u8;
                        let new_r = (pill_color.r as f32 * alpha + bg_r * (1.0 - alpha)) as u8;
                        
                        buffer[offset] = new_b;
                        buffer[offset + 1] = new_g;
                        buffer[offset + 2] = new_r;
                     }
                 }
            }
        }
    }
    
    let toggle_off_y = center_y - (width / 2.0) + radius + 2.0;
    let toggle_on_y = center_y + (width / 2.0) - radius - 2.0;
    
    let current_toggle_y = if dark_mode { toggle_on_y } else { toggle_off_y };
    let toggle_radius = radius - 4.0;
    
    draw_circle(buffer, pitch, center_x, current_toggle_y, toggle_radius, &toggle_color, buf_width, buf_height);
}

pub fn clear_buffer(buffer: &mut [u8], pitch: u32, width: u16, height: u16, r: u8, g: u8, b: u8) {
    for y in 0..height {
        for x in 0..width {
            let offset = (y as u32 * pitch + x as u32 * 4) as usize;
            if offset + 4 <= buffer.len() {
                buffer[offset] = b;
                buffer[offset + 1] = g;
                buffer[offset + 2] = r;
                buffer[offset + 3] = 0;
            }
        }
    }
}


