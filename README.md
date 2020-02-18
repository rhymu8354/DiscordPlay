# DiscordTest

This is a test program which uses the
[Discord](https://github.com/rhymu8354/Discord.git) library to experiment with
Discord.

## Usage

    Usage: DiscordPlay

    Perform Discord experiment.

## Supported platforms / recommended toolchains

This is a portable C++11 application which depends only on the C++11 compiler,
the C and C++ standard libraries, and other C++11 libraries with similar
dependencies, so it should be supported on almost any platform.  The following
are recommended toolchains for popular platforms.

* Windows -- [Visual Studio](https://www.visualstudio.com/) (Microsoft Visual
  C++)
* Linux -- clang or gcc
* MacOS -- Xcode (clang)

## Building

This application is not intended to stand alone.  It is intended to be included
in a larger solution which uses [CMake](https://cmake.org/) to generate the
build system and provide the application with its dependencies.

There are two distinct steps in the build process:

1. Generation of the build system, using CMake
2. Compiling, linking, etc., using CMake-compatible toolchain

### Prerequisites

* [CMake](https://cmake.org/) version 3.8 or newer
* C++11 toolchain compatible with CMake for your development platform (e.g.
  [Visual Studio](https://www.visualstudio.com/) on Windows)
* [Discord](https://github.com/rhymu8354/Discord.git) - a library which
  implements the formal and informal specifications provided by Discord for
  interfacing programs with the Discord infrastructure.
* [Http](https://github.com/rhymu8354/Http.git) - a library which implements
  [RFC 7230](https://tools.ietf.org/html/rfc7230), "Hypertext Transfer Protocol
  (HTTP/1.1): Message Syntax and Routing".
* [HttpNetworkTransport](https://github.com/rhymu8354/HttpNetworkTransport.git) -
  a library which implements the transport interfaces needed by the `Http`
  library, in terms of the network endpoint and connection abstractions
  provided by the `SystemAbstractions` library.
* [StringExtensions](https://github.com/rhymu8354/StringExtensions.git) - a
  library containing C++ string-oriented libraries, many of which ought to be
  in the standard library, but aren't.
* [SystemAbstractions](https://github.com/rhymu8354/SystemAbstractions.git) - a
  cross-platform adapter library for system services whose APIs vary from one
  operating system to another
* [TlsDecorator](https://github.com/rhymu8354/TlsDecorator.git) - an adapter to
  use `LibreSSL` to encrypt traffic passing through a network connection
  provided by `SystemAbstractions`
* [WebSockets](https://github.com/rhymu8354/WebSockets.git) - a library which
  implements [RFC 6455](https://tools.ietf.org/html/rfc6455), "The WebSocket
  Protocol".

### Build system generation

Generate the build system using [CMake](https://cmake.org/) from the solution
root.  For example:

```bash
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A "x64" ..
```

### Compiling, linking, et cetera

Either use [CMake](https://cmake.org/) or your toolchain's IDE to build.
For [CMake](https://cmake.org/):

```bash
cd build
cmake --build . --config Release
```
