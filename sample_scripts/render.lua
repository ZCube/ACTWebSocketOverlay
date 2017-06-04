-- just simple example

_G.charName = "YOU"
--icon, color, name, dps, maxhit
columns_length = 4
columns = {'icon', 'color', 'display_name', 'owner'}
columns_width = {100,100,100,100}

function string.fromhex(str)
    return (str:gsub('..', function (cc)
        return string.char(tonumber(cc, 16))
    end))
end

function string.tohex(str)
    return (str:gsub('.', function (c)
        return string.format('%02X', string.byte(c))
    end))
end

function render(use_input, root)
	flags = ImGuiWindowFlags_NoTitleBar
	if(not use_input) then
		flags = flags | ImGuiWindowFlags_NoInputs
	end
	
	shoulddraw, p_opened = imgui.Begin("test###"..window_id, nil, flags)
	w, h, uv0x, uv0y, uv1x, uv1y = getImage("center")
	if shoulddraw then
		tbl = {}
		if root then
			_G.combatant = root["combatant"]
		end
		if _G.combatant then
			for key,value in pairs(_G.combatant) do --pseudocode
				row = {}
				for idx,col in pairs(columns) do
					table.insert (row, value[col])
					-- Text
					imgui.Text(value[col])
				end
				table.insert(tbl, row)
				w, h, uv0x, uv0y, uv1x, uv1y = getImage(row[1])
				-- Image
				imgui.Image(texture_id, 300, 300, uv0x, uv0y, uv1x, uv1y, 1,1,1,1, 0,0,0,0)
				-- DrawList_AddImage
				imgui.DrawList_AddImage(texture_id, winx, winy, winx+150, winy+150, uv0x, uv0y, uv1x, uv1y, tonumber("ffffffff", 16))
			end
			winx, winy = imgui.GetWindowPos()
			-- GetWindowContentRegionWidth
			width = imgui.GetWindowContentRegionWidth()
			-- GetWindowPos
			winx, winy = imgui.GetWindowPos()
			for key,value in pairs(_G.combatant) do --pseudocode
				for idx,col in pairs(columns) do
					table.insert (row, value[col])
				end
				table.insert(tbl, row)
			end
		end
		winx, winy = imgui.GetWindowPos()
		imgui.DrawList_AddText(winx + 40, winy + 40, tonumber("ffffffff", 16), "AddText test")
		height = 20
		imgui.Text(imgui.GetWindowContentRegionWidth())
		winx, winy = imgui.GetWindowPos()
	end
	imgui.End()
	
	-- dump to debug window
	-- val = jsonEncodePretty(root)
	-- imgui.Text(val)
end
