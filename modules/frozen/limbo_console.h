#ifndef LIMBO_CONSOLE_H
#define LIMBO_CONSOLE_H

#include "core/variant/callable.h"
#include <core/config/engine.h>
#include <core/object/object.h>

#define LIMBO_CONSOLE_AUTOLOAD_NAME "LimboConsole"

#define ENGINE Engine::get_singleton()
#define LIMBO LimboConsole::get_singleton()

class LimboConsole {
private:
	static Object *get_singleton() {
		if (ENGINE->has_singleton(LIMBO_CONSOLE_AUTOLOAD_NAME)) {
			return ENGINE->get_singleton_object(LIMBO_CONSOLE_AUTOLOAD_NAME);
		}
		return nullptr;
	}
public:
	/// Registers a callable as a command with optional name and description.
	/// Name can have up to 4 space-separated identifiers (e.g., "command sub1 sub2 sub3").
	static void register_command(const Callable &p_func, const String &p_name = String(), const String &p_description = String()) {
		if (LIMBO) {
			LIMBO->call("register_command", p_func, p_name, p_description);
		}
	}
	/// Unregisters the command specified by its name.
	static void unregister_command(const String &p_name) {
		if (LIMBO) {
			LIMBO->call("unregister_command", p_name);
		}
	}
	/// Returns true if a command or alias is registered by the given name.
	static bool has_command(const String &p_name) {
		if (LIMBO) {
			return LIMBO->call("has_command", p_name);
		}
		return false;
	}
	/// Parses the command line and executes the command if it's valid.
	/// If p_silent is true, the command output will not be printed to console.
	static void execute_command(const String &p_command_line, bool p_silent = false) {
		if (LIMBO) {
			LIMBO->call("execute_command", p_command_line, p_silent);
		}
	}
	/// Executes commands from a file specified by file path.
	/// If p_silent is true, the commands will be executed without printing output.
	static void execute_script(const String &p_script, bool p_silent = false) {
		if (LIMBO) {
			LIMBO->call("execute_script", p_script, p_silent);
		}
	}
	/// Prints an info message to the console.
	static void info(const String &p_line) {
		if (LIMBO) {
			LIMBO->call("print_info", p_line);
		}
	}
	/// Prints an error message to the console.
	static void error(const String &p_line) {
		if (LIMBO) {
			LIMBO->call("print_error", p_line);
		}
	}
	/// Prints a warning message to the console.
	static void warn(const String &p_line) {
		if (LIMBO) {
			LIMBO->call("print_warning", p_line);
		}
	}
	/// Prints a debug message to the console.
	static void debug(const String &p_line) {
		if (LIMBO) {
			LIMBO->call("print_debug", p_line);
		}
	}
	/// Prints a line using boxed ASCII art style.
	static void print_boxed(const String &p_line) {
		if (LIMBO) {
			LIMBO->call("print_boxed", p_line);
		}
	}
	/// Prints a line to the console, and optionally to standard output.
	static void print_line(const String &p_line, bool p_print_to_stdout = false) {
		if (LIMBO) {
			LIMBO->call("print_line", p_line, p_print_to_stdout);
		}
	}
	/// Clears all messages in the console.
	static void clear_console() {
		if (LIMBO) {
			LIMBO->call("clear_console");
		}
	}
	/// Opens the console if enabled.
	static void open_console() {
		if (LIMBO) {
			LIMBO->call("open_console");
		}
	}
	/// Closes the console.
	static void close_console() {
		if (LIMBO) {
			LIMBO->call("close_console");
		}
	}
	/// Returns true if the console is currently open.
	static bool is_open() {
		if (LIMBO) {
			return LIMBO->call("is_open");
		}
		return false;
	}
	/// Toggles the console visibility between open and closed states.
	static void toggle_console() {
		if (LIMBO) {
			LIMBO->call("toggle_console");
		}
	}
};

#endif //LIMBO_CONSOLE_H