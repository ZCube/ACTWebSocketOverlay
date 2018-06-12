SET(STB_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/reshade/deps/stb)

INCLUDE_DIRECTORIES(
	${STB_DIRECTORY}
)

INCLUDE_DIRECTORIES(
	../external/jsoncpp/include
	../external/beast/include
	../external/boost
	../external/lua/src
	${CMAKE_BINARY_DIR}/external/lua
	../external
	../prebuilt/openssl/${address_model}/include
)

LINK_DIRECTORIES(
	../prebuilt/openssl/${address_model}/lib
)

ADD_SUBDIRECTORY(awio)

