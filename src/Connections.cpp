/**
 * @file Connections.cpp
 *
 * This module contains the implementations of the Connections class.
 *
 * © 2020 by Richard Walters
 */

#include "Connections.hpp"
#include "ConnectWebSocket.hpp"
#include "Diagnostics.hpp"
#include "WebSocket.hpp"

#include <Http/IClient.hpp>
#include <memory>
#include <mutex>
#include <stddef.h>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <unordered_map>
#include <Uri/Uri.hpp>

/**
 * This contains the private properties of a Connections class instance.
 */
struct Connections::Impl {
    // Properties

    std::shared_ptr< Http::IClient > httpClient;
    std::unordered_map< int, std::shared_ptr< Http::IClient::Transaction > > httpClientTransactions;
    SystemAbstractions::DiagnosticsSender diagnosticsSender;
    std::mutex mutex;
    int nextHttpClientTransactionId = 1;

    // Methods

    Impl()
        : diagnosticsSender("Connections")
    {
    }
};

Connections::~Connections() noexcept = default;

Connections::Connections()
    : impl_(new Impl())
{
}

void Connections::Configure(const std::shared_ptr< Http::IClient >& client) {
    impl_->httpClient = client;
    impl_->httpClient->SubscribeToDiagnostics(
        impl_->diagnosticsSender.Chain(),
        DIAG_LEVEL_HTTP_CLIENT
    );
}

SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate Connections::SubscribeToDiagnostics(
    SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
    size_t minLevel
) {
    return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
}

auto Connections::QueueResourceRequest(
    const ResourceRequest& request
) -> ResourceRequestTransaction {
    std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
    impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
        1,
        "%s request for %s",
        request.method.c_str(),
        request.uri.c_str()
    );
    ResourceRequestTransaction transaction;
    const auto responsePromise = std::make_shared< std::promise< Response > >();
    Http::Request httpRequest;
    const auto id = impl_->nextHttpClientTransactionId++;
    httpRequest.method = request.method;
    httpRequest.target.ParseFromString(request.uri);
    if (
        (httpRequest.target.GetScheme() == "https")
        || (httpRequest.target.GetScheme() == "wss")
    ) {
        httpRequest.target.SetPort(443);
    }
    for (const auto& header: request.headers) {
        httpRequest.headers.SetHeader(header.key, header.value);
    }
    httpRequest.body = request.body;
    auto& httpClientTransaction = impl_->httpClientTransactions[id];
    httpClientTransaction = impl_->httpClient->Request(httpRequest);
    std::weak_ptr< Impl > implWeak(impl_);
    httpClientTransaction->SetCompletionDelegate(
        [
            id,
            implWeak,
            responsePromise
        ]{
            auto impl = implWeak.lock();
            if (impl == nullptr) {
                return;
            }
            std::unique_lock< decltype(impl->mutex) > lock(impl->mutex);
            auto httpClientTransactionsEntry = impl->httpClientTransactions.find(id);
            if (httpClientTransactionsEntry == impl->httpClientTransactions.end()) {
                return;
            }
            const auto httpClientTransaction = httpClientTransactionsEntry->second;
            (void)impl->httpClientTransactions.erase(httpClientTransactionsEntry);
            lock.unlock();
            impl->diagnosticsSender.SendDiagnosticInformationFormatted(
                1,
                "Response: %u %s",
                httpClientTransaction->response.statusCode,
                httpClientTransaction->response.reasonPhrase.c_str()
            );
            impl->diagnosticsSender.SendDiagnosticInformationString(
                0,
                "Headers: ---------------"
            );
            for (const auto& header: httpClientTransaction->response.headers.GetAll()) {
                impl->diagnosticsSender.SendDiagnosticInformationFormatted(
                    0,
                    "%s: %s",
                    ((std::string)header.name).c_str(),
                    header.value.c_str()
                );
            }
            impl->diagnosticsSender.SendDiagnosticInformationString(
                0,
                "Body: ------------------------"
            );
            if (!httpClientTransaction->response.body.empty()) {
                impl->diagnosticsSender.SendDiagnosticInformationString(
                    0,
                    httpClientTransaction->response.body
                );
            }
            impl->diagnosticsSender.SendDiagnosticInformationString(
                0,
                "------------------------"
            );
            Response response;
            response.status = httpClientTransaction->response.statusCode;
            response.body = httpClientTransaction->response.body;
            for (const auto& header: httpClientTransaction->response.headers.GetAll()) {
                response.headers.push_back({
                    header.name,
                    header.value
                });
            }
            responsePromise->set_value(std::move(response));
        }
    );
    transaction.response = responsePromise->get_future();
    transaction.cancel =  [
        id,
        implWeak,
        responsePromise
    ]{
        auto impl = implWeak.lock();
        if (impl == nullptr) {
            return;
        }
        std::unique_lock< decltype(impl->mutex) > lock(impl->mutex);
        auto httpClientTransactionsEntry = impl->httpClientTransactions.find(id);
        if (httpClientTransactionsEntry == impl->httpClientTransactions.end()) {
            return;
        }
        const auto httpClientTransaction = httpClientTransactionsEntry->second;
        (void)impl->httpClientTransactions.erase(httpClientTransactionsEntry);
        lock.unlock();
        responsePromise->set_value({499});
    };
    return transaction;
}

