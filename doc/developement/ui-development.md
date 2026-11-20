# User Interface Development and Usage

The user interface in Khiops is defined at an abstract level, allowing developers to focus on the application logic without concern for GUI details.

The developer simply needs to instantiate a `view` object with its corresponding `model` object to enable user interaction through a GUI. The GUI engine is fully responsible for designing the windows and dialog boxes based on the abstract specifications of the view.

Additionally, a tool called **genere** can automatically generate C++ classes for models and views from a simple list of attributes in GUI cards or lists. This allows developers to concentrate solely on the application logic, greatly simplifying the development process.

## Class Hierarchy

This section presents the class hierarchy for the GUI components, illustrating the inheritance structure and relationships among the various UI classes:

- **UIObject**
  - **UIData**
    - **UIUnit**
      - **UICard**
        - UIObjectView
        - UIConfirmationCard
        - UIQuestionCard
        - UIFileChooserCard
      - **UIList**
        - UIObjectArrayView
    - **UIElement**
      - UIBooleanElement
      - UICharElement
      - UIIntElement
      - UIDoubleElement
      - UIStringElement
  - **UIAction**

## Features of UI Components
- Enable rapid and straightforward creation of user interfaces
  - Simplify data viewing and updating, organized within interface units such as cards or lists
  - Support the triggering of user actions
- Applications can be fully developed and tested in C++, leveraging a clear separation between:
  - **Model classes** for application logic
  - **View classes** for the user interface, allowing an abstract definition of the GUI
- The interface can be dynamically deployed in different modes using the `UIObject::SetUIMode` method:
  - **Graphic** mode: with Windows, dialog boxes, and menus (using Java or similar frameworks)
  - **Textual** mode: in a shell environment, displaying data and prompting user input


## Usage of UI Components
- Restricted to the classes **UICard** and **UIList**.
- Declare the data and actions managed by the interface unit, then invoke the `Open` method.
- An API facilitates data exchange between the program and the interface.

### Note on the Model-View-Controller (MVC) Pattern

Each subclass of `UIUnit` typically implements two key methods to synchronize data between the model and the view:
```
// Refresh the interface with data
// This handler is called before opening and after each action,
// from the interface unit to its sub-units
virtual void EventRefresh();

// Update data from the interface
// This handler is called after closing and before each action,
// from the sub-units to the interface unit
virtual void EventUpdate();
```

When the `Open` method is invoked, the specifications of the abstract GUI model are transferred to a GUI engine (such as Java via JNI), which creates actual windows and dialog boxes based on these specifications.

The `EventRefresh` and `EventUpdate` methods are used as follows:
- **Opening a UI unit (card or list):** Call `EventRefresh` before displaying the GUI to synchronize data.
- **Closing a UI unit:** Call `EventUpdate` before closing to save any changes.
- **Triggering a user action:**
  - Call `EventUpdate` beforehand to update the model data.
  - Execute the corresponding action at the model level.
  - Call `EventRefresh` after the action completes to update the GUI accordingly.

## Saving and Re-executing Sessions
- Provides services to record and replay sessions of interface interactions
- Command files support comments using `//` for better readability and documentation

Essentially, every data update within the GUI and each user-triggered action are automatically recorded, enabling precise re-execution of the session.

## Command Line Parameter Handling
- The `ParseMainParameters` method of `UIObject`:
  - Specifies storage options for command sessions.
  - Analyzes the command line, manages errors.
  - Prepares functionality execution if everything is correct.

## Command Line Options

The default command line options are
```
   -e <file>                 store logs in the file
   -b                        batch mode, with no GUI
   -i <file>                 replay commands stored in the file
   -j <file>                 json file used to set replay parameters
   -o <file>                 record commands in the file
   -O <file>                 same as -o option, but without replay
   (-r <string>:<string>)... search and replace in the command file
   -p <file>                 store last progression messages
   -h                        print help
```

# Automatic Generation of User Interface

The **genere** tool automatically generate C++ c model and view classe, given a specification file describing a list of attributes, with mainly they name, type, GUI widget and label.

## Genere Tool Options
The command line option of the genere tool  are:
```
genere [options] <ClassName> <ClassLabel> <AttributeFileName>
          <ClassName>: base name for generated classes
          <ClassLabel>: label for generated classes
          <AttributeFileName>: name of the file (.dd) containing the attribute specifications
        Options:
          -nomodel
              no generation of class <ClassName>
          -noarrayview
              no generation of class <ClassName>ArrayView
          -noview
              no generation of classes <ClassName>View and <ClassName>ArrayView
          -nousersection
              no generation of user sections
          -specificmodel <SpecificModelClassName>
              (optionnal) name of a specific model class, to use instead of ClassName
          -super <SuperClassName>
              (optionnal) name of the parent class
          -outputdir <DirName>
              ouput directory for generation (default: current directory)
```

