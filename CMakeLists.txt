# CMakeLists.txt for Discord
#
# © 2020 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This DiscordPlay)

set(Sources
    src/main.cpp
    src/Connections.cpp
    src/Connections.hpp
    src/TimeKeeper.cpp
    src/TimeKeeper.hpp
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Applications
)

target_link_libraries(${This} PUBLIC
    Discord
    Http
    HttpNetworkTransport
    StringExtensions
    SystemAbstractions
    TlsDecorator
    WebSockets
)

if(UNIX AND NOT APPLE)
    target_link_libraries(${This} PRIVATE
        -static-libstdc++
    )
endif(UNIX AND NOT APPLE)

add_custom_command(TARGET ${This} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:tls,SOURCE_DIR>/../apps/openssl/cert.pem $<TARGET_FILE_DIR:${This}>
)
