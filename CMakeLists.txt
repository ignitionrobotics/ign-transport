cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

#============================================================================
# Initialize the project
#============================================================================
project(ignition-transport4 VERSION 4.0.0)

#============================================================================
# Find ignition-cmake
#============================================================================
# If you get an error at this line, you need to install ignition-cmake
find_package(ignition-cmake0 0.4.0 REQUIRED)

#============================================================================
# Configure the project
#============================================================================
ign_configure_project()

#============================================================================
# Set project-specific options
#============================================================================

# ignition-transport currently has no options that are unique to it

#============================================================================
# Search for project-specific dependencies
#============================================================================
message(STATUS "\n\n-- ====== Finding Dependencies ======")

#--------------------------------------
# Find Protobuf
if(MSVC)
  # On Windows we require version 3+ in order to have proper support for
  # shared libraries.
  set(REQ_PROTOBUF_VER 3)
  set(PROTOBUF_PURPOSE "Windows requires version 3+ to ensure proper support for DLLs")
else()
  set(REQ_PROTOBUF_VER 2.3.0)
  set(PROTOBUF_PURPOSE)
endif()
ign_find_package(IgnProtobuf
                 VERSION ${REQ_PROTOBUF_VER}
                 REQUIRED
                 PRETTY Protobuf
                 PURPOSE ${PROTOBUF_PURPOSE})

#--------------------------------------
# Find ZeroMQ
ign_find_package(ZeroMQ VERSION 4 REQUIRED PRIVATE)

#--------------------------------------
# Find cppzmq
ign_find_package(CPPZMQ REQUIRED PRIVATE
  PKGCONFIG_IGNORE # NOTE: cppzmq does not seem to offer a pkg-config file
  PRETTY CppZMQ)

#--------------------------------------
# Find uuid
if (MSVC)
  message (STATUS "UUID: Using Windows RPC UuidCreate function\n")
else()
  ign_find_package(UUID REQUIRED)
endif()

#--------------------------------------
# Find ignition-msgs
ign_find_package(ignition-msgs1 REQUIRED)
set(IGN_MSGS_VER ${ignition-msgs1_VERSION_MAJOR})

#--------------------------------------
# Find ifaddrs
ign_find_package(IFADDRS QUIET)
if (IFADDRS_FOUND)
  set (HAVE_IFADDRS ON CACHE BOOL "HAVE IFADDRS" FORCE)
else ()
  set (HAVE_IFADDRS OFF CACHE BOOL "HAVE IFADDRS" FORCE)
endif()

#--------------------------------------
# Find if ign command is available
find_program(HAVE_IGN_TOOLS ign)

#============================================================================
# Configure the build
#============================================================================
ign_configure_build(QUIT_IF_BUILD_ERRORS)

#============================================================================
# ign command line support
#============================================================================
add_subdirectory(conf)

#============================================================================
# Test the command line commands
#============================================================================
add_subdirectory(tools)

#============================================================================
# Create package information
#============================================================================
ign_create_packages()
