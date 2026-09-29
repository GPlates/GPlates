# Tests 'check_release_commit.py' on synthetic repositories.
#
# The wheel workflow runs that script only on a pushed release tag, so a mistake in it would
# otherwise be found at a release - which is how one was found: it allowed only '#' headings in the
# changelog, and the changelog underlines its headings instead, so the release after a candidate
# would have been refused. Hence the release commit here renames a heading in a copy of the real
# changelog, the way the 'release-wheel' skill says to, rather than in a made-up one.
#
# Registered with CTest as 'pygplates-release-commit-test' (pygplates/test/CMakeLists.txt), and
# runnable directly: 'python pygplates/wheel/check_release_commit_test.py'.

import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
SCRIPT = HERE / "check_release_commit.py"
CHANGELOG = HERE.parent.parent / "CHANGELOG-pyGPlates.md"

# Without the variables that point git at a repository (git sets them for a hook it runs, so a
# test run from a hook would otherwise commit into the real one), and without the override.
ENV = {name: value for name, value in os.environ.items()
       if not name.startswith("GIT_") and name != "PYGPLATES_RELEASE_ALLOW_CHANGES_SINCE_CANDIDATE"}

GIT_CONFIG = ["-c", "user.name=release-commit-test", "-c", "user.email=test@invalid",
              "-c", "commit.gpgsign=false", "-c", "tag.gpgsign=false",
              "-c", "core.autocrlf=false", "-c", "core.hooksPath=no-hooks"]


def rename_first_section(changelog, title):
    """The changelog with its first section heading (the one below the file's title) renamed,
    in whichever heading style the changelog uses."""
    lines = changelog.split("\n")
    headings = 0
    for i, line in enumerate(lines):
        text = line.rstrip("\r")
        eol = line[len(text):]
        underlined = (text.strip() and i + 1 < len(lines)
                      and re.fullmatch(r"=+|-+", lines[i + 1].strip()))
        if not (text.startswith("#") or underlined):
            continue
        headings += 1
        if headings < 2:
            continue
        if underlined:
            lines[i] = title + eol
            underline = lines[i + 1].rstrip("\r")
            lines[i + 1] = underline[0] * len(title) + lines[i + 1][len(underline):]
        else:
            lines[i] = text.split(" ", 1)[0] + " " + title + eol
        return "\n".join(lines)
    raise AssertionError(f"'{CHANGELOG.name}' has no section heading below its title.")


