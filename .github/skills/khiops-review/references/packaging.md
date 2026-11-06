# Packaging Review

Load the [CMake instructions](../../../instructions/cmake-changes.instructions.md)
for native packaging and shared installation. Load the
[wheel instructions](../../../instructions/python-wheel.instructions.md) for
pip/conda effects and the [CI instructions](../../../instructions/ci-workflows.instructions.md)
for packaging workflows. Check current recipes and scripts before applying
examples or inventories from documentation.

## Trace the deliverable

For each actually affected distribution, trace:

```text
configuration --> build target --> install component --> package contents
                                                        |
                                                        v
                               installed entry point --> runtime dependencies
```

- Check which components and artifacts are included/excluded, and whether
  executable bits, headers, shared libraries, resources, docs/licenses, and
  launchers reach their intended destinations.
- Distinguish build dependencies from runtime dependencies, and packaged files
  from external prerequisites. Verify platform/architecture conditions and
  library lookup/loader behavior using evidence, not platform guesses.
- Follow the authoritative version source through format-specific conversion,
  package metadata, filenames, tag checks, and publication. Avoid independent
  manually maintained copies of the version.
- Check relocatability, install prefixes, paths with spaces, environment setup,
  interpreter discovery, and launcher exit-code propagation.
- Verify installation tests execute the delivered artifact with its declared
  dependencies, not an available development-tree executable.

## Select formats by impact

- **Wheels:** inspect the actual per-package configuration, selected build
  targets/components, ABI/platform tags, library handling, and runtime metadata.
  Preserve MPI-free package boundaries and avoid leaking core dependencies into
  library/transfer packages. Follow configuration replacement between builds.
- **DEB/RPM:** inspect shared CPack rules and format-specific dependencies,
  components, paths, and install/removal scripts. Changes to package managers
  must preserve failure propagation and dependency resolution.
- **NSIS/Windows:** inspect installer contents, prerequisites, launchers,
  environment/path behavior, and upgrade/uninstall handling when changed.
- **Conda:** inspect recipes and build scripts as well as install rules.
  Do not assume the standard CMake installation path is used by a recipe.
- **Archives:** check their layout and whether omitted dependency management is
  clearly documented rather than assume native-package behavior.
- **Build containers:** check their consuming workflows, reproducibility,
  platform assumptions, and dependency availability.

A shared rule can affect several formats even if only one recipe is in the
diff. Conversely, do not demand unrelated packaging changes for every C++
edit. Prefer existing shared policy, without creating a universal packaging
abstraction for genuinely different formats.

Report unavailable platform/installation evidence explicitly and propose the
smallest relevant package-content and clean-install/runtime validations.
Do not build, install, uninstall, sign, publish, or run containers in this review.
