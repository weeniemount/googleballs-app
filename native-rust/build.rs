use std::{env, io};

#[cfg(windows)]
extern crate winres;

fn main() -> io::Result<()> {
    // Only compile resources on Windows
    if env::var_os("CARGO_CFG_WINDOWS").is_some() {
        #[cfg(windows)]
        {
            let mut res = winres::WindowsResource::new();
            res.set_icon("images/icon.ico");
            res.set("ProductName", "Google Balls");
            res.set("FileDescription", "Google Balls");
            res.set("CompanyName", "weenie");
            res.compile()?;
        }
    }
    Ok(())
}
