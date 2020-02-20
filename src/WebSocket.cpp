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
#include <vector>

/**
 * This contains the private properties of a WebSocket class instance.
 */
struct WebSocket::Impl {
    // Properties

    std::shared_ptr< WebSockets::WebSocket > adaptee;
    SystemAbstractions::DiagnosticsSender diagnosticsSender;
    std::recursive_mutex mutex;
    ReceiveCallback onText;
    std::vector< std::string > storedData;

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
        decltype(onText) onTextSample(onText);
        lock.unlock();
        if (onTextSample == nullptr) {
            lock.lock();
            storedData.push_back(std::move(data));
        } else {
            onTextSample(std::move(data));
            lock.lock();
        }
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
    std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
    return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
}

void WebSocket::Configure(std::shared_ptr< WebSockets::WebSocket >&& adaptee) {
    std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
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
    std::unique_lock< decltype(impl_->mutex) > lock(impl_->mutex);
    impl_->onText = std::move(onText);
    if (
        !impl_->storedData.empty()
        && (impl_->onText != nullptr)
    ) {
        decltype(impl_->storedData) storedData;
        storedData.swap(impl_->storedData);
        decltype(impl_->onText) onTextSample(impl_->onText);
        lock.unlock();
        for (auto& message: storedData) {
            onTextSample(std::move(message));
        }
    }
}
