imgui.SetNextWindowPos(0, 0)
imgui.SetNextWindowSize(500,400)
if imgui.Begin("Test", nil, { "NoTitleBar", "NoResize", "NoMove", "NoBringToFrontOnFocus" }) then
	imgui.End()
end
