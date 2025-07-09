import argparse
import dataclasses
import re
import subprocess
import os
from tomlkit import loads, dumps, string

# --- Configuration Constants ---
# Name of the TOML file containing challenge definitions
CHALLENGES_FILE = "challenges.toml"
# Name of the temporary file for challenge code
CHALLENGE_CODE_FILE = "challenge"
# Name of the temporary file for test results
CHALLENGE_RESULT_FILE = "challengeresult"
# Default command to compile the language (your 'fj' executable)
COMPILER_CMD = ["make", "--no-print-directory", "fj"]
# Default command for valgrind tests
VALGRIND_CMD = ["make", "val", "--no-print-directory"]
# Default command to run your compiled program
PROGRAM_RUN_CMD = ["./fj"]
# Default command to run gdb on failure
GDB_CMD = ["make", "gdb"]

# --- Helper Functions ---

def escape_ansi(s: str) -> str:
    """
    Removes ANSI escape codes from a string, trims trailing whitespace from each line,
    and performs some text replacements.
    """
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    
    # 1. Remove ANSI codes
    cleaned_s = ansi_escape.sub('', s)
    
    # 2. Trim trailing whitespace from each line
    lines = cleaned_s.splitlines()
    trimmed_lines = [line.rstrip() for line in lines]
    st = "\n".join(trimmed_lines) # Join back with newlines
    
    st = st.replace("├", " ")
    st = st.replace("│", " ")
    st = st.replace("╰", " ")
    
    # 4. Finally, strip overall leading/trailing whitespace (if any remains)
    st = st.strip()
    
    return st
def colored_print(text: str, color_code: int, bold: bool = False):
    """Prints text with ANSI color codes."""
    bold_code = ";1" if bold else ""
    print(f"\033[{color_code}{bold_code}m{text}\033[0m")

# --- Data Structures for Test Definition and Results ---

@dataclasses.dataclass
class Challenge:
    """Represents a single test challenge defined in challenges.toml."""
    name: str
    challenge_code: str  # The code to be written to CHALLENGE_CODE_FILE
    expected_result: str = "" # The expected output from the program
    # Future extensibility: Add 'comparison_mode' (exact, stripped, regex)
    # comparison_mode: str = "exact"

@dataclasses.dataclass
class TestResult:
    """Stores the outcome of running a single challenge."""
    challenge_name: str
    passed: bool
    actual_output: str = ""
    expected_output: str = ""
    error_message: str = ""
    is_valgrind_test: bool = False
    execution_error: bool = False # True if the program itself returned a non-zero exit code

# --- Main Test Runner Class ---