auto Connections::QueueWebSocketRequest(
    const WebSocketRequest& request
) -> WebSocketRequestTransaction {
    // Log that we are about to make a WebSocket connection attempt.
    std::unique_lock< decltype(impl_->mutex) > lock(impl_->mutex);
    impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
        1,
        "WebSocket request for %s",
        request.uri.c_str()
    );

    // Set up a transaction object to be returned, with a promise
    // whose future is made available in the transaction.
    WebSocketRequestTransaction transaction;
    std::promise< std::shared_ptr< Discord::WebSocket > > webSocketPromise;
    transaction.webSocket = webSocketPromise.get_future();

    // Set up a means of canceling the transaction.
    bool cancelWebSocketConnection = false;
    std::weak_ptr< Impl > implWeak(impl_);
    transaction.cancel =  [
        &cancelWebSocketConnection,
        implWeak
    ]{
        auto impl = implWeak.lock();
        if (impl == nullptr) {
            return;
        }
        std::unique_lock< decltype(impl->mutex) > lock(impl->mutex);
        cancelWebSocketConnection = true;
    };

    // Instantiate and use the WebSocket implementation class
    // to begin connecting to the server.
    const auto webSocketDiagnosticsSender = std::make_shared< SystemAbstractions::DiagnosticsSender >("WebSocket");
    webSocketDiagnosticsSender->SubscribeToDiagnostics(impl_->diagnosticsSender.Chain());
    auto webSocketConnectionResults = ConnectWebSocket(
        impl_->httpClient,
        request.uri,
        webSocketDiagnosticsSender
    );

    // Wait until either the transaction is canceled or the WebSocket
    // connection attempt completes (either success or failure).
    while (
        !cancelWebSocketConnection
        && (
            webSocketConnectionResults.connectionFuture.wait_for(
                std::chrono::milliseconds(100)
            ) != std::future_status::ready
        )
    ) {
    }

    // If transaction was canceled, set the promise with a null pointer.
    // Otherwise, provide the wrapped WebSocket.
    if (cancelWebSocketConnection) {
        webSocketPromise.set_value(nullptr);
    } else {
        auto webSocket = webSocketConnectionResults.connectionFuture.get();
        if (webSocket == nullptr) {
            impl_->diagnosticsSender.SendDiagnosticInformationString(
                3,
                "WebSocket connection failed"
            );
            webSocketPromise.set_value(nullptr);
        } else {
            impl_->diagnosticsSender.SendDiagnosticInformationString(
                1,
                "WebSocket connected"
            );
            const auto webSocketWrapper = std::make_shared< WebSocket >();
            webSocketWrapper->SubscribeToDiagnostics(
                impl_->diagnosticsSender.Chain(),
                DIAG_LEVEL_WEB_SOCKET_WRAPPER
            );
            webSocketWrapper->Configure(std::move(webSocket));
            webSocketPromise.set_value(std::move(webSocketWrapper));
        }
    }
    return transaction;
}
