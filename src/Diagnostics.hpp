#pragma once

/**
 * @file Diagnostics.hpp
 *
 * This module declares all the diagnostic level thresholds for
 * various components in this application
 *
 * © 2020 by Richard Walters
 */

#include <stddef.h>

constexpr size_t DIAG_LEVEL_CONNECTIONS_INTERFACE = 1;
constexpr size_t DIAG_LEVEL_HTTP_CLIENT = 0;
constexpr size_t DIAG_LEVEL_TLS_DECORATOR = 2;
constexpr size_t DIAG_LEVEL_NETWORK_CONNECTION = 1;
constexpr size_t DIAG_LEVEL_NETWORK_TRANSPORT = 0;
constexpr size_t DIAG_LEVEL_WEB_SOCKET = 0;
constexpr size_t DIAG_LEVEL_WEB_SOCKET_WRAPPER = 0;
