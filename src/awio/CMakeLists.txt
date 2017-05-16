INCLUDE_DIRECTORIES(
	${CMAKE_HOME_DIRECTORY}/external/imgui
)

ADD_LIBRARY(ActWebSocketImguiOverlay
	SHARED
	${CMAKE_HOME_DIRECTORY}/external/imgui/imconfig.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui.cpp
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_draw.cpp
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_internal.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/stb_rect_pack.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/stb_textedit.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/stb_truetype.h
	overlay.cpp
	overlay.def
	Resource.rc
	resource.h
)

TARGET_LINK_LIBRARIES(ActWebSocketImguiOverlay
	jsoncpp_lib_static
	${Boost_LIBRARIES}
)

include_directories(
	${CMAKE_HOME_DIRECTORY}/external/imgui/examples/directx11_example
)

ADD_EXECUTABLE(test_main
	${CMAKE_HOME_DIRECTORY}/external/imgui/imconfig.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui.cpp
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_demo.cpp
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_draw.cpp
	${CMAKE_HOME_DIRECTORY}/external/imgui/examples/directx11_example/imgui_impl_dx11.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/examples/directx11_example/imgui_impl_dx11.cpp
	${CMAKE_HOME_DIRECTORY}/external/imgui/imgui_internal.h
	main.cpp
	Resource.rc
	resource.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/stb_rect_pack.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/stb_textedit.h
	${CMAKE_HOME_DIRECTORY}/external/imgui/stb_truetype.h
)

TARGET_LINK_LIBRARIES(test_main
	jsoncpp_lib_static
	${Boost_LIBRARIES}
	ActWebSocketImguiOverlay
	d3d11.lib
	d3dcompiler.lib
	dxgi.lib
)
