SET(STB_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/reshade/deps/stb)

INCLUDE_DIRECTORIES(
	${STB_DIRECTORY}
)

INCLUDE_DIRECTORIES(
	../external/jsoncpp/include
	../external/beast/include
	../external/boost
	../external
)

LINK_DIRECTORIES(
	${Boost_LIBRARY_DIRS}
)

ADD_SUBDIRECTORY(awio)

