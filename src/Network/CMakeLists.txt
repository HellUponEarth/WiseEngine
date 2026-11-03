# Code Modification - Build System: Isolate the pinned Asio headers behind the transport API.
# Scope: Network module. Performance: no sockets until explicitly opened. Toggle: caller-owned lifetime.
add_library(Network STATIC Transport.cpp Transport.h)
target_link_libraries(Network PUBLIC Core PRIVATE ThirdParty::asio)
wiseengine_target(Network)
# End of Code Modification - Build System
