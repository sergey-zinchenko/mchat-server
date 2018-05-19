project(MChat-Server)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

set (JSON_FIND_REQUIRED on)
set (LIBEV_FIND_REQUIRED on)
set (LIBEV_FIND_QUIETLY on)

FIND_PACKAGE(LIBEV)
FIND_PACKAGE(OpenSSL)
FIND_PACKAGE(JSON)
FIND_PACKAGE(UUID)

set(MC_SOURCES
	base64.c
	main.c
	srvclientutils.c
	srvcore.c
	srvevents.c
	srvsockutils.c
	sys_messages.c
)

set(MC_HEADERS
	base64.h
	main.h
	srvclientutils.h
	srvcore.h
	srvevents.h
	srvsockutils.h
	srvtypes.h
	sys_messages.h
)

include_directories(
	${LIBEV_INCLUDE_DIR}
	${JSON_INCLUDE_DIRS}
	${OPENSSL_INCLUDE_DIRS}
	${UUID_INCLUDE_DIRS}
)

add_executable( MChat-Server 
	${MC_SOURCES}
	${MC_HEADERS}
)

target_link_libraries( MChat-Server 
	${LIBEV_LIBRARIES} 
	${JSON_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	${UUID_LIBRARIES}
)