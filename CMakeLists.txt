set(CMAKE_LEGACY_CYGWIN_WIN32 0)
set(CADABRA_CMAKE_VERSION 3.1)

cmake_minimum_required(VERSION ${CADABRA_CMAKE_VERSION})
set(CMAKE_CXX_STANDARD 14)
project(Cadabra)


#---------------------------------------------------------------------------
# Preamble
#---------------------------------------------------------------------------

include(cmake/functions.cmake)

# Include Visual Studio specific build commands
if (MSVC)
	include(cmake/windows.cmake)
endif()

# Make sure the build type is non-empty.
if (NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CADABRA_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE MATCHES "^Debug$")
	set(CADABRA_DEBUG_BUILD TRUE)
endif()

# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

# Get version information. 
include(cmake/version.cmake)
print_header("Building Cadabra version ${CADABRA_VERSION_MAJOR}.${CADABRA_VERSION_MINOR}.${CADABRA_VERSION_PATCH} (${SYSTEM_BITS}-bit)")
message(STATUS "Build id '${CADABRA_VERSION_BUILD}' dated ${CADABRA_VERSION_DATE}")
message(STATUS "Build mode is set to '${CMAKE_BUILD_TYPE}'")

# Notify about install directory
if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
	message(STATUS "Install directory not set")
else()
	message(STATUS "Install directory set to ${CMAKE_INSTALL_PREFIX}")
endif()

# Include packaging logic.
include(cmake/packaging.cmake)

# Aliases for directories
set(CADABRA_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(CADABRA_CLIENT_SERVER_DIR ${CADABRA_ROOT_DIR}/client_server)
set(CADABRA_CORE_DIR ${CADABRA_ROOT_DIR}/core)
set(CADABRA_FRONTEND_DIR ${CADABRA_ROOT_DIR}/frontend)
set(CADABRA_IMAGES_DIR ${CADABRA_ROOT_DIR}/images)
set(CADABRA_LIBS_DIR ${CADABRA_ROOT_DIR}/libs)


#---------------------------------------------------------------------------
# User options and other notifications
#---------------------------------------------------------------------------

# Provide option to build with Python 3 (default) or Python 2.
option(USE_PYTHON_3 "Use Python 3 if ON, or fall back to Python 2 if OFF" ON)

option(PACKAGING_MODE "Run in packaging mode, overriding path settings" OFF)
if (PACKAGING_MODE)
	message(STATUS "Building in packaging mode")
	if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
	else()
		MESSAGE(FATAL_ERROR "Building with -DPACKAGING_MODE=ON also requires -DCMAKE_INSTALL_PREFIX=/usr")
	endif()
else()
	message(STATUS "Building in user mode")
endif()

option(ENABLE_JUPYTER "Enable building the Jupyter kernel" OFF)

if(ENABLE_JUPYTER)
  message(STATUS "Building the Jupyter kernel")
  # Currently only possible when building against Conda.
  set(CONDA_FOUND TRUE)
else()
  set(CONDA_FOUND FALSE)
  message(STATUS "Not building the Jupyter kernel")
endif()

if(NOT ENABLE_JUPYTER)
  option(BUILD_TESTS "Build tests" ON)
  if (BUILD_TESTS)
  	message(STATUS "Building tests")
  	# Allows tests to be built in all subdirectories.
  	enable_testing()
 endif()
else()
  set(BUILD_TESTS OFF)
endif()

option(ENABLE_MATHEMATICA "Enable Mathematica support" ON)

option(ENABLE_FRONTEND    "Enable the UI frontend" ON)

option(ENABLE_SYSTEM_JSONCPP "Use the system-provided jsoncpp library" OFF)

option(INSTALL_TARGETS_ONLY "Only install targets; skipping icons, shared libraries etc..." OFF)
if (INSTALL_TARGETS_ONLY)
	message(STATUS "INSTALL_TARGETS_ONLY is enabled, please make sure all auxillary files and programs Cadabra requires are already installed")
endif()

#---------------------------------------------------------------------------
# Compiler flags.
#---------------------------------------------------------------------------

# - Set the C++ standard to C++14
# - Turn optimizations on
# - Turn off warnings we don't need

# GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	if (ENABLE_FRONTEND)
		if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
			message(FATAL_ERROR "GCC version must be at least 4.9 for regex support! See http://askubuntu.com/questions/428198/getting-installing-gcc-g-4-9-on-ubuntu and then set the environment variables CXX to g++-4.9 and CC to gcc-4.9. You may have to erase the build directory before re-running cmake.")
		endif()
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -fvisibility=hidden -Wno-unused-but-set-variable")
endif()

# Clang
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# For Clang, need to additionally check version to avoid compiler bugs
	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
		message(FATAL_ERROR "Clang version must be at least 3.5 to avoid known bugs.")
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -fvisibility=hidden -Wall -Wextra -Wunused")
endif() 

# Visual Studio
if(MSVC)
	# Disable specific warnings
	set(MSVC_FLAGS
		"/wd4250"						# inherits via dominance (rampant in the GTKMM codebase)
		"/wd4101"						# unreferenced local variable
		"/wd4244"						# conversion from x to y, possible loss of data
		"/wd4267"						# same as 4244
		"/wd4305"						# truncation from '' to 'char'
		"/wd4309"						# truncation of constant value
		"/wd4390"						# empty control statement, due to a DEBUG macro which requires trailing ;
		"/wd4996"						# deprecated POSIX functions
		"-D_CRT_SECURE_NO_WARNINGS"		# don't warn about deprecated functions
		"-D_SCL_SECURE_NO_WARNINGS"		# don't warn about unsafe function calls (e.g. std::copy with raw pointers)
		"-DNOMINMAX"					# prevent windows headers from defining min and max macros
		"-DWIN32_LEAN_AND_MEAN"			# stop windows from including a bunch of garbage
		"-DBOOST_ALL_DYN_LINK"			# ensure boost's auto-linking is enabled
	)
	foreach(FLAG ${MSVC_FLAGS})
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
	endforeach()
endif()


#---------------------------------------------------------------------------
# Configure the various parts of Cadabra.
#---------------------------------------------------------------------------

# Ensure that all installed binaries have an RPATH setting
# which enables them to find libtexengine, libcadabra_client and
# libcadabra_server without any ld.so.conf settings.
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config/postinst.in" 
	"${CMAKE_CURRENT_BINARY_DIR}/postinst" 
	@ONLY
)

