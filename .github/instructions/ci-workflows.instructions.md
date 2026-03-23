---
name: ci-workflows
description: "Use when modifying GitHub Actions workflows, CI triggers, matrices, packaging pipelines, or release automation under .github/workflows/."
applyTo: [".github/workflows/**"]
---

# GitHub Actions Workflow Instructions for Khiops

## Scope

This file applies to workflow changes in `.github/workflows/`.

Current workflows (11):
- `run-unit-tests.yml`
- `run-standard-tests.yml`
- `pre-commit-checks.yml`
- `pack-pip.yml`
- `pack-debian.yml`
- `pack-rpm.yml`
- `pack-nsis.yml`
- `pack-macos.yml`
- `test-conda.yml`
- `build-linux-pack-containers.yml`
- `update-kni-tutorial.yml`

## Trigger Strategy

Keep trigger behavior aligned with repository conventions:
- **PR-triggered workflows**: run only when relevant files change (source changes for tests, packaging/CMake changes for packaging)
- **Tag-triggered workflows**: packaging/release workflows
- **Manual workflows** (`workflow_dispatch`): heavy or exceptional jobs (macOS packaging, conda tests, container builds, tutorial sync)

When adding a new workflow or trigger:
- Use `paths` filters to avoid unnecessary CI load
- Prefer reusing existing trigger patterns before introducing new ones
- Ensure the trigger matches the expected development workflow (PR validation vs release artifact build)

## Test Workflows

Testing responsibilities:
- `run-unit-tests.yml`: unit tests and fast validation
- `run-standard-tests.yml`: end-to-end scenarios (Khiops, Coclustering, KNITransfer), serial and parallel paths
- `pre-commit-checks.yml`: formatting and repository policy checks (`clang-format`, `black`, `cmake-format`, copyright, encoding)

Guidelines:
- Keep unit and integration scopes separated
- Ensure parallel test coverage remains explicit when relevant
- Keep per-platform matrices synchronized with supported platforms

## Packaging Workflows

Packaging responsibilities:
- `pack-pip.yml`: Python wheels (`khiops-core`, `khiops-kni`)
- `pack-debian.yml`: DEB packages
- `pack-rpm.yml`: RPM packages
- `pack-nsis.yml`: Windows NSIS installer
- `pack-macos.yml`: macOS package/archive

Guidelines:
- Keep package naming/versioning consistent with `KHIOPS_VERSION`
- Keep packaging workflow logic aligned with CMake packaging files:
  - `packaging/install.cmake`
  - `packaging/packaging.cmake`
- Do not duplicate business logic in workflows when CMake/packaging scripts already implement it

## Platform Matrix and Compatibility

Supported CI targets include Windows, Linux, and macOS, with x64/ARM64 where applicable.

When changing matrices:
- Keep consistency with official support matrix
- Validate architecture-specific conditions (e.g., ARM64-specific container/image constraints)
- Avoid dropping existing platform coverage without explicit rationale

## Best Practices

- Prefer small, composable job steps over monolithic shell scripts
- Cache only stable dependencies; avoid stale build artifact reuse
- Keep workflow names and job names explicit and stable
- Use descriptive step names to ease CI debugging
- Keep secrets usage minimal and scoped per job

## Common Pitfalls

- Adding broad triggers (`on: [push, pull_request]`) without `paths` filters
- Duplicating build/test logic that already exists in scripts/CMake
- Diverging release/tag behavior across packaging workflows
- Changing matrix dimensions in one workflow but not the related ones
- Introducing platform-specific commands without proper conditionals

## Related Instruction Files

- CMake and CPack behavior: `.github/instructions/cmake-changes.instructions.md`
- Wheel packaging behavior: `.github/instructions/python-wheel.instructions.md`
- C++ build/test impacts: `.github/instructions/cpp-changes.instructions.md`
