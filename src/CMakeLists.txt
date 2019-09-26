add_subdirectory(QR-Code-generator)
add_subdirectory(QR-Code-scanner)
add_subdirectory(daemon)
add_subdirectory(libwalletqt)
add_subdirectory(model)
add_subdirectory(zxcvbn-c)

qt5_add_resources(QML_QRC ../qml.qrc)

# Compile source files (.h/.cpp)
file(GLOB SOURCE_FILES
		"*.h"
		"*.cpp"
        "main/*.h"
        "main/*.cpp"
        "libwalletqt/WalletManager.cpp"
        "libwalletqt/Wallet.cpp"
        "libwalletqt/PendingTransaction.cpp"
        "libwalletqt/TransactionHistory.cpp"
        "libwalletqt/TransactionInfo.cpp"
        "libwalletqt/QRCodeImageProvider.cpp" QR
        "QR-Code-generator/BitBuffer.cpp"
        "QR-Code-generator/QrCode.cpp"
        "QR-Code-generator/QrSegment.cpp"
        "libwalletqt/AddressBook.cpp"
        "libwalletqt/Subaddress.cpp"
        "libwalletqt/SubaddressAccount.cpp"
        "libwalletqt/UnsignedTransaction.cpp"
        "libwalletqt/WalletManager.h"
        "libwalletqt/Wallet.h"
        "libwalletqt/PendingTransaction.h"
        "libwalletqt/TransactionHistory.h"
        "libwalletqt/TransactionInfo.h"
        "libwalletqt/QRCodeImageProvider.h"
        "QR-Code-generator/BitBuffer.h"
        "QR-Code-generator/QrCode.h"
        "QR-Code-generator/QrSegment.h"
        "libwalletqt/Transfer.h"
        "libwalletqt/AddressBook.h"
        "libwalletqt/Subaddress.h"
        "libwalletqt/SubaddressAccount.h"
        "libwalletqt/UnsignedTransaction.h"
		"daemon/*.h"
		"daemon/*.cpp"
		"model/*.h"
		"model/*.cpp"
#		"src/QR-Code-generator/*.h"
#		"src/QR-Code-generator/*.cpp"
#		"src/QR-Code-scanner/*.h"
#		"src/QR-Code-scanner/*.cpp"
        "qt/*.h"
        "qt/*.cpp"
)

message(STATUS ${QML_QRC})
add_executable(monero-gui main/main.cpp
        ${SOURCE_FILES}
        ${QML_QRC}
        )

# OpenGL
target_include_directories(monero-gui PUBLIC ${OPENGL_INCLUDE_DIR})
message(STATUS "OpenGL: include dir at ${OPENGL_INCLUDE_DIR}")
message(STATUS "OpenGL: libraries at ${OPENGL_LIBRARIES}")

# OpenSSL
target_include_directories(monero-gui PUBLIC ${OPENSSL_INCLUDE_DIR})
message(STATUS "OpenSSL: Version ${OPENSSL_VERSION}")
message(STATUS "OpenSSL: include dir at ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL: libraries at ${OPENSSL_LIBRARIES} ${OPENSSL_SSL_LIBRARIES}")

# Zbar (for QR scanner)
#find_package(ZBar0)
#target_include_directories(monero-gui PUBLIC ${ZBAR_INCLUDE_DIR})
#message(STATUS "libzbar: include dir at ${ZBAR_INCLUDE_DIR}")
#message(STATUS "libzbar: libraries at ${ZBAR_LIBRARIES}")

# Unwind
target_include_directories(monero-gui PUBLIC ${LIBUNWIND_INCLUDE_DIR})
message(STATUS "libunwind: include dir at ${LIBUNWIND_INCLUDE_DIR}")
message(STATUS "libunwind: libraries at ${LIBUNWIND_LIBRARIES}")

# LibUSB
target_include_directories(monero-gui PUBLIC ${LibUSB_INCLUDE_DIRS})
message(STATUS "libusb: include dir at ${LibUSB_INCLUDE_DIRS}")
message(STATUS "libusb: libraries at ${LibUSB_LIBRARIES}")

# HIDApi
target_include_directories(monero-gui PUBLIC ${HIDAPI_INCLUDE_DIRS})
message(STATUS "libhidapi: include dir at ${HIDAPI_INCLUDE_DIRS}")
message(STATUS "libhidapi: libraries at ${HIDAPI_LIBRARIES}")

# Boost
#if(DEBUG)
#    set(Boost_DEBUG ON)
#endif()
target_include_directories(monero-gui PUBLIC ${Boost_INCLUDE_DIRS})
message(STATUS "Using Boost include dir at ${Boost_INCLUDE_DIRS}")
message(STATUS "Using Boost libraries at ${Boost_LIBRARIES}")


target_include_directories(monero-gui PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

file(GLOB_RECURSE SRC_SOURCES *.cpp)
file(GLOB_RECURSE SRC_HEADERS *.h)

target_include_directories(monero-gui PUBLIC
	${CMAKE_SOURCE_DIR}/monero/include
    ${CMAKE_SOURCE_DIR}/monero/src
    #${CMAKE_SOURCE_DIR}/monero
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/daemon
	${CMAKE_CURRENT_SOURCE_DIR}/libwalletqt
	${CMAKE_CURRENT_SOURCE_DIR}/model
	${CMAKE_CURRENT_SOURCE_DIR}/QR-Code-generator
	#${CMAKE_CURRENT_SOURCE_DIR}/QR-Code-scanner
	${CMAKE_CURRENT_SOURCE_DIR}/daemon/zxcvbn-c
    ${LibUSB_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
)

target_compile_definitions(monero-gui
	PUBLIC
	${Qt5Widgets_DEFINITIONS}
    ${Qt5Qml_DEFINITIONS}
)

set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++0x")

target_link_libraries(monero-gui
        ${CMAKE_BINARY_DIR}/lib/libwallet_merged.a
        #${ZBAR_LIBRARIE}  QR
	    ${CMAKE_BINARY_DIR}/monero/external/db_drivers/liblmdb/liblmdb.a
		${CMAKE_SOURCE_DIR}/monero/lib/libepee.a
		${CMAKE_SOURCE_DIR}/monero/lib/libepee_readline.a
        ${CMAKE_BINARY_DIR}/monero/external/unbound/libunbound.a
        ${SODIUM_LIBRARY}
        ${CMAKE_SOURCE_DIR}/monero/lib/libeasylogging.a
        #/usr/lib/x86_64-linux-gnu/libXau.a 
        #/usr/lib/x86_64-linux-gnu/libxcb.a
        #/usr/lib/x86_64-linux-gnu/libX11.a
        ${OPENGL_gl_LIBRARY}
        #${XRANDR_LIBRARIES}
        ${X11_LIBRARIES}
        #/usr/lib/x86_64-linux-gnu/libglib-2.0.so.0
        #${OPENGL_LIBRARY}
        pthread dl Xt xcb X11
        ${Boost_LIBRARIES}
		${OPENSSL_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${LibUSB_LIBRARIES}
		${HIDAPI_LIBRARIES}
		pthread
		Qt5::Core
		Qt5::Quick
		Qt5::Widgets
		Qt5::Gui
		Qt5::Network
		Qt5::Qml
		Qt5::Multimedia
		Qt5::Xml
		Qt5::XmlPatterns
		Qt5::Svg
        )

install(TARGETS monero-gui
		DESTINATION ${CMAKE_INSTALL_PREFIX}
)
