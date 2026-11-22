/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SerialCli.h"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <utility>

#include <strings.h>

namespace cli {
    SerialCli::SerialCli(OutputCallback onOutput, const char *helpTitle)
        : _onOutput(std::move(onOutput)), _helpTitle(helpTitle != nullptr ? helpTitle : "Serial commands") {
        if (!_onOutput) {
            _onOutput = [](const char *data, size_t len) {
                (void)data;
                (void)len;
            };
        }

        _buffer[0] = '\0';
    }

    bool SerialCli::registerCommand(const CommandDef &cmd) {
        if (cmd.name == nullptr || cmd.name[0] == '\0' || !cmd.handler) {
            return false;
        }

        /* Re-registering a name replaces the earlier definition. */
        for (size_t i = 0; i < _commandCount; ++i) {
            if (strcasecmp(_commands[i].name, cmd.name) == 0) {
                _commands[i] = cmd;
                return true;
            }
        }

        if (_commandCount >= MAX_COMMANDS) {
            return false;
        }

        _commands[_commandCount++] = cmd;
        return true;
    }

    void SerialCli::feed(char c) {
        if (c == 0x7F || c == '\b') {
            if (_pos > 0) {
                _pos--;
                _onOutput("\b \b", 3);
            }
            return;
        }

        if (c == '\n' || c == '\r') {
            /* '\r\n' dispatches once: the second terminator finds an empty buffer. */
            if (_pos > 0) {
                _buffer[_pos] = '\0';
                _pos = 0;
                _onOutput("\r\n", 2);
                dispatch(_buffer.data());
            }
            return;
        }

        if (_pos < INPUT_BUFFER_SIZE - 1) {
            _buffer[_pos++] = c;
            _onOutput(&c, 1);
        }
        /* Buffer full: drop the character, never overflow. */
    }

    void SerialCli::dispatch(char *line) {
        line += strspn(line, " ");

        if (*line == '\0') {
            return;
        }

        /*
         * Tokenize in place. The line buffer is not needed after dispatch, so
         * this costs no extra stack on the SAMD21.
         */
        constexpr size_t MAX_ARGS = 16;
        std::array<char *, MAX_ARGS> argv{};
        int argc = 0;

        char *p = line;
        while (*p != '\0' && argc < static_cast<int>(MAX_ARGS)) {
            p += strspn(p, " ");
            if (*p == '\0') {
                break;
            }

            argv[argc++] = p;
            p += strcspn(p, " ");
            if (*p == ' ') {
                *p++ = '\0';
            }
        }

        if (argc == 0) {
            return;
        }

        if (strcasecmp(argv[0], "help") == 0) {
            cmdHelp(argc, argv.data());
            return;
        }

        for (size_t i = 0; i < _commandCount; ++i) {
            if (strcasecmp(_commands[i].name, argv[0]) == 0) {
                _commands[i].handler(argc, argv.data());
                return;
            }
        }

        writef("Unknown command: '%s'. Try 'help'.\r\n", argv[0]);
    }

    void SerialCli::write(const char *text) {
        if (text == nullptr) {
            return;
        }

        _onOutput(text, strlen(text));
    }

    void SerialCli::writef(const char *format, ...) {
        /* 160 bytes hold an 18-character name, its help text, and the line ending. */
        char line[160];

        va_list args;
        va_start(args, format);
        int len = vsnprintf(line, sizeof(line), format, args);
        va_end(args);

        if (len <= 0) {
            return;
        }

        len = static_cast<int>(std::min(static_cast<size_t>(len), sizeof(line) - 1));

        _onOutput(line, static_cast<size_t>(len));
    }

    size_t SerialCli::commandCount(void) const {
        return _commandCount;
    }

    const CommandDef *SerialCli::commandAt(size_t index) const {
        if (index >= _commandCount) {
            return nullptr;
        }

        return &_commands[index];
    }

    void SerialCli::cmdHelp([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {

        char header[96];
        int len = snprintf(header, sizeof(header), "=== %s ===\r\n", _helpTitle);
        if (len > 0) {
            size_t outLen = std::min(static_cast<size_t>(len), sizeof(header) - 1);
            _onOutput(header, outLen);
        }

        for (size_t i = 0; i < _commandCount; ++i) {
            writef("  %-18s %s\r\n", _commands[i].name, _commands[i].help != nullptr ? _commands[i].help : "");
        }
    }
}
