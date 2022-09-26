# Change Log

This project uses [Semantic Versioning](http://semver.org/).

## [1.0.1](https://github.com/verbit-ai/verbit-streaming-cpp-sdk/releases/tag/v1.0.1) (2022-09-27)

- Support `ws_url` that already has params (new auth scheme)

## [1.0.0](https://github.com/verbit-ai/verbit-streaming-cpp-sdk/releases/tag/v1.0.0) (2022-04-22)

- SONAME: `libverbit_streaming.so.1`
- Fix `constexpr std::string` compile error under C++17
- Create `libverbit_streaming.so` symlink for linker
- Make `install` target fix

## [0.0.1](https://github.com/verbit-ai/verbit-streaming-cpp-sdk/releases/tag/v0.0.1) (2022-03-02)

- First pre-release: Functional C++ SDK with WAV file example client
- SONAME: `libverbit_streaming.so.0`
- Connection retry with exponential backoff not yet implemented
