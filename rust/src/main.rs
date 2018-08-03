extern crate exec;

use std::env;
use std::ffi::OsString;
use std::path::{Path, PathBuf};
use std::process;

/// Follow symlinks starting with `link`, until a real file is found.
fn walk_symlink(link: &Path) -> PathBuf {
    let mut p = PathBuf::from(link);

    while let Ok(dest) = p.read_link() {
        p = p.parent().unwrap_or(&p).join(dest);
    };

    p
}

/// Exec a script `script` using an interpreter `relcmd` relative to script
///
/// If successful, do not return (the target process has been exec()ed).
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

/// Entry point for the program
fn main() {
    let mut argv = env::args_os();

    let result = match argv.len() {
        0...2 => {
            let program = argv.next().unwrap_or_else(|| "relexec".into());
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
                &argv.collect::<Vec<_>>(),
            )
        }
    };

    if let Err(msg) = result {
        eprintln!("{}", msg);
        process::exit(1);
    }
}
