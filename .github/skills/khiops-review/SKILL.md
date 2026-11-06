---
name: khiops-review
description: "Review the current Khiops branch before opening a PR. Use for pre-PR review, branch review, or checking readiness for a pull request across C++, CI, packaging, CMake, Python, and Java. Requires a clean working tree and produces an evidence-based report without modifying files."
argument-hint: "[target-branch-or-ref]"
user-invocable: true
---

# Khiops Review

Review the entire committed branch, not just the latest commit. Be direct,
specific, and proportionate. No praise, reassurance, personal judgments, or
manufactured criticism. No findings is a valid result, not proof of correctness.

## Invocation and boundaries

- `/khiops-review origin/main`: review against that explicit target.
- `/khiops-review`: ask the user to confirm the intended PR target.
  The repository default branch can be a suggestion, not an automatic choice.
- A tracking upstream usually identifies the published source branch, not the
  PR target. Never substitute it for the target.

This is a non-mutating review. Do not edit or format files, stage, stash, clean,
commit, switch branches, fetch, push, create a PR, or install dependencies.
Do not configure/build or run tests, hooks, or scripts that generate files,
update references, or otherwise modify the project. Propose write-producing
validation as a separate user action. Do not save the report to a file unless
separately requested after the review.

Use read-only inspection and checks known to be non-mutating. Read commands
before running unfamiliar scripts. Existing test logs are evidence only for
the configuration and revision they actually tested. A skill specifies these
boundaries; it does not enforce a read-only tool sandbox.

Do the review directly; this skill does not require separate agents per domain.
It is not a substitute for tests, CI, or a dedicated security audit.

## 1. Check prerequisites and pin the comparison

Use Git with pagers disabled. First run:

```bash
git --no-optional-locks status --porcelain=v1 --untracked-files=all --ignore-submodules=none
```

Any output means staged changes, unstaged changes, untracked non-ignored files,
or a dirty submodule blocks the review. Identify the obstruction and stop.
Do not resolve it automatically. Ignored build outputs alone do not block review.
A failed status command is an error, not evidence of a clean tree.

Resolve the source branch and HEAD:

```bash
git symbolic-ref --quiet --short HEAD
git rev-parse --verify HEAD
```

Stop with an explanation for detached HEAD or any unresolved prerequisite.
Use an explicitly supplied PR target; otherwise ask for confirmation before
substantive review. Do not infer consent from the default branch or upstream.

Resolve the target to a commit and pin it. For example, with the confirmed
reference stored as `target`:

```bash
git rev-parse --verify --end-of-options "${target}^{commit}"
```

Treat references as data, quote arguments, and never evaluate user input as
shell code. Use the resulting target and HEAD **hashes** for subsequent reads:

```text
git merge-base --all <target-hash> <head-hash>
git --no-pager diff --no-ext-diff --no-textconv --find-renames --name-status <base-hash> <head-hash> --
git --no-pager diff --no-ext-diff --no-textconv --find-renames --stat <base-hash> <head-hash> --
git --no-pager diff --no-ext-diff --no-textconv --find-renames <base-hash> <head-hash> -- <paths>
```

Angle-bracket values above are placeholders, not literal shell arguments.
Require exactly one merge base. Missing references, no common ancestor
(including shallow-history limits), or multiple merge bases stop review; do not
silently pick another ref, use the target tip as the base, or use `HEAD~1`.

Record source branch, target reference and hash, merge-base hash, and reviewed
HEAD hash. State that references are local and no fetch was performed; their
freshness relative to the server is unverified. If a refresh is needed, stop
and request that separate action.

The scope is the net, cumulative diff from merge base to HEAD, including
additions, deletions, renames, file modes, and binary/submodule changes. A
two-tip target-to-HEAD diff would wrongly include target-only changes.
If the diff is empty, state "No changes to review" and do not invent findings.

## 2. Load applicable rules and classify impacts

Read applicable workspace and file instructions, then load only the references
needed for the changed areas. The mapping is additive, not exclusive:

| Changed area or effect | Load |
|---|---|
| C++ headers/sources | [C++ review](./references/cpp.md) |
| Workflows and composite actions | [CI review](./references/ci.md) |
| CMake files, modules, and presets | [CMake review](./references/cmake.md) |
| Packaging, installation, launchers, dependencies, release artifacts | [Packaging review](./references/packaging.md), plus CI/CMake when affected |
| Python files, including tools and examples | [Python review](./references/python.md) |
| Java files and Java integration | [Java review](./references/java.md) |
| UI definitions, views, generation, or GUI behavior | [UI instructions](../../instructions/ui-changes.instructions.md), plus relevant language/build references |
| Everything else | Common criteria and applicable repository instructions |

A wheel workflow can require CI, packaging, and CMake review. A shared install
rule can affect multiple package formats without modifying their recipes.
Follow actual callers/consumers rather than reviewing every platform
mechanically. Shell/batch scripts and Dockerfiles are not excluded: classify
their purpose and inspect their shell, platform, and runtime behavior.
For renames and deletions, check both old and new consumers and references.

