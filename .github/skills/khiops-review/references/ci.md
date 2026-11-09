# CI Review

Read the [CI instructions](../../../instructions/ci-workflows.instructions.md).
Inspect current workflow/action definitions and their consumers, not a copied
inventory of workflow names or runner versions.

## Execution graph and coverage

- Trace events, path filters, branch/tag filters, and manual inputs to jobs.
  Does each affected change actually trigger the validation it needs?
  Do exclusions or renamed paths accidentally suppress it?
- Trace `needs`, job/step conditions, output names and string/boolean handling.
  Check skipped/failed dependencies, cancellation, and whether required checks
  can finish with the intended result.
- Check supported platform/architecture combinations against current project
  configuration. Explain deliberate coverage reductions; do not assume one OS
  result establishes all others.
- Follow composite action inputs, defaults, outputs, and called scripts across
  workflow boundaries. Verify the input names used by callers actually exist.

## Failure semantics and artifacts

- Verify shell selection, quoting, environment/output propagation, working
  directories, and platform-specific commands, including shell/batch wrappers.
- Follow exit codes through pipes, retries, conditionals, and
  `continue-on-error`. A failed or skipped test must not become a successful
  validation merely because a later command succeeded.
- Check artifact producers/consumers, names, matrix collisions, paths, retention
  needs, and missing-output behavior. Ensure tests exercise the built or
  installed artifact intended for delivery, not an unrelated source-tree binary.
- Review cache keys and restore behavior for configuration/platform isolation;
  do not mistake stale build outputs for validated new artifacts.

## Simplicity and release boundaries

- Reuse existing composite actions for repeated policy. Keep responsibilities
  coherent between workflows, actions, scripts, and CMake; avoid both copied
  business logic and excessive parameterization for a single call site.
- Distinguish PR validation, tag release, and manual publication paths. Check
  version checks and publication/signing conditions before expensive build or
  release actions.
- Inspect permissions, credentials, and untrusted input boundaries relevant to
  the change. Do not claim a general security audit from this checklist.
- Document non-obvious conditions and inputs; keep names useful for diagnosing
  failures rather than adding commentary to obvious steps.

Read existing CI results only with their tested commit/event/matrix context.
If execution evidence is unavailable, state the affected unverified paths.
Do not dispatch workflows, publish, or install dependencies during this review.
