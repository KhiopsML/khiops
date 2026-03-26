---
name: cpp-changes
description: "Use when modifying C++ code in Learning, Norm, or Parallel modules. Covers naming conventions, code style, assertion patterns, memory management, and pre-commit requirements."
applyTo: ["**/*.cpp", "**/*.h"]
---

# C++ Coding Instructions for Khiops

## Code Style & Formatting

### Line Length & Indentation

- **Max line length**: 120 characters, prefer ≤ 100
- **Indentation**: **Tabs** (not spaces)
- **Braces**: Allman style — opening brace on its own line at same indentation

```cpp
// ✅ Correct
void MyClass::DoSomething()
{
	if (condition)
	{
		// Code here
	}
}

// ❌ Wrong: Braces on same line
void MyClass::DoSomething() {
    if (condition) {  // Also wrong: spaces instead of tabs
```

### File Naming

- **Convention**: `UpperCamelCase.cpp` and `UpperCamelCase.h`
- **Class-to-file mapping**: Use one main class per file; filename matches class name
- **Module prefix**:
  - **Learning module**: `KW*` (e.g., `KWData.h`, `KWDataPreparation.cpp`)
  - **Learning sub-modules**:
    - `KD*`: Domain knowledge / feature construction (`KDDomainKnowledge/`)
    - `KNI*`: Khiops Native Interface (`KhiopsNativeInterface/`, `KNITransfer/`)
    - `MH*`: Histogram models (`MHHistograms/`, `genum/`, `khisto/`)
    - `SNB*`: Selective Naive Bayes predictor (`SNBPredictor/`)
    - `DT*`: Decision trees and random forests (`DTForest/`)
    - `CC*`: Coclustering (`MODL_Coclustering/`)
    - `KI*`: Model interpretation / Shapley values (`KIInterpretation/`)
  - **Parallel module**: `PL*` (e.g., `PLParallelTask.h`, `PLTaskInput.cpp`)
  - **Norm module**: Core utilities (e.g., `ALString.h`, `Object.h`)

### Comments

- **Language**: French (with the accented characters replaced with their ASCII counterparts, company convention)
- **Style**: `//` only (never `/* */`)
- **Placement**: Preceded by a blank line
- **Group comments**: Use banner `///////////` before group descriptions

```cpp
// Calcul des bornes de discretisation optimales
int DiscretizeVariable()
{
	// ...
}

///////////
// Methodes utilitaires de validation des donnees
void CheckDataIntegrity()
{
	// ...
}
```

## Variable Naming (Hungarian Notation)

Khiops uses Hungarian notation extensively. Prefix variables with type indicators:

| Prefix | Type | Example |
|--------|------|---------|
| `n` | `int` | `nSize`, `nIndex` |
| `b` | `boolean` | `bIsValid`, `bHasChildren` |
| `c` | `char` | `cDelimiter` |
| `l` | `longint` | `lFileSize` |
| `d` | `double` | `dMeanValue`, `dThreshold` |
| `s` | `ALString` / `char*` | `sLabel`, `sFileName` |
| `o` | `Object*` | `oDatabase`, `oTable` |
| `i`, `j`, `k` | Loop indices | `for (int i = 0; i < n; i++)` |
| `io` | `IntObject*` | `ioCounter` |
| `do` | `DoubleObject*` | `doStdDev` |
| `oa` | `ObjectArray*` | `oaColumns` |
| `ol` | `ObjectList*` | `olItems` |
| `ov` | `ObjectVector*` | `ovData` |
| `iv` | `IntVector*` | `ivIndices` |
| `dv` | `DoubleVector*` | `dvScores` |
| `sv` | `StringVector*` | `svLabels` |

```cpp
// ✅ Correct
int nCount = 0;
double dSum = 0.0;
ALString sFileName = "data.txt";
ObjectArray* oaColumns = new ObjectArray;

// ❌ Wrong: Missing type prefix
int count = 0;           // Should be nCount
double sum = 0.0;        // Should be dSum
string fileName = "";    // Should be sFileName + ALString
```

## Class Structure Rules

### Constructor & Initialization

- **One constructor per class, no parameters**
- **Initialization in dedicated methods**: `Initialize()`, `InitializeWithData()`
- **Avoid constructor side-effects**: Use `MasterInitialize()`/`SlaveInitialize()` for parallel tasks

