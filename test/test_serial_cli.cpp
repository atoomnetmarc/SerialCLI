/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "SerialCli.h"

#include <cassert>
#include <cstdio>
#include <string>

namespace {
    struct Fixture {
        std::string output;
        cli::SerialCli cli;

        explicit Fixture(const char *title = nullptr)
            : cli([this](const char *data, size_t len) { output.append(data, len); }, title) {}

        void feed(const char *text) {
            while (*text != '\0') {
                cli.feed(*text++);
            }
        }
    };

    /* Verify that CR, LF, and CRLF each dispatch once. */
    void testCrLf() {
        Fixture f;
        int calls = 0;
        assert(f.cli.registerCommand({"state", [&](int, char **) { ++calls; f.cli.write("OK\r\n"); }, "Show state"}));

        f.feed("state\r\n");
        assert(calls == 1);
        assert(f.output == "state\r\nOK\r\n");

        f.output.clear();
        f.feed("state\n");
        assert(calls == 2);

        f.output.clear();
        f.feed("state\r");
        assert(calls == 3);
    }

    /* Verify that both backspace codes remove the last buffered character. */
    void testBackspace() {
        Fixture f;
        int calls = 0;
        assert(f.cli.registerCommand({"state", [&](int, char **) { ++calls; }, "Show state"}));

        f.feed("state");
        f.cli.feed('\b');
        f.feed("e\r\n");
        assert(calls == 1);
        assert(f.output == "state\b \be\r\n");

        f.output.clear();
        f.feed("state");
        f.cli.feed(0x7F);
        f.feed("e\r\n");
        assert(calls == 2);
        assert(f.output == "state\b \be\r\n");
    }

    /* Verify that input beyond the buffer is dropped without overflow. */
    void testBufferOverflow() {
        Fixture f;
        std::string input(cli::SerialCli::INPUT_BUFFER_SIZE + 16, 'a');
        f.feed(input.c_str());
        f.cli.feed('\r');

        std::string expected(cli::SerialCli::INPUT_BUFFER_SIZE - 1, 'a');
        assert(f.output.find(expected) != std::string::npos);
        assert(f.output.find(expected + "a") == std::string::npos);
    }

    /* Verify that a repeated name replaces the earlier registration. */
    void testDuplicateRegistration() {
        Fixture f;
        int first = 0;
        int second = 0;
        assert(f.cli.registerCommand({"state", [&](int, char **) { ++first; }, "first"}));
        assert(f.cli.registerCommand({"STATE", [&](int, char **) { ++second; }, "second"}));
        assert(f.cli.commandCount() == 1);
        assert(f.cli.commandAt(0) != nullptr);
        assert(std::string(f.cli.commandAt(0)->help) == "second");
        assert(f.cli.commandAt(1) == nullptr);

        f.feed("state\r\n");
        assert(first == 0);
        assert(second == 1);
    }

    /* Verify that invalid command definitions are rejected. */
    void testEmptyName() {
        Fixture f;
        auto handler = [](int, char **) {};
        assert(!f.cli.registerCommand({"", handler, "empty"}));
        assert(!f.cli.registerCommand({nullptr, handler, "null"}));
        assert(!f.cli.registerCommand({"valid", {}, "no handler"}));
        assert(f.cli.commandCount() == 0);
    }

    /* Verify the custom and default help headers and the command lines. */
    void testHelpOutput() {
        Fixture f("Test Controller");
        assert(f.cli.registerCommand({"state", [](int, char **) {}, "Show state"}));
        f.feed("HeLp\r\n");

        char line[128];
        int len = snprintf(line, sizeof(line), "  %-18s %s\r\n", "state", "Show state");
        assert(len > 0);
        assert(f.output == std::string("HeLp\r\n=== Test Controller ===\r\n") + line);

        Fixture plain;
        plain.feed("help\r\n");
        assert(plain.output.find("=== Serial commands ===\r\n") != std::string::npos);
    }

    /* Verify the response to an unregistered command. */
    void testUnknownCommand() {
        Fixture f;
        f.feed("bogus\r\n");
        assert(f.output == "bogus\r\nUnknown command: 'bogus'. Try 'help'.\r\n");
    }

    /* Verify that writef truncates to the internal line buffer. */
    void testWritefTruncation() {
        Fixture f;
        std::string longText(300, 'x');
        f.cli.writef("%s", longText.c_str());
        assert(f.output.size() == 159);
        assert(f.output == longText.substr(0, 159));

        f.output.clear();
        f.cli.writef("%d", 42);
        assert(f.output == "42");
    }

    /* Verify that command names and the built-in help match any ASCII case. */
    void testCaseInsensitive() {
        Fixture f;
        int calls = 0;
        assert(f.cli.registerCommand({"State", [&](int, char **) { ++calls; }, "Show state"}));
        f.feed("STATE\r\n");
        f.feed("state\r\n");
        f.feed("StAtE\r\n");
        assert(calls == 3);
    }
}

int main() {
    testCrLf();
    testBackspace();
    testBufferOverflow();
    testDuplicateRegistration();
    testEmptyName();
    testHelpOutput();
    testUnknownCommand();
    testWritefTruncation();
    testCaseInsensitive();
    return 0;
}
