import pytest
import subprocess
from pathlib import Path

def get_c_files(directory: Path):
    """Recursively collect all .c files in the given directory."""
    return list(directory.rglob('*.c'))

# Generate the list of .c files from the testfiles directory
test_files = get_c_files(Path('./tests/testfiles/passing'))


# Use pytest.mark.parametrize to dynamically create a test for each .c file
@pytest.mark.parametrize('c_file', test_files)
def test_compile_and_run_c_file(c_file):
    """Test compiling and running a .c file with the ifcc-test.py script."""
    # Assuming ifcc-test.py is in the current directory and accepts a file path as an argument
    result = subprocess.run(['python3', './tests/ifcc-test.py', str(c_file)], capture_output=True, text=True)

    # Assert that the test script succeeded (you may want to adjust this based on your script's behavior)
    assert result.returncode == 0, f"Test failed for {c_file}: {result.stdout} {result.stderr}"