```cpp
// ✅ Correct
class KWData : public Object
{
public:
	KWData();
	void Initialize();

	// ...
};

// ❌ Wrong: Multi-param constructor
class KWData : public Object
{
public:
	KWData(const ALString& sName, int nRows) { }  // Don't do this
};
```

### Access Control

- **No `private` sections** — use `protected` for implementation details
- **All attributes accessed via `Get`/`Set` accessors**, never directly
- **Standard accessor pattern**:

```cpp
// ✅ Correct
class KWData : public Object
{
public:
	void SetName(const ALString& sValue);
	const ALString& GetName() const;

protected:
	ALString sName;
};

// ❌ Wrong: Direct attribute access
oData->sName = "new name";  // Use SetName() instead
```

### Standard Methods

Every `Object` subclass should implement:

```cpp
class MyClass : public Object
{
public:
	// Operations sur les donnees
	Object* Clone() const override;
	void CopyFrom(const Object* oSource) override;
	int Compare(const Object* oOther) const override;

	// Controle d'integrite
	boolean Check() const override;
	const ALString GetObjectLabel() const override;

	// Suivi de la memoire
	longint GetUsedMemory() const override;

	// Gestion des erreurs
	void AddError(const ALString& sLabel, const ALString& sMessage);
	void AddWarning(const ALString& sLabel, const ALString& sMessage);
};
```

## C++ Feature Restrictions

Khiops deliberately restricts modern C++ features for consistency, safety, and cross-platform portability:

| Feature | Policy | Notes |
|---------|--------|-------|
| `const` | ✅ **DO use** | Always on methods and parameters |
| `override` | ✅ **DO use** | Mark inherited methods |
| `inline` | ✅ **DO use** | For frequent accessors and critical loops |
| Method overloading | ❌ **DON'T** | Use specific names (e.g., `GetValueAsInt()`, `GetValueAsString()`) |
| C++ references | ❌ **DON'T** | Exception: `ALString` passed as `const ALString&` |
| Multiple inheritance | ❌ **DON'T** | Single inheritance from `Object` only, always `public` |
| C++ STL | ❌ **Mostly DON'T** | Exceptions: `fstream`, `cout`/`cin`, `regex`, `chrono` |
| Templates | ❌ **DON'T** | Use `ObjectArray`, `ObjectList`, etc. instead |
| C++ exceptions | ❌ **DON'T** | Use assertions and error codes instead |
| Multi-threading | ❌ **DON'T** | Use `Parallel` library (MPI) instead |
| C API | ❌ **DON'T** | Use Khiops libraries (rare exceptions for low-level) |
| Third-party libraries | ❌ **DON'T** | Exception: MPI, googletest only |

```cpp
// ✅ Correct: Use custom containers
ObjectArray oaItems;
oaItems.Add(new MyObject);

// ❌ Wrong: STL, templates
std::vector<MyObject*> items;
template<typename T> class Container { };

// ✅ Correct: Use Khiops error handling
require(oObject != NULL);
if (nValue < 0) AddError("Invalid value");

// ❌ Wrong: Exceptions
if (nValue < 0) throw std::invalid_argument("...");
```

## Assertions & Design by Contract

Follow the **Design by Contract** paradigm. The `Norm` library provides assertion macros:

| Macro | When to Use | Example |
|-------|------------|---------|
| `require` | Method pre-condition | After variable declarations |
| `ensure` | Method post-condition | Before `return` |
| `assert` | Generic assertions | Anywhere in code |
| `check` | Null pointer check | `check(object)` ≡ `assert(object != NULL)` |
| `cast` | Safe downcast | `cast(MyClass*, object)` with type check in debug |

```cpp
// ✅ Correct
void KWData::SetRowCount(int nValue)
{
	// Pre-condition
	require(nValue >= 0);

	nRowCount = nValue;

	// Post-condition
	ensure(GetRowCount() == nValue);
}

// ❌ Wrong: No pre/post conditions
void KWData::SetRowCount(int nValue)
{
	nRowCount = nValue;
}

// ✅ Correct: Check null
check(oColumn);
oColumn->SetName("col1");

// ✅ Correct: Safe cast
cast(KWNumericColumn*, oColumn)->SetMin(0.0);
```

