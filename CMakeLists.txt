cmake_minimum_required(VERSION 3.16)
project(wxSolitaire LANGUAGES C CXX)

# Objective-C++ is only needed for the macOS help bridge (help_mac.mm). Enable
# it after project() so the toolchain has resolved the target system: APPLE is
# true even when cross-compiling for Android from a macOS host, so gate on the
# resolved CMAKE_SYSTEM_NAME instead.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Classic Mac OS (wxRetro68): 68k (Retro68) or PowerPC (RetroPPC).
if(CMAKE_SYSTEM_NAME MATCHES "^Retro(68|PPC)$")
    set(RETRO68 ON)
endif()

# --- wxWidgets discovery ---------------------------------------------------
# On Android the game is built on top of the wxWidgets "Qt" port (wxQt) that
# ships prebuilt (static) inside the Qt Android kit, so we cannot use CMake's
# find_package(wxWidgets); instead we query that kit's wx-config directly and
# let Qt6 turn the target into an APK via androiddeployqt.
if(ANDROID)
    set(WX_CONFIG_EXECUTABLE "/usr/local/qt-6.9.3-android-arm64/bin/wx-config"
        CACHE FILEPATH "Path to the Android (wxQt) wx-config")
    if(NOT EXISTS "${WX_CONFIG_EXECUTABLE}")
        message(FATAL_ERROR "Android wx-config not found at ${WX_CONFIG_EXECUTABLE}")
    endif()

    execute_process(COMMAND ${WX_CONFIG_EXECUTABLE} --cxxflags
        OUTPUT_VARIABLE WX_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _wx_rc)
    execute_process(COMMAND ${WX_CONFIG_EXECUTABLE} --libs core,base,xrc,xml,html
        OUTPUT_VARIABLE WX_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _wx_rc EQUAL 0)
        message(FATAL_ERROR "Failed to query Android wx-config")
    endif()

    separate_arguments(WX_CXXFLAGS_LIST UNIX_COMMAND "${WX_CXXFLAGS}")
    separate_arguments(WX_LIBS_LIST     UNIX_COMMAND "${WX_LIBS}")

    # wxQt is built on QtWidgets/QtGui/QtCore; linking these also lets Qt's
    # CMake integration generate the Android deployment settings + apk target.
    # PrintSupport is pulled in by wxQt's print code (QPrinter/QPrinterInfo).
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
elseif(RETRO68)
    # wxWidgets from the sysroot's wx-config, never the host's: FindwxWidgets
    # is unreliable when cross-compiling and would pick up the host's wx.
    if(NOT wxWidgets_CONFIG_EXECUTABLE)
        set(wxWidgets_CONFIG_EXECUTABLE ${CMAKE_SYSTEM_PREFIX_PATH}/bin/wx-config)
    endif()
    execute_process(COMMAND ${wxWidgets_CONFIG_EXECUTABLE} --cxxflags core base xrc html xml
        OUTPUT_VARIABLE WX_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _wxr1)
    execute_process(COMMAND ${wxWidgets_CONFIG_EXECUTABLE} --libs core base xrc html xml
        OUTPUT_VARIABLE WX_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _wxr2)
    if(_wxr1 OR _wxr2)
        message(FATAL_ERROR "wx-config failed: ${wxWidgets_CONFIG_EXECUTABLE}")
    endif()
    separate_arguments(WX_CXXFLAGS_LIST NATIVE_COMMAND "${WX_CXXFLAGS}")
    separate_arguments(WX_LIBS_LIST NATIVE_COMMAND "${WX_LIBS}")
else()
    # Find wxWidgets (xrc component needed for embedded card resources)
    find_package(wxWidgets REQUIRED COMPONENTS core base xrc xml html)

    if(NOT WIN32)
        include(${wxWidgets_USE_FILE})
    endif()
endif()

# Build bin2c host tool
set(BIN2C ${CMAKE_BINARY_DIR}/bin2c)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(HostCompile)
host_compile(${CMAKE_CURRENT_SOURCE_DIR}/bin2c.c ${BIN2C})

