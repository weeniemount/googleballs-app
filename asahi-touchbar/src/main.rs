use anyhow::Result;
use drm::buffer::Buffer;
use drm::control::{ClipRect, Device as ControlDevice};
use input::{
    event::{
        touch::{TouchEvent, TouchEventPosition},
        Event,
    },
    Libinput, LibinputInterface,
};
use libc::{O_ACCMODE, O_RDONLY, O_RDWR, O_WRONLY};
use std::fs::{File, OpenOptions};
use std::os::fd::OwnedFd;
use std::os::unix::fs::OpenOptionsExt;
use std::path::Path;
use std::time::{Duration, Instant};

mod display;
mod googleballs;

use display::DrmBackend;
use googleballs::{draw_circle, clear_buffer, GoogleBalls};

struct Interface;

impl LibinputInterface for Interface {
    fn open_restricted(&mut self, path: &Path, flags: i32) -> Result<OwnedFd, i32> {
        let mode = flags & O_ACCMODE;
        OpenOptions::new()
            .custom_flags(flags)
            .read(mode == O_RDONLY || mode == O_RDWR)
            .write(mode == O_WRONLY || mode == O_RDWR)
            .open(path)
            .map(|file| file.into())
            .map_err(|err| err.raw_os_error().unwrap_or(-1))
    }
    fn close_restricted(&mut self, fd: OwnedFd) {
        _ = File::from(fd);
    }
}

fn main() -> Result<()> {
    unsafe {
        libc::signal(libc::SIGINT, signal_handler as usize);
        libc::signal(libc::SIGTERM, signal_handler as usize);
    }

    println!("Opening Touch Bar...");
    
    let _guard = ServiceGuard::new();

    let drm = DrmBackend::open_card()?;
    let mode = drm.mode;
    let (disp_width, disp_height) = mode.size();
    let pitch = drm.db.pitch();

    println!("Touch Bar: {}x{} (pitch: {})", disp_width, disp_height, pitch);

    let mut balls = GoogleBalls::new(disp_width, disp_height);
    
    let buffer_size = (pitch * disp_height as u32) as usize;
    let mut back_buffer = vec![0u8; buffer_size];
    
    let mut input = Libinput::new_with_udev(Interface);
    if let Err(e) = input.udev_assign_seat("seat-touchbar") {
        println!("Warning: Could not assign seat-touchbar: {:?}", e);
        if let Err(e2) = input.udev_assign_seat("seat0") {
            println!("Warning: Could not assign seat0 either: {:?}", e2);
        }
    }
    
    let mut last_frame = Instant::now();

    println!("Running animation. Press Ctrl+C to exit.");

    let DrmBackend { card, mut db, fb, .. } = drm;
    
    let mut map = card.map_dumb_buffer(&mut db)?;

    while !SHOULD_EXIT.load(Ordering::Relaxed) {
        loop {
            input.dispatch().ok();
            let mut got_event = false;
            for event in input.by_ref() {
                got_event = true;
                if let Event::Touch(te) = event {
                    match te {
                        TouchEvent::Down(dn) => {
                            let x = dn.x_transformed(disp_height as u32) as f32;
                            let y = dn.y_transformed(disp_width as u32) as f32;
                            balls.set_touch_raw(x, y);
                            balls.handle_touch_start();
                        }
                        TouchEvent::Motion(mtn) => {
                            let x = mtn.x_transformed(disp_height as u32) as f32;
                            let y = mtn.y_transformed(disp_width as u32) as f32;
                            balls.set_touch_raw(x, y);
                        }
                        TouchEvent::Up(_) => {
                            balls.clear_touch();
                        }
                        _ => {}
                    }
                }
            }
            if !got_event {
                break;
            }
        }

        balls.update();

        let (bg_r, bg_g, bg_b) = if balls.dark_mode {
            (20, 20, 20)
        } else {
            (255, 255, 255)
        };
        
        clear_buffer(&mut back_buffer, pitch, disp_width, disp_height, bg_r, bg_g, bg_b);
        
        googleballs::draw_switch(
            &mut back_buffer, 
            pitch, 
            balls.dark_mode, 
            disp_width, 
            disp_height
        );
        
        for ball in &balls.balls {
            draw_circle(
                &mut back_buffer,
                pitch,
                ball.pos.x,
                ball.pos.y,
                ball.radius,
                &ball.color,
                disp_width,
                disp_height,
            );
        }

        {
            let fb_slice = map.as_mut();
            fb_slice[..back_buffer.len()].copy_from_slice(&back_buffer);
        }

        card.dirty_framebuffer(fb, &[ClipRect::new(0, 0, disp_height, disp_width)])?;

        last_frame = Instant::now();
    }
    
    println!("Exiting...");
    Ok(())
}

use std::sync::atomic::{AtomicBool, Ordering};
use std::process::Command;

static SHOULD_EXIT: AtomicBool = AtomicBool::new(false);

extern "C" fn signal_handler(_: i32) {
    SHOULD_EXIT.store(true, Ordering::Relaxed);
}

struct ServiceGuard;

impl ServiceGuard {
    fn new() -> Self {
        println!("Stopping tiny-dfr...");
        let _ = Command::new("systemctl")
            .arg("stop")
            .arg("tiny-dfr")
            .status();
        ServiceGuard
    }
}

impl Drop for ServiceGuard {
    fn drop(&mut self) {
        println!("Starting tiny-dfr...");
        let _ = Command::new("systemctl")
            .arg("start")
            .arg("tiny-dfr")
            .status();
    }
}
