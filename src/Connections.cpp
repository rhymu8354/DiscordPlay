/**
 * @file Connections.cpp
 *
 * This module contains the implementations of the Connections class.
 *
 * © 2020 by Richard Walters
 */

#include "Connections.hpp"

/**
 * This contains the private properties of a Connections class instance.
 */
struct Connections::Impl {
    std::shared_ptr< Http::Client > http;
};

Connections::~Connections() noexcept = default;

Connections::Connections()
    : impl_(new Impl())
{
}

void Connections::Configure(const std::shared_ptr< Http::Client >& client) {
    impl_->http = client;
}

auto Connections::QueueResourceRequest(
    const ResourceRequest& request
) -> std::future< Response > {
    std::promise< Response > responsePromise;
    responsePromise.set_value({404});
    return responsePromise.get_future();
}

std::future< std::shared_ptr< Discord::WebSocket > > Connections::QueueWebSocketRequest(
    const WebSocketRequest& request
) {
    std::promise< std::shared_ptr< Discord::WebSocket > > webSocketPromise;
    webSocketPromise.set_value(nullptr);
    return webSocketPromise.get_future();
}
