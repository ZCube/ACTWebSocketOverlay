--
-- Insert attributes of icon, color, display_name, owner to combatant JSON data.
--

charName = "YOU"
combatant = {}
lastMessage = "{}"
encounter = {}
rdps = "0"
rhps = "0"
zone = "Unknown"
duration = "00:00"
-- icon, color, name, act_name
table_name = {'act_name', 'icon', 'color', 'display_name', 'owner'}
table_info = {'name', 'Job', 'name', 'name', 'name', 'name'}

function trim(s)
  return s:match'^%s*(.*%S)' or ''
end

name_to_job_map = {}

--rook
name_to_job_map["auto-tourelle tour"] = "rook"
name_to_job_map["selbstschuss-gyrocopter läufer"] = "rook"
name_to_job_map["オートタレット・ルーク"] = "rook"
name_to_job_map["자동포탑 룩"] = "rook"
name_to_job_map["rook autoturret"] = "rook"

--bishop
name_to_job_map["auto-tourelle fou"] = "bishop"
name_to_job_map["selbstschuss-gyrocopter turm"] = "bishop"
name_to_job_map["オートタレット・ビショップ"] = "bishop"
name_to_job_map["자동포탑 비숍"] = "bishop"
name_to_job_map["bishop autoturret"] = "bishop"

--emerald
name_to_job_map["emerald carbuncle"] = "emerald"
name_to_job_map["카벙클 에메랄드"] = "emerald"
name_to_job_map["カーバンクル・エメラルド"] = "emerald"

--topaz
name_to_job_map["topaz carbuncle"] = "topaz"
name_to_job_map["카벙클 토파즈"] = "topaz"
name_to_job_map["カーバンクル・トパーズ"] = "topaz"

--eos
name_to_job_map["eos"] = "eos"
name_to_job_map["フェアリー・エオス"] = "eos"
name_to_job_map["요정 에오스"] = "eos"

--selene
name_to_job_map["selene"] = "selene"
name_to_job_map["フェアリー・セレネ"] = "selene"
name_to_job_map["요정 셀레네"] = "selene"

--garuda
name_to_job_map["garuda-egi"] = "garuda"
name_to_job_map["ガルーダ・エギ"] = "garuda"
name_to_job_map["가루다 에기"] = "garuda"

--ifrit
name_to_job_map["ifrit-egi"] = "ifrit"
name_to_job_map["イフリート・エギ"] = "ifrit"
name_to_job_map["이프리트 에기"] = "ifrit"

--titan
name_to_job_map["titan-egi"] = "titan"
name_to_job_map["タイタン・エギ"] = "titan"
name_to_job_map["타이탄 에기"] = "titan"

name_to_job_map["Limit Break"] = "limit break";

function script(root)
	-- from websocket
	-- root = jsonDecode(data)
	
	if root then
		-- get user name
		if root["type"] == "broadcast" and root["msgtype"] == "SendCharName" then
			charName = root["msg"]["charName"]
		end
		-- get combatant info
		if root["type"] == "broadcast" and root["msgtype"] == "CombatData" then
				combatant = root["msg"]["Combatant"]
				encounter = root["msg"]["Encounter"]
				rdps = encounter["encdps"]
				rhps = encounter["enchps"]
				zone = encounter["CurrentZoneName"]
				duration = encounter["duration"]
		end
	end
	
	-- -- preference test
	-- setColor3(storage, "Test1", 0.2, 0.3, 0.4);
	-- x,y,z = getColor3(storage, "Test1")
	-- print(x,y,z)
	-- setColor3(storage, "Test1", 0.1, 0.2, 0.3);
	-- x,y,z = getColor3(storage, "Test1")
	-- print(x,y,z)

	-- setColor4(storage, "Test2", 0.2, 0.3, 0.4, 0.5);
	-- x,y,z,w = getColor4(storage, "Test2")
	-- print(x,y,z,w)
	-- setColor4(storage, "Test2", 0.1, 0.2, 0.3, 0.4);
	-- x,y,z,w = getColor4(storage, "Test2")
	-- print(x,y,z,w)

	-- setString(storage, "Test3", "t1");
	-- x = getString(storage, "Test3")
	-- print(x)
	-- setString(storage, "Test3", "t2");
	-- x = getString(storage, "Test3")
	-- print(x)
	
	-- setBoolean(storage, "Test4", false);
	-- x = getBoolean(storage, "Test4")
	-- print(x)
	-- setBoolean(storage, "Test4", true);
	-- x = getBoolean(storage, "Test4")
	-- print(x)
	
	-- setInt(storage, "Test5", 1);
	-- x = getInt(storage, "Test5")
	-- print(x)
	-- setInt(storage, "Test5", 5);
	-- x = getInt(storage, "Test5")
	-- print(x)
	
	-- setFloat(storage, "Test6", 1.2);
	-- x = getFloat(storage, "Test6")
	-- print(x)
	-- setFloat(storage, "Test6", 2.4);
	-- x = getFloat(storage, "Test6")
	-- print(x)
	-- -- preference test end
	
	-- return table
	ret = {}
	
	-- combatant check
	if combatant then
		for key,value in pairs(combatant) do
			data = {}
			-- get icon, color, display_name, owner
			for idx,col in pairs(table_info) do
				table.insert (data, value[col])
			end
			
			cnt = 0
			-- default owner
			data[5] = ""
			-- name seperating "name (owner)"
			nameonly = data[4]
			for word in string.gmatch(data[4], "[ ]*([^()]+)[ ]*") do
				if cnt == 0 then
					-- display_name
					nameonly = trim(word)
				else
					-- owner
					data[5] = trim(word)
				end
				cnt = cnt + 1
			end
			
			-- icon setting
			if name_to_job_map[nameonly] ~= null then
				data[2] = name_to_job_map[nameonly]
			end
			-- color setting from icon
			data[3] = data[2]
			-- 'YOU' and 'Limit Break' Color from name
			if data[4] == "YOU" or data[4] == "Limit Break" then
				data[3] = data[4]
			end
			-- icon to lower
			data[2] = data[2]:lower()
			-- color to lower
			data[3] = data[3]:lower()

			-- display_name setting
			if data[4] == "YOU" then
				data[4] = charName
			end
			
			-- return table
			ret[data[4]] = combatant[data[1]]
			for i = 2,5 do
				-- append color, display_name, owner
				ret[data[4]][table_name[i]] = data[i]
			end
		end
	end
	
	-- send to render.lua
	tbl = {combatant = ret, rdps = rdps, rhps = rhps, zone = zone,duration = duration }
	return tbl
end

