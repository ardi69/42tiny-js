# TinyJS Library

TinyJS is a lightweight, modular JavaScript interpreter designed for seamless integration into C++ projects. With a compact code base and a focus on essential functionality, TinyJS allows you to incorporate scripting capabilities into your application without a heavy overhead.

## Features

- **JavaScript Interpreter:**  
  Supports a significant subset of JavaScript syntax and semantics, allowing you to execute scripts within your C++ applications.

- **Modular Extension:**  
  - **Date Functions:**  
    Provides basic operations for handling dates and times.
  - **Math Functions:**  
    Integrates fundamental mathematical operations and functions.
  - **String Functions:**  
    Offers comprehensive string manipulation and processing.
  - **Array Functions:**  
    Provides basic operations for handling arrays.

- **Threading Support:**  
  Built-in support for concurrent execution, enabling scripts to run in separate threads—ideal for multi-core environments.

- **Configurability:**  
  Use the `config.h` file to adjust parameters and feature switches, tailoring the library to different needs and platforms.

- **Time Measurement and Logging:**  
  Integrated time logging (via `time_logger.h`) helps with debugging and optimization by precisely measuring script execution times.

## Installation

### Prerequisites
- A modern C++ compiler (C++17 or later recommended)
- Standard C++ build tools (e.g., Make, CMake, etc.)

### Building
1. Clone or extract the repository:
   ```bash
   git clone https://github.com/YourRepository/TinyJS.git
   ```
   or extract the ZIP archive:
   ```bash
   unzip 42tiny-js-master.zip
   ```
2. Navigate to the project directory:
   ```bash
   cd 42tiny-js-master
   ```
3. Build the project:
   - Using Make:
     ```bash
     make
     ```
   - Alternatively, you can create a CMake script to integrate the library into your project.

## Usage

### Integrating into Your Project
Include the header file `TinyJS.h` in your C++ files and compile the corresponding .cpp files with your project:
```cpp
#include "TinyJS.h"

int main() {
    TinyJS::CTinyJS interpreter;
    interpreter.execute("var x = 10; print(x);");
    return 0;
}
```

### Extending Functionality
Take advantage of the modular extensions to make date, math, and string functions available in your scripts. The implementations in files such as `TinyJS_MathFunctions.cpp` provide examples of how to add or enhance functionality.

### Threading and Concurrency
If your project requires parallel execution, use the threading modules. The header file `TinyJS_Threading.h` offers examples and interfaces to execute scripts in separate threads.

## Example

```cpp
#include "TinyJS.h"

int main() {
    // Initialize the interpreter
    TinyJS::CTinyJS interpreter;

    // Execute a simple script
    interpreter.execute("print('Hello, TinyJS!');");

    // Use a math function
    interpreter.execute("var result = Math.sqrt(16); print('Square root of 16 is ' + result);");

    // Use date functions
    interpreter.execute("var now = new Date(); print('Current date: ' + now.toString());");

    return 0;
}
```

## Customization and Configuration

### config.h
Adjust parameters such as debug levels, feature switches, or platform-specific settings in the `config.h` file to optimally integrate TinyJS into your project.

### Extensions
TinyJS's structure makes it easy to add your own modules and functions. Refer to the files `TinyJS_Functions.cpp` and `TinyJS_StringFunctions.cpp` for examples of how to implement additional functionality.

## Changelog

For an overview of changes and improvements, please refer to the [CHANGELOG.md](CHANGELOG.md) file.

## License

TinyJS is released under an open-source license. Please refer to the `LICENSE` file (if available) or contact the project maintainers for details.

## Support and Contributions

If you encounter issues or have suggestions for new features, please open an issue in the repository or submit a pull request. Your feedback is important to us!

## Conclusion

TinyJS provides a simple and efficient way to integrate JavaScript scripting into C++ applications. With its modular architecture, support for extensions, threading, and logging, it is ideally suited for embedded systems and applications that benefit from dynamic scripting.

---

Happy Coding!
