extern crate exec;

use std::env;
use std::ffi::OsString;
use std::path::{Path, PathBuf};
use std::process;

fn walk_symlink(link: &Path) -> PathBuf {
    let mut p = PathBuf::from(link);

    loop {
        match p.read_link() {
            Ok(dest) => {
                p = dest;
            }
            Err(_) => break p,
        }
    }
}

fn exec_relative(relcmd: OsString, script: OsString, argv: &[OsString]) -> Result<(), String> {
    let relcmd = Path::new(&relcmd);
    let origin_script = walk_symlink(Path::new(&script));

    let dir = origin_script.parent().unwrap_or(&origin_script);
    let target_interp = dir.join(relcmd);

    let mut cmd = exec::Command::new(&target_interp);
    cmd.arg(&script).args(argv);

    let err = cmd.exec();
    Err(format!(
        "Error executing {}: {}",
        target_interp.display(),
        err
    ))
}

fn main() {
    let mut argv = env::args_os();

    let result = match argv.len() {
        0...2 => {
            let program = argv.next().unwrap_or_else(|| OsString::from("relexec"));
            Err(format!(
                "Usage: {} <interpreter> <file> ...",
                Path::new(&program).display()
            ))
        }
        _ => {
            argv.next();
            exec_relative(
                argv.next().unwrap(),
                argv.next().unwrap(),
                &argv.collect::<Vec<OsString>>(),
            )
        }
    };

    match result {
        Ok(_) => (),
        Err(msg) => {
            eprintln!("{}", msg);
            process::exit(1);
        }
    }
}
