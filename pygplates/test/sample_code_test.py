#!/usr/bin/env python

"""
Run every sample script in the pyGPlates API documentation ('docs/pygplates/sample-code/*.py').

Each script is the code of one documentation page (the page includes it through the 'sample-code'
directive - see 'docs/pygplates/conf.py.in'), written to run in a directory holding the data files
it names ('rotations.rot', 'topologies.gpml', ...). So each one runs in its own temporary directory
seeded with a copy of the test fixtures, and passes if it exits zero. The documented "Output" blocks
are captured from real geodata and are not compared: the fixtures are small stand-ins that let the
scripts run, not the models the pages describe.

Usage: sample_code_test.py <sample-code-dir> [<fixtures-dir>]
"""

import os
import shutil
import subprocess
import sys
import tempfile

# Fail rather than pass on an empty (or nearly empty) run, in case the sample directory is wrong.
# There were 57 scripts when this was written.
MINIMUM_EXPECTED_SAMPLES = 40


def copy_files(source_dir, working_dir):
    """Copy the files (not the sub-directories) of 'source_dir', if it exists, into 'working_dir'."""
    if not os.path.isdir(source_dir):
        return
    for name in os.listdir(source_dir):
        if os.path.isfile(os.path.join(source_dir, name)):
            shutil.copy(os.path.join(source_dir, name), working_dir)


def run_sample(sample_path, fixtures_dir):
    with tempfile.TemporaryDirectory() as working_dir:
        # Three layers, each over the one before:
        # - the fixtures shared with the unit tests,
        # - 'fixtures/sample-code/', for every sample (eg, a 'topologies.gpml' with ridges and subduction
        #   zones, where the unit tests' one has only unclassified sections),
        # - 'fixtures/sample-code/<script>/', for a script whose page shows its own input (eg,
        #   'create_topological_features' reproduces its 'features.gpml' in full).
        sample_code_fixtures_dir = os.path.join(fixtures_dir, 'sample-code')
        copy_files(fixtures_dir, working_dir)
        copy_files(sample_code_fixtures_dir, working_dir)
        copy_files(os.path.join(sample_code_fixtures_dir, os.path.splitext(os.path.basename(sample_path))[0]),
                   working_dir)
        return subprocess.run(
            [sys.executable, sample_path],
            cwd=working_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True)


def main():
    # Absolute, since each script runs with a temporary directory as its working directory.
    sample_code_dir = os.path.abspath(sys.argv[1])
    fixtures_dir = os.path.abspath(sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), 'fixtures'))

    samples = sorted(name for name in os.listdir(sample_code_dir) if name.endswith('.py'))
    if len(samples) < MINIMUM_EXPECTED_SAMPLES:
        print("Expected at least %d sample scripts in '%s', found %d." % (
            MINIMUM_EXPECTED_SAMPLES, sample_code_dir, len(samples)))
        return 1

    failures = 0
    for sample in samples:
        result = run_sample(os.path.join(sample_code_dir, sample), fixtures_dir)
        if result.returncode == 0:
            print('ok   ', sample)
        else:
            failures += 1
            print('FAIL ', sample, '(exit code %d)' % result.returncode)
            print(result.stdout)
    print('%d of %d sample scripts ran' % (len(samples) - failures, len(samples)))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
