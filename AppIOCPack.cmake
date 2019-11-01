set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AppIO Boost Asio ZMQ application interface")
set(CPACK_PACKAGE_DESCRIPTION
        "The AppIO library provides bindings to AsyncZMQ and Boost Asio.
         Interface creates event loop and provide pub/sub, app-config and timer interfaces.
This     Library is intended to work with C++ applications and Lua.")
set(CPACK_PACKAGE_VERSION               "0.1")

# Debian-specific packaging
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE   "all")
set(CPACK_DEBIAN_PACKAGE_DEPENDS        "libazmq-dev(>= 1.0.0), rapidjson-dev, nlohmann-json3-dev ")
set(CPACK_DEBIAN_PACKAGE_NAME           "libappio-dev")
set(CPACK_DEBIAN_PACKAGE_SECTION        "libdevel")

include(CPack)