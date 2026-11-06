# Java Review

Use the common criteria and nearby Java conventions. Do not impose Khiops
C++-specific restrictions on Java constructors, access modifiers, exceptions,
imports, or documentation.

- Inspect compatibility with the Java level and JAR build configuration
  actually used by the project.
- For GUI changes, read the applicable
  [UI instructions](../../../instructions/ui-changes.instructions.md) and
  inspect relevant Swing event/state handling, refresh/update behavior,
  cancellation, and resource lifetimes without assuming C++ view callbacks
  directly define Java widget behavior.
- For JNI changes, follow both sides of the interface: method signatures,
  object/string lifetimes, conversions, errors, and native-library loading.
- Check that changed classes/resources reach the JAR and intended installers,
  and that launcher/classpath assumptions remain consistent where affected.
- Check naming, responsibilities, duplication, failure reporting, and accurate
  contracts/documentation under existing Java conventions.

Keep the review proportional to these rare changes, but never silently omit
them. Report unavailable GUI/JNI validation and the relevant pending build or
interaction checks; do not run write-producing builds or GUI sessions here.
