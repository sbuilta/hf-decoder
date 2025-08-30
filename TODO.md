# TODO

This document outlines the initial implementation plan for the HF FT8/JS8 decoder system.
Each task represents a step toward realizing the architecture in `ARCHITECTURE.docx`.

## Project setup
- [x] Initialize C++ project structure with CMake and `src/`, `include/`, and `docs/` directories.
- [x] Document build dependencies (librtlsdr, FFTW3, Liquid-DSP, SQLite, CivetWeb or similar HTTP server).
- [x] Provide scripts or instructions to install required libraries on Raspberry Pi 3B+.

## RF input module
- [x] Implement RTL-SDR interface using `librtlsdr` to tune frequency, set sample rate, and stream IQ data.
- [x] Add band preset table and command API for frequency changes.
- [x] Apply digital down‑conversion and decimation to produce a ~12 kHz complex baseband stream.
- [x] Maintain a ring buffer of IQ samples aligned to 15‑second time slots using NTP-synchronized system clock.

## DSP and decoding engine
- [x] Implement synchronization and signal detection using FFT-based search for FT8/JS8 Costas patterns.
- [x] Demodulate 8‑FSK symbols and refine frequency/time offsets for each candidate signal.
- [x] Integrate LDPC(174,91) decoder and CRC check; consider existing GPL-compatible libraries (e.g., `ft8_lib`).
- [x] Decode FT8 message types and JS8 free‑text payloads into human‑readable strings.
- [x] Compute SNR per message using signal/noise power measurements referenced to 2.5 kHz bandwidth.
- [x] Structure decoder to handle multiple concurrent signals and optional JS8 decoding.

## Data storage
- [x] Define SQLite schema (timestamp, band, frequency, mode, SNR, message text, etc.).
- [x] Implement database module to insert batches of decoded messages and query recent entries.

## Networking and UI
- [x] Embed lightweight HTTP server to serve static dashboard and JSON/SSE endpoints.
- [x] Provide API routes to change band and toggle modes; update SDR module accordingly.
- [x] Implement front‑end page showing recent messages, current band/mode, and decoding status indicator.

## Concurrency and infrastructure
- [ ] Use threads or async tasks: SDR capture, decode engine, database logger, and web server.
- [ ] Implement thread-safe queues or message passing between modules.
- [ ] Add logging and configuration facilities.

## Testing and calibration
- [ ] Create unit tests for DSP components and message parsing using recorded FT8/JS8 samples.
- [ ] Verify real-time performance on Raspberry Pi and refine algorithm parameters.