**Important**: Assertions are **active in debug builds only**. In release, they compile to empty statements. To debug assertion failures:

1. Set breakpoint in `GlobalExit()` (`src/Norm/base/Standard.cpp`)
2. Run in debug mode — execution halts at the failing assertion with full call stack

## Memory Management

Khiops has **custom memory allocator** optimized for scalability. Debug mode tracks all allocations:

```
Memory stats (number of pointers, and memory space)
  Alloc: 735444  Free: 735444  MaxAlloc: 11228
  Requested: 77674392  Granted: 100574416  Free: 100574416  MaxGranted: 20819400
```

### Memory Leak Detection

If allocation is not freed:

1. Call `MemSetAllocIndexExit(<id>)` at `main()` start
2. Set breakpoint in `GlobalExit()`
3. Run debug build — execution halts with full stack trace

```cpp
int main()
{
	MemSetAllocIndexExit(42);  // Debuggage d'un identifiant d'allocation specifique

	// ... votre code ...

	return 0;  // Point d'arret dans GlobalExit() en cas de fuite memoire
}
```

### Allocation Patterns

```cpp
// ✅ Correct: dynamic allocation for collections
ObjectArray* oaData = new ObjectArray;
oaData->Add(new MyObject);

// Cleanup when done
delete oaData;

// ✅ Correct: stack allocation for small objects
ALString sLabel;
sLabel = "my_label";

// ❌ Wrong: STL containers (use ObjectArray instead)
std::vector<Object*> data;
```

## Pre-commit Hooks & Formatting

All commits must pass `.pre-commit-config.yaml`:

1. **clang-format**: C++ code formatting (strict, non-negotiable)
2. **Copyright check**: Year must match current year
3. **Encoding check**: Must be UTF-8

### Running Pre-commit Checks Locally

```bash
# Install pre-commit
pip install pre-commit

# Run all checks
pre-commit run --all-files

# Format C++ file manually
clang-format -i src/path/to/file.cpp

# Check copyright year
python scripts/check-obsolete-copyright.py src/path/to/file.cpp

# Check encoding
python scripts/check-encoding.py src/path/to/file.cpp
```

### Common Formatting Fixes

```cpp
// ❌ Will fail clang-format
void MyClass::Function(  ){int x=5;if(true){do_something( );}}

// ✅ After clang-format
void MyClass::Function()
{
	int x = 5;
	if (true)
	{
		do_something();
	}
}
```

## Module-Specific Conventions

### Learning Module (`KW*` prefix)

- **Data structures**: Use `ObjectArray`, `ObjectDictionary` from Norm
- **Error handling**: `AddError()`, `AddWarning()`, `Check()` methods
- **Parallel tasks**: Inherit from `PLParallelTask` if parallelizable
- **Sub-module prefixes**:
  - `KD*`: Domain knowledge / feature construction (`KDDomainKnowledge/`)
  - `KNI*`: Khiops Native Interface (`KhiopsNativeInterface/`, `KNITransfer/`)
  - `MH*`: Histogram models (`MHHistograms/`, `genum/`, `khisto/`)
  - `SNB*`: Selective Naive Bayes predictor (`SNBPredictor/`)
  - `DT*`: Decision trees and random forests (`DTForest/`)
  - `CC*`: Coclustering (`MODL_Coclustering/`)
  - `KI*`: Model interpretation / Shapley values (`KIInterpretation/`)

### Parallel Module (`PL*` prefix)

- **Shared variables**: `DeclareSharedParameter()`, `DeclareTaskInput()`, `DeclareTaskOutput()`
- **Master/slave pattern**: Subclass `PLParallelTask` and implement all pure virtual methods:
  - `GetTaskName() const` — unique task name
  - `Create() const` — returns a new instance of the task
  - `MasterInitialize()` — master initialization
  - `MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)` — prepare input for next slave
  - `MasterAggregateResults()` — aggregate results from a slave
  - `MasterFinalize(boolean bProcessEndedCorrectly)` — master cleanup
  - `SlaveInitialize()` — slave initialization
  - `SlaveProcess()` — main slave processing (reads `taskInput`, writes `taskOutput`)
  - `SlaveFinalize(boolean bProcessEndedCorrectly)` — slave cleanup
