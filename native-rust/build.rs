use std::io;

#[cfg(windows)]
extern crate winres;

fn main() -> io::Result<()> {
    #[cfg(windows)]
    {
        let mut res = winres::WindowsResource::new();
        res.set_icon("src/images/icon.ico");
        res.set("ProductName", "Google Balls");
        res.set("FileDescription", "Google balls real!!!!!!!!!!!");
        res.set("CompanyName", "weenie");
        res.compile()?;
    }
    Ok(())
}