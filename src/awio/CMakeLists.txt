SET(IMGUI_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/reshade/deps/imgui)
SET(STB_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/reshade/deps/stb)
SET(NUKLEAR_DIRECTORY ${CMAKE_HOME_DIRECTORY}/external/nuklear)

INCLUDE_DIRECTORIES(
	${IMGUI_DIRECTORY}
	${STB_DIRECTORY}
	${NUKLEAR_DIRECTORY}
)

IF(USE_SSL)
	ADD_DEFINITIONS(
		-DUSE_SSL
	)
ENDIF(USE_SSL)

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
	Table.h
	Table.cpp
	overlay.cpp
	overlay.def
	Resource.rc
	resource.h
	version.h
)

TARGET_LINK_LIBRARIES(ActWebSocketImguiOverlay
	jsoncpp_lib_static
	boost
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
	ActWebSocketImguiOverlayWithLua
	ActWebSocketImguiOverlay
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
	ActWebSocketImguiOverlayWithLua
	ActWebSocketImguiOverlay
	d3d9.lib
	d3dcompiler.lib
)

include_directories(${CMAKE_HOME_DIRECTORY}/external/imgui_lua_bindings)

ADD_DEFINITIONS(
	-DENABLE_IM_LUA_END_STACK
)

file(GLOB IMGUI_SRC
	${IMGUI_DIRECTORY}/im*.h
	${IMGUI_DIRECTORY}/im*.cpp
)
file(GLOB STB_SRC
	${IMGUI_DIRECTORY}/stb_*.h
)

ADD_LIBRARY(ActWebSocketImguiOverlayWithLua
	SHARED
	${IMGUI_SRC}
	${STB_SRC}
	imgui_lua.h
	imgui_lua.cpp
	sandbox.cpp
	overlay.def
	Resource.rc
	Serializable.h
	Serializable.cpp
	Preference.h
	Preference.cpp
	Table.h
	Table.cpp
	AWIOOverlay.h
	AWIOOverlay.cpp
	OverlayContext.h
	OverlayContextLua.h
	OverlayContextLua.cpp
	OverlayContextBase.h
	OverlayContextBase.cpp
	OverlayMain.h
	OverlayMain.cpp
	resource.h
	version.h
)

source_group(lib/imgui FILES
	${IMGUI_SRC}
)
source_group(lib/stb FILES
	${STB_SRC}
)

TARGET_LINK_LIBRARIES(ActWebSocketImguiOverlayWithLua
	liblua
	jsoncpp_lib_static
	boost
)

IF(USE_SSL)
	TARGET_LINK_LIBRARIES(ActWebSocketImguiOverlayWithLua
		libeay32.lib
		ssleay32.lib
	)
	TARGET_LINK_LIBRARIES(ActWebSocketImguiOverlay
		libeay32.lib
		ssleay32.lib
	)
ENDIF(USE_SSL)
