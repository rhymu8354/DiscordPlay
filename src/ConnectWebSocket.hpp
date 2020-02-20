#pragma once

/**
 * @file ConnectWebSocket.hpp
 *
 * This module declares the ConnectWebSocket function.
 *
 * © 2018, 2020 by Richard Walters
 */

#include <functional>
#include <future>
#include <Http/IClient.hpp>
#include <memory>
#include <stdint.h>
#include <string>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <WebSockets/WebSocket.hpp>

/**
 * This is used to return values from the MakeConnection function.
 */
struct MakeConnectionResults {
    /**
     * This is a mechanism to access the result of the connection attempt.
     * If the connection is successful, it will yield a WebSocket object
     * reference.  Otherwise, it will yield nullptr, indicating that the
     * connection could not be made.
     */
    std::future< std::shared_ptr< WebSockets::WebSocket > > connectionFuture;

    /**
     * This is a function which can be called to abort the connection
     * attempt early.
     */
    std::function< void() > abortConnection;
};

/**
 * This method is called to asynchronously attempt to connect to a web
 * server and upgrade the connection a WebSocket.
 *
 * @param[in] http
 *     This is the web client object to use to make the connection.
 *
 * @param[in] uri
 *     This is the URI of the WebSocket server to which to connect.
 *
 * @param[in] diagnosticsSender
 *     This is the object to use to publish any diagnostic messages.
 *
 * @param[in] configuration
 *     These are the configurable parameters to set for the WebSocket.
 *
 * @return
 *     A structure is returned containing information and tools to
 *     use in coordinating with the asynchronous connection operation.
 */
MakeConnectionResults ConnectWebSocket(
    std::shared_ptr< Http::IClient > http,
    const std::string& uri,
    std::shared_ptr< SystemAbstractions::DiagnosticsSender > diagnosticsSender,
    WebSockets::WebSocket::Configuration configuration = WebSockets::WebSocket::Configuration()
);
