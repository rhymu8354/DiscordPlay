/**
 * @file WebSocket.cpp
 *
 * This module contains the implementations of the WebSocket class.
 *
 * © 2020 by Richard Walters
 */

#include "Diagnostics.hpp"
#include "WebSocket.hpp"

#include <mutex>
#include <stddef.h>

/**
 * This contains the private properties of a WebSocket class instance.
 */
struct WebSocket::Impl {
    // Properties

    std::shared_ptr< WebSockets::WebSocket > adaptee;
    SystemAbstractions::DiagnosticsSender diagnosticsSender;
    std::mutex mutex;

    // Methods

    Impl()
        : diagnosticsSender("WebSocketAdapter")
    {
    }

    void OnBinary(
        std::string&& data,
        std::unique_lock< decltype(mutex) >& lock
    ) {
    }

    void OnClose(
        std::unique_lock< decltype(mutex) >& lock
    ) {
    }

    void OnPing(
        std::string&& data,
        std::unique_lock< decltype(mutex) >& lock
    ) {
    }

    void OnPong(
        std::string&& data,
        std::unique_lock< decltype(mutex) >& lock
    ) {
    }

    void OnText(
        std::string&& data,
        std::unique_lock< decltype(mutex) >& lock
    ) {
        diagnosticsSender.SendDiagnosticInformationFormatted(
            3,
            "Received Text Message: %s",
            data.c_str()
        );
    }
};

WebSocket::~WebSocket() noexcept = default;

WebSocket::WebSocket()
    : impl_(new Impl())
{
}

SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate WebSocket::SubscribeToDiagnostics(
    SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
    size_t minLevel
) {
    return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
}

void WebSocket::Configure(std::shared_ptr< WebSockets::WebSocket >&& adaptee) {
    impl_->adaptee = std::move(adaptee);
    impl_->adaptee->SubscribeToDiagnostics(
        impl_->diagnosticsSender.Chain(),
        DIAG_LEVEL_WEB_SOCKET
    );
    std::weak_ptr< Impl > weakImpl(impl_);
    impl_->adaptee->SetDelegates({
        [weakImpl](std::string&& data){  // ping
            auto impl = weakImpl.lock();
            if (impl == nullptr) {
                return;
            }
            std::unique_lock< decltype(impl->mutex) > lock(impl->mutex);
            impl->OnPing(std::move(data), lock);
        },
        [weakImpl](std::string&& data){  // pong
            auto impl = weakImpl.lock();
            if (impl == nullptr) {
                return;
            }
            std::unique_lock< decltype(impl->mutex) > lock(impl->mutex);
            impl->OnPong(std::move(data), lock);
        },
        [weakImpl](std::string&& data){  // text
            auto impl = weakImpl.lock();
            if (impl == nullptr) {
                return;
            }
            std::unique_lock< decltype(impl->mutex) > lock(impl->mutex);
            impl->OnText(std::move(data), lock);
        },
        [weakImpl](std::string&& data){  // binary
            auto impl = weakImpl.lock();
            if (impl == nullptr) {
                return;
            }
            std::unique_lock< decltype(impl->mutex) > lock(impl->mutex);
            impl->OnBinary(std::move(data), lock);
        },
        [weakImpl](                      // close
            unsigned int code,
            std::string&& reason
        ){  // text
            auto impl = weakImpl.lock();
            if (impl == nullptr) {
                return;
            }
            std::unique_lock< decltype(impl->mutex) > lock(impl->mutex);
            impl->OnClose(lock);
        }
    });
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
