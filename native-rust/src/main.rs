use googleballs::run_game;
use macroquad::prelude::*;

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
    run_game().await;
}