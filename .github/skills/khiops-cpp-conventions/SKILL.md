---
name: khiops-cpp-conventions
description: "Use when editing or reviewing Khiops C++ source files (.cpp, .h); applies project naming, formatting, and validation conventions."
---

# Khiops C++ Conventions

## Quick Reference

**Before editing any `.cpp` or `.h` file:**

1. **Read the authoritative rules**: [`cpp-changes.instructions.md`](../../instructions/cpp-changes.instructions.md)

2. **Critical rules to remember**:
   - **Comments**: French, ASCII-only (no accents: e->e, a->a, c->c, etc.)
   - **Indentation**: Tabs only (never spaces)
   - **Braces**: Allman style (opening brace on own line)
   - **Variables**: Hungarian notation (`nSize`, `bOk`, `sFileName`, etc.)
   - **Module prefixes**: `KW*`, `PL*`, `KD*`, `SNB*`, `DT*`, etc.

3. **Pre-commit verification**:
   ```bash
   clang-format -i <file>
   scripts/check-encoding.py <file>
   scripts/check-obsolete-copyright.py <file>
   ```

## More Details

All detailed rules, examples, and exceptions are in
[`cpp-changes.instructions.md`](../../instructions/cpp-changes.instructions.md).

**This is the single source of truth. When in doubt, refer there.**

Use the `KhiopsC++` agent (defined in [`AGENTS.md`](../../agents/AGENTS.md)) for streamlined C++ editing with
conventions automatically enforced.