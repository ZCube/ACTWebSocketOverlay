SET(BUILD_SHARED_LIBS OFF CACHE STRING "" FORCE)
SET(BUILD_STATIC_LIBS ON CACHE STRING "" FORCE)

ADD_SUBDIRECTORY(lua)
set_target_properties ( liblua PROPERTIES OUTPUT_NAME liblua CLEAN_DIRECT_OUTPUT 1 )

set_target_properties (liblua PROPERTIES FOLDER external/lua)
set_target_properties (lua PROPERTIES FOLDER external/lua)
set_target_properties (luac PROPERTIES FOLDER external/lua)
set_target_properties (wlua PROPERTIES FOLDER external/lua)

