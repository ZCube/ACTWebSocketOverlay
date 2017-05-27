SET(STB_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/reshade/deps/stb)

INCLUDE_DIRECTORIES(
	${STB_DIRECTORY}
)

INCLUDE_DIRECTORIES(
	../external/jsoncpp/include
	../external/beast/include
	../external/boost
	../external/v8pp
	../external
	../prebuilt/v8/include
)

LINK_DIRECTORIES(
	../prebuilt/v8/lib/${address_model}
)

ADD_SUBDIRECTORY(awio)

