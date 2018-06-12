set ROOT=%CD%
pushd external\imgui_lua_bindings\
perl generate_imgui_bindings.pl < ..\reshade\deps\imgui\imgui.h > "imgui_iterator.cpp"
REM perl generate_imgui_bindings.pl < ..\reshade\deps\imgui\imgui_internal.h >> "imgui_iterator.cpp"
popd
