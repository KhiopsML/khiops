# C++ Review

## Load the authoritative rules first

Read the [coding guidelines](../../../../doc/developement/coding-guidelines.md)
and [C++ instructions](../../../instructions/cpp-changes.instructions.md).
Review the changed code against those rules before considering general C++
advice. Preserve documented exceptions and distinguish generated code from
handwritten code. For UI changes, also read the
[UI instructions](../../../instructions/ui-changes.instructions.md).

Do not introduce a second coding standard here. In particular, do not recommend
language/library modernization that the project forbids. Check naming, includes,
class organization, method contracts, and documentation against the actual guide.
An exception expressly allowed by the guide is not a violation.

## Correctness and lifetime

- Identify who owns each allocated object, who deletes it, and whether a
  container owns its elements. Check replacement, transfer, partial failure,
  interruption, reuse, and every early return for leaks or double deletion.
- Check initialization, reset and reuse of work state, index/range bounds,
  nullability, integer overflow, and floating-point edge cases where relevant.
  Follow container semantics rather than infer them from generic C++ containers.
- Check important preconditions, postconditions, and invariants, especially at
  API boundaries. Use project contract macros where they clarify or protect a
  meaningful contract; do not expect `require` or `ensure` in every method or
  duplicate validation already performed at the appropriate layer. Their
  absence alone is not a finding unless an important contract is left unchecked
  or undocumented. Debug-only assertions must not substitute for handling a
  possible runtime/user-input error in a release build.
- Follow project error reporting and propagation. Reject success-shaped
  fallbacks or partially updated outputs that conceal a failed operation.
- Check API callers, persisted formats, and user messages affected by a change.
  Inspect relevant non-regression expectations without updating references
  merely to make a changed result pass.

## Algorithms, design, and documentation

- Establish the algorithmic intent and invariants before judging an unfamiliar
  implementation. Compare with nearby implementations and existing Norm helpers.
- Identify unnecessary passes, repeated allocations, extra copies, and growth
  in time/memory cost on realistic inputs. Explain the affected scale rather
  than demand speculative optimization.
- Check that responsibilities belong in the class/module being changed. Prefer
  a cohesive local solution to new hierarchies or a generic framework for one
  use case. Flag unnecessary state and duplicated sources of truth.
- Follow the guide's class/method and implementation documentation rules.
  Specify missing contracts, ownership, assumptions, or algorithm explanations;
  do not replace required documentation with a generic "self-documenting code"
  argument.

## Conditional surfaces

- Do not review generated Flex/Bison output as handwritten C++: exclude
  `src/Norm/base/JSONLex.inc`, `src/Norm/base/JSONYac.cpp`,
  `src/Norm/base/JSONYac.hpp`, `src/Learning/KWData/KWCLex.inc`,
  `src/Learning/KWData/KWCYac.cpp`, and `src/Learning/KWData/KWCYac.hpp`.
  Review the corresponding `.lex`/`.yac` grammar sources and CMake generation
  rules when they change. Treat generated-output-only diff churn as generated
  code, not a code-review finding.
- For parallel logic, read the relevant sections of the
  [parallel library guide](../../../../src/Parallel/doc/bibliotheque%20parallele.md).
  Check task registration, master/slave and shared-variable lifetimes, resource
  grants, empty partitions, sequential/simulated/real execution, interruptions,
  and error propagation where the diff affects them. Do not propose threads.
- For generated views, inspect the source definitions, preserved custom
  sections, model/view synchronization, and generation dependencies.
- For native interfaces, inspect callers and ABI/data-lifetime implications.

## Validation to assess, not automatically execute

Identify the smallest affected build target and focused unit/non-regression
tests, adding sequential/parallel or debug/release configurations when relevant.
Assess whether tests verify the actual changed behavior and boundary failures.
Formatting, build, and test commands that write files remain pending separate
actions under the main skill's non-mutating policy.
