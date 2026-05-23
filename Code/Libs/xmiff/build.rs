use std::fs;
use std::path::Path;

fn main() {
    // After build completes, copy exe to output folder
    let target_dir = "target/release";
    let output_dir = "../bin";
    
    fs::create_dir_all(output_dir).ok();
    
    #[cfg(windows)]
    let exe_name = "xmiff.exe";
    #[cfg(not(windows))]
    let exe_name = "xmiff";
    
    let src = Path::new(target_dir).join(exe_name);
    let dst = Path::new(output_dir).join(exe_name);
    
    if src.exists() {
        fs::copy(&src, &dst).ok();
    }
}