According to the options, the following C++ classes are generated:
- one model class, named *`ClassName`*, sub-class of `Object`
- one view class of type card, named *`ClassName`View*, subclass of `UIObjectView`
- one view class of type listd, named *`ClassName`ArrayView*,, subclass of `UIObjectArrayView`

## Attribute Specification File

The list of attributes related to a GUI card or list is described in a **data definition** file with the extension `.dd`.

The attribute description file starts with a header line that defines the fields, followed by one line per attribute of the class to be generated.
The order of the fields is not important, and only three fields are mandatory.
The fields are separated by `;`.

The attributes to specify are as follows. Default values are provided and will be used when optional characteristics are not explicitly defined.

| Field         | Mandatory | Description                                                                                                                                                                                                                            | Default Value  |
| ------------- | --------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------- |
| **Rank**      | Yes       | Order number of the attribute within the generated class                                                                                                                                                                               |                |
| **Name**      | Yes       | Name of the attribute used in getters/setters                                                                                                                                                                                          |                |
| **Type**      | Yes       | Data type of the attribute: `boolean`, `char`, `int`, `double`, `ALString`                                                                                                                                                             |                |
| **Status**    | No        | Attribute status, options include:   <br> - **Standard** (or none): default memory field <br> - **Derived**: derived (read-only) attribute                                                                                             | Standard       |
| **Style**     | No        | Presentation style in GUIs. Styles are defined in classes within `UserInterface.h`, with their default style: <br> - `UIBooleanElement` <br> - `UICharElement` <br> - `UIIntElement` <br> - `UIDoubleElement` <br> - `UIStringElement` |                |
| **Invisible** | No        | Indicates if the attribute should be hidden in GUIs.                                                                                                                                                                                   | `false`        |
| **Label**     | No        | Display label for the attribute (shown in GUIs).                                                                                                                                                                                       | Same as `Name` |

**Example**: `src/Learning/KWUserInterface/KWRecodingSpec.dd`
```
Rank;Name;Type;Style;Label
1;FilterAttributes               ;boolean ;CheckBox   ;Keep informative variables only
2;MaxFilteredAttributeNumber     ;int     ;Spinner    ;Max number of filtered variables
3;KeepInitialSymbolAttributes    ;boolean ;CheckBox   ;Keep initial categorical variables
4;KeepInitialContinuousAttributes;boolean ;CheckBox   ;Keep initial numerical variables
5;RecodeSymbolAttributes         ;ALString;ComboBox   ;Categorical recoding method
6;RecodeContinuousAttributes     ;ALString;ComboBox   ;Numerical recoding method
7;RecodeBivariateAttributes      ;ALString;ComboBox   ;Pairs recoding method
8;RecodeProbabilisticDistance    ;boolean ;CheckBox   ;Recode using prob distance (expert)
```

## User Specification Traceability

It may be necessary to make specific modifications to the source code generated by the `genere` tool.
The question then is: what happens during regeneration of the sources?

To handle this, modifications should only be made within designated user sections.
These sections are marked with specific tags, prefixed by `// ## `, such as:

```cpp
KWTrainParameters::KWTrainParameters()
{
	nMaxEvaluatedAttributeNumber = 0;

	// ## Custom constructor

	sClassifierCriterion = "none";

	// ##
}
```

During regeneration, the `genere` tool automatically incorporates these custom modifications as follows:
- Standard generated parts are replaced with the new versions.
- Modified sections (within the user sections) are copied into the new version, preserving your specific changes.

## Integration in the Khiops Project

The `.dd` attribute specification files are rarely modified, and regeneration is performed on demand by setting the `GENERATE_VIEWS` option to `ON` in the `CMakePresets.json` file located at the root of the repository:
```
        "GENERATE_VIEWS": "ON",
```

When a `.dd` file is used to specify a model or view class within a source directory, the corresponding `CMakeLists.txt` file must be configured for each use of the `genere` tool.
This is done by adding a line such as:

```
    generate_gui_add_view(KWDatabase "Database" KWDatabase.dd KWDatabase.dd.log -nomodel -noarrayview)
```

Additionally, at the end of the `CMakeLists.txt file`, you should include the following line to ensure proper build order:
```
    # Add dependency to ensure that genere will be built before the gui generation
    generate_gui_add_view_add_dependency(KWUserInterface)
```