@unittest.skipIf(shutil.which("git") is None, "git not found")
class ReleaseCommitTest(unittest.TestCase):

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.repo = pathlib.Path(self.directory.name)
        self.changelog = CHANGELOG.read_bytes().decode("utf-8")
        self.git("init", "-q")
        self.git("symbolic-ref", "HEAD", "refs/heads/main")
        self.count = 0

    def tearDown(self):
        self.directory.cleanup()

    def git(self, *args):
        result = subprocess.run(["git", "-C", str(self.repo), *GIT_CONFIG, *args],
                                capture_output=True, encoding="utf-8", env=ENV)
        if result.returncode != 0:
            raise AssertionError(f"'git {' '.join(args)}' failed:\n{result.stderr}")
        return result.stdout.strip()

    def commit(self, target, changelog=None, touch=False):
        """A commit setting the pyGPlates target (GPlates frozen, as on a series branch), with
        the changelog given and, if 'touch', some other change as well."""
        (self.repo / "cmake" / "modules").mkdir(parents=True, exist_ok=True)
        (self.repo / "cmake" / "modules" / "VersionRelease.cmake").write_text(
            f"set(GPLATES_FROZEN_VERSION 2.6.0-2)\nset(PYGPLATES_RELEASE_VERSION {target})\n")
        (self.repo / "CHANGELOG-pyGPlates.md").write_bytes(
            (changelog if changelog is not None else self.changelog).encode("utf-8"))
        if touch or self.count == 0:
            self.count += 1
            (self.repo / "file").write_text(f"{self.count}\n")
        self.git("add", "-A")
        self.git("commit", "-q", "--allow-empty", "-m", target)

    def check(self, tag, version, override=False):
        env = dict(ENV)
        if override:
            env["PYGPLATES_RELEASE_ALLOW_CHANGES_SINCE_CANDIDATE"] = "true"
        return subprocess.run([sys.executable, str(SCRIPT), tag, version], cwd=self.repo,
                              capture_output=True, encoding="utf-8", env=env)

    def assert_accepted(self, tag, version, override=False):
        result = self.check(tag, version, override)
        self.assertEqual(result.returncode, 0, f"'{tag}' was refused:\n{result.stderr}")

    def assert_refused(self, tag, version, phrase):
        result = self.check(tag, version)
        self.assertNotEqual(result.returncode, 0, f"'{tag}' was accepted:\n{result.stdout}")
        self.assertIn(phrase, " ".join(result.stderr.split()))

    def candidate(self):
        """The previous release, then a candidate for 1.1.0 on top of it."""
        self.commit("1.0.0")
        self.git("tag", "PyGPlates-1.0.0")
        self.commit("1.1.0rc1", touch=True)
        self.git("tag", "PyGPlates-1.1.0rc1")

    def test_release_renaming_the_changelog_heading(self):
        self.candidate()
        renamed = rename_first_section(self.changelog, "pyGPlates 1.1.0")
        self.assertNotEqual(renamed, self.changelog)
        self.commit("1.1.0", renamed)
        self.git("tag", "PyGPlates-1.1.0")
        self.assert_accepted("PyGPlates-1.1.0", "1.1.0")

    def test_release_changing_the_changelog_text(self):
        self.candidate()
        self.commit("1.1.0", self.changelog.replace("\n* ", "\n* Also: ", 1))
        self.git("tag", "PyGPlates-1.1.0")
        self.assert_refused("PyGPlates-1.1.0", "1.1.0", "other than its headings")
        self.assert_accepted("PyGPlates-1.1.0", "1.1.0", override=True)

    def test_release_changing_another_file(self):
        self.candidate()
        self.commit("1.1.0", touch=True)
        self.git("tag", "PyGPlates-1.1.0")
        self.assert_refused("PyGPlates-1.1.0", "1.1.0", "also changes: file")

    def test_release_not_directly_on_the_candidate(self):
        self.candidate()
        self.commit("1.1.0rc1", touch=True)
        self.commit("1.1.0")
        self.git("tag", "PyGPlates-1.1.0")
        self.assert_refused("PyGPlates-1.1.0", "1.1.0", "not one commit on top of")

    def test_candidate_after_candidate(self):
        self.candidate()
        self.commit("1.1.0rc2", touch=True)
        self.git("tag", "PyGPlates-1.1.0rc2")
        self.assert_accepted("PyGPlates-1.1.0rc2", "1.1.0rc2")

    def test_tag_not_naming_the_version(self):
        self.candidate()
        self.assert_refused("PyGPlates-1.1.0rc1", "1.1.0rc2", "does not match the version")

    def test_development_version(self):
        self.candidate()
        self.commit("1.1.0rc2", touch=True)
        self.git("tag", "PyGPlates-1.1.0rc2.dev1")
        self.assert_refused("PyGPlates-1.1.0rc2.dev1", "1.1.0rc2.dev1", "not a release version")

    def test_tag_not_at_head(self):
        self.candidate()
        self.commit("1.1.0rc2", touch=True)
        self.assert_refused("PyGPlates-1.1.0rc1", "1.1.0rc1", "does not point at the commit")

    def test_release_sorting_below_one_in_its_history(self):
        self.candidate()
        self.commit("1.1.0")
        self.git("tag", "PyGPlates-1.1.0")
        self.commit("1.0.9", touch=True)
        self.git("tag", "PyGPlates-1.0.9")
        self.assert_refused("PyGPlates-1.0.9", "1.0.9", "does not sort above 'PyGPlates-1.1.0'")

    def test_patch_release_below_a_later_series(self):
        # 'PyGPlates-1.2.0' is on another branch, so not in the patch release's history.
        self.candidate()
        self.commit("1.1.0")
        self.git("tag", "PyGPlates-1.1.0")
        self.git("switch", "-q", "-c", "release/pygplates-1.2", "PyGPlates-1.0.0")
        self.commit("1.2.0", touch=True)
        self.git("tag", "PyGPlates-1.2.0")
        self.git("switch", "-q", "main")
        self.commit("1.1.1", touch=True)
        self.git("tag", "PyGPlates-1.1.1")
        self.assert_accepted("PyGPlates-1.1.1", "1.1.1")


if __name__ == "__main__":
    unittest.main()
