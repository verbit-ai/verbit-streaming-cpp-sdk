# Change Log

This project uses [Semantic Versioning](http://semver.org/).

## [1.1.4](https://github.com/verbit-ai/verbit-streaming-cpp-sdk/releases/tag/v1.1.4) (2026-02-05)

- Make ping keepalive time configurable from default 30s
- Reset keepalive time on receipt of caption response, in addition to ping (resilience)

## [1.1.3](https://github.com/verbit-ai/verbit-streaming-cpp-sdk/releases/tag/v1.1.3) (2026-01-23)

- Automatically retry WebSocket connection a few times, after failure (resilience)

## [1.1.1](https://github.com/verbit-ai/verbit-streaming-cpp-sdk/releases/tag/v1.1.1) (2024-03-18)

- Adjust timeout for shutdown at end of stream to 15s
- Fix rare uncaught exception on WebSocket close

## [1.1.0](https://github.com/verbit-ai/verbit-streaming-cpp-sdk/releases/tag/v1.1.0) (2024-02-01)

- Improved resilience for sending and receiving end-of-stream signals to service host
- Enable WebSocketStreamingClient logging at run time with call to `log_path()`
- Add `stop_stream()` method to WebSocketStreamingClient to provide another shutdown option
- Stability improvements for various exit scenarios
- Add keepalive thread to close websocket in case it becomes unresponsive

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
