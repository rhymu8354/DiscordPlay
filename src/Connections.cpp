/**
 * @file Connections.cpp
 *
 * This module contains the implementations of the Connections class.
 *
 * © 2020 by Richard Walters
 */

#include "Connections.hpp"

#include <Http/IClient.hpp>
#include <memory>
#include <mutex>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <unordered_map>

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
        0,
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
                0,
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
    impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
        3,
        "WebSocket request for %s",
        request.uri.c_str()
    );
    WebSocketRequestTransaction transaction;
    std::promise< std::shared_ptr< Discord::WebSocket > > webSocketPromise;
    webSocketPromise.set_value(nullptr);
    transaction.webSocket = webSocketPromise.get_future();
    return transaction;
}
