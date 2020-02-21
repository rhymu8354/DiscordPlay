#pragma once

/**
 * @file WebSocket.hpp
 *
 * This module declares the WebSocket implementation.
 *
 * © 2020 by Richard Walters
 */

#include <Discord/WebSocket.hpp>
#include <memory>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <WebSockets/WebSocket.hpp>

/**
 * This is the implementation of Discord::WebSocket used
 * by the application.
 */
class WebSocket
    : public Discord::WebSocket
{
    // Lifecycle Methods
public:
    ~WebSocket() noexcept;
    WebSocket(const WebSocket&) = delete;
    WebSocket(WebSocket&&) noexcept = delete;
    WebSocket& operator=(const WebSocket&) = delete;
    WebSocket& operator=(WebSocket&&) noexcept = delete;

    // Public Methods
public:
    /**
     * This is the constructor of the class.
     */
    WebSocket();

    SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
        SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
        size_t minLevel = 0
    );

    void Configure(std::shared_ptr< WebSockets::WebSocket >&& adaptee);

    // Discord::WebSocket
public:
    virtual void Binary(std::string&& message) override;
    virtual void Close(unsigned int code) override;
    virtual void Text(std::string&& message) override;
    virtual void RegisterBinaryCallback(ReceiveCallback&& onBinary) override;
    virtual void RegisterCloseCallback(CloseCallback&& onClose) override;
    virtual void RegisterTextCallback(ReceiveCallback&& onText) override;

    // Private properties
private:
    /**
     * This is the type of structure that contains the private
     * properties of the instance.  It is defined in the implementation
     * and declared here to ensure that it is scoped inside the class.
     */
    struct Impl;

    /**
     * This contains the private properties of the instance.
     */
    std::shared_ptr< Impl > impl_;
};
