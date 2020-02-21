/**
 * @file main.cpp
 *
 * This module holds the main() function, which is the entrypoint
 * to the program.
 *
 * © 2020 by Richard Walters
 */

#include "Connections.hpp"
#include "Diagnostics.hpp"
#include "TimeKeeper.hpp"

#include <Discord/Gateway.hpp>
#include <Http/Client.hpp>
#include <Http/Request.hpp>
#include <HttpNetworkTransport/HttpClientNetworkTransport.hpp>
#include <memory>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DiagnosticsStreamReporter.hpp>
#include <SystemAbstractions/File.hpp>
#include <SystemAbstractions/NetworkConnection.hpp>
#include <thread>
#include <Timekeeping/Scheduler.hpp>
#include <TlsDecorator/TlsDecorator.hpp>
#include <WebSockets/WebSocket.hpp>

namespace {

    /**
     * This function prints to the standard error stream information
     * about how to use this program.
     */
    void PrintUsageInformation() {
        fprintf(
            stderr,
            (
                "Usage: DiscordPlay\n"
                "\n"
                "Perform Discord experiment.\n"
            )
        );
    }

    /**
     * This flag indicates whether or not the web client should shut down.
     */
    bool shutDown = false;

    /**
     * This contains variables set through the operating system environment
     * or the command-line arguments.
     */
    struct Environment {
        Discord::Gateway::Configuration configuration;
    };

    /**
     * This function is set up to be called when the SIGINT signal is
     * received by the program.  It just sets the "shutDown" flag
     * and relies on the program to be polling the flag to detect
     * when it's been set.
     *
     * @param[in] sig
     *     This is the signal for which this function was called.
     */
    void InterruptHandler(int) {
        shutDown = true;
    }

