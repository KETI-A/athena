TARGET = venus
TEMPLATE = app

QT += core qml network quick positioning positioning-private location widgets gui location

SOURCES += main.cpp
SOURCES += logfilepositionsource.cpp
SOURCES += clientapplication.cpp

HEADERS += logfilepositionsource.h
HEADERS += clientapplication.h

# Workaround for QTBUG-38735
QT_FOR_CONFIG += location-private

qml_resources.files = \
    qmldir \
    Main.qml \
    helper.js \
    map/MapComponent.qml \
    map/MapSliders.qml \
    map/Marker.qml \
    map/MiniMap.qml \
    menus/ItemPopupMenu.qml \
    menus/MainMenu.qml \
    menus/MapPopupMenu.qml \
    menus/MarkerPopupMenu.qml \
    forms/Geocode.qml \
    forms/GeocodeForm.ui.qml\
    forms/Message.qml \
    forms/MessageForm.ui.qml \
    forms/ReverseGeocode.qml \
    forms/ReverseGeocodeForm.ui.qml \
    forms/RouteCoordinate.qml \
    forms/Locale.qml \
    forms/LocaleForm.ui.qml \
    forms/RouteAddress.qml \
    forms/RouteAddressForm.ui.qml \
    forms/RouteCoordinateForm.ui.qml \
    forms/RouteList.qml \
    forms/RouteListDelegate.qml \
    forms/RouteListHeader.qml \
    resources/marker.png \
    resources/marker_blue.png \
    resources/vehicle.png \
    resources/scale.png \
    resources/scale_end.png

qml_resources.prefix = /qt/qml/Venus

RESOURCES = qml_resources

target.path = $$[QT_INSTALL_EXAMPLES]/location/venus
INSTALLS += target