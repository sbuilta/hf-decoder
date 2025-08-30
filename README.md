# HF Decoder

HF Decoder is a software suite for decoding FT8 and JS8 digital radio signals on low-power hardware such as the Raspberry Pi 3B+. It captures IQ samples from an RTL-SDR, performs synchronization and FSK demodulation, decodes payloads, and exposes the results via a lightweight web interface and SQLite database.

## Build and Install (Raspberry Pi 3B+)

1. **Install prerequisite packages**
   On a fresh Raspberry Pi OS Lite (64-bit), update the package index and install
   the development libraries used by the decoder:
   ```bash
   sudo apt-get update
   sudo apt-get install -y \
       build-essential cmake git \
       librtlsdr-dev libfftw3-dev libliquid-dev \
       sqlite3 libsqlite3-dev \
       libcivetweb-dev
   ```
   These commands are also available in `scripts/install_deps_rpi.sh` if you
   prefer to run them after cloning the repository.

2. **Clone the repository**
   ```bash
   git clone https://github.com/example/hf-decoder.git
   cd hf-decoder
   ```

3. **Configure the project**
   ```bash
   cmake -S . -B build -DBUILD_MAIN=ON
   ```

4. **Compile**
   ```bash
   cmake --build build
   ```

5. **Install or run**
   - Run directly from the build tree:
     ```bash
     ./build/hfdecoder
     ```
   - Optionally copy `hfdecoder` and `docs/hfdecoder.conf` into a system directory such as `/usr/local/bin` and `/etc/hfdecoder/`.

## Developmental Test Plan

After building, validate functionality with the following steps:

1. **Unit tests** – Run the Catch2 test suite against recorded FT8/JS8 samples.
   ```bash
   ctest --test-dir build
   ```
2. **Live decode check** – Connect an RTL-SDR tuned to an active FT8/JS8 band and verify decoded messages appear on the console and in the SQLite database.
3. **Web dashboard** – Visit the HTTP server (default `http://localhost:8080`) and confirm recent decodes are displayed and API endpoints respond.
4. **Persistence** – Restart the application and verify previous decodes remain accessible via database queries.

## Future Work

* Performance profiling and optimization to guarantee real-time operation on Raspberry Pi-class hardware.
* Comprehensive end-to-end integration tests covering SDR capture through web serving.
* Support for additional digital modes and configuration via the web UI.
* Packaging scripts for Debian/Ubuntu to simplify deployment.
* Continuous integration to build and test across platforms and compilers.