    /**
     * This function updates the program environment to incorporate
     * any applicable command-line arguments.
     *
     * @param[in] argc
     *     This is the number of command-line arguments given to the program.
     *
     * @param[in] argv
     *     This is the array of command-line arguments given to the program.
     *
     * @param[in,out] environment
     *     This is the environment to update.
     *
     * @param[in] diagnosticsSender
     *     This is the object to use to publish any diagnostic messages.
     *
     * @return
     *     An indication of whether or not the function succeeded is returned.
     */
    bool ProcessCommandLineArguments(
        int argc,
        char* argv[],
        Environment& environment,
        const SystemAbstractions::DiagnosticsSender& diagnosticsSender
    ) {
        size_t state = 0;
        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i]);
            switch (state) {
                case 0: { // next argument
                    state = 0;
                } break;

                default: break;
            }
        }
        return true;
    }

    /**
     * This function loads the trusted certificate authority (CA) certificate
     * bundle from the file system, where it's expected to be sitting
     * side-by-side the program's image, with the name "cert.pem".
     *
     * @param[out] caCerts
     *     This is where to store the loaded CA certificate bundle.
     *
     * @param[in] diagnosticsSender
     *     This is the object to use to publish any diagnostic messages.
     *
     * @return
     *     An indication of whether or not the function succeeded is returned.
     */
    bool LoadCaCerts(
        std::string& caCerts,
        const SystemAbstractions::DiagnosticsSender& diagnosticsSender
    ) {
        SystemAbstractions::File caCertsFile(
            SystemAbstractions::File::GetExeParentDirectory()
            + "/cert.pem"
        );
        if (!caCertsFile.OpenReadOnly()) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "unable to open root CA certificates file '%s'",
                caCertsFile.GetPath().c_str()
            );
            return false;
        }
        std::vector< uint8_t > caCertsBuffer(caCertsFile.GetSize());
        if (caCertsFile.Read(caCertsBuffer) != caCertsBuffer.size()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "unable to read root CA certificates file"
            );
            return false;
        }
        caCerts.assign(
            (const char*)caCertsBuffer.data(),
            caCertsBuffer.size()
        );
        return true;
    }

    /**
     * This function starts the client with the given transport layer.
     *
     * @param[in,out] client
     *     This is the client to start.
     *
     * @param[in] environment
     *     This contains variables set through the operating system
     *     environment or the command-line arguments.
     *
     * @param[in] caCerts
     *     This is the trusted certificate authority (CA) certificate bundle to
     *     use to verify certificates at the TLS layer.
     *
     * @param[in] diagnosticsSender
     *     This is the object to use to publish any diagnostic messages.
     *
     * @return
     *     An indication of whether or not the function succeeded is returned.
     */
    bool StartClient(
        Http::Client& client,
        const std::shared_ptr< TimeKeeper >& timeKeeper,
        const Environment& environment,
        const std::string& caCerts,
        const SystemAbstractions::DiagnosticsSender& diagnosticsSender
    ) {
        auto transport = std::make_shared< HttpNetworkTransport::HttpClientNetworkTransport >();
        auto diagnosticMessageDelegate = diagnosticsSender.Chain();
        transport->SubscribeToDiagnostics(
            diagnosticMessageDelegate,
            DIAG_LEVEL_NETWORK_TRANSPORT
        );
        Http::Client::MobilizationDependencies deps;
        transport->SetConnectionFactory(
            [
                diagnosticMessageDelegate,
                caCerts
            ](
                const std::string& scheme,
                const std::string& serverName
            ) -> std::shared_ptr< SystemAbstractions::INetworkConnection > {
                const auto connection = std::make_shared< SystemAbstractions::NetworkConnection >();
                connection->SubscribeToDiagnostics(
                    diagnosticMessageDelegate,
                    DIAG_LEVEL_NETWORK_CONNECTION
                );
                if (
                    (scheme == "https")
                    || (scheme == "wss")
                ) {
                    const auto tlsDecorator = std::make_shared< TlsDecorator::TlsDecorator >();
                    tlsDecorator->ConfigureAsClient(connection, caCerts, serverName);
                    tlsDecorator->SubscribeToDiagnostics(
                        diagnosticMessageDelegate,
                        DIAG_LEVEL_TLS_DECORATOR
                    );
                    return tlsDecorator;
                } else {
                    return connection;
                }
            }
        );
        deps.transport = transport;
        deps.timeKeeper = timeKeeper;
        client.Mobilize(deps);
        return true;
    }

    /**
     * This function stops the client.
     *
     * @param[in,out] client
     *     This is the client to stop.
     */
    void StopClient(Http::Client& client) {
        client.Demobilize();
    }

}

/**
 * This function is the entrypoint of the program.
 *
 * The program is terminated after the SIGINT signal is caught.
 *
 * @param[in] argc
 *     This is the number of command-line arguments given to the program.
 *
 * @param[in] argv
 *     This is the array of command-line arguments given to the program.
 */
