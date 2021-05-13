from unittest import TestCase, main
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess


ROOT = Path(__file__).parent


class RelexecTests:
    """Implementation of tests for relexec.

    This class is abstract because it does not define the relexec
    implementation; concrete instances will be created for the C and Rust
    versions.
    """

    @classmethod
    def setUpClass(cls):
        """Set up a tempdir containing an interpreter and a script.

        The 'interpreter' is a Bash script which will print its argv. The
        script is a nonsense script that will not be executed, but with a
        relexec shebang that points to the 'interpreter' script.

        """
        cls.tmpdir = TemporaryDirectory()
        tmp = cls.tmp = Path(cls.tmpdir.name)

        relexec = cls.relexec = cls.get_executable().resolve()

        cls.interp = cls.write_script(
            'interp',
            "#!/bin/sh\n" +
            'echo "$0" "$@"\n'
        )

        cls.script = cls.write_script(
            'script',
            f"#!{relexec} interp\n\n" +
            "roar 'hello world'\n" +
            "yeet 0\n\n"
        )

    @classmethod
    def write_script(cls, name: str, text: str) -> Path:
        """Write a script into the temporary directory."""
        path = cls.tmp.joinpath(name)
        path.write_text(text)
        path.chmod(0o755)
        return path

    @classmethod
    def tearDownClass(cls):
        """Delete the temporary directory and its contents."""
        cls.tmpdir.cleanup()

    def run_script(self, script, *args) -> str:
        """Run the script in the temp dir."""
        assert script == 'script'
        return subprocess.check_output(
            [self.script, *args],
            text=True
        ).strip()

    def test_run_script(self):
        """When the script is run, the interpreter is executed."""
        out = self.run_script('script')
        tmp = self.tmp
        self.assertEqual(out, f"{tmp}/interp {tmp}/script")

    def test_run_script_arg(self):
        """Arguments passed to the script are pass to the interpreter."""
        out = self.run_script('script', '--commit', 'HEAD')
        tmp = self.tmp
        self.assertEqual(out, f"{tmp}/interp {tmp}/script --commit HEAD")

    def test_no_args(self):
        """When we run relexec with no arguments it returns an error."""
        result = subprocess.run([self.relexec], capture_output=True)
        self.assertEqual(result.returncode, 2)

    def test_one_args(self):
        """When we run relexec with one argument it returns an error.

        Relexec requires two arguments: the relative path, and the target
        absolute path.
        """
        result = subprocess.run(
            [self.relexec, 'interp'],
            capture_output=True,
        )
        self.assertEqual(result.returncode, 2)

    def test_two_args(self):
        """When we run relexec with two arguments it execs the interpreter.

        This is equivalent to the usage in a script but is here invoked
        directly.
        """
        result = subprocess.run(
            [self.relexec, 'interp', self.script],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0)
        self.assertEqual(
            result.stdout,
            f'{self.tmp}/interp {self.tmp}/script\n'
        )

    def test_exec_failure(self):
        """If relexec exits with an error if execing the interpreter fails."""
        deactivated = self.tmp / 'interp~'
        self.interp.rename(deactivated)
        try:
            result = subprocess.run([self.script], capture_output=True)
        finally:
            deactivated.rename(self.interp)
        self.assertEqual(result.returncode, 127)

    def test_script_symlink(self):
        """The script can be run from a symlink.

        The interpreter used is relative to the target of the link.
        """
        with TemporaryDirectory() as dir2:
            other = Path(dir2) / 'other'
            other.symlink_to(self.script)
            stdout = subprocess.check_output(
                [other],
                cwd=dir2,
                text=True
            )
            self.assertEqual(
                stdout,
                f'{self.tmp}/interp {other}\n'
            )


class RelexecCTest(RelexecTests, TestCase):
    """Run the test case against the C implementation of relexec."""

    @classmethod
    def get_executable(cls) -> Path:
        """Build the C executable and return a path to it."""
        subprocess.check_call(
            ['make'],
            cwd=ROOT / 'c'
        )
        return ROOT / 'c/relexec'


class RelexecRustTest(RelexecTests, TestCase):
    """Run the test case against the Rust implementation of relexec."""

    @classmethod
    def get_executable(cls) -> Path:
        """Build the Rust executable and return a path to it."""
        subprocess.check_call(
            ['cargo', 'build', '--release'],
            cwd=ROOT / 'rust'
        )
        return ROOT / 'rust/target/release/relexec'


if __name__ == '__main__':
    main()
