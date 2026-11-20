# Debugging and Testing

Prerequisite: [Set up your environment](./setting-up-dev-environment.md).

## Memory allocator


Khiops implements its own memory allocator, designed for high scalability, speed, memory efficiency, and low heap fragmentation.

In debug mode, the allocator keeps track of all allocated and free memory, with a summary displayed at the end of the program.

**Example:**
```plaintext
Memory stats (number of pointers, and memory space)
  Alloc: 735444  Free: 735444  MaxAlloc: 11228
  Requested: 77674392  Granted: 100574416  Free: 100574416  MaxGranted: 20819400
```

### Detecting Memory Leaks

If a memory leak occurs (that is, allocated memory is not freed), the summary includes additional information about the memory that was not freed.
For example, if your code contains a line like:
```cpp
Object* myObject = new Object;`
```
and you forget to delete it, you will see a warning similar to:

```plaintext
NewMem alloc block warning id=3602 000001C4C0215630 (8): Block not free

Memory stats (number of pointers, and memory space)
  Alloc: 735445  Free: 735444  MaxAlloc: 11229
  Requested: 77674400  Granted: 100574424  Free: 100574416  MaxGranted: 20819408

NewMem Warning: Block not free: 1       Memory not free: 8
```

### How to identify the source of the memory leak

- Call the `MemSetAllocIndexExit` function at the beginning of the main function, passing the block `id` as an argument:
```cpp
MemSetAllocIndexExit(3602);
```
- Set a breakpoint in the `GlobalExit()` function (located in `src/Norm.base/Standard.cpp`), the only place where the Khiops libraries call the `exit` function.

With your IDE running in debug mode, execution will halt at the instruction responsible for allocating the unfreed memory, allowing you to examine the full call stack and pinpoint the leak.



## Programming by Contract (Assertions)

**Do** follow the [design by contract paradigm](https://en.wikipedia.org/wiki/Design_by_contract).

Programming by contract is an approach where each software component explicitly defines its obligations and guarantees.
This paradigm promotes robustness, clear documentation, and early error detection by automatically verifying these conditions through assertions.

To this end, the `Norm` library provides assertion macros to test the state of the program.
For example:
```c++
assert(oInstance != NULL)
```
will make the program exit abruptly if `oInstance` is a `NULL` pointer.

`Norm` provides the following macros:
- `require`: Checks a method precondition (place it after the variable declarations)
- `ensure`: Checks a method postcondition (place it before the `return` statement)
- `assert`: A generic assertion that can be used anywhere in the code
- `check`: A specialized assertion that checks whether a pointer is not `NULL` (`check(oInstance)` is equivalent to `assert(oInstance != NULL)`)
- `cast`: Casts an `Object` subclass, checking the cast in debug mode (e.g., `myObject = cast(MyClass*, oInstance)`)

Do not hesitate to add assertions to make explicit hypotheses on the state of the program as they
can be seen as *executable comments* that enhance code clarity and robustness.


While the semantics differ, all assertions share the same failure mechanism: they cause an abrupt program termination.

#### Debugging Assertion Failures

To debug an assertion failure, set a breakpoint in the `GlobalExit()` function (located in [src/Norm.base/Standard.cpp](../../src/Norm/base/Standard.cpp)), which is the only call to the `exit` function within the Khiops libraries.

With your IDE in debug mode, execution will halt at the instruction responsible for the assertion failure, allowing you to examine the full call stack and identify the cause.


#### Example
```c++
// Recursive function to compute factorial
int MathUtils::Factorial(int nValue) const
{
    int nResult;

    require(nValue >= 0);

    if (nValue <= 1)
        nResult = 1;
    else
        nResult = nValue * Factorial(nValue-1);

    ensure(nResult >= nValue);
    return nResult;
}
```

> [!NOTE]  Only in debug builds
> The Khiops codebase has thousands of assertions, many executing the `Check` method  inherited from the `Object` class, which may contain exhaustive checks an object's state. For this reason, **assertion macros are available only in debug builds**. In release builds, they are converted to empty statements to favor execution speed.


## Error Management

Errors can be categorized into **programming errors** and **user errors**.

### Programming Errors
- These must be resolved before delivering the product.
- They are managed using assertions.
- The code for detecting programming errors is active only in **debug mode**.

### User Errors
- These can occur at any time, depending on user inputs.
- The detection code remains active regardless of the mode (debug or release).

### Handling User Errors
User errors are managed through the following steps:

1. **Detection**: By validating user inputs.
2. **Emission of the error**: Using methods inherited from the `Object` class:
   - `AddMessage`, `AddWarning`, `AddError`, etc., with the *error label* as an argument to indicate the *severity*.
   - `GetClassLabel`: To identify the *category* of the error (e.g., `File`).
   - `GetObjectLabel`: To specify the *location* of the error (e.g., `MyFile.txt`).
3. **Management**: Such as displaying the error or saving it to a log file, using the `Global` class.

This structured approach ensures that errors are detected, reported, and handled appropriately, improving the robustness and user experience of the application.

## Display and Tracing

During the debugging process, displaying object values or tracing execution flow is often very useful.

### Displaying the Content of an Object

To implement a display method, simply override the following method in your class derived from `Object`:
```cpp
void Write(ostream& ost) const;
```

Once implemented, you can display an object on the standard output using the `<<` operator:
```cpp
SampleObject soTest;

// Initialization
soTest.SetInt(1);
soTest.SetString("Hello");

// Display
cout << "Value of object: " << soTest << endl;
```

### Using Trace in Method Implementations

For complex algorithms, tracing is often necessary to follow the execution flow.
To make trace activation easy and flexible, use the following pattern:
- Define a Boolean constant, e.g., `const boolean bTrace = false;`, to enable or disable tracing.
- For multiple trace types, use distinct variable names (e.g., `bTraceDetails`).
- Declaring the variable as `const` allows the compiler to optimize away the trace code when tracing is disabled.

```cpp
SomeClass::SomeMethod()
{
    const boolean bTrace = false;
    // <some code>

    if (bTrace)
    {
        cout << "Some trace information" << endl;
        // ...
    }
}
```

This pattern makes it easy to toggle detailed debugging output without removing or commenting out code, facilitating efficient debugging and cleaner production code.

## Unit tests

Unit tests can be implemented at the `class` level following a standard pattern:

```c++
static void Test();
```

This method:
- Is a class-level function with no arguments.
- Is designed to test the key features of a non-trivial class.
- Can output results to standard output, which can serve as reference output for validation.

These unit test methods can then be executed using the [GoogleTest](https://google.github.io/googletest/) framework.
For detailed instructions on the process, see [Unit Tests](./unit-tests.md).


## Integration tests

Integration tests are performed using the Khiops test tool; see `../../test/LearningTestTool/README.md`.

`LearningTest` automates Khiops non-regression tests with over 800 scenarios, handling platform variations.

> [!NOTE] Running test from the IDE
> To run the LearningTest suites from the IDE, set the following environment variables:
> - `KHIOPS_API_MODE`: Set to `true` to enable API mode during development. Paths are > relative to the current working directory; otherwise, in GUI mode, they are relative to > the data directory.
> - `KhiopsFastExitMode`: Set to `false` to use the scenario files and execute all > instructions without exiting after the first error.
>
> These variables are set by the `kht_test` script in `LearningTestTool`.

[LearningTestTool](../../test/LearningTestTool/README.md) manages execution and analysis, supporting quick and stress tests, integrated into CI/CD for reliable validation.
