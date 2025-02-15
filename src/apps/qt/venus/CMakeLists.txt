# Generated from venus.pro.

cmake_minimum_required(VERSION 3.16)
project(venus LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/location/venus")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Network Quick Positioning Location Charts Widgets)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(venus WIN32 MACOSX_BUNDLE
    main.cpp
    logfilepositionsource.cpp
    clientapplication.cpp
)

target_link_libraries(venus PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Location
    Qt::Network
    Qt::Positioning
    Qt::Qml
    Qt::Quick
    Qt::Charts
    Qt::Widgetsi
)

qt_add_qml_module(venus
    URI Venus
    VERSION 1.0
    QML_FILES
        "forms/Geocode.qml"
        "forms/GeocodeForm.ui.qml"
        "forms/Locale.qml"
        "forms/LocaleForm.ui.qml"
        "forms/Message.qml"
        "forms/MessageForm.ui.qml"
        "forms/ReverseGeocode.qml"
        "forms/ReverseGeocodeForm.ui.qml"
        "forms/RouteAddress.qml"
        "forms/RouteAddressForm.ui.qml"
        "forms/RouteCoordinate.qml"
        "forms/RouteCoordinateForm.ui.qml"
        "forms/RouteList.qml"
        "forms/RouteListDelegate.qml"
        "forms/RouteListHeader.qml"
        "forms/graph.qml"
        "helper.js"
        "map/MapComponent.qml"
        "map/MapSliders.qml"
        "map/Marker.qml"
        "map/MiniMap.qml"
        "Main.qml"
        "menus/ItemPopupMenu.qml"
        "menus/MainMenu.qml"
        "menus/MapPopupMenu.qml"
        "menus/MarkerPopupMenu.qml"
    RESOURCES
        "resources/marker.png"
        "resources/marker_blue.png"
        "resources/vehicle.png"
        "resources/scale.png"
        "resources/scale_end.png"
    )

if(QT_FEATURE_geoservices_maplibregl)
    target_link_libraries(venus PUBLIC
        Qt::Sql
    )
endif()

if (ANDROID)
    # Download & link to OpenSSL libs
    include(FetchContent)
    FetchContent_Declare(
      android_openssl
      DOWNLOAD_EXTRACT_TIMESTAMP true
      URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)

    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(venus)
endif()

install(TARGETS venus
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