int main(int argc, char* argv[]) {
#ifdef _WIN32
    //_crtBreakAlloc = 18;
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif /* _WIN32 */
    // Set up a handler for SIGINT to set our "shutDown" flag.
    const auto previousInterruptHandler = signal(SIGINT, InterruptHandler);

    // Turn off standard output stream buffering.
    (void)setbuf(stdout, NULL);

    // Set up diagnostic message publisher that prints diagnostic messages
    // to the standard error stream.
    const auto diagnosticsPublisher = SystemAbstractions::DiagnosticsStreamReporter(stderr, stderr);

    // Set up diagnostics sender representing the application, and
    // register the diagnostic message publisher.
    const auto diagnosticsSender = std::make_shared< SystemAbstractions::DiagnosticsSender >("DiscordPlay");
    diagnosticsSender->SubscribeToDiagnostics(diagnosticsPublisher);
    const auto diagnosticsMessageDelegate = diagnosticsSender->Chain();

    // Process command line and environment variables.
    Environment environment;
    environment.configuration.userAgent = "DiscordBot";
    if (!ProcessCommandLineArguments(argc, argv, environment, *diagnosticsSender)) {
        PrintUsageInformation();
        return EXIT_FAILURE;
    }

    // Load trusted certificate authority (CA) certificate bundle to use
    // at the TLS layer of web connections.
    std::string caCerts;
    if (!LoadCaCerts(caCerts, *diagnosticsSender)) {
        return EXIT_FAILURE;
    }

    // Set up a clock and scheduler for use by the HTTP client and Discord
    // gateway user agent.
    const auto timeKeeper = std::make_shared< TimeKeeper >();
    const auto scheduler = std::make_shared< Timekeeping::Scheduler >();
    scheduler->SetClock(timeKeeper);

    // Set up an HTTP client to be used to connect to web APIs.
    const auto client = std::make_shared< Http::Client >();
    const auto diagnosticsSubscription = client->SubscribeToDiagnostics(diagnosticsSender->Chain());
    if (
        !StartClient(
            *client,
            timeKeeper,
            environment,
            caCerts,
            *diagnosticsSender
        )
    ) {
        return EXIT_FAILURE;
    }

    // Set up connections interface for Discord.
    auto connections = std::make_shared< Connections >();
    connections->Configure(client);
    (void)connections->SubscribeToDiagnostics(
        diagnosticsSender->Chain(),
        DIAG_LEVEL_CONNECTIONS_INTERFACE
    );

    // Set up a Discord Gateway interface and subscribe
    // to diagnostic messages from it.
    Discord::Gateway gateway;
    gateway.SetScheduler(scheduler);
    gateway.RegisterDiagnosticMessageCallback(
        [diagnosticsMessageDelegate](
            size_t level,
            std::string&& message
        ){
            diagnosticsMessageDelegate(
                "Gateway",
                level,
                message
            );
        }
    );

    // Connect Discord gateway.
    diagnosticsSender->SendDiagnosticInformationString(
        3,
        "Connecting to Discord gateway"
    );
    auto connected = gateway.Connect(
        connections,
        environment.configuration
    );
    if (
        connected.wait_for(std::chrono::seconds(5))
        != std::future_status::ready
    ) {
        diagnosticsSender->SendDiagnosticInformationString(
            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
            "Timeout connecting to Discord gateway"
        );
        gateway.Disconnect();
        (void)connected.get();
        return EXIT_FAILURE;
    }
    if (!connected.get()) {
        diagnosticsSender->SendDiagnosticInformationString(
            SystemAbstractions::DiagnosticsSender::Levels::ERROR,
            "Failed to connect to Discord gateway"
        );
        return EXIT_FAILURE;
    }
    diagnosticsSender->SendDiagnosticInformationString(
        3,
        "Gateway connected"
    );

    // Set up callback for if WebSocket is closed.
    const auto webSocketClosedPromise = std::make_shared< std::promise< void > >();
    gateway.RegisterCloseCallback(
        [webSocketClosedPromise]{
            webSocketClosedPromise->set_value();
        }
    );
    auto webSocketClosedFuture = webSocketClosedPromise->get_future();

    // Loop until interrupted with SIGINT.
    diagnosticsSender->SendDiagnosticInformationString(
        3,
        "Press <Ctrl>+<C> (and then <Enter>, if necessary) to exit."
    );
    while (
        !shutDown
        && (
            webSocketClosedFuture.wait_for(
                std::chrono::milliseconds(100)
            ) != std::future_status::ready
        )
    ) {
    }

    // Shut down Discord gateway and its dependencies.
    gateway.Disconnect();

    // Shut down the client, since we no longer need it.
    StopClient(*client);

    // We're all done!
    (void)signal(SIGINT, previousInterruptHandler);
    diagnosticsSender->SendDiagnosticInformationString(
        3,
        "Exiting."
    );
    return EXIT_SUCCESS;
}
