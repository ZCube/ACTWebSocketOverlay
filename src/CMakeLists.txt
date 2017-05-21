SET(STB_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/reshade/deps/stb)

INCLUDE_DIRECTORIES(
	${STB_DIRECTORY}
)

INCLUDE_DIRECTORIES(
	../external/jsoncpp/include
	${Boost_INCLUDE_DIRS}
	../external/beast/include
	../external
)

LINK_DIRECTORIES(
	${Boost_LIBRARY_DIRS}
)

ADD_SUBDIRECTORY(awio)

