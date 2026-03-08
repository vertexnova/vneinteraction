# VertexNova Coding Guidelines

**Version:** 1.1  
**Last Updated:** January 2026  
**C++ Standard:** C++20 (VneTemplate uses C++20)

## Table of Contents

1. [Introduction](#introduction)
2. [General Principles](#general-principles)
3. [Naming Conventions](#naming-conventions)
4. [Code Formatting](#code-formatting)
5. [Header Files](#header-files)
6. [Classes and Structs](#classes-and-structs)
7. [Functions](#functions)
8. [Initialization](#initialization)
9. [Memory Management](#memory-management)
10. [Error Handling](#error-handling)
11. [Thread Safety](#thread-safety)
12. [Modern C++ Features](#modern-c-features)
13. [Templates](#templates)
14. [Objective-C++ Guidelines](#objective-c-guidelines)
15. [Best Practices](#best-practices)
16. [Code Review Checklist](#code-review-checklist)

---

## Introduction

These coding guidelines establish standards for writing C++ and Objective-C++ code in the VertexNova engine project. The guidelines prioritize:

- **Clarity and Readability**: Code should be self-documenting and easy to understand
- **Modern C++**: Leverage C++17/20 features for safety, performance, and expressiveness
- **Consistency**: Uniform style across the entire codebase
- **Maintainability**: Code that can be easily modified and extended
- **Cross-Platform Compatibility**: Code that works across Windows, macOS, Linux, iOS, and Web

These guidelines draw inspiration from:
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Epic C++ Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines.html)
- [NVIDIA Carbonite SDK Coding Style](https://docs.omniverse.nvidia.com/kit/docs/carbonite/latest/CODING.html)

---

## General Principles

### Philosophy

1. **Prefer clarity over cleverness**: Write code that is obvious, not clever
2. **Make invalid states unrepresentable**: Use the type system to prevent errors
3. **Prefer compile-time checks over runtime checks**: Use `constexpr`, `static_assert`, and strong typing
4. **Follow the Rule of Zero/Five**: Let the compiler generate special member functions when possible
5. **Use RAII**: Resource management through object lifetime
6. **Prefer composition over inheritance**: Favor composition and interfaces over deep inheritance hierarchies

### Code Organization

- **One class per header file**: Each class should have its own header file
- **Logical grouping**: Related classes should be in the same directory/namespace
- **Minimal dependencies**: Include only what you need, forward declare when possible
- **Clear module boundaries**: Use namespaces to organize code logically

---

## Naming Conventions

### Summary Table

| Construct | Style | Example |
|-----------|-------|---------|
| Classes/Structs | PascalCase | `Buffer`, `ShaderCompiler` |
| Interface classes | `I` + PascalCase | `IRenderer`, `IBuffer` |
| Enums | PascalCase | `LogSink`, `ShaderStage` |
| Enum values | `e` + PascalCase + explicit value | `eNone = 0`, `eConsole = 1` |
| Type aliases | PascalCase | `EntityId`, `BufferHandle` |
| Functions/Methods | camelCase | `initialize()`, `createBuffer()` |
| Constants | `k` + PascalCase | `kMaxBufferSize` |
| Private/Protected members | snake_case + `_` | `buffer_size_`, `is_initialized_` |
| Public members | snake_case | `buffer_size`, `is_initialized` |
| Local variables | snake_case | `buffer_size`, `file_path` |
| Function parameters | snake_case | `buffer_size`, `usage` |
| Static members (private) | `s_` + snake_case + `_` | `s_instance_count_` |
| Static members (public) | `s_` + snake_case | `s_instance_count` |
| Global variables | `g_` + snake_case | `g_instance`, `g_config` |
| Booleans | `is_`, `has_`, `can_`, `should_` prefix | `is_ready_`, `has_alpha_` |
| Macros | ALL_CAPS | `VNE_ASSERT`, `VNE_PLATFORM_WINDOWS` |
| Namespaces | lowercase | `vne`, `xgl`, `xwin` |
| File names | snake_case | `shader_compiler.h` |
| Header guards | `#pragma once` | `#pragma once` |

### General Rules

- **Be descriptive**: Names should clearly indicate purpose
- **Avoid abbreviations**: Use full words unless the abbreviation is widely understood (e.g., `id`, `max`, `min`)
- **Consistency**: Use the same naming style throughout the codebase

### Types (Classes, Structs, Type Aliases)

Use **PascalCase** for all type names:

```cpp
// Classes
class ShaderCompiler { };
class MetalSurface { };
class LogManager { };

// Structs
struct LoggerConfig { };
struct SurfaceDescriptor { };
struct ValidationResult { };

// Type aliases (no T prefix)
using BufferHandle = uint32_t;
using EntityId = uint64_t;
using DeviceId = std::string;
```

### Interface Classes

Use **`I` prefix** followed by **PascalCase** for abstract interface classes:

```cpp
// Interface classes
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void render() = 0;
    virtual void present() = 0;
};

class IBuffer {
public:
    virtual ~IBuffer() = default;
    virtual void* map() = 0;
    virtual void unmap() = 0;
};

class IInputHandler {
public:
    virtual ~IInputHandler() = default;
    virtual void onKeyPressed(int key_code) = 0;
    virtual void onMouseMoved(float x, float y) = 0;
};
```

### Enums

Use **PascalCase** for enum names. Use **`e` prefix** with **PascalCase** for enum values, and always specify explicit values:

```cpp
// Enum class with e-prefixed values and explicit values
enum class LogSink {
    eNone = 0,
    eConsole = 1,
    eFile = 2,
    eBoth = 3
};

enum class ShaderStage {
    eVertex = 0,
    eFragment = 1,
    eCompute = 2,
    eGeometry = 3,
    eTessControl = 4,
    eTessEvaluation = 5
};

enum class BufferUsage {
    eNone = 0,
    eVertex = 1 << 0,
    eIndex = 1 << 1,
    eUniform = 1 << 2,
    eStorage = 1 << 3,
    eTransferSrc = 1 << 4,
    eTransferDst = 1 << 5
};
```

### Functions and Methods

Use **camelCase** for all functions and methods:

```cpp
class Renderer {
public:
    void initialize();
    bool createBuffer(size_t size);
    void setViewport(int width, int height);
    void submitCommandBuffer();
    
    // Getters use camelCase too
    int getWidth() const;
    bool isInitialized() const;

private:
    void initializeInternal();
    bool validateState();
    void cleanupResources();
};

// Free functions
void processVertices(const std::vector<Vertex>& vertices);
std::vector<uint32_t> compileShader(const std::string& source);
```

### Variables

Use **snake_case** for all variables:

```cpp
// Local variables
int window_width = 1920;
int window_height = 1080;
std::string shader_source;
std::vector<uint32_t> spirv_binary;

// Function parameters
void setViewport(int width, int height, float min_depth, float max_depth);
```

### Member Variables

Use **snake_case** with conventions based on visibility:

```cpp
class Buffer {
public:
    // Public members: snake_case (no trailing underscore)
    size_t size;
    BufferUsage usage;
    bool is_mapped;

protected:
    // Protected members: snake_case with trailing underscore
    uint32_t handle_;
    Device* device_;

private:
    // Private members: snake_case with trailing underscore
    void* data_;
    bool is_initialized_;
    size_t capacity_;
};
```

### Boolean Variables

Use descriptive prefixes (`is_`, `has_`, `can_`, `should_`):

```cpp
class Texture {
private:
    bool is_loaded_;
    bool has_alpha_;
    bool can_resize_;
    bool should_generate_mipmaps_;
};

// Local booleans
bool is_valid = validateInput(data);
bool has_error = result.error_code != 0;
```

### Static Member Variables

Use **`s_` prefix** with **snake_case**:

```cpp
class Renderer {
public:
    // Public static: s_ prefix, no trailing underscore
    static int s_instance_count;
    static std::string s_default_shader_path;

private:
    // Private static: s_ prefix with trailing underscore
    static Device* s_device_;
    static bool s_is_initialized_;
};
```

### Global Variables

Use **`g_` prefix** with **snake_case**:

```cpp
// Global variables (use sparingly)
extern Device* g_device;
extern Config g_config;
extern bool g_is_debug_mode;
```

### Constants

Use **`k` prefix** followed by **PascalCase** for compile-time constants.

For file-scope constants, place them in an **anonymous namespace** (preferred over `static`):

```cpp
// File-scope constants in anonymous namespace
namespace {
constexpr int kMaxBufferSize = 1024 * 1024;
constexpr float kPi = 3.14159265359f;
constexpr const char* kDefaultShaderPath = "shaders/default.glsl";
}  // namespace

// Class constants (use static constexpr)
class Renderer {
public:
    static constexpr int kMaxTextures = 16;
    static constexpr float kDefaultFov = 60.0f;
    static constexpr size_t kMinBufferAlignment = 256;
};
```

### Namespaces

Use **lowercase** for namespace names:

```cpp
namespace vne {
namespace xgl {
    // Graphics API code
    class Device { };
    class Buffer { };
}

namespace xwin {
    // Window/Platform code
    class Window { };
    class Surface { };
}

namespace log {
    // Logging code
    class Logger { };
}
}

// C++17 nested namespace syntax
namespace vne::xgl::backend {
    class VulkanDevice { };
    class MetalDevice { };
}
```

### Macros

Use **ALL_CAPS** with project prefix:

```cpp
#define VNE_ASSERT(condition) /* ... */
#define VNE_UNUSED(variable) ((void)(variable))
#define VNE_DISABLE_COPY(ClassName) \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete

#define VNE_PLATFORM_WINDOWS 1
#define VNE_PLATFORM_MACOS 2
#define VNE_PLATFORM_LINUX 3
```

### File Names

Use **snake_case** for source files:

```
shader_compiler.h
shader_compiler.cpp
metal_surface.h
metal_surface.mm
uikit_window_manager.h
uikit_window_manager.mm
```

---

## Code Formatting

### Indentation and Spacing

- **4 spaces** per indentation level (no tabs)
- **120 characters** maximum line length
- **Trailing whitespace**: Remove all trailing whitespace
- **End of line**: Use LF (`\n`) for all platforms

### Braces

Use **attached braces** (opening brace on same line):

```cpp
if (condition) {
    // code
}

class MyClass {
public:
    void method() {
        // code
    }
};
```

**Exception**: Function definitions with long return types can use newline:

```cpp
template<typename T>
std::vector<T>
MyClass::processData(const std::vector<T>& input) {
    // code
}
```

### Spacing

```cpp
// Around operators
int result = a + b;
bool is_valid = (x > 0) && (y < 100);

// After keywords
if (condition) { }
for (int i = 0; i < count; ++i) { }
while (condition) { }

// In function calls
function(arg1, arg2, arg3);

// In template declarations
template<typename T, typename U>
void function(T t, U u);
```

### Pointer and Reference Alignment

Use **left-aligned** (attached to type):

```cpp
int* ptr;           // Pointer attached to type
int& ref;           // Reference attached to type
const char* str;    // Const pointer
const int& value;   // Const reference
```

---

## Header Files

### Header Guards

Use **`#pragma once`** for header guards:

```cpp
#pragma once

// Header content here
```

### Include Order

Order includes in groups, separated by blank lines:

1. Corresponding header file (for `.cpp` files)
2. Project headers (`"vertexnova/..."`)
3. System headers (`<...>`)
4. Third-party headers

```cpp
// shader_compiler.cpp
#include "shader_compiler.h"              // 1. Corresponding header

#include "vertexnova/logging/logging.h"   // 2. Project headers
#include "vertexnova/graphics/xgl/types.h"

#include <vector>                          // 3. System headers
#include <string>
#include <memory>

#include <shaderc/shaderc.hpp>            // 4. Third-party headers
```

### Header File Structure

```cpp
#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   January 2026
 * ----------------------------------------------------------------------
 */

// 1. Includes (in order: project, system, third-party)
#include "vertexnova/graphics/xgl/types.h"

#include <vector>
#include <string>
#include <memory>

// 2. Forward declarations
namespace vne::xgl {
    class Device;
    struct SurfaceDescriptor;
}

// 3. Namespace
namespace vne::xgl {

// 4. Class/struct/enum definitions
class MetalSurface {
public:
    MetalSurface();
    ~MetalSurface();
    
    // Rule of Five
    MetalSurface(const MetalSurface&) = delete;
    MetalSurface& operator=(const MetalSurface&) = delete;
    MetalSurface(MetalSurface&&) noexcept;
    MetalSurface& operator=(MetalSurface&&) noexcept;
    
    bool initialize(const SurfaceDescriptor& desc);
    void cleanup();
    
private:
    bool is_initialized_;
    SurfaceDescriptor descriptor_;
};

}  // namespace vne::xgl
```

### Forward Declarations

Prefer forward declarations over includes when possible:

```cpp
// In header file
class Device;                    // Forward declaration
struct SurfaceDescriptor;        // Forward declaration

// Include only when necessary
#include <vector>                // Need std::vector
#include <string>                // Need std::string
```

---

## Classes and Structs

### Class Organization

Order of declarations:

1. Public types and constants
2. Public constructors and destructor
3. Public methods
4. Protected members (if any)
5. Private members

```cpp
class Buffer {
public:
    // 1. Types and constants
    static constexpr size_t kMaxSize = 1024 * 1024;
    
    // 2. Constructors and destructor
    Buffer();
    explicit Buffer(size_t size);
    ~Buffer();
    
    // Rule of Five
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept;
    Buffer& operator=(Buffer&&) noexcept;
    
    // 3. Public methods
    bool initialize(size_t size);
    void* map();
    void unmap();
    size_t getSize() const { return size_; }
    
protected:
    // 4. Protected members (if any)
    
private:
    // 5. Private members
    uint32_t handle_;
    size_t size_;
    bool is_mapped_;
};
```

### Structs vs Classes

- **Structs**: Use for data containers, POD types, simple aggregates
- **Classes**: Use for types with invariants, encapsulation, behavior

```cpp
// Struct: Simple data container
struct Vertex {
    float position[3];
    float normal[3];
    float tex_coord[2];
};

// Class: Has behavior and invariants
class Buffer {
public:
    bool initialize(size_t size);  // Enforces invariants
    void* map();                   // Behavior
private:
    uint32_t handle_;              // Encapsulation
};
```

### Rule of Zero/Five

Prefer Rule of Zero when possible:

```cpp
class SimpleContainer {
    std::vector<int> data_;       // Compiler-generated copy/move is fine
};

// Explicitly delete when needed
class NonCopyable {
public:
    NonCopyable() = default;
    ~NonCopyable() = default;
    
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;
};
```

### Constructors

Use explicit constructors to prevent implicit conversions:

```cpp
class Buffer {
public:
    explicit Buffer(size_t size);     // Prevents implicit conversion
};

// Buffer b = 1024;  // Compile error
// Buffer b(1024);   // OK
```

Use member initializer lists:

```cpp
class Renderer {
public:
    Renderer(Device* device, int width, int height)
        : device_(device)
        , width_(width)
        , height_(height)
        , is_initialized_(false) {
        // Constructor body
    }
    
private:
    Device* device_;
    int width_;
    int height_;
    bool is_initialized_;
};
```

---

## Functions

### Function Signatures

Use clear parameter names in declarations:

```cpp
// Good: Clear parameter names
bool createBuffer(size_t size, BufferUsage usage, Buffer** out_buffer);

// Avoid: Unclear
bool createBuffer(size_t, BufferUsage, Buffer**);
```

Prefer references over pointers for non-nullable parameters:

```cpp
// Reference for non-nullable
void processData(const std::vector<int>& data);

// Pointer for nullable
void setCallback(Callback* callback);  // nullptr means no callback
```

### Return Values

Prefer return values over output parameters:

```cpp
// Preferred: Return value
std::vector<uint32_t> compileShader(const std::string& source);

// Avoid: Output parameter
void compileShader(const std::string& source, std::vector<uint32_t>* out);
```

Use `[[nodiscard]]` for functions whose return values should not be ignored:

```cpp
[[nodiscard]] bool initialize();
[[nodiscard]] std::unique_ptr<Buffer> createBuffer();
```

### Const Correctness

Mark methods const when they don't modify object state:

```cpp
class Buffer {
public:
    size_t getSize() const { return size_; }
    bool isMapped() const { return is_mapped_; }
    
    void* map() { /* modifies state */ }  // Not const
};
```

### Noexcept

Mark functions noexcept when they don't throw:

```cpp
class Buffer {
public:
    size_t getSize() const noexcept { return size_; }
    Buffer(Buffer&& other) noexcept;
};
```

---

## Initialization

### Prefer Brace Initialization

Use **brace initialization `{}`** (uniform initialization) for most cases:

```cpp
// Preferred: Brace initialization
int value{42};
std::string name{"VertexNova"};
std::vector<int> numbers{1, 2, 3, 4, 5};

// For class members
class Renderer {
private:
    int width_{1920};
    int height_{1080};
    bool is_initialized_{false};
    std::vector<Buffer> buffers_{};
};
```

### When to Use Parentheses

Use **parentheses `()`** when:

1. **Calling constructors with specific semantics**:
   ```cpp
   std::vector<int> ten_zeros(10);      // 10 elements, all zero
   std::vector<int> one_ten{10};        // 1 element with value 10
   ```

2. **Avoiding narrowing conversion errors** (when intentional):
   ```cpp
   int x = 3.14;     // Warning: narrowing
   int x{3.14};      // Error: narrowing not allowed
   int x(3.14);      // OK if intentional (but avoid)
   ```

### Member Initialization

Prefer **in-class member initializers** over constructor initialization lists when the value is a default:

```cpp
// Preferred: In-class initialization for defaults
class Buffer {
public:
    explicit Buffer(size_t size) : size_{size} { }
    
private:
    size_t size_;
    bool is_mapped_{false};           // Default value in-class
    void* data_{nullptr};             // Default value in-class
    uint32_t handle_{0};              // Default value in-class
};

// Use initializer list for non-default values
class Renderer {
public:
    Renderer(Device* device, int width, int height)
        : device_{device}
        , width_{width}
        , height_{height} {
        // Constructor body
    }
    
private:
    Device* device_;
    int width_;
    int height_;
    bool is_initialized_{false};      // Default in-class
};
```

### Aggregate Initialization

Use **designated initializers** (C++20) for structs:

```cpp
struct LoggerConfig {
    std::string name;
    LogLevel level{LogLevel::eInfo};
    bool async{false};
    std::string file_path;
};

// C++20 designated initializers
LoggerConfig config{
    .name = "app",
    .level = LogLevel::eDebug,
    .async = true,
    .file_path = "app.log"
};

// Pre-C++20: Named parameter idiom or builder pattern
LoggerConfig config;
config.name = "app";
config.level = LogLevel::eDebug;
```

---

## Memory Management

### Smart Pointers

Prefer smart pointers over raw pointers:

```cpp
// Use std::unique_ptr for exclusive ownership
std::unique_ptr<Buffer> createBuffer() {
    return std::make_unique<Buffer>(size);
}

// Use std::shared_ptr for shared ownership
std::shared_ptr<Texture> texture = Texture::create("texture.png");

// Use std::weak_ptr to break circular references
class Node {
private:
    std::weak_ptr<Node> parent_;
    std::vector<std::shared_ptr<Node>> children_;
};
```

### RAII

Use RAII for resource management:

```cpp
class FileHandle {
public:
    explicit FileHandle(const std::string& path) 
        : file_(fopen(path.c_str(), "r")) {
        if (!file_) {
            throw std::runtime_error("Failed to open file");
        }
    }
    
    ~FileHandle() {
        if (file_) {
            fclose(file_);
        }
    }
    
    FILE* get() { return file_; }
    
private:
    FILE* file_;
};
```

### Arrays and Vectors

Prefer `std::vector` over C arrays:

```cpp
// Preferred
std::vector<int> indices;
std::vector<Vertex> vertices;

// Use std::array for fixed-size arrays
std::array<float, 3> position;
std::array<float, 4> color;
```

---

## Error Handling

### Exceptions

Use exceptions for error handling:

```cpp
class Buffer {
public:
    static std::unique_ptr<Buffer> create(size_t size) {
        if (size == 0) {
            throw std::invalid_argument("Buffer size must be > 0");
        }
        
        auto buffer = std::make_unique<Buffer>();
        if (!buffer->initialize(size)) {
            throw std::runtime_error("Failed to initialize buffer");
        }
        
        return buffer;
    }
};
```

### Error Codes

Use error codes when performance is critical or interfacing with C APIs:

```cpp
enum class Result {
    eSuccess = 0,
    eInvalidParameter = 1,
    eOutOfMemory = 2,
    eDeviceLost = 3
};

Result createBuffer(size_t size, Buffer** out_buffer);
```

### Assertions

Use assertions for programming errors (bugs), not user errors:

```cpp
#include <cassert>

void setViewport(int width, int height) {
    assert(width > 0 && height > 0);  // Programming error if false
    // ...
}
```

### Error Handling Strategy Summary

| Scenario | Approach | Example |
|----------|----------|---------|
| **Programming error (bug)** | `assert()` | Null pointer passed to non-nullable parameter |
| **Expected failure** | Return `bool` or error code | File not found, network timeout |
| **Exceptional condition** | Exception | Out of memory, corrupted data |
| **Nullable return** | `std::optional<T>` | Cache lookup that may miss |
| **Operation result** | `Result` enum or `std::expected` | Graphics API calls |

---

## Thread Safety

### Documentation Requirements

Document thread safety guarantees for all public APIs using Doxygen tags:

```cpp
/**
 * @brief Get the singleton instance of the LogManager
 * 
 * @return Reference to the LogManager instance
 * 
 * @threadsafe This function is thread-safe. The instance is created
 *             using a thread-safe singleton pattern.
 */
LogManager& getInstance();

/**
 * @brief Write a log message
 * 
 * @param message The message to log
 * 
 * @threadsafe This function is thread-safe and can be called from
 *             multiple threads concurrently.
 */
void log(const std::string& message);

/**
 * @brief Configure the logger
 * 
 * @param config The configuration to apply
 * 
 * @warning Not thread-safe. Must be called before any logging occurs
 *          or while holding an exclusive lock.
 */
void configure(const LoggerConfig& config);
```

### Thread Safety Annotations

Use standard annotations in comments:

| Annotation | Meaning |
|------------|---------|
| `@threadsafe` | Safe to call from multiple threads |
| `@note Thread-safe` | Alternative form |
| `@warning Not thread-safe` | Must be externally synchronized |
| `@note Requires lock` | Caller must hold specified lock |

### Synchronization Primitives

Use appropriate synchronization:

```cpp
#include <mutex>
#include <shared_mutex>

class ThreadSafeCache {
public:
    // Reader - shared lock (multiple readers allowed)
    std::optional<Value> get(const Key& key) const {
        std::shared_lock lock{mutex_};
        auto it = cache_.find(key);
        return it != cache_.end() ? std::optional{it->second} : std::nullopt;
    }
    
    // Writer - exclusive lock
    void put(const Key& key, const Value& value) {
        std::unique_lock lock{mutex_};
        cache_[key] = value;
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<Key, Value> cache_;
};
```

### Lock-Free Alternatives

Consider lock-free alternatives for high-performance scenarios:

```cpp
#include <atomic>

class AtomicCounter {
public:
    void increment() noexcept {
        count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    int64_t get() const noexcept {
        return count_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<int64_t> count_{0};
};
```

### Thread Safety Guidelines

1. **Prefer immutable data**: Immutable objects are inherently thread-safe
2. **Minimize shared state**: Reduce the need for synchronization
3. **Use RAII for locks**: Always use `std::lock_guard` or `std::unique_lock`
4. **Avoid deadlocks**: Always acquire locks in consistent order
5. **Document guarantees**: Every public API should document its thread safety

---

## Modern C++ Features

### Auto

Use `auto` when the type is obvious from context:

```cpp
// Good: Type is obvious
auto buffer = createBuffer();
auto count = items.size();
auto it = map.find(key);

// Prefer explicit type when clarity matters
std::unique_ptr<Buffer> buffer = createBuffer();
```

### Range-Based For Loops

Prefer range-based for loops:

```cpp
// Preferred
for (const auto& item : items) {
    processItem(item);
}

// Modify in place
for (auto& item : items) {
    item.update();
}
```

### Lambda Expressions

Use lambdas for short, local functions:

```cpp
std::sort(items.begin(), items.end(), 
    [](const auto& a, const auto& b) { return a.value < b.value; });

auto processor = [&data](int index) {
    processItem(data[index]);
};
```

### Constexpr

Use `constexpr` for compile-time constants and functions:

```cpp
constexpr int kMaxBuffers = 256;
constexpr float kPi = 3.14159265359f;

constexpr int square(int x) {
    return x * x;
}

constexpr int squared = square(5);  // Computed at compile time
```

### If Constexpr

Use `if constexpr` for compile-time conditionals:

```cpp
template<typename T>
void process(T value) {
    if constexpr (std::is_integral_v<T>) {
        processInteger(value);
    } else {
        processOther(value);
    }
}
```

### Structured Bindings

Use structured bindings for tuples and pairs:

```cpp
auto [it, inserted] = map.insert({key, value});
if (inserted) {
    // New element was inserted
}

for (const auto& [key, value] : map) {
    processPair(key, value);
}
```

### Strongly-Typed Enums

Always use `enum class` instead of plain `enum`:

```cpp
// Preferred
enum class LogLevel {
    eTrace = 0,
    eDebug = 1,
    eInfo = 2,
    eWarn = 3,
    eError = 4,
    eFatal = 5
};
```

### nullptr

Always use `nullptr` instead of `NULL` or `0`:

```cpp
Device* device = nullptr;
if (device == nullptr) { }
```

### std::optional

Use `std::optional<T>` for values that may or may not be present:

```cpp
#include <optional>

// Return optional for operations that may fail without error
std::optional<Texture> loadTexture(const std::string& path) {
    if (!fileExists(path)) {
        return std::nullopt;  // No texture, but not an error
    }
    return Texture{path};
}

// Usage
if (auto texture = loadTexture("icon.png")) {
    render(*texture);  // Dereference with *
}

// With value_or for defaults
auto texture = loadTexture("icon.png").value_or(defaultTexture());

// Class members
class Cache {
private:
    std::optional<Config> cached_config_;  // May not be loaded yet
};
```

### std::string_view

Use `std::string_view` for non-owning string references:

```cpp
#include <string_view>

// Function that reads but doesn't own the string
void log(std::string_view message) {
    // No allocation, works with std::string, const char*, string literals
    std::cout << message << '\n';
}

// Works with any string-like input
log("Hello");                    // const char*
log(std::string{"World"});       // std::string
log(some_string.substr(0, 10)); // substring view

// Parsing without allocation
std::string_view parseToken(std::string_view input) {
    auto pos = input.find(' ');
    return input.substr(0, pos);
}
```

**Caution**: Don't store `string_view` beyond the lifetime of the source string.

### std::span (C++20)

Use `std::span<T>` for non-owning array/container views:

```cpp
#include <span>

// Function that works with any contiguous container
void processVertices(std::span<const Vertex> vertices) {
    for (const auto& vertex : vertices) {
        // Process vertex
    }
}

// Works with any contiguous data
std::vector<Vertex> vec_vertices;
std::array<Vertex, 100> arr_vertices;
Vertex c_vertices[50];

processVertices(vec_vertices);   // std::vector
processVertices(arr_vertices);   // std::array
processVertices(c_vertices);     // C array

// Subspan
processVertices(std::span{vec_vertices}.subspan(0, 10));  // First 10
```

### std::variant

Use `std::variant<Ts...>` for type-safe unions:

```cpp
#include <variant>

// Type-safe alternative to union
using ShaderValue = std::variant<int, float, glm::vec3, glm::vec4>;

void setUniform(const std::string& name, const ShaderValue& value) {
    std::visit([&name](const auto& v) {
        setUniformImpl(name, v);
    }, value);
}

// Error handling with variant
using Result = std::variant<Texture, ErrorCode>;

Result loadTexture(const std::string& path) {
    if (!fileExists(path)) {
        return ErrorCode::eFileNotFound;
    }
    return Texture{path};
}
```

---

## Templates

### Template Parameters

Use descriptive names for template parameters:

```cpp
template<typename ElementType, size_t ArraySize>
class FixedArray { };

// T is acceptable for simple, generic templates
template<typename T>
T clamp(T value, T min_val, T max_val);
```

### Concepts (C++20)

Use concepts to constrain templates:

```cpp
template<std::integral T>
T square(T value) {
    return value * value;
}

template<typename T>
concept Drawable = requires(T t) {
    t.draw();
};

template<Drawable T>
void render(const T& object) {
    object.draw();
}
```

---

## Objective-C++ Guidelines

### File Organization

Separate Objective-C and C++ code when possible:

```cpp
// metal_surface.h (C++ header)
#pragma once
#include "vertexnova/graphics/xgl/types.h"

namespace vne::xgl {
    class MetalSurface {
    public:
        bool initialize(const SurfaceDescriptor& desc);
    };
}
```

```objc
// metal_surface.mm (Objective-C++ implementation)
#include "metal_surface.h"
#import <Metal/Metal.h>

namespace vne::xgl {
    bool MetalSurface::initialize(const SurfaceDescriptor& desc) {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        // ...
    }
}
```

### Include/Import Order

Order includes/imports:

1. C++ headers (`#include`)
2. Objective-C headers (`#import`)
3. System frameworks

```cpp
// metal_surface.mm
#include "metal_surface.h"           // 1. C++ headers
#include "vertexnova/logging/logging.h"

#import <Metal/Metal.h>              // 2. Objective-C headers
#import <QuartzCore/CAMetalLayer.h>

#ifdef VNE_PLATFORM_MACOS
#import <Cocoa/Cocoa.h>               // 3. System frameworks
#elif defined(VNE_PLATFORM_IOS)
#import <UIKit/UIKit.h>
#endif
```

### Memory Management

Use ARC (Automatic Reference Counting):

- ARC is enabled by default in modern Objective-C++
- Use `__strong`, `__weak`, `__unsafe_unretained` when needed
- Avoid manual `retain`/`release` calls

```objc
id<MTLDevice> device = MTLCreateSystemDefaultDevice();
CAMetalLayer* layer = [[CAMetalLayer alloc] init];

// Weak reference to break cycles
__weak id<MTLDevice> weak_device = device;
```

### Naming Conventions

Follow Objective-C naming conventions for Objective-C code, C++ conventions for C++ code:

```objc
// Objective-C style
@interface MetalSurfaceWrapper : NSObject
- (BOOL)initializeWithDescriptor:(SurfaceDescriptor*)descriptor;
- (void)cleanup;
@property (nonatomic, strong) CAMetalLayer* layer;
@end

// C++ style for C++ code
namespace vne::xgl {
    class MetalSurface {
    public:
        bool initialize(const SurfaceDescriptor& desc);
    };
}
```

---

## Best Practices

### Performance

1. **Measure before optimizing**: Profile first, optimize second
2. **Avoid premature optimization**: Write clear code first
3. **Use `const` and `constexpr`**: Enable compiler optimizations
4. **Prefer stack allocation**: Use stack when possible
5. **Cache-friendly data structures**: Consider memory layout

### Safety

1. **Use strong types**: Avoid primitive obsession
2. **Validate inputs**: Check parameters at boundaries
3. **Use RAII**: Automatic resource management
4. **Prefer const**: Immutability reduces bugs
5. **Avoid undefined behavior**: Know your language

### Testing

1. **Write testable code**: Small, focused functions
2. **Use dependency injection**: Test with mocks
3. **Test edge cases**: Boundary conditions, null inputs
4. **Keep tests fast**: Unit tests should run quickly

### Documentation

Use Doxygen-style comments for public APIs:

```cpp
/**
 * @brief Compiles GLSL source code to SPIR-V binary
 * 
 * This function compiles the provided GLSL source code to SPIR-V format,
 * which can be used across multiple graphics backends (Vulkan, Metal, etc.).
 * 
 * @param source GLSL source code to compile
 * @param stage Shader stage (vertex, fragment, compute, etc.)
 * @param entry_point Entry point function name (default: "main")
 * @return SPIR-V binary data as vector of uint32_t, empty on failure
 * 
 * @note This function is thread-safe
 */
std::vector<uint32_t> compileGLSLToSPIRV(
    const std::string& source,
    ShaderStage stage,
    const std::string& entry_point = "main");
```

---

## Code Review Checklist

### General
- Code is formatted with `.clang-format`
- No compiler warnings
- Code compiles on all target platforms

### Naming
- Classes/Structs use PascalCase
- Functions/Methods use camelCase
- Variables use snake_case
- Private members have trailing underscore
- Constants use kPascalCase prefix
- Enums values use ePascalCase prefix with explicit values

### Initialization
- Brace initialization `{}` used where appropriate
- In-class member initializers for defaults
- No uninitialized variables

### Modern C++
- Uses C++17/20 features appropriately
- Smart pointers used instead of raw pointers (where applicable)
- `std::optional` for nullable returns
- `std::string_view` for non-owning string parameters
- `const` correctness applied
- `noexcept` specified where appropriate
- Rule of Zero/Five followed

### Thread Safety
- Thread safety documented with `@threadsafe` or `@warning Not thread-safe`
- Proper synchronization for shared state
- RAII used for locks (`std::lock_guard`, `std::unique_lock`)
- No potential deadlocks

### Safety
- No memory leaks
- No undefined behavior
- Input validation where necessary
- Error handling implemented

### Documentation
- Public APIs documented
- Thread safety guarantees documented
- Complex logic explained
- Examples provided for non-obvious APIs

---

## Tools and Automation

### Formatting

Use `.clang-format` for automatic formatting:

```bash
# Format a file
clang-format -i path/to/file.cpp

# Format all C++ files
find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

### Linting

Use clang-tidy for static analysis:

```bash
clang-tidy file.cpp -- -std=c++17
```

---

## Conclusion

These guidelines are living documents and should evolve with the project. When in doubt:

1. **Be consistent** with existing code
2. **Prioritize clarity** over cleverness
3. **Follow modern C++ best practices**
4. **Ask for review** when uncertain

---

**References:**
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Epic C++ Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines.html)
- [NVIDIA Carbonite SDK Coding Style](https://docs.omniverse.nvidia.com/kit/docs/carbonite/latest/CODING.html)
