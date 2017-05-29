SET(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
SET(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
SET(JSONCPP_WITH_WARNING_AS_ERROR OFF CACHE BOOL "" FORCE)
SET(JSONCPP_WITH_STRICT_ISO OFF CACHE BOOL "" FORCE)
SET(JSONCPP_WITH_PKGCONFIG_SUPPORT "Generate and install .pc files" OFF CACHE BOOL "" FORCE)

include_directories(
	${CMAKE_BUILD_DIR}/external/zlib
	zlib
)

ADD_SUBDIRECTORY(zlib)
ADD_SUBDIRECTORY(jsoncpp)
ADD_SUBDIRECTORY(boost)
ADD_SUBDIRECTORY(lua)

include_directories(
	${CMAKE_BUILD_DIR}/external/zlib
	zlib
)

set_target_properties (zlib PROPERTIES FOLDER external/zlib)
set_target_properties (zlibstatic PROPERTIES FOLDER external/zlib)
set_target_properties (example PROPERTIES FOLDER external/zlib)
set_target_properties (minigzip PROPERTIES FOLDER external/zlib)
set_target_properties (boost PROPERTIES FOLDER external)
set_target_properties (jsoncpp_lib_static PROPERTIES FOLDER external)
set_target_properties (liblua PROPERTIES FOLDER external/lua)
set_target_properties (lua PROPERTIES FOLDER external/lua)
set_target_properties (luac PROPERTIES FOLDER external/lua)
set_target_properties (wlua PROPERTIES FOLDER external/lua)
