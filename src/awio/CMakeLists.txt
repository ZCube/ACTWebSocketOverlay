SET(IMGUI_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/reshade/deps/imgui)
SET(STB_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/reshade/deps/stb)
SET(NUKLEAR_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/nuklear)

INCLUDE_DIRECTORIES(
	${IMGUI_DIRECTORY}
	${STB_DIRECTORY}
	${NUKLEAR_DIRECTORY}
)

ADD_LIBRARY(ActWebSocketImguiOverlay
	SHARED
	${IMGUI_DIRECTORY}/imconfig.h
	${IMGUI_DIRECTORY}/imgui.cpp
	${IMGUI_DIRECTORY}/imgui.h
	${IMGUI_DIRECTORY}/imgui_draw.cpp
	${IMGUI_DIRECTORY}/imgui_internal.h
	${IMGUI_DIRECTORY}/stb_rect_pack.h
	${IMGUI_DIRECTORY}/stb_textedit.h
	${IMGUI_DIRECTORY}/stb_truetype.h
	overlay.cpp
	overlay.def
	Resource.rc
	resource.h
	version.h
)

TARGET_LINK_LIBRARIES(ActWebSocketImguiOverlay
	jsoncpp_lib_static
	boost
	libeay32.lib
	ssleay32.lib
#	libcrypto-41.lib
#	libtls-15.lib
#	libssl-43.lib
)

ADD_LIBRARY(loader
	SHARED
	loader.cpp
	overlay.def
	Resource.rc
	resource.h
	version.h
)

TARGET_LINK_LIBRARIES(loader
)

include_directories(
	${IMGUI_DIRECTORY}/examples/directx11_example
	${IMGUI_DIRECTORY}/examples/directx9_example
)

ADD_EXECUTABLE(test_main
	${IMGUI_DIRECTORY}/imconfig.h
	${IMGUI_DIRECTORY}/imgui.cpp
	${IMGUI_DIRECTORY}/imgui.h
	${IMGUI_DIRECTORY}/imgui_demo.cpp
	${IMGUI_DIRECTORY}/imgui_draw.cpp
	${IMGUI_DIRECTORY}/examples/directx11_example/imgui_impl_dx11.h
	#${IMGUI_DIRECTORY}/examples/directx11_example/imgui_impl_dx11.cpp
	${IMGUI_DIRECTORY}/imgui_internal.h
	main.cpp
	Resource.rc
	resource.h
	${IMGUI_DIRECTORY}/stb_rect_pack.h
	${IMGUI_DIRECTORY}/stb_textedit.h
	${IMGUI_DIRECTORY}/stb_truetype.h
)

TARGET_LINK_LIBRARIES(test_main
	jsoncpp_lib_static
	boost
	ActWebSocketImguiOverlay
	Sandbox
	d3d11.lib
	d3dcompiler.lib
	dxgi.lib
)

ADD_EXECUTABLE(test_main_dx9
	${IMGUI_DIRECTORY}/imconfig.h
	${IMGUI_DIRECTORY}/imgui.cpp
	${IMGUI_DIRECTORY}/imgui.h
	${IMGUI_DIRECTORY}/imgui_demo.cpp
	${IMGUI_DIRECTORY}/imgui_draw.cpp
	${IMGUI_DIRECTORY}/examples/directx9_example/imgui_impl_dx9.h
	#${IMGUI_DIRECTORY}/examples/directx9_example/imgui_impl_dx9.cpp
	${IMGUI_DIRECTORY}/imgui_internal.h
	main_dx9.cpp
	Resource.rc
	resource.h
	${IMGUI_DIRECTORY}/stb_rect_pack.h
	${IMGUI_DIRECTORY}/stb_textedit.h
	${IMGUI_DIRECTORY}/stb_truetype.h
)

TARGET_LINK_LIBRARIES(test_main_dx9
	jsoncpp_lib_static
	boost
	ActWebSocketImguiOverlay
	Sandbox
	d3d9.lib
	d3dcompiler.lib
)


ADD_LIBRARY(Sandbox
	SHARED
	${IMGUI_DIRECTORY}/imconfig.h
	${IMGUI_DIRECTORY}/imgui.cpp
	${IMGUI_DIRECTORY}/imgui.h
	${IMGUI_DIRECTORY}/imgui_draw.cpp
	${IMGUI_DIRECTORY}/imgui_internal.h
	${IMGUI_DIRECTORY}/stb_rect_pack.h
	${IMGUI_DIRECTORY}/stb_textedit.h
	${IMGUI_DIRECTORY}/stb_truetype.h
	sandbox.cpp
	overlay.def
	Resource.rc
	resource.h
	version.h
)

TARGET_LINK_LIBRARIES(Sandbox
	jsoncpp_lib_static
	boost
	v8.dll.lib
	v8_libbase.dll.lib
	v8_libplatform.dll.lib
)
