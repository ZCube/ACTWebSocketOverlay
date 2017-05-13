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

