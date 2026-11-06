---
name: cpp-changes
description: "Use when modifying Khiops C++ source or header files. Covers Khiops-specific implementation practices and validation."
applyTo: "**/*.cpp, **/*.h"
---

# C++ Change Instructions

## Coding Rules

Before editing C++ code, consult the relevant sections of the [Khiops Coding Guidelines](../../doc/developement/coding-guidelines.md) and follow them. Read the full guide when a change spans multiple areas. It is the authoritative source for C++ style, naming, class structure, and language/library restrictions.

For module-specific architecture and naming, follow the [workspace instructions](../copilot-instructions.md) and nearby code.

## Khiops-Specific Practices

- Use the `Norm` contract macros (`require`, `ensure`, `assert`, `check`, and `cast`) for preconditions, postconditions, and runtime checks. Assertions are active in debug builds only.
- Use `PLParallelTask` for parallel work; do not introduce threads. See `src/Parallel/doc/bibliotheque parallele.md` for its master/slave model and shared-variable rules.
- For assertion failures or memory leaks, use a debug build and the breakpoint/debugging guidance in the [workspace instructions](../copilot-instructions.md).

## Validation

- Format changed C++ files with `clang-format`.
- Run `scripts/check-encoding.py` and `scripts/check-obsolete-copyright.py` on changed files.
- Build and run the focused tests for the affected module, following the [workspace build and test guidance](../copilot-instructions.md).
