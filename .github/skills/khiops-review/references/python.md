# Python Review

Apply the common criteria to scripts, test infrastructure, and examples, not
only imported modules. Follow applicable project instructions and nearby code
rather than imposing an unrelated Python architecture.

- Check clear function/module responsibilities, explicit names, existing
  helpers, and unnecessary classes/configuration. Keep simple scripts simple.
- Follow error reporting, exception specificity, return codes, and cleanup.
  Reject broad catches and successful-looking defaults that hide failed work.
- Inspect file/process lifetimes, subprocess arguments and shell use, timeout
  and interruption handling, environment mutation, path resolution, encoding,
  and platform assumptions when affected.
- Verify compatibility with the interpreter baseline and dependencies actually
  declared by the relevant tool. Do not propose a new package when an existing
  helper or straightforward standard-library code satisfies the need.
- Check input validation, empty/malformed input, deterministic output, and
  destructive-operation boundaries. Identify tests that distinguish the changed
  behavior rather than only testing the helper in isolation.
- Check command help and documentation against actual arguments, outputs,
  side effects, and failure semantics.

## LearningTestTool

For changes under `test/LearningTestTool/`, read the
[LearningTestTool instructions](../../../instructions/learning-test-tool.instructions.md).

Inspect directory-level detection, shared constants, family membership,
platform/computing contexts, reference selection, and result comparison where
affected. Changes to tolerance, warnings, retries, or timeout policy can conceal
regressions: require an explained behavior change and relevant evidence.

Review reference updates against the actual outputs and all affected context
variants; do not approve whole-directory replacement just because comparison
now passes. Existing comparison summaries may truncate details. Read full
underlying differences before claiming equivalence.

Do not rerun comparisons or maintenance commands that write logs or reference
files during review. Specify focused pending scenarios instead. For Python
packaging configuration, also load the packaging reference; TOML recipes are
not ordinary Python source and C++ wheels must not be assumed to contain Python.
