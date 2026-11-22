/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <Arduino.h>

#include <cstring>

#include "SerialCli.h"

namespace {
    cli::SerialCli serialCli([](const char *data, size_t len) {
        Serial.write(reinterpret_cast<const uint8_t *>(data), len);
    },
                             "Example CLI");

    /* Reply to a liveness check. */
    void cmdPing(int argc, char **argv) {
        (void)argc;
        (void)argv;
        serialCli.write("pong\r\n");
    }

    /* Switch the onboard LED on or off. */
    void cmdLed(int argc, char **argv) {
        if (argc < 2) {
            serialCli.write("usage: led on|off\r\n");
            return;
        }

        bool on = strcmp(argv[1], "on") == 0;
        digitalWrite(LED_BUILTIN, on ? LOW : HIGH);
        serialCli.writef("LED: %s\r\n", on ? "on" : "off");
    }

    /* Report milliseconds since boot. */
    void cmdUptime(int argc, char **argv) {
        (void)argc;
        (void)argv;
        serialCli.writef("UPTIME_MS: %lu\r\n", millis());
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    serialCli.registerCommand({"ping", cmdPing, "Reply with pong"});
    serialCli.registerCommand({"led", cmdLed, "Set the onboard LED: led on|off"});
    serialCli.registerCommand({"uptime", cmdUptime, "Show milliseconds since boot"});
}

void loop() {
    while (Serial.available()) {
        serialCli.feed(static_cast<char>(Serial.read()));
    }
}
