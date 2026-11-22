#!/bin/bash
#
# Keep the calendar version prefix (YYYY.MM) in library.json current.
# The agent owns YYYY.MM: on a code change this script sets it to the current
# year/month and, when the prefix rolls over, resets Micro to 1. It never
# increments Micro itself -- that bump belongs to the human. Idempotent: when
# the prefix already matches the current month it does nothing.
# Usage: ./tools/bump-version.sh
#
# Copyright Marc Ketel
# SPDX-License-Identifier: Apache-2.0
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="$SCRIPT_DIR/../library.json"

CURRENT_PREFIX="$(date +%Y.%m)"

FILE_PREFIX="$(sed -nE 's/.*"version": "([0-9]{4}\.[0-9]{2})\.[0-9]+".*/\1/p' "$VERSION_FILE" | head -n 1)"

if [ -z "$FILE_PREFIX" ]; then
    echo "[ERROR] Could not read the YYYY.MM prefix from $VERSION_FILE"
    exit 1
fi

if [ "$CURRENT_PREFIX" = "$FILE_PREFIX" ]; then
    echo "Version prefix already current ($CURRENT_PREFIX)."
    exit 0
fi

sed -i -E "s/(\"version\": )[0-9]{4}\.[0-9]{2}\.[0-9]+/\1${CURRENT_PREFIX}.1/" "$VERSION_FILE"

echo "Version prefix bumped from $FILE_PREFIX to $CURRENT_PREFIX (Micro reset to 1)."
