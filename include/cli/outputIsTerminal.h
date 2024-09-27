#pragma once
/// \file Contains the \p outputIsTerminal function.

/// Whether the output (stdout/stderr) is going to a terminal or something else.
/// Allows you to detect output being piped to a file.
bool outputIsTerminal();
