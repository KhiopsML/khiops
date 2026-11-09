# CMake Review

Read the [CMake instructions](../../../instructions/cmake-changes.instructions.md).
Use actual targets, presets, options, and dependency declarations as evidence;
examples in instructions are not an inventory of configured behavior.

- Check target membership, link dependency visibility, include paths, compile
  definitions, and platform settings. Prefer the existing target-based helpers
  and conventions to global settings that leak into unrelated targets.
- Follow optional-feature branches and target existence with features disabled,
  especially MPI, tests, JARs, and generation when affected. Do not assume a
  dependency or executable exists in every configuration.
- Inspect preset inheritance and overrides without hardcoding personal paths.
  Check shared versus user-local configuration and the supported CMake/compiler
  baseline from current project files.
- Trace custom commands' inputs, outputs, dependencies, and consumers. Check
  incremental rebuilds, generated-file availability, ordering, and paths with
  spaces. Do not flag intentional project patterns solely because a different
  generic CMake style is possible.
- Trace installed targets/files to centralized installation rules and named
  components. Check build-tree versus install-tree paths and package-specific
  conditions. Load the packaging reference for consumer effects.
- Inspect shell commands, quoting, environment assumptions, and error handling
  in configure/build helpers. Avoid silent fallback to a different feature or
  dependency when the requested configuration cannot work.
- Check that new options, helper functions, and layers solve an actual need
  rather than duplicate configuration or hide a simple condition.
- Verify tests are registered and included in the intended configurations;
  documentation must match actual flags, presets, and target names.

Identify focused configure/build/test combinations that would distinguish the
changed branches. Do not run configure, generate, build, install, or CPack during
the non-mutating review; report the commands/configurations as pending.