- **Optional override**: `ComputeResourceRequirements()` — declare memory/disk needs before launch
- **No multi-threading**: All parallelization via MPI

### Norm Module (Core)

- **Base classes**: `Object`, `ALString`, numeric types
- **No cross-module dependencies upward**: Norm never depends on Learning or Parallel
- **Self-contained utilities**: Collections, string handling, memory management

## Parallel Library (`PLParallelTask`)

Full reference: `src/Parallel/doc/bibliotheque parallele.md`

### Architecture

The parallel library uses MPI (Message Passing Interface) for distributed computing. It is split into two libraries:
- **`PLParallelTask`**: user-facing library — the sole entry point for developing parallel features
- **`PLMPI`**: technical library wrapping MPI calls (can be replaced by another framework)
- **`PLSamples`**: didactic examples

`PLParallelTask` can be used **without** `PLMPI` and without MPI installed — the program will run sequentially. Adding `PLMPI` later enables parallel execution of the same code.

### Master/Slave Framework

Create a parallel task by subclassing `PLParallelTask` and implementing virtual methods:

**Slave methods:**

| Method | Description |
|--------|-------------|
| `SlaveInitialize` | Slave initialization |
| `SlaveProcess` | Parallel processing (called in loop) |
| `SlaveFinalize` | Slave cleanup |

**Master methods:**

| Method | Description |
|--------|-------------|
| `MasterInitialize` | Master initialization |
| `MasterPrepareTaskInput` | Prepare input data for the next slave |
| `MasterAggregateResults` | Aggregate results received from a slave |
| `MasterFinalize` | Master cleanup |

Calling `Run()` executes the task. Slaves are instantiated dynamically; the number of slaves is computed automatically based on available resources.

### Execution Modes

- **Parallel**: multiple MPI processes, full master/slave protocol
- **Sequential**: when not enough cores are available; no MPI calls, minimal overhead. Same user code as parallel
- **Simulated parallel**: single process but mimics parallel behavior (serialization, separate master/slave objects). Enable with `PLParallelTask::SetParallelSimulated(true)`. **If the program works in simulated parallel mode, it will work in parallel.**

Development workflow: **debug in sequential, test in simulated parallel, deploy in parallel**.

### Shared Variables

Data exchange between master and slaves uses shared variables, declared in the task constructor:

| Declaration | Role | Sent |
|------------|------|------|
| `DeclareSharedParameter` | Constants | After `MasterInitialize` -> before `SlaveInitialize` (once) |
| `DeclareTaskInput` | Slave inputs | After `MasterPrepareTaskInput` -> before `SlaveProcess` (each iteration) |
| `DeclareTaskOutput` | Slave outputs | After `SlaveProcess` -> before `MasterAggregateResults` (each iteration) |

Write access is controlled by assertions — each variable type can only be modified in specific methods.

**Simple types:** `PLShared_Boolean`, `PLShared_Int`, `PLShared_Double`, `PLShared_LongInt`, `PLShared_String`, `PLShared_Char`

**Vectors:** `PLShared_StringVector`, `PLShared_IntVector`, `PLShared_LongintVector`, `PLShared_DoubleVector`

**Containers:** `PLShared_ObjectArray`, `PLShared_ObjectDictionary`, `PLShared_ObjectList` (contained objects must implement `PLSharedObject`)

**Custom types:** subclass `PLSharedObject`, implement `Create()`, `SerializeObject(PLSerializer*)`, `DeserializeObject(PLSerializer*)`. Convention: prefix with `PLShared_`, provide `SetValue`/`GetValue` accessors.

### Resource Management

The class `RMParallelResourceManager` computes the number of slaves based on:
- System resources (auto-discovered: RAM, cores, disk)
- User constraints (`RMResourceConstraints`, set via UI)
- Task requirements (override `ComputeResourceRequirements()`)

Requirements to declare per master and slave:
- Memory min/max
- Disk space min/max
- Memory used by shared variables
- Allocation policy (master-preferred, slave-preferred, or balanced)

Allocated resources are accessible via `GetSlaveResourceGrant()` and `GetMasterResourceGrant()`.