The linked project rules are authoritative; reference checklists complement
them, not replace them. Do not freeze package versions or platform inventories
in the review. Read actual configuration: illustrative snippets in instructions
are not proof of current behavior. Identify missing or conflicting rules
explicitly and ask when resolving the conflict would change a finding.

## 3. Review the change and its integration

Inventory every changed file before choosing focused reads. Read complete
changed functions, relevant surrounding code, callers, tests, and docs as
needed, using the pinned HEAD version. Use the merge-base version to establish
what behavior changed, not to audit unrelated legacy code.

Apply these questions across all domains:

- **Correctness:** Does the change implement the stated need, including boundary
  cases, failures, cleanup, and supported operating modes? Does it preserve
  intended behavior and compatibility? Do not invent requirements when intent
  is unavailable; state what needs clarification.
- **Simplicity:** Is every abstraction, option, dependency, fallback, and branch
  needed? Prefer the simplest solution that fully serves the need, not the
  fewest lines. Reject speculative generality and complexity without a benefit.
- **Modularity:** Are responsibilities cohesive and logic in the right layer?
  Is an existing helper suitable? Flag meaningful duplication without demanding
  an abstraction for every similar pair of lines.
- **Readability:** Do names, flow, and interfaces make the behavior understandable
  without mentally simulating unrelated layers?
- **Documentation:** Are contracts, ownership, assumptions, non-obvious choices,
  and changed user-facing behavior documented accurately under project rules?
  Identify the missing information; do not demand comments that restate code.
- **Validation:** Do tests exercise the changed behavior and failure paths, not
  just the happy path or a proxy? Identify relevant missing tests and exact
  pending commands/configurations without running write-producing validation.

Trace a suspected issue to evidence before reporting it. Inspect downstream
effects outside the diff when necessary. Do not report pre-existing issues
unless this change activates or worsens them; explain that connection.

Do not silently omit files when the diff is large or evidence is inaccessible.
Record reviewed and unreviewed areas, and mark the review incomplete if relevant
code coverage remains unfinished. Binary content that cannot be inspected is
an explicit limitation, not an assumed pass.

## 4. Calibrate findings

Separate:

1. **Required corrections**: established bugs, explicit rule violations, or
   concrete significant design/documentation defects. A functional bug is not
   required to justify blocking unnecessary complexity or misplaced
   responsibilities. Explain the cost; personal preference is not a blocker.
   Order these by demonstrated impact and urgency.
2. **Non-blocking improvements**: concrete benefits that can reasonably wait.
   Omit purely cosmetic preferences without a project rule or useful benefit.
3. **Verification points**: unresolved assumptions or missing evidence, clearly
   separated from proven defects. Say what evidence would settle them.

Each finding needs a precise file/line (or deleted entity, job, or configuration
key), the problem, its consequence or violated rule, supporting evidence, and
the simplest proportionate correction direction. Link the applicable rule for
convention violations. Consolidate duplicate symptoms with one root cause.
Do not use unexplained numerical confidence scores or propose a larger rewrite
when a local correction suffices.

Calibration examples, not findings to copy:
- Duplicated release policy in independent scripts that must stay synchronized:
  can require correction if the changed paths demonstrate the maintenance
  hazard; suggest one existing source of truth.
- A different but equally clear helper name with no naming-rule violation:
  omit rather than turn taste into a blocker.
- Suspected loader behavior on an untested OS: a verification point until
  configuration, documentation, or execution evidence establishes the defect.

## 5. Recheck and report

Before declaring completion, recheck source branch, HEAD, and working-tree
cleanliness using the prerequisite commands. If any changed, report the
original reviewed snapshot and mark the review incomplete; do not imply
coverage of new commits or local edits. Do not clean up someone else's changes.

Respond in the user's language. Use the following compact structure; keep empty
sections brief and use linked file locations:

```text
Scope
- Source branch and reviewed HEAD:
- Target reference and pinned target hash:
- Merge base:
- Local references only; no fetch; server freshness unverified.
- Reviewed domains/files and any coverage limits:

Required corrections
- [Priority] Location - problem; evidence/consequence; correction direction.
  Or: None found within the reviewed scope.

Non-blocking improvements
- Location - concrete benefit and proportionate suggestion.
  Or: None.

Verification points
- Location/area - uncertainty, missing evidence, and how to settle it.
  Or: None.

Validation
- Performed: commands/evidence, revision/configuration, actual results.
- Not performed: useful commands/platforms/tests and why they remain pending.

Outcome
- Required corrections found / No required corrections found within reviewed
  scope / Incomplete review (reason).
```

Use the incomplete outcome if scope/prerequisites cannot be established or
relevant review coverage is unfinished, even if some defects were already
found. Pending execution tests do not alone mean source review is unfinished,
but their absence must remain visible and must not become "tests passed".
If there were no changes, say so explicitly. Never claim the branch is
guaranteed correct, fully tested, or approved merely because no issue was found.
