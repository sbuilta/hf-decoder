#!/usr/bin/env bash
set -e

# Install build essentials and project dependencies on Raspberry Pi 3B+
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git \
    librtlsdr-dev libfftw3-dev libliquid-dev \
    sqlite3 libsqlite3-dev \
    libcivetweb-dev
