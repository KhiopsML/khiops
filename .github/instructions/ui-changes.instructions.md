---
name: ui-changes
description: "Use when modifying Khiops UI model/view classes, .dd attribute specification files, or UI generation flow with genere and generate_gui.cmake."
applyTo: ["**/*.dd", "src/**/UI*.h", "src/**/UI*.cpp", "src/**/*View*.h", "src/**/*View*.cpp", "scripts/generate_gui.cmake"]
---

# UI Development Instructions for Khiops

Full reference: https://github.com/KhiopsML/khiops/wiki/User-Interface-Development-and-Usage

## UI Architecture

The UI is defined at an abstract level using a Model-View pattern. Instantiate a `view` object with its `model` object; the GUI engine handles window/dialog creation.

**Class hierarchy:**
- `UIObject` -> `UIData` -> `UIUnit` -> `UICard` (contains `UIObjectView`, `UIConfirmationCard`, `UIQuestionCard`, `UIFileChooserCard`) and `UIList` (contains `UIObjectArrayView`)
- `UIObject` -> `UIData` -> `UIElement` -> `UIBooleanElement`, `UICharElement`, `UIIntElement`, `UIDoubleElement`, `UIStringElement`
- `UIObject` -> `UIAction`

**UI modes** (set via `UIObject::SetUIMode`):
- **Graphic**: Windows/dialog boxes (Java via JNI)
- **Textual**: Shell-based input/output

## MVC Synchronization Pattern

Each `UIUnit` subclass implements two handlers:
- `EventRefresh()`: called before opening and after each action (interface -> sub-units), synchronizes model data to the view
- `EventUpdate()`: called after closing and before each action (sub-units -> interface), saves view changes to the model

Rule of thumb:
- Keep `EventRefresh()` side-effect-free except for UI state updates
- Validate and persist changes in `EventUpdate()`

## `genere` Tool

`genere` automatically generates C++ model and view classes from `.dd` attribute specification files:

```bash
genere [options] <ClassName> <ClassLabel> <AttributeFileName>
```

Generates up to 3 classes:
- `ClassName` (model, subclass of `Object`)
- `ClassNameView` (card view, subclass of `UIObjectView`)
- `ClassNameArrayView` (list view, subclass of `UIObjectArrayView`)

**Key options:** `-nomodel`, `-noview`, `-noarrayview`, `-super <SuperClassName>`, `-outputdir <Dir>`

## `.dd` Attribute Specification Files

`.dd` files are semicolon-separated and describe GUI card/list attributes.

| Field | Required | Description |
|-------|----------|-------------|
| Rank | Yes | Order number in the class |
| Name | Yes | Attribute name (used in getters/setters) |
| Type | Yes | `boolean`, `char`, `int`, `double`, `ALString` |
| Status | No | `Standard` (default) or `Derived` (read-only) |
| Style | No | GUI widget style (e.g., `CheckBox`, `Spinner`, `ComboBox`) |
| Invisible | No | Hide in GUI (default: `false`) |
| Label | No | Display label (default: same as Name) |

Example (`KWRecodingSpec.dd`):
```
Rank;Name;Type;Style;Label
1;FilterAttributes;boolean;CheckBox;Keep informative variables only
2;MaxFilteredAttributeNumber;int;Spinner;Max number of filtered variables
```

## Preserving Manual Customizations

When generated files are regenerated, only sections in custom markers are preserved:
- `// ## Custom ...`
- `// ##`

Place all manual edits inside these blocks.

## CMake Integration for UI Generation

UI generation is wired through `scripts/generate_gui.cmake` and the `generate_gui_add_view` helpers.

Typical usage in `CMakeLists.txt`:

```cmake
generate_gui_add_view(KWDatabase "Database" KWDatabase.dd KWDatabase.dd.log -nomodel -noarrayview)
generate_gui_add_view_add_dependency(KWUserInterface)
```

Enable generation with:
- `GENERATE_VIEWS=ON` in `CMakePresets.json`/`CMakeUserPresets.json`
- or `-D GENERATE_VIEWS=ON` at configure time

Important:
- The `.dd.log` output must be part of target sources so regeneration is triggered when `.dd` changes.

## Command-Line Options (UI/Batch Usage)

Default options for Khiops executables:
```
-e <file>    store logs in the file
-b           batch mode, with no GUI
-i <file>    replay commands stored in a file
-j <file>    JSON file used to set replay parameters
-o <file>    record commands in a file
-O <file>    same as -o but without replay
-r <s>:<s>   search and replace in the command file
-p <file>    store last progression messages
-h           print help
```

## Common Pitfalls

- Editing generated code outside `// ## Custom` blocks (changes lost on regeneration)
- Forgetting to enable `GENERATE_VIEWS` when changing `.dd` files
- Declaring fields in `.dd` without corresponding model/view logic expectations
- Mixing UI-mode assumptions: always verify behavior in both graphic and batch/textual execution paths
