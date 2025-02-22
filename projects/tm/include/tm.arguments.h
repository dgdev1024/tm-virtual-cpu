/// @file   tm.arguments.h
/// @brief  Provides functions for parsing and accessing command-line arguments
///
/// This file provides functions for parsing and accessing command-line arguments
/// using a UNIX-style parsing convention:
///
/// `-s` or `--long` for flags.
/// `-s value` or `--long value` for key-value pairs.
/// `-s value1 value2 ...` or `--long value1 value2 ...` for multi-value key-value pairs.
/// `--` to separate options from positional arguments.

#pragma once
#include <tm.common.h>

void tm_capture_arguments (int p_argc, char** p_argv);
void tm_release_arguments ();
bool tm_has_argument (const char* p_longform, const char p_shortform);
const char* tm_get_argument_value (const char* p_longform, const char p_shortform);
const char* tm_get_argument_value_at (const char* p_longform, const char p_shortform, size_t p_index);
