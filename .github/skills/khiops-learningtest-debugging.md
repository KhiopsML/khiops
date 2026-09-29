---
name: khiops-learningtest-debugging
description: "Use when the user reports failing LearningTest tests and provides a LearningTest checkout or test directory."
---

# Debugging Failing LearningTests

Use when the user provides a `LearningTest` path and reports failures. A leaf containing `test.prm` means `kht_test`
already ran it; start with its `comparisonResults.log` and `results/`. Use
[`learning-test-tool.instructions.md`](../instructions/learning-test-tool.instructions.md) for comparison and
reference-update details.

Defaults: `r`, `-p 4`, and `--family full-no-kni`. Do not ask first; adjust only when evidence requires it.

## Workflow

1. **Check versions first**:
   ```bash
   diff -u <Khiops_repo>/test/LearningTest/version.txt <LearningTest_dir>/version.txt
   ```
   Stop if they differ.

2. **Check branch context**:
   ```bash
   git -C <Khiops_repo> branch --show-current
   git -C <Khiops_repo> diff --stat main...HEAD
   ```
   On a branch other than `main`, treat its changes as the regression source.

3. **Collect failures** without re-running the suite:
   ```bash
   kht_apply <LearningTest_dir> errors -p 4 --family full-no-kni
   ```

4. **Diagnose each test** from the complete `comparisonResults.log`, then compare actual `results/` with the selected
   `results.ref*`. Read execution markers first: `process_timeout_error.log`, `return_code_error.log`,
   `stdout_error.log`, and `stderr_error.log`; then inspect output files such as `err.txt`. Log values may be
   truncated, so read the actual files.

   Treat crashes, timeouts, numeric drift, and unexpected output as regressions: do not update references. Update
   references only for confirmed intentional behavior changes, following the instruction file for all `results.ref*`
   variants and `time.log`. Never use `makeref`, `clean`, `cleanref`, or `deleteref` without explicit confirmation.

   When the test takes a long time (e.g. in debug mode), there are several ways to reduce the scope of the bug:
    - isolate the part of the scenario that actually fails: a test scenario may contain several repeated steps. If the bug appears at the i-th `ComputeStats`, temporarily comment out the preceding i-1 `ComputeStats` steps, provided that the failing step does not depend on artifacts they produce.
    - reduce the size of the dataset using the sampling rate:
      - lower the relevant `*.DatabaseSpec.Sampling.SampleNumberPercentage` value while the failure remains reproducible.


5. **Re-run or verify one test** as needed:
   ```bash
   kht_test <test_dir> r -p 4
   kht_test <test_dir> check -p 4
   ```
   Continue after `0 error(s)`.

6. **Report** fixes, branch regressions, and unresolved cases grouped by root cause.