class TestRunner:
    """
    Manages loading challenges, compiling the language, running tests,
    and reporting results.
    """
    def __init__(self,
                 challenges_file: str = CHALLENGES_FILE,
                 challenge_code_file: str = CHALLENGE_CODE_FILE,
                 challenge_result_file: str = CHALLENGE_RESULT_FILE,
                 compiler_cmd: list[str] = None,
                 valgrind_cmd: list[str] = None,
                 program_run_cmd: list[str] = None,
                 gdb_cmd: list[str] = None,
                 fail_fast: bool = False): # Added fail_fast parameter

        self.challenges_file = challenges_file
        self.challenge_code_file = challenge_code_file
        self.challenge_result_file = challenge_result_file
        self.compiler_cmd = compiler_cmd if compiler_cmd is not None else COMPILER_CMD
        self.valgrind_cmd = valgrind_cmd if valgrind_cmd is not None else VALGRIND_CMD
        self.program_run_cmd = program_run_cmd if program_run_cmd is not None else PROGRAM_RUN_CMD
        self.gdb_cmd = gdb_cmd if gdb_cmd is not None else GDB_CMD
        self.challenges: dict[str, Challenge] = {}
        self.fail_fast = fail_fast # Store fail_fast setting

    def load_challenges(self) -> None:
        """
        Loads test challenges from the TOML file.
        """
        try:
            with open(self.challenges_file, "r") as f:
                toml_data = loads(f.read())
            
            for name, data in toml_data.items():
                if "challenge" not in data:
                    colored_print(f"Warning: Challenge '{name}' missing 'challenge' key. Skipping.", 33) # Yellow
                    continue
                self.challenges[name] = Challenge(
                    name=name,
                    challenge_code=data["challenge"],
                    expected_result=data.get("result", "").strip()
                )
            colored_print(f"Loaded {len(self.challenges)} challenges from {self.challenges_file}", 90) # Grey
        except FileNotFoundError:
            colored_print(f"Error: Challenges file '{self.challenges_file}' not found.", 31, True)
            exit(1)
        except Exception as e:
            colored_print(f"Error loading challenges: {e}", 31, True)
            exit(1)

    def _write_challenge_code(self, code: str) -> None:
        """Writes the challenge code to a temporary file."""
        with open(self.challenge_code_file, "w") as f:
            f.write(code)

    def _read_challenge_result(self) -> str:
        """Reads the test result from the temporary file."""
        try:
            with open(self.challenge_result_file, "r") as f:
                return f.read()
        except FileNotFoundError:
            return "" # Return empty string if file not found (e.g., no output)

    def compile_language(self) -> bool:
        """
        Compiles the programming language using the specified compiler command.
        Returns True on success, False on failure.
        """
        colored_print("Compiling language...", 36) # Cyan
        try:
            # Use subprocess.run for better control and error handling
            result = subprocess.run(
                self.compiler_cmd,
                capture_output=True,
                text=True,
                check=True # Raise CalledProcessError if non-zero exit code
            )
            colored_print("Compilation successful.", 92, True) # Green
            return True
        except subprocess.CalledProcessError as e:
            colored_print(f"Compilation failed with exit code {e.returncode}:", 31, True) # Red
            colored_print(f"STDOUT:\n{e.stdout}", 31)
            colored_print(f"STDERR:\n{e.stderr}", 31)
            return False
        except FileNotFoundError:
            colored_print(f"Error: Compiler command '{' '.join(self.compiler_cmd)}' not found. Is 'make' in your PATH?", 31, True)
            return False
        except Exception as e:
            colored_print(f"An unexpected error occurred during compilation: {e}", 31, True)
            return False

    def run_challenge(self, challenge: Challenge, use_valgrind: bool = False) -> TestResult:
        """
        Runs a single challenge and compares its output to the expected result.
        """
        self._write_challenge_code(challenge.challenge_code)
        
        cmd = self.valgrind_cmd if use_valgrind else self.program_run_cmd
        
        actual_output = ""
        error_message = ""
        execution_error = False

        try:
            # Execute the program/valgrind and redirect output to result file
            with open(self.challenge_result_file, "w") as outfile:
                process = subprocess.run(
                    cmd,
                    stdout=outfile, # Redirect stdout to the file
                    stderr=subprocess.PIPE, # Capture stderr
                    text=True,
                    check=False # Do not raise exception for non-zero exit codes yet
                )
            
            actual_output = self._read_challenge_result()
            
            if process.returncode != 0:
                execution_error = True
                error_message = f"Execution returned error code {process.returncode}:\n{process.stderr.strip()}"
                
            # Clean and compare outputs
            cleaned_actual = escape_ansi(actual_output).strip()
            cleaned_expected = escape_ansi(challenge.expected_result).strip()

            if execution_error:
                return TestResult(
                    challenge_name=challenge.name,
                    passed=False,
                    actual_output=actual_output,
                    expected_output=challenge.expected_result,
                    error_message=error_message,
                    is_valgrind_test=use_valgrind,
                    execution_error=True
                )
            elif cleaned_actual != cleaned_expected:
                return TestResult(
                    challenge_name=challenge.name,
                    passed=False,
                    actual_output=actual_output,
                    expected_output=challenge.expected_result,
                    error_message=f"Actual:{actual_output}",
                    is_valgrind_test=use_valgrind
                )
            else:
                return TestResult(
                    challenge_name=challenge.name,
                    passed=True,
                    actual_output=actual_output,
                    expected_output=challenge.expected_result,
                    is_valgrind_test=use_valgrind
                )
        except FileNotFoundError:
            error_message = f"Error: Command '{' '.join(cmd)}' not found. Is './fj' or 'make val' executable and in PATH?"
            return TestResult(
                challenge_name=challenge.name,
                passed=False,
                error_message=error_message,
                is_valgrind_test=use_valgrind
            )
        except Exception as e:
            error_message = f"An unexpected error occurred during challenge execution: {e}"
            return TestResult(
                challenge_name=challenge.name,
                passed=False,
                error_message=error_message,
                is_valgrind_test=use_valgrind
            )

    def run_tests(self, challenge_name: str = None, use_valgrind: bool = False) -> list[TestResult]:
        """
        Runs tests, either for a specific challenge or all challenges.
        Returns a list of TestResult objects.
        If self.fail_fast is True, stops after the first failed test.
        """
        results: list[TestResult] = []

        if challenge_name:
            if challenge_name not in self.challenges:
                colored_print(f"Error: Challenge '{challenge_name}' not found.", 31, True)
                return []
            colored_print(f"Running test for challenge: {challenge_name} (Valgrind: {use_valgrind})", 34) # Blue
            result = self.run_challenge(self.challenges[challenge_name], use_valgrind)
            results.append(result)
            if self.fail_fast and not result.passed:
                colored_print("Fail-fast enabled: Stopping after first failure.", 33) # Yellow
                return results
        else:
            colored_print(f"Running all challenges (Valgrind: {use_valgrind})", 34) # Blue
            for name, challenge in self.challenges.items():
                result = self.run_challenge(challenge, use_valgrind)
                results.append(result)
                if self.fail_fast and not result.passed:
                    colored_print("Fail-fast enabled: Stopping after first failure.", 33) # Yellow
                    break # Stop the loop on first failure
        return results

    def report_results(self, results: list[TestResult], run_gdb_on_failure: bool = False) -> bool:
        """
        Prints a summary of test results. Returns True if all tests passed, False otherwise.
        """
        all_passed = True
        failed_tests = []

        for result in results:
            if not result.passed:
                all_passed = False
                failed_tests.append(result)
                colored_print(f"FAILED: {result.challenge_name}", 31, True) # Red
                colored_print("─"*40, 31)
                if result.error_message:
                    colored_print(result.error_message, 31)
                if result.expected_output:
                    colored_print("Expected Output:", 33) # Yellow
                    print(result.expected_output)
                colored_print("─"*40, 31)

        total_tests = len(results)
        passed_count = sum(1 for r in results if r.passed)
        failed_count = total_tests - passed_count

        colored_print(f"Passed: {passed_count}", 92, True) # Light Green
        colored_print(f"Failed: {failed_count}", 31, True) # Red

        if not all_passed and run_gdb_on_failure:
            colored_print("\nTests failed. Running GDB...", 31, True)
            try:
                subprocess.run(self.gdb_cmd, check=True)
            except subprocess.CalledProcessError as e:
                colored_print(f"GDB command failed: {e}", 31)
            except FileNotFoundError:
                colored_print(f"Error: GDB command '{' '.join(self.gdb_cmd)}' not found. Is 'make gdb' configured correctly?", 31, True)
        
        if all_passed:
            colored_print("\nAll tests passed!", 92, True) # Light Green
        else:
            colored_print("Some tests failed.", 31, True) # Red

        return all_passed

    def cleanup_temp_files(self) -> None:
        """Removes temporary files created during testing."""
        for f in [self.challenge_code_file, self.challenge_result_file, "fj"]:
            if os.path.exists(f):
                try:
                    os.remove(f)
                except OSError as e:
                    colored_print(f"Error cleaning up {f}: {e}", 33) # Yellow

