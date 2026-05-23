use std::{
    collections::HashMap,
    fs, io,
    io::{BufWriter, Write},
    path::Path,
};

use clap::Parser;

#[derive(Debug)]
struct Entry {
    id: i32,
    volume: f32,
    filename: String,
    cutoff: f32,
}

#[derive(Parser, Debug)]
#[command(name = "xmiff")]
#[command(about = "Compile XMF sound effect data to binary format")]
#[command(version)]
struct Args {
    /// Path to directory containing sfxid.h header
    #[arg(short = 'i', long)]
    header_dir: String,

    /// Input XMF file
    input: String,

    /// Output DAT file
    output: String,
}

struct SfxCompiler {
    enum_map: HashMap<String, i32>,
}

impl SfxCompiler {
    fn new(header_dir: &str) -> io::Result<Self> {
        let mut compiler = Self {
            enum_map: HashMap::new(),
        };
        compiler.load_enum_from_header(header_dir)?;
        Ok(compiler)
    }

    fn load_enum_from_header(&mut self, header_dir: &str) -> io::Result<()> {
        let header_file = Path::new(header_dir).join("sfxid.h");

        let content = fs::read_to_string(&header_file)
            .map_err(|e| io::Error::new(e.kind(), format!("Cannot find {}: {}", header_file.display(), e)))?;

        let enum_start = content
            .find("enum ID")
            .ok_or_else(|| io::Error::new(io::ErrorKind::NotFound, "Could not find enum ID"))?;

        let brace_start = content[enum_start..]
            .find('{')
            .ok_or_else(|| io::Error::new(io::ErrorKind::InvalidData, "Missing enum opening brace"))?
            + enum_start;

        let brace_end = content[brace_start..]
            .find('}')
            .ok_or_else(|| io::Error::new(io::ErrorKind::InvalidData, "Missing enum closing brace"))?
            + brace_start;

        let enum_body = &content[brace_start + 1..brace_end];
        let mut enum_id = 0;

        for line in enum_body.lines() {
            let line = line.trim();

            if line.is_empty() || line.starts_with("//") {
                continue;
            }

            let line = line.split("//").next().unwrap_or("").trim();

            if line.is_empty() {
                continue;
            }

            if let Some(eq_pos) = line.find('=') {
                let name = line[..eq_pos].trim();
                let value_str = line[eq_pos + 1..].trim().trim_end_matches(',');

                if let Ok(value) = value_str.parse::<i32>() {
                    self.enum_map.insert(name.to_string(), value);
                    enum_id = value + 1;
                }
            } else {
                let name = line.trim_end_matches(',').trim();
                if !name.is_empty() && name != "LAST" {
                    self.enum_map.insert(name.to_string(), enum_id);
                    enum_id += 1;
                }
            }
        }

        println!("Loaded {} sound IDs from header", self.enum_map.len());
        Ok(())
    }

    fn parse_xmf(&self, xmf_file: &str) -> io::Result<Vec<Entry>> {
        let content = fs::read_to_string(xmf_file)?;
        let cleaned = Self::remove_comments(&content);

        let mut entries = Vec::new();

        for line in cleaned.lines() {
            let line = line.trim();

            if !line.starts_with("SFXCHUNK") {
                continue;
            }

            let open = match line.find('(') {
                Some(v) => v,
                None => continue,
            };

            let close = match line.rfind(')') {
                Some(v) => v,
                None => continue,
            };

            let args = &line[open + 1..close];
            let parts = split_args(args);

            if parts.len() < 3 {
                continue;
            }

            let sfx_id_name = parts[0].trim();
            let volume: f32 = match parts[1].trim().parse() {
                Ok(v) => v,
                Err(_) => continue,
            };
            let filename = parts[2].trim().trim_matches('"');
            let cutoff: f32 = parts.get(3)
                .and_then(|s| s.trim().parse().ok())
                .unwrap_or(0.5);

            let id = match self.enum_map.get(sfx_id_name) {
                Some(&id) => id,
                None => {
                    eprintln!("Warning: '{}' not found in enum, skipping", sfx_id_name);
                    continue;
                }
            };

            entries.push(Entry {
                id,
                volume,
                filename: filename.to_string(),
                cutoff,
            });
        }

        println!("Parsed {} sound chunks from {}", entries.len(), xmf_file);
        Ok(entries)
    }

    fn remove_comments(content: &str) -> String {
        let mut result = String::new();
        let mut chars = content.chars().peekable();

        while let Some(ch) = chars.next() {
            if ch == '/' {
                match chars.peek() {
                    Some(&'/') => {
                        chars.next();
                        while let Some(c) = chars.next() {
                            if c == '\n' {
                                result.push('\n');
                                break;
                            }
                        }
                    }
                    Some(&'*') => {
                        chars.next();
                        while let Some(c) = chars.next() {
                            if c == '*' && chars.peek() == Some(&'/') {
                                chars.next();
                                break;
                            }
                        }
                    }
                    _ => result.push(ch),
                }
            } else {
                result.push(ch);
            }
        }

        result
    }

    fn write_dat(&self, entries: &[Entry], output_file: &str) -> io::Result<()> {
        let file = fs::File::create(output_file)?;
        let mut writer = BufWriter::new(file);

        let last_value = entries.iter().map(|e| e.id).max().unwrap_or(0) + 1;
        writer.write_all(&(last_value as u32).to_le_bytes())?;

        for entry in entries {
            writer.write_all(&entry.id.to_le_bytes())?;
            writer.write_all(&entry.volume.to_le_bytes())?;

            let mut filename_buf = [0u8; 32];
            let filename_bytes = entry.filename.as_bytes();
            let len = filename_bytes.len().min(31);

            if filename_bytes.len() > 31 {
                eprintln!("Warning: filename '{}' truncated to 31 chars", entry.filename);
            }

            filename_buf[..len].copy_from_slice(&filename_bytes[..len]);
            writer.write_all(&filename_buf)?;
            writer.write_all(&entry.cutoff.to_le_bytes())?;
        }

        writer.flush()?;
        println!("Wrote {} entries to {}", entries.len(), output_file);

        Ok(())
    }
}

fn split_args(s: &str) -> Vec<String> {
    let mut result = Vec::new();
    let mut current = String::new();
    let mut in_string = false;

    for c in s.chars() {
        match c {
            '"' => {
                in_string = !in_string;
                current.push(c);
            }
            ',' if !in_string => {
                result.push(current.trim().to_string());
                current.clear();
            }
            _ => current.push(c),
        }
    }

    if !current.is_empty() {
        result.push(current.trim().to_string());
    }

    result
}

fn main() {
    let args = Args::parse();

    if !Path::new(&args.header_dir).is_dir() {
        eprintln!("Error: Header directory '{}' not found", args.header_dir);
        std::process::exit(1);
    }

    if !Path::new(&args.input).is_file() {
        eprintln!("Error: Input file '{}' not found", args.input);
        std::process::exit(1);
    }

    // Build output path in same directory as input
    let output_path = Path::new(&args.input)
        .parent()
        .unwrap_or_else(|| Path::new("."))
        .join(&args.output);

    if let Err(e) = run(&args.header_dir, &args.input, output_path.to_str().unwrap()) {
        eprintln!("Error: {}", e);
        std::process::exit(1);
    }

    println!("✓ Successfully compiled {} -> {}", args.input, output_path.display());
}

fn run(header_dir: &str, xmf_file: &str, output_file: &str) -> io::Result<()> {
    let compiler = SfxCompiler::new(header_dir)?;
    let entries = compiler.parse_xmf(xmf_file)?;
    compiler.write_dat(&entries, output_file)?;
    Ok(())
}