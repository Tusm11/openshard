# Building OpenShard

## Requirements

- C++17 compiler (MSVC, Clang, GCC)
- CMake 3.15+
- CURL (optional, for Phase 0)

## Build

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
```

For Unix/Linux:
```bash
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make
```

## Running Tests

```bash
cd build
ctest
```

Or run individually:
```bash
Release/test_phase123.exe    # Phase 1-3
Release/test_phase0.exe      # Phase 0 (if CURL available)
```

## Project Phases

- **Phase 0**: Remote range retrieval validation
- **Phase 1**: TAR archive reader
- **Phase 2**: Provider-side index generation
- **Phase 3**: Selective retrieval engine
- **Phase 4+**: HTTP gateway, multi-file, retrieval planner, caching, concurrency

## Output

- `archive_reader.lib` - TAR reading library
- `openshard_index.lib` - Index management
- `retrieval_engine.lib` - Selective retrieval core