# Suffixes
if(WIN32)
	set(STATIC_LIB_SUFFIX "lib")
	set(SHARED_LIB_SUFFIX "dll")
	set(PYTHON_MOD_SUFFIX "pyd")
else()
	set(STATIC_LIB_SUFFIX "a")
	set(SHARED_LIB_SUFFIX "so")
	set(PYTHON_MOD_SUFFIX "so")
endif()


#---------------------------------------------------------------------------
# Configure Mathematica (if enabled).
#---------------------------------------------------------------------------

if(ENABLE_MATHEMATICA)
	print_header("Configuring Mathematica")
	find_package(Mathematica COMPONENTS WSTP)
endif()


#---------------------------------------------------------------------------
# Configure Python.
#---------------------------------------------------------------------------

print_header("Configuring Python")

if(USE_PYTHON_3)
	set(PYTHON_POSTFIX "3")
	message(STATUS "Building for use with Python 3 (good!)")
else()
	set(PYTHON_POSTFIX "")
	message(STATUS "Building for use with Python 2 (consider upgrading!)")
endif()

add_subdirectory(libs/pybind11)

message(STATUS "Found python ${PYTHON_LIBRARIES}")

# We install the python module in our 'share' path, not in
# the site-wide path as we used to. 
set(PYTHON_SITE_PATH share/cadabra2/python)
if(NOT WIN32)
  execute_process(
	 COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.getsitepackages()[0]);"
	 OUTPUT_VARIABLE OLD_PYTHON_SITE_PATH 
	 OUTPUT_STRIP_TRAILING_WHITESPACE
	 )
endif()


#---------------------------------------------------------------------------
# Add subdirectories to project.
#---------------------------------------------------------------------------

# Core/packages
add_subdirectory(client_server)
add_subdirectory(core)
add_subdirectory(core/packages)

# Frontend
if(ENABLE_FRONTEND)
  add_subdirectory(frontend)
endif()

# Tests
if(BUILD_TESTS)
  add_subdirectory(tests)
endif()

add_subdirectory(web2 EXCLUDE_FROM_ALL)

# Generate the core/Config.hh file; this needs to come as late as possible
# in this CMakeLists.txt to ensure that all variables have been set.
configure_file(
	"${PROJECT_SOURCE_DIR}/core/Config.hh.in" 
	"${PROJECT_SOURCE_DIR}/core/Config.hh"
)

#---------------------------------------------------------------------------
# Provide uninstall target.
#---------------------------------------------------------------------------

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
	IMMEDIATE @ONLY
)

add_custom_target(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
) 

print_header("All scripts completed")
