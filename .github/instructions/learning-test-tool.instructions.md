---
description: "Use when modifying LearningTestTool Python scripts, test families, result comparison logic, or test infrastructure under test/LearningTestTool/. Covers test tool commands, directory conventions, result context management, and comparison patterns."
applyTo: "test/LearningTestTool/**"
---

# LearningTestTool Instructions

## Overview

LearningTestTool is the **non-regression test infrastructure** for Khiops. It manages 800+ test cases across 40+ suites for three tools (KMODL, MODL_Coclustering, KNITransfer). See `test/LearningTestTool/README.md` for full usage details.

## Directory Structure

```
test/LearningTestTool/
  ├─ py/                     # All Python implementation
  │   ├─ kht_test.py         # Main command: run tests & compare results
  │   ├─ kht_apply.py        # Apply maintenance instructions on test dirs
  │   ├─ kht_collect_results.py  # Gather results by filter (errors, warnings)
  │   ├─ kht_export.py       # Backup/archive test trees
  │   ├─ kht_env.py          # Display Khiops environment variables
  │   ├─ kht_help.py         # Show command overview
  │   ├─ _kht_constants.py   # All shared constants (paths, tool names, timeouts)
  │   ├─ _kht_utils.py       # Utility functions, directory type validation
  │   ├─ _kht_families.py    # Test family definitions (basic, full, complete)
  │   ├─ _kht_results_management.py  # Result context: platform, computing mode
  │   ├─ _kht_check_results.py       # Cross-platform result comparison engine
  │   ├─ _kht_standard_instructions.py  # Built-in maintenance instructions
  │   └─ _kht_one_shot_instructions.py  # One-off maintenance instructions
  ├─ sh/                     # Linux/macOS shell wrappers → py/
  └─ cmd/                    # Windows batch wrappers → py/
```

## File Naming Convention

- **`kht_*.py`**: User-facing command scripts (entry points)
- **`_kht_*.py`**: Internal modules (prefixed with `_`, not called directly)

## LearningTest Directory Hierarchy

The tool operates on a strict 4-level directory hierarchy:

| Level | Name | Example | Contains |
|-------|------|---------|----------|
| home dir | `LearningTest/` | `test/LearningTest/` | Tool dirs + dataset collections |
| tool dir | `Test<Tool>/` | `TestKhiops/` | Suite dirs |
| suite dir | `<SuiteName>/` | `Standard/` | Test dirs |
| test dir | `<TestName>/` | `IrisLight/` | `test.prm`, `results/`, `results.ref*` |

Each test dir contains:
- `test.prm` — Khiops scenario file (required)
- `test.json` — Optional JSON parameters
- `results/` — Current test output
- `results.ref*` — Reference results (may have context suffixes)
- `comparisonResults.log` — Comparison output

## Three Khiops Tools

| Tool Name | Executable | Test Dir | Parallel |
|-----------|-----------|----------|----------|
| `Khiops` | `MODL` | `TestKhiops/` | Yes |
| `Coclustering` | `MODL_Coclustering` | `TestCoclustering/` | Yes |
| `KNI` | `KNITransfer` | `TestKNI/` | No |

Constants are in `_kht_constants.py`: `TOOL_NAMES`, `TOOL_EXE_NAMES`, `TOOL_DIR_NAMES`, `PARALLEL_TOOL_NAMES`.

## Test Families

Defined in `_kht_families.py`. Each family is a list of test suites per tool:

| Family | Scope | Approximate Duration |
|--------|-------|---------------------|
| `basic` | Minimal smoke test (`Standard` suite only) | ~1 min |
| `full` | All non-regression suites (default) | ~1 hour |
| `full-no-kni` | Same as `full` without KNI | ~1 hour |
| `complete` | Exhaustive testing | ~1 day |
| `all` | All subdirectories (management, not testing) | N/A |

To add a test suite to a family, add it to `FAMILY_TEST_SUITES[family, tool]` in `_kht_families.py`.

## Main Commands

### `kht_test` — Run Tests

```bash
kht_test <source_dir> <binaries> [options]
```

- **`source_dir`**: test/suite/tool/home dir (auto-detected level); can be an individual test dir like `TestKhiops/CrashTests/trainclassifier_KWDatabaseSlicerTask`
- **`binaries`**: path to tool executables, or aliases `r` (release), `d` (debug), `check` (comparison only, no execution)
- Key options: `-f/--family`, `-p/--processes N` (MPI processes), `--min-test-time`, `--max-test-time`, `--test-timeout-limit`

Examples:
```bash
# Run all CrashTests with 4 MPI processes using release binary
kht_test /path/to/LearningTest/TestKhiops/CrashTests r -p 4

# Compare results only (no re-run)
kht_test /path/to/LearningTest/TestKhiops/CrashTests check -f full -p 4

# Run a single test dir with debug binary
kht_test /path/to/LearningTest/TestKhiops/Standard/Iris d -p 1
```

### `kht_apply` — Maintenance Instructions

```bash
kht_apply <instruction> [source_dir]
```

Built-in instructions: `errors`, `makeref`, `list`, `logs`. Custom one-shot instructions defined in `_kht_one_shot_instructions.py`.

### `kht_collect_results` — Gather Results

```bash
kht_collect_results <source_dir> <target_dir> [--all|--errors|--warnings]
```

### `kht_export` — Archive Tests

```bash
kht_export <source_dir> <target_dir> [--all|--scripts|--references|--datasets]
```

## Result Context Management

Reference results can specialize by **computing mode** and **platform** via directory name suffixes:

```
results.ref                          # Default (fallback)
results.ref-parallel                 # Parallel-specific
results.ref-sequential               # Sequential-specific
results.ref-Darwin_Linux             # macOS or Linux
results.ref-parallel-Darwin_Linux    # Parallel + macOS/Linux
results.ref-Windows                  # Windows-specific
```