### Task Registration

All parallel tasks must be registered at program startup (in `KWLearningProject.cpp` for MODL):
```cpp
PLParallelTask::RegisterTask(new MyTask);
```
This allows sleeping slaves to instantiate the correct task class when woken by the master.

### Best Practices & Pitfalls

- **Variable scoping**: master variables (`Master*` methods only) and slave variables (`Slave*` methods only) are **separate** — a class attribute set in `MasterInitialize` has no value in slave methods. Use shared variables for cross-process data.
- **Naming convention**: prefix shared variables with `input_`, `output_`, or `shared_` to clarify their role
- **Constructor**: avoid using the constructor for initialization — use `MasterInitialize`/`SlaveInitialize` instead (constructor runs on both master and slave)
- **Registration**: declaring a shared variable is not enough — it must be registered in the constructor with `DeclareSharedParameter`, `DeclareTaskInput`, or `DeclareTaskOutput`
- **Return values**: all virtual methods return `true` on success, `false` on failure. Returning `false` stops the task and `Run()` returns `false`
- **Example classes** (sorted by complexity): `PEHelloWorldTask` -> `PEProtocolTestTask` -> `PEPiTask` -> `PEIOParallelTestTask` -> `PEFileSearchTask`

## Common Pitfalls

### ❌ Using STL Instead of Khiops Collections

```cpp
// Wrong
std::vector<KWColumn*> columns;
std::map<string, int> labelCounts;

// Right
ObjectArray oaColumns;
StringIntDictionary* sdLabelCounts = new StringIntDictionary;
```

### ❌ Not Checking Pre-conditions

```cpp
// Wrong: No require()
void SetValue(int nValue)
{
	nMyValue = nValue;
}

// Right: Validate input
void SetValue(int nValue)
{
	require(nValue >= 0);
	nMyValue = nValue;
}
```

### ❌ Using C++ Exceptions

```cpp
// Wrong
if (error_condition) throw std::runtime_error("...");

// Right
require(!error_condition);  // Pre-condition
if (error_condition) AddError("Context", "Error message");
```

### ❌ Forgetting Copyright Year in New Files

```cpp
// ❌ Wrong or outdated
// Copyright (c) 2020 Orange
// SPDX-License-Identifier: BSD-3-Clause-Clear

// ✅ Correct (current year)
// Copyright (c) 2026 Orange
// SPDX-License-Identifier: BSD-3-Clause-Clear
```

## Testing & Debugging

### Error Management

Two categories:
- **Programming errors**: caught by assertions, active in debug mode only, must be fixed before delivery
- **User errors**: caught by input validation, active in all modes

User errors are handled via `Object` methods: `AddMessage`, `AddWarning`, `AddError` with `GetClassLabel`/`GetObjectLabel` for context. The `Global` class manages display and logging.

### Object Display

Override `void Write(ostream& ost) const` in `Object` subclasses, then use stream output for inspection:

```cpp
cout << myObject;
```

### Unit Tests

Static `Test()` methods on classes, run via GoogleTest:

```cpp
class KWData : public Object
{
public:
	// ...
	static boolean Test();
};

boolean KWData::Test()
{
	boolean bOK = true;

	// Test logic here
	bOK = bOK && (GetValue() == expected);

	return bOK;
}
```

Build with `-DTESTING=ON`, run with `ctest`.

### Integration Tests

Integration tests use the python library LearningTestTool (`test/LearningTestTool/`). `LearningTest` automates non-regression scenarios and handles platform variations.

### Tracing

Use a trace flag toggle for debugging:

```cpp
// At function top
const boolean bTrace = false;  // Toggle for debugging

if (bTrace)
	cout << "Debug: value = " << nValue << endl;

// Compiler optimizes away when false
```

## Resources

- **Full style guide**: [Coding Guidelines](https://github.com/KhiopsML/khiops/wiki/Coding-Guidelines)
- **Debugging guide**: [Debugging and Testing](https://github.com/KhiopsML/khiops/wiki/Debugging-and-Testing)
- **Design by Contract**: [Wikipedia](https://en.wikipedia.org/wiki/Design_by_contract)
- **clang-format config**: `.clang-format` in repository root
