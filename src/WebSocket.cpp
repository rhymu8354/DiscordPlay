/**
 * @file WebSocket.cpp
 *
 * This module contains the implementations of the WebSocket class.
 *
 * © 2020 by Richard Walters
 */

#include "WebSocket.hpp"

/**
 * This contains the private properties of a WebSocket class instance.
 */
struct WebSocket::Impl {
};

WebSocket::~WebSocket() noexcept = default;

WebSocket::WebSocket()
    : impl_(new Impl())
{
}

void WebSocket::Binary(std::string&& message) {
}

void WebSocket::Close() {
}

void WebSocket::Text(std::string&& message) {
}

void WebSocket::RegisterBinaryCallback(ReceiveCallback&& onBinary) {
}

void WebSocket::RegisterCloseCallback(CloseCallback&& onClose) {
}

void WebSocket::RegisterTextCallback(ReceiveCallback&& onText) {
}
