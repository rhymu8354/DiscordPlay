#pragma once

/**
 * @file Connections.hpp
 *
 * This module declares the Connections implementation.
 *
 * © 2020 by Richard Walters
 */

#include <Discord/Connections.hpp>
#include <Http/IClient.hpp>
#include <memory>
#include <SystemAbstractions/DiagnosticsSender.hpp>

/**
 * This is the implementation of Discord::Connections used
 * by the application.
 */
class Connections
    : public Discord::Connections
{
    // Lifecycle Methods
public:
    ~Connections() noexcept;
    Connections(const Connections&) = delete;
    Connections(Connections&&) noexcept = delete;
    Connections& operator=(const Connections&) = delete;
    Connections& operator=(Connections&&) noexcept = delete;

    // Public Methods
public:
    /**
     * This is the constructor of the class.
     */
    Connections();

    void Configure(const std::shared_ptr< Http::IClient >& client);

    SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
        SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
        size_t minLevel = 0
    );

    // Discord::Connections
public:
    virtual ResourceRequestTransaction QueueResourceRequest(
        const ResourceRequest& request
    ) override;

    virtual WebSocketRequestTransaction QueueWebSocketRequest(
        const WebSocketRequest& request
    ) override;

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
