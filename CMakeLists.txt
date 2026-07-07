cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

# This plugin uses both the frontend API (dock + hotkeys) and Qt (dock UI).
option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

# --- Workaround: strip the legacy AGL framework from the OBS imported targets'
# link interface. Newer macOS SDKs (macOS 15 / 26 "Tahoe") removed AGL, but the
# pinned libobs still lists it as a link dependency, which propagates onto this
# plugin's link line and fails with "ld: framework 'AGL' not found". This plugin
# does not use AGL, so removing it is safe.
#
# We enumerate ALL imported targets (skipping aliases, which cannot be modified)
# so we catch AGL wherever it actually lives -- e.g. on the real `libobs` target
# behind the `OBS::libobs` alias -- and log where it was found.
if(APPLE)
  get_property(_imported_targets DIRECTORY PROPERTY IMPORTED_TARGETS)
  foreach(_t IN LISTS _imported_targets)
    get_target_property(_aliased ${_t} ALIASED_TARGET)
    if(_aliased)
      continue()
    endif()
    foreach(
      _prop
      IN
      ITEMS
        INTERFACE_LINK_LIBRARIES
        INTERFACE_LINK_LIBRARIES_RELEASE
        INTERFACE_LINK_LIBRARIES_RELWITHDEBINFO
        INTERFACE_LINK_LIBRARIES_DEBUG
        INTERFACE_LINK_LIBRARIES_MINSIZEREL
        INTERFACE_LINK_OPTIONS
    )
      get_target_property(_vals ${_t} ${_prop})
      if(_vals)
        set(_orig "${_vals}")
        list(FILTER _vals EXCLUDE REGEX "(^|[/:,> ])AGL([.]framework)?($|[,>])")
        if(NOT "${_vals}" STREQUAL "${_orig}")
          set_target_properties(${_t} PROPERTIES ${_prop} "${_vals}")
          message(STATUS "AGL workaround: stripped AGL from ${_t} (${_prop})")
        endif()
      endif()
    endforeach()
  endforeach()
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/plugin-support.cpp
    src/plugin-support.hpp
    src/config.cpp
    src/config.hpp
    src/active-browser.cpp
    src/active-browser.hpp
    src/scene-tracker.cpp
    src/scene-tracker.hpp
    src/status-dock.cpp
    src/status-dock.hpp
    src/vendor-request.cpp
    src/vendor-request.hpp
)

# Vendored obs-websocket-api.h (for the optional dial / absolute-volume vendor requests)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps")

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
