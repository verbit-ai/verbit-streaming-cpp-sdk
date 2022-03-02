# Verbit Streaming C++ SDK

## Overview

This is a C++ SDK for Verbit's Streaming Transcription API.

A client using the C++ SDK makes a WebSocket connection to Verbit's Streaming Speech Recognition services, streams media to the service, and gets transcription responses back from it.

The C++ SDK can be downloaded from [GitHub](https://github.com/verbit-ai/verbit-streaming-cpp-sdk).

## Requirements

- C++11 (currently tested with `g++` v4.8+)
- [WebSocket++](https://github.com/zaphoyd/websocketpp)
  - this is a header-only library
  - requires Boost **headers** for compilation only (libraries not needed)
    - tested with Boost 1.71.0 in Ubuntu 20.04.3 LTS
    - tested with Boost 1.65.1 in Ubuntu 18.04.6 LTS
- [JSON for Modern C++](https://github.com/nlohmann/json) 3.x
  - this is a header-only library
- [Doxygen](https://www.doxygen.nl/) and [Graphviz](https://graphviz.org/) (if building documentation)

## Installation

To compile and install the SDK from source:

        $ make all [ doc ]
        $ sudo make [ TARGET=/usr/local ] install [ install-doc ]

Ubuntu 18.04 LTS and 20.04 LTS packages are also available; the package can be built from source:

        $ make package

## Responses

Verbit's Streaming Speech Recognition services can currently produce two different types of responses; the client can request either or both:

1. **Transcript**: this type of response contains the recognized words since the beginning of the current utterance. Like in real human speech, the stream of words is segmented into utterances in automatic speech recognition. An utterance is recognized incrementally, processing more of the incoming audio at each step. Each utterance starts at a specific start-time and extends its end-time with each step, yielding the most updated result.
Note that sequential updates for the same utterance will overlap, each response superseding the previous one - until a response signaling the end of the utterance is received (having `is_final == True`).
The `alternatives` array might contain different hypotheses, ordered by confidence level.

   Example "transcript" responses can be found in [examples/responses/transcript.md](https://github.com/verbit-ai/verbit-streaming-python-sdk/blob/main/examples/responses/transcript.md).

2. **Captions**: this type of response contains the recognized words within a specific time window. In contrast to the incremental nature of "transcript"-type responses, the "captions"-type responses are non-overlapping and consecutive. 
Only one "captions"-type response covering a specific time-span in the audio will be returned (or none, if no words were uttered).
The `is_final` field is always `True` because no updates will be output for the same time-span. The `alternatives` array will always have only one item for captions.

   Example "captions" responses can be found in [examples/responses/captions.md](https://github.com/verbit-ai/verbit-streaming-python-sdk/blob/main/examples/responses/captions.md).

The full JSON response schema can be found in [examples/responses/schema.md](https://github.com/verbit-ai/verbit-streaming-python-sdk/blob/main/examples/responses/schema.md).

## Creating the Order

In order to use Verbit's Streaming Speech Recognition services, you must place an order using Verbit's Ordering API. Please refer to the "Ordering API" section of [the Python (reference) SDK documentation](https://github.com/verbit-ai/verbit-streaming-python-sdk) for details.

## Running the Example

After placing the order and obtaining the access token, you can run the client example. This sends audio from a WAV file, and receives Captions-type responses which are emitted to `stdout`.

        $ make bin/example_client
        $ export VERBIT_WS_TOKEN="<your_access_token>"
        $ bin/example_client test-files/fox-and-grapes.wav

The example source can be used to guide the implementation of your own custom client:

- `examples/example_client.cpp` is the main client program
  - consult `WebSocketStreamingClient` in the SDK documentation for details
- `examples/wav_media_generator.*` shows how to create a custom media generator
  - consult `MediaGenerator` in the SDK documentation for details

## SDK Documentation

Documentation for the C++ SDK is installed to `/usr/local/share/doc/verbit_streaming` by `make install` (or `/usr/share/doc/verbit_streaming` by the Ubuntu package). You can build the documentation in the `doc` subdirectory with:

        $ make doc

## Additional Testing

The C++ SDK comes with a set of unit and client tests that can be used to ensure correct functioning of the streaming client. To run them:

        $ make test

The C++ SDK also comes with a test WebSocket server, compatible with Verbit's Streaming Speech Recognition services, which can be used for client testing. It requires the [UUID library](https://packages.ubuntu.com/focal/uuid-dev).

By default the test server listens on port `9002/tcp`. To start it:

        $ make run-test-server
        test-bin/test_server [ port ]

You can then run the client example against the test server with:

        $ export VERBIT_WS_TOKEN="<any_string_that_is_at_least_40_characters_long>"
        $ make run-test-example
        bin/example_client [ -u wss://localhost:9002 ] test-files/thats-good.wav

The test server currently only supports Captions-type responses. It returns fake transcription text (_i.e._ it does no speech processing on the received media).