Suffix syntax (defined in `_kht_results_management.py`):
- `-` (AND): separates type axes (computing, platform)
- `_` (OR): separates values within an axis

The comparison engine selects the most specialized matching `results.ref*` directory for the current context (platform + sequential/parallel).

## Result Comparison (`_kht_check_results.py`)

The comparison engine is hierarchical:
1. File count per directory
2. File names
3. Per file: line count → line content → field content (tab-separated) → token content (JSON/KDIC)

**Tolerance mechanisms** (cross-platform resilience):
- Filters copyright lines, MPI process prefixes (`[0] `), debug memory stats
- Numerical tolerance for floating-point differences (warnings, not errors)
- Tolerance for scenario failures matching reference
- Tolerance for accented characters in filenames (system-dependent encoding)
- Tolerance for resource-related error messages differing across platforms

**Error detection** via special files in results (by priority):
1. `process_timeout_error.log`
2. `return_code_error.log`
3. `stdout_error.log`
4. `stderr_error.log`

## comparisonResults.log Format

Every test dir contains a `comparisonResults.log` after a run or a `check`. Its structure is:

```
<TestName> comparison
current comparison context : ['parallel', 'Darwin']
[used results.ref-<suffix> dir among (...)]   ← only shown when not results.ref

file <absolute_path_to_results_file>
OK                                            ← file matches reference
  -- or --
line N field M <new value> -> <old value>     ← differing field (truncated at ~80 chars)
K error(s)

...

SUMMARY
X warning(s)
Y error(s)
Problem file types: err.txt, .khj, ...
[Note: ...]
[Portability: ...]
```

**Key reading rules:**
- The diff line format is `<new value in results/> -> <old value in results.ref*/>`  — new on the left, old on the right.
- Values are truncated with `...` in the log; always read the actual file for the complete string before doing replacements.
- `Problem file types` summarises which file extensions have errors.
- `Portability` notes which specialized `results.ref*` dir was selected.

## Updating References After Message/Output Changes

When C++ user messages change and tests fail **only** because of those message differences (not algorithmic changes), the correct fix is to update the reference files **in-place** (not copy the whole `results/` directory):

### Workflow

1. Read `comparisonResults.log` to identify which files differ and on which lines.
2. Open both `results/<file>` (new) and `results.ref*/<file>` (old) to get the **complete** old and new strings (the log truncates them).
3. For **every** `results.ref*` directory that exists in the test dir (e.g. `results.ref`, `results.ref-parallel`, `results.ref-Darwin_Linux`, …), replace only the changed message text in the affected files — leave all other content untouched.
4. Copy `results/time.log` into **each** `results.ref*` directory.
5. Verify with `kht_test <test_dir> check` — expect `0 error(s)`.

### Pitfalls

- In `.khj` files (JSON), path separators are escaped as `\/`; a path like `../datasets/Iris/Iris.txt` becomes `.\/..\/datasets\/Iris\/Iris.txt` inside the JSON string. Use the raw bytes from `grep` on the results file, not the log summary.
- When the same old string appears multiple times in a file but maps to **different** new strings (e.g. first occurrence → Classifier, second → Regressor), use `str.replace(old, new, 1)` for the first and a plain `str.replace` for the rest — do **not** use a global regex that cannot distinguish positions.
- Always update **all** `results.ref*` variants, not just the one currently selected by the comparison context.

## Timeout Management

Defined in `_kht_constants.py`:
```
timeout(test) = MIN_TIMEOUT + TIMEOUT_RATIO × reference_time(test)
```
- `MIN_TIMEOUT` = 600 seconds
- `TIMEOUT_RATIO` = 10
- Up to 3 retries on timeout

## Environment Variables

Key variables used during tests (set automatically by the tool):

| Variable | Purpose |
|----------|---------|
| `KhiopsExpertMode` | Enable expert mode |
| `KhiopsCrashTestMode` | Enable crash test mode |
| `KhiopsFastExitMode` | Fast exit mode |
| `KhiopsDefaultMemoryLimit` | Default memory limit |
| `KhiopsHardMemoryLimitMode` | Hard memory limit mode |
| `KHIOPS_API_MODE` | API mode |

User-configurable variables: `KhiopsPreparationTraceMode`, `KhiopsParallelTrace`, `KhiopsFileServerActivated`, `KhiopsMemStatsLogFileName`, etc. See `kht_env.py` for full list.

## Conventions & Pitfalls

### ✅ DO

- Use constants from `_kht_constants.py` — never hardcode tool names, directory names, or file names
- Add new test suites to the appropriate family in `_kht_families.py`
- Follow the 4-level directory hierarchy strictly (home → tool → suite → test)
- Keep `results.ref*` directory names normalized (respect type order, value order)
- Use `_kht_utils.py` validation functions (`check_test_dir`, `check_suite_dir`, etc.)
- Comments in French (project convention for this codebase)

### ❌ DON'T

- Add a tool without updating all dictionaries in `_kht_constants.py` (`TOOL_NAMES`, `TOOL_EXE_NAMES`, `TOOL_DIR_NAMES`) — assertions will catch inconsistencies
- Create `results.ref*` directories with non-normalized names
- Modify comparison tolerance without understanding cross-platform impact
- Bypass the directory hierarchy validation (the tool auto-detects the level)
- Use absolute paths in `test.prm` scenarios — paths are relative to the LearningTest root
- **Call `kht_apply` with any destructive instruction** — the following are forbidden without explicit user confirmation:
  - `makeref`  — copy test results files to reference dir for current context
  - `clean` — deletes `results/` dir and comparison log
  - `cleanref` — delete reference results files for current context
  - `deleteref` — delete reference results files and dirs for all contexts
