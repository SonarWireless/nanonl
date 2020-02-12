CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

PROJECT(nanonl C)
INCLUDE(GNUInstallDirs)
INCLUDE(CheckIncludeFile)

INCLUDE_DIRECTORIES(include)

ADD_DEFINITIONS(-std=gnu17 -O2 -fno-strict-aliasing -Wall -Wextra -Wno-unused-parameter -Wno-unused-function)

# make assert() happy
STRING(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

OPTION(ENABLE_GENERIC   "Enable netlink generic support."               ON)
OPTION(ENABLE_80211     "Enable simple nl80211 API. (implies generic)." ON)
OPTION(ENABLE_80211_DBG "Enable debug messages in nl80211 API.        " OFF)
OPTION(ENABLE_NETFILTER "Enable nfnetlink support."                     OFF)
OPTION(ENABLE_NFQUEUE   "Enable nfqueue support (implies netfilter)."   OFF)
OPTION(ENABLE_CONNTRACK "Enable conntrack support (implies netfilter)." OFF)
OPTION(ENABLE_IFINFO    "Enable interface info support."                OFF)
OPTION(ENABLE_IFADDR    "Enable interface address support."             OFF)
OPTION(ENABLE_ND        "Enable neighbor discovery support."            OFF)
OPTION(ENABLE_ALL       "Enable support for everything."                OFF)
OPTION(ENABLE_SAMPLES   "Build sample code."                            OFF)
OPTION(ENABLE_TESTS     "Build test code."                              OFF)

CHECK_INCLUDE_FILE(linux/netlink.h HAVE_LINUX_NETLINK)
IF (NOT HAVE_LINUX_NETLINK)
	MESSAGE(FATAL_ERROR "Could not locate netlink header")
ENDIF()

IF (ENABLE_ALL)
	SET(ENABLE_GENERIC ON)
	SET(ENABLE_80211 ON)
	SET(ENABLE_NFQUEUE ON)
	SET(ENABLE_CONNTRACK ON)
	SET(ENABLE_IFINFO ON)
	SET(ENABLE_IFADDR ON)
	SET(ENABLE_ND ON)
	SET(ENABLE_NETFILTER ON)
ENDIF()

IF (ENABLE_80211)
	SET(ENABLE_GENERIC ON)
	CHECK_INCLUDE_FILE(linux/nl80211.h HAVE_LINUX_NL80211)
	IF (NOT HAVE_LINUX_NL80211)
		MESSAGE(FATAL_ERROR "Could not locate nl80211 header")
	ENDIF()
ENDIF()

IF (ENABLE_NFQUEUE)
	SET(ENABLE_NETFILTER ON)
ENDIF()

IF (ENABLE_CONNTRACK)
	SET(ENABLE_NETFILTER ON)
ENDIF()

IF (ENABLE_80211_DBG AND ENABLE_80211)
	ADD_COMPILE_DEFINITIONS("NL80211_DEBUG=1")
ENDIF()

IF (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
ENDIF()

MESSAGE(STATUS "  Build type:                " ${CMAKE_BUILD_TYPE})
MESSAGE(STATUS "  Enable generic:            " ${ENABLE_GENERIC})
MESSAGE(STATUS "  Enable nl80211:            " ${ENABLE_80211})
MESSAGE(STATUS "  nl80211 debug:             " ${ENABLE_80211_DBG})
MESSAGE(STATUS "  Enable netfilter:          " ${ENABLE_NETFILTER})
MESSAGE(STATUS "  Enable conntrack:          " ${ENABLE_CONNTRACK})
MESSAGE(STATUS "  Enable nfqueue:            " ${ENABLE_NFQUEUE})
MESSAGE(STATUS "  Enable ifinfo:             " ${ENABLE_IFINFO})
MESSAGE(STATUS "  Enable ifaddr:             " ${ENABLE_IFADDR})
MESSAGE(STATUS "  Enable neighbor discovery: " ${ENABLE_ND})
MESSAGE(STATUS "  Enable samples:            " ${ENABLE_SAMPLES})
MESSAGE(STATUS "  Enable tests:              " ${ENABLE_TESTS})

SET(NANO_SRC
	src/nl.c
)

IF (ENABLE_GENERIC)
	LIST(APPEND NANO_SRC src/nl_gen.c)
ENDIF()

IF (ENABLE_80211)
	LIST(APPEND NANO_SRC src/nl_80211.c)
ENDIF()

IF (ENABLE_NETFILTER)
	LIST(APPEND NANO_SRC src/nl_nf.c)
ENDIF()

IF (ENABLE_CONNTRACK)
	LIST(APPEND NANO_SRC src/nl_nfct.c)
ENDIF()

IF (ENABLE_NFQUEUE)
	LIST(APPEND NANO_SRC src/nl_nfqueue.c)
ENDIF()

IF (ENABLE_IFINFO)
	LIST(APPEND NANO_SRC src/nl_ifinfo.c)
ENDIF()

IF (ENABLE_IFADDR)
	LIST(APPEND NANO_SRC src/nl_ifaddr.c)
ENDIF()

IF (ENABLE_ND)
	LIST(APPEND NANO_SRC src/nl_nd.c)
ENDIF()

IF (ENABLE_SAMPLES)
	IF (ENABLE_GENERIC)
		ADD_EXECUTABLE(genl-find-family example/genl-find-family.c $<TARGET_OBJECTS:nlobject>)
	ENDIF()
	IF (ENABLE_CONNTRACK)
		ADD_EXECUTABLE(dump-conntrack        example/dump-conntrack.c        $<TARGET_OBJECTS:nlobject>)
		ADD_EXECUTABLE(monitor-conntrack-del example/monitor-conntrack-del.c $<TARGET_OBJECTS:nlobject>)
	ENDIF()
	IF (ENABLE_IFADDR)
		ADD_EXECUTABLE(dump-ip-addrs       example/dump-ip-addrs.c       $<TARGET_OBJECTS:nlobject>)
		ADD_EXECUTABLE(monitor-addr-change example/monitor-addr-change.c $<TARGET_OBJECTS:nlobject>)
	ENDIF()
	IF (ENABLE_ND)
		ADD_EXECUTABLE(monitor-neighbors example/monitor-neighbors.c $<TARGET_OBJECTS:nlobject>)
		ADD_EXECUTABLE(dump-neighbors    example/dump-neighbors.c    $<TARGET_OBJECTS:nlobject>)
	ENDIF()
	IF (ENABLE_NFQUEUE)
		ADD_EXECUTABLE(nfqueue example/nfqueue.c $<TARGET_OBJECTS:nlobject>)
	ENDIF()
	IF (ENABLE_80211)
		ADD_EXECUTABLE(nl_80211_reg_domain example/nl_80211_reg_domain.c $<TARGET_OBJECTS:nlobject>)
	ENDIF()
ENDIF()

ADD_LIBRARY(nlobject OBJECT ${NANO_SRC})
SET_TARGET_PROPERTIES(nlobject PROPERTIES POSITION_INDEPENDENT_CODE 1)

ADD_LIBRARY(nanonl        SHARED $<TARGET_OBJECTS:nlobject>)
ADD_LIBRARY(nanonl-static STATIC $<TARGET_OBJECTS:nlobject>)
SET_TARGET_PROPERTIES(nanonl-static PROPERTIES OUTPUT_NAME nanonl)

#add_executable(main reg.c $<TARGET_OBJECTS:nlobject>)

CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/nanonl.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/nanonl.pc"
	@ONLY
)

INSTALL(TARGETS nanonl        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(TARGETS nanonl-static LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(
	DIRECTORY ${CMAKE_SOURCE_DIR}/include/nanonl
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nanonl
	FILES_MATCHING PATTERN "*.h"
)
