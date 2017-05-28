
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
	# ../prebuilt/libressl/include
	../prebuilt/openssl/${address_model}/include
	../prebuilt/v8/include
)

LINK_DIRECTORIES(
	../prebuilt/v8/lib/${address_model}
	# ../prebuilt/libressl/${address_model}
	../prebuilt/openssl/lib/${address_model}
)

ADD_SUBDIRECTORY(awio)