# --- Main Execution Block ---

def main():
    parser = argparse.ArgumentParser(
        description="Custom testing utility for your programming language."
    )
    parser.add_argument(
        "challenge_name",
        nargs="?", # Optional argument
        help="Name of a specific challenge to run. If omitted, all challenges run."
    )
    parser.add_argument(
        "--valgrind",
        action="store_true",
        help="Run tests with Valgrind (uses 'make val')."
    )
    parser.add_argument(
        "--no-cleanup",
        action="store_true",
        help="Do not remove temporary files after testing."
    )
    parser.add_argument(
        "--no-gdb",
        action="store_true",
        help="Do not run 'make gdb' on test failure."
    )
    parser.add_argument( # Added new argument for fail-fast
        "--fail-fast",
        action="store_true",
        help="Stop running tests after the first failure."
    )

    args = parser.parse_args()

    # Pass the fail_fast argument to the TestRunner constructor
    runner = TestRunner(fail_fast=args.fail_fast)
    runner.load_challenges()

    # Compile the language first
    if not runner.compile_language():
        colored_print("Exiting due to compilation failure.", 31, True)
        exit(1)

    # Run the tests
    test_results = runner.run_tests(args.challenge_name, args.valgrind)

    # Report results and optionally run gdb
    all_passed = runner.report_results(test_results, run_gdb_on_failure=not args.no_gdb)

    # Clean up temporary files
    if not args.no_cleanup:
        runner.cleanup_temp_files()
    
    exit(0 if all_passed else 1)

if __name__ == "__main__":
    main()
