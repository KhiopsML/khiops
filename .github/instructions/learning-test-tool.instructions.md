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
kht_test <binaries_dir> [source_dir] [options]
```

- **`binaries_dir`**: path to tool executables, or aliases `r` (release), `d` (debug), `check` (comparison only)
- **`source_dir`**: test/suite/tool/home dir (auto-detected level)
- Key options: `--family`, `--n` (MPI processes), `--min-test-time`, `--max-test-time`, `--timeout-limit`

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
