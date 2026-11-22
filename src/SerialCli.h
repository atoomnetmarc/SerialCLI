/*
 *
 * Copyright Marc Ketel
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#pragma once

#include <array>
#include <cstddef>
#include <functional>

/*
 * Compile-time budget. Each project overrides these through build_flags so the
 * command table and the input buffer fit the target's RAM.
 */
#ifndef CLI_MAX_COMMANDS
#define CLI_MAX_COMMANDS 32
#endif

#ifndef CLI_INPUT_BUFFER_SIZE
#define CLI_INPUT_BUFFER_SIZE 256
#endif

namespace cli {
    struct CommandDef {
        const char *name;
        std::function<void(int argc, char **argv)> handler;
        const char *help;
    };

    using OutputCallback = std::function<void(const char *data, size_t len)>;

    /*
     * Push-driven command line. The owner feeds received characters with
     * feed() and supplies an output callback; the class never touches a UART
     * itself and never blocks. Command names match without regard to ASCII
     * case.
     */
    class SerialCli {
    public:
        static constexpr size_t MAX_COMMANDS = CLI_MAX_COMMANDS;
        static constexpr size_t INPUT_BUFFER_SIZE = CLI_INPUT_BUFFER_SIZE;

        /* Construct the parser with an output callback and optional help title. */
        explicit SerialCli(OutputCallback onOutput, const char *helpTitle = "Serial commands");

        /* Register a command, replacing an existing name; false on invalid input or a full table. */
        bool registerCommand(const CommandDef &cmd);

        /* Feed one received character into the parser. */
        void feed(char c);

        /* Write a NUL-terminated string through the output callback. */
        void write(const char *text);

        /* Write a printf-formatted string, truncated to the internal line buffer. */
        void writef(const char *format, ...) __attribute__((format(printf, 2, 3)));

        /* Return the number of registered commands. */
        size_t commandCount(void) const;

        /* Return the command at index, or nullptr when out of range. */
        const CommandDef *commandAt(size_t index) const;

    private:
        OutputCallback _onOutput;
        const char *_helpTitle;
        std::array<CommandDef, MAX_COMMANDS> _commands{};
        size_t _commandCount = 0;
        std::array<char, INPUT_BUFFER_SIZE> _buffer{};
        size_t _pos = 0;

        /* Dispatch a complete, mutable command line. */
        void dispatch(char *line);

        /* Print the help header and one line per registered command. */
        void cmdHelp(int argc, char **argv);
    };
}
