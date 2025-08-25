# Build Dependencies

The project relies on the following libraries:

- **librtlsdr**: Interface to RTL-SDR hardware for RF capture.
- **FFTW3**: Fast Fourier Transform routines used in DSP stages.
- **Liquid-DSP**: Signal processing primitives for demodulation and filtering.
- **SQLite3**: Embedded database for storing decoded messages.
- **CivetWeb** (or similar HTTP server library): Serves the web dashboard and API.

## Raspberry Pi 3B+ Setup

Run the following script to install dependencies on a Raspberry Pi 3B+:

```bash
./scripts/install_deps_rpi.sh
```
