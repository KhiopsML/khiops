# Contributing to Khiops

Thank you for your interest in contributing to Khiops!

The developer documentation lives in the [doc/development](./doc/development/README.md) directory. This page is a guided tour of it, in the order you will most likely need it.

## Getting started

1. [Set up your development environment](./doc/development/setting-up-dev-environment.md): system requirements, Git configuration, cloning the repository, installing the `pre-commit` hooks and MPI.
2. [Build Khiops](./doc/development/building-khiops.md): CMake presets and custom build options.

## Writing code

- [Coding guidelines](./doc/development/coding-guidelines.md): style, naming conventions and C++ language restrictions. Follow them for any C++ change.
- [User interface development](./doc/development/ui-development.md): Khiops model/view classes, the `.dd` attribute files and the `genere` tool.
- [Debugging](./doc/development/debugging-and-testing.md): memory allocator and leak detection, assertions, error management and tracing.
- [Updating copyright notices each year](./doc/development/update-copyright.md): a yearly maintenance task. Most files are updated automatically by the `pre-commit` hook.

## Submitting your changes

Install the `pre-commit` hooks as described in the [environment setup](./doc/development/setting-up-dev-environment.md). They check formatting, copyright notices and file encoding before each commit, so that your changes are accepted by the CI.

## Testing

Run the tests before submitting your changes:

- [Unit tests](./doc/development/unit-tests.md): GoogleTest-based tests, including MPI tests (build with `-DTESTING=ON`).
- [Integration tests](./doc/development/debugging-and-testing.md#integration-tests): the non-regression scenarios of `LearningTest`, run with the [LearningTestTool](./test/LearningTestTool/README.md).

## Packaging and releases

- [Release process](./doc/development/release-process.md): release checklist, versioning scheme and Git tags.
- [Manual package testing protocol](./doc/development/manual-package-testing-protocol.md): checklists to run on the Windows, Linux and macOS packages before a release.

## License

Khiops is distributed under the [BSD 3-Clause Clear License](./LICENSE).
