cmake_minimum_required(VERSION 3.14)
if(APPLE)
    project(wxSolitaire LANGUAGES C CXX OBJCXX)
else()
    project(wxSolitaire LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets (xrc component needed for embedded card resources)
find_package(wxWidgets REQUIRED COMPONENTS core base xrc xml html)

if(NOT WIN32)
    include(${wxWidgets_USE_FILE})
endif()

# Build bin2c host tool
set(BIN2C ${CMAKE_BINARY_DIR}/bin2c)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(HostCompile)
host_compile(${CMAKE_CURRENT_SOURCE_DIR}/bin2c.c ${BIN2C})

# Find wxrc tool for XRC compilation
find_program(WXRC_EXECUTABLE NAMES wxrc wxrc-3.2 wxrc-3.1 wxrc-3.0)
if(NOT WXRC_EXECUTABLE)
    message(FATAL_ERROR "wxrc not found - required for XRC resource compilation")
endif()

# --- Card resource pipeline: XRC -> XRS -> C header ---
file(GLOB CARD_BMP_FILES "${CMAKE_SOURCE_DIR}/bmp/*.bmp")
set(CARDS_XRC ${CMAKE_SOURCE_DIR}/cards.xrc)
set(CARDS_XRS ${CMAKE_BINARY_DIR}/cards.xrs)
set(CARDS_XRS_H ${CMAKE_BINARY_DIR}/cards_xrs.h)

add_custom_command(
    OUTPUT ${CARDS_XRS}
    COMMAND ${WXRC_EXECUTABLE} ${CARDS_XRC} -o ${CARDS_XRS}
    DEPENDS ${CARDS_XRC} ${CARD_BMP_FILES}
    COMMENT "Compiling cards.xrc -> cards.xrs"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command(
    OUTPUT ${CARDS_XRS_H}
    COMMAND ${BIN2C} ${CARDS_XRS} ${CARDS_XRS_H} cards_xrs
    DEPENDS ${CARDS_XRS} ${BIN2C}
    COMMENT "Embedding cards.xrs -> cards_xrs.h via bin2c"
)

# --- App icon: PNG -> C header via bin2c ---
set(ICON_PNG ${CMAKE_SOURCE_DIR}/solitaire.iconset/icon_128x128.png)
set(ICON_PNG_H ${CMAKE_BINARY_DIR}/solitaire_icon.h)

add_custom_command(
    OUTPUT ${ICON_PNG_H}
    COMMAND ${BIN2C} ${ICON_PNG} ${ICON_PNG_H} solitaire_icon
    DEPENDS ${ICON_PNG} ${BIN2C}
    COMMENT "Embedding app icon -> solitaire_icon.h via bin2c"
)

# Source files
set(SOURCES
    wxsol_main.cpp
    wxsol_game.cpp
    wxsol_col.cpp
    wxsol_util.cpp
    wxsol_cards.cpp
    ${CARDS_XRS_H}
    ${ICON_PNG_H}
)
set_source_files_properties(${CARDS_XRS_H} PROPERTIES GENERATED TRUE)
set_source_files_properties(${ICON_PNG_H} PROPERTIES GENERATED TRUE)

# Help assets shared across platforms
set(HELP_BUNDLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wxsolitaire.help)
set(HELP_CHM_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/wxsolitaire.chm)

# Platform-specific settings
if(APPLE)
    # Build as a macOS .app bundle
    set(MACOSX_BUNDLE_BUNDLE_NAME "Solitaire")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.wxsolitaire.app")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")

    # Icon
    set(ICON_FILE ${CMAKE_SOURCE_DIR}/solitaire.icns)
    if(EXISTS ${ICON_FILE})
        set_source_files_properties(${ICON_FILE} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")
        list(APPEND SOURCES ${ICON_FILE})
        set(MACOSX_BUNDLE_ICON_FILE solitaire.icns)
    endif()

    add_executable(wxSolitaire MACOSX_BUNDLE ${SOURCES} help_mac.mm)
    target_link_libraries(wxSolitaire "-framework Carbon" "-framework AppKit")

    # Copy the .help bundle into <App>.app/Contents/Resources/wxsolitaire.help
    if(EXISTS ${HELP_BUNDLE_DIR})
        add_custom_command(TARGET wxSolitaire POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${HELP_BUNDLE_DIR}
                $<TARGET_BUNDLE_CONTENT_DIR:wxSolitaire>/Resources/wxsolitaire.help
            COMMENT "Copying wxsolitaire.help into app bundle Resources")
    endif()
elseif(WIN32)
    # Windows: build as a GUI application
    add_executable(wxSolitaire WIN32 ${SOURCES} sol.rc)

    # Place the .chm next to the built .exe so the Help menu can find it.
    if(EXISTS ${HELP_CHM_FILE})
        add_custom_command(TARGET wxSolitaire POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${HELP_CHM_FILE}
                $<TARGET_FILE_DIR:wxSolitaire>/wxsolitaire.chm
            COMMENT "Copying wxsolitaire.chm next to executable")
        install(FILES ${HELP_CHM_FILE} DESTINATION bin)
    endif()
else()
    # Linux / other Unix
    add_executable(wxSolitaire ${SOURCES})

    install(TARGETS wxSolitaire RUNTIME DESTINATION bin)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxSolitaire.desktop      DESTINATION share/applications)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxSolitaire.metainfo.xml DESTINATION share/metainfo)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sizes/                   DESTINATION share/icons/hicolor)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxSolitaire.6            DESTINATION share/man/man6)

    # Ship both help formats so the runtime can fall back to HTML from the .help bundle.
    if(EXISTS ${HELP_BUNDLE_DIR})
        install(DIRECTORY ${HELP_BUNDLE_DIR} DESTINATION share/wxSolitaire)
    endif()
    if(EXISTS ${HELP_CHM_FILE})
        install(FILES ${HELP_CHM_FILE} DESTINATION share/wxSolitaire)
    endif()
endif()

target_include_directories(wxSolitaire PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
target_link_libraries(wxSolitaire ${wxWidgets_LIBRARIES})