# Find wxrc tool for XRC compilation.
# When cross-compiling for Android the wxrc inside the Qt kit is an Android
# ELF binary that cannot run on the host, so force the host wxrc instead.
if(ANDROID)
    set(WXRC_EXECUTABLE "/usr/local/bin/wxrc-3.3"
        CACHE FILEPATH "Host wxrc used to compile XRC resources")
elseif(RETRO68)
    # The sysroot's wxrc is a classic Mac program; use the host one.
    find_program(WXRC_EXECUTABLE NAMES wxrc wxrc-3.3 wxrc-3.2
        PATHS /usr/local/bin /opt/homebrew/bin NO_DEFAULT_PATH)
    if(NOT WXRC_EXECUTABLE)
        find_program(WXRC_EXECUTABLE NAMES wxrc wxrc-3.3 wxrc-3.2 NO_CMAKE_SYSTEM_PATH)
    endif()
else()
    find_program(WXRC_EXECUTABLE NAMES wxrc wxrc-3.3 wxrc-3.2 wxrc-3.1 wxrc-3.0)
endif()
if(NOT WXRC_EXECUTABLE OR NOT EXISTS "${WXRC_EXECUTABLE}")
    message(FATAL_ERROR "wxrc not found (${WXRC_EXECUTABLE}) - required for XRC resource compilation")
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
if(RETRO68)
    # =======================================================================
    #  Classic Mac OS (Retro68): 68k (Retro68) or PowerPC (RetroPPC)
    #
    #  Built against wxRetro68 (wxUniversal on QuickDraw) from the Retro68
    #  sysroot. Produces Solitaire.bin (MacBinary), Solitaire.APPL and
    #  Solitaire.dsk (an HFS disk image) for the one architecture;
    #  retro68/build.sh builds both and merges them into a single fat
    #  application on one disk image.
    # =======================================================================

    # The HTML help, zipped and embedded, shown in a wxHtmlWindow (classic
    # Mac OS has no browser to hand it to).
    set(HELP_SRC_DIR ${HELP_BUNDLE_DIR}/Contents/Resources/en.lproj)
    file(GLOB HELP_FILES RELATIVE ${HELP_SRC_DIR}
        ${HELP_SRC_DIR}/*.html ${HELP_SRC_DIR}/*.css
        ${HELP_SRC_DIR}/html/*.html ${HELP_SRC_DIR}/images/*)
    list(SORT HELP_FILES)
    set(HELP_DEPS)
    foreach(_f ${HELP_FILES})
        list(APPEND HELP_DEPS ${HELP_SRC_DIR}/${_f})
    endforeach()
    set(HELP_ZIP ${CMAKE_BINARY_DIR}/help.zip)
    add_custom_command(
        OUTPUT ${HELP_ZIP}
        COMMAND ${CMAKE_COMMAND} -E tar cf ${HELP_ZIP} --format=zip ${HELP_FILES}
        WORKING_DIRECTORY ${HELP_SRC_DIR}
        DEPENDS ${HELP_DEPS}
        COMMENT "Zipping Solitaire Help (Retro68)"
    )
    set(HELP_DATA_H ${CMAKE_BINARY_DIR}/help_data.h)
    add_custom_command(
        OUTPUT ${HELP_DATA_H}
        COMMAND ${BIN2C} ${HELP_ZIP} ${HELP_DATA_H} help_data
        DEPENDS ${HELP_ZIP} ${BIN2C}
        COMMENT "Embedding Solitaire Help as C array (Retro68)"
    )

    add_executable(wxSolitaire ${SOURCES} ${HELP_DATA_H}
        ${CMAKE_SOURCE_DIR}/retro68/help_retro68.cpp)
    target_compile_options(wxSolitaire PRIVATE ${WX_CXXFLAGS_LIST}
        -ffunction-sections -fdata-sections)
    target_link_options(wxSolitaire PRIVATE -Wl,--gc-sections)
    target_link_libraries(wxSolitaire PRIVATE
        -Wl,--start-group ${WX_LIBS_LIST} -Wl,--end-group)
    # Relink when the wx libraries change (wx-config gives flags, so CMake
    # does not know about them): the full paths, plus -l names found in the
    # -L directories.
    set(WX_LIB_FILES)
    set(_wx_lib_dirs)
    foreach(_arg ${WX_LIBS_LIST})
        if(_arg MATCHES "^-L(.+)$")
            list(APPEND _wx_lib_dirs "${CMAKE_MATCH_1}")
        elseif(_arg MATCHES "\\.a$" AND EXISTS "${_arg}")
            list(APPEND WX_LIB_FILES "${_arg}")
        elseif(_arg MATCHES "^-l(.+)$")
            foreach(_dir ${_wx_lib_dirs})
                if(EXISTS "${_dir}/lib${CMAKE_MATCH_1}.a")
                    list(APPEND WX_LIB_FILES "${_dir}/lib${CMAKE_MATCH_1}.a")
                    break()
                endif()
            endforeach()
        endif()
    endforeach()
    list(REMOVE_DUPLICATES WX_LIB_FILES)
    set_property(TARGET wxSolitaire APPEND PROPERTY LINK_DEPENDS ${WX_LIB_FILES})

    # -DSOLITAIRE_TRACE=ON: log startup to "wxtest.log" next to the
    # application and shut the Mac down once it is up, for unattended
    # emulator runs (retro68/run-basilisk.sh).
    option(SOLITAIRE_TRACE "Trace startup to wxtest.log (classic Mac OS testing)" OFF)
    if(SOLITAIRE_TRACE)
        target_sources(wxSolitaire PRIVATE ${CMAKE_SOURCE_DIR}/retro68/trace_retro68.cpp)
        target_compile_definitions(wxSolitaire PRIVATE SOLITAIRE_TRACE=1)
    endif()

    set(SOLITAIRE_REZ_DEFINES)
    if(CMAKE_SYSTEM_NAME STREQUAL "RetroPPC")
        set(SOLITAIRE_REZ_DEFINES SOLITAIRE_PPC=1)
    endif()

    include(Retro68Application)
    retro68_application(wxSolitaire
        NAME "Solitaire"
        CREATOR WXSL
        RESOURCES ${CMAKE_SOURCE_DIR}/retro68/wxSolitaire.r
        REZ_INCLUDES ${CMAKE_SOURCE_DIR}/retro68
        REZ_DEFINES ${SOLITAIRE_REZ_DEFINES})
elseif(ANDROID)
    # Build the game as an Android application library (Qt emits
    # libwxSolitaire_<abi>.so) and let Qt6/androiddeployqt package the APK.
    qt_add_executable(wxSolitaire ${SOURCES} android_stubs.cpp)

    # wxQt compile flags (include dirs, -D__WXQT__, ...) from wx-config.
    target_compile_options(wxSolitaire PRIVATE ${WX_CXXFLAGS_LIST})

    # Link order matters: the static wx archives reference Qt symbols, so they
    # must appear before the Qt libraries on the link line.
    target_link_libraries(wxSolitaire PRIVATE
        ${WX_LIBS_LIST}
        Qt6::Widgets Qt6::Gui Qt6::Core Qt6::PrintSupport)

    set_target_properties(wxSolitaire PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/android
        QT_ANDROID_TARGET_SDK_VERSION 34
        QT_ANDROID_MIN_SDK_VERSION    28
        QT_ANDROID_ABIS               "arm64-v8a"
        QT_ANDROID_VERSION_NAME       "1.0"
        QT_ANDROID_VERSION_CODE       1)
elseif(APPLE)
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
if(NOT ANDROID AND NOT RETRO68)
    target_link_libraries(wxSolitaire ${wxWidgets_LIBRARIES})
endif()
