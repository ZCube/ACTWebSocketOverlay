#include <imgui.h>
#include "imgui_internal.h"
#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <json/json.h>
#include <fstream>
#include <algorithm>
#include <thread>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <beast/core.hpp>
#include <beast/websocket.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>

static boost::mutex mutex;
static bool show_name = true;
static int column_max = 3;
static int global_opacity = 100;
static char websocket_port[256] = { 0, };

class Table
{
public:
	class Column
	{
	public:
		Column(std::string title_, std::string index_, int size_, int sizeWeight_, ImVec2 align_ = ImVec2(0.5f,0.5f), bool visible_ = true)
			: Title(title_)
			, index(index_)
			, size(size_)
			, sizeWeight(sizeWeight_)
			, align(align_)
			, visible(visible_)
		{

		}
		int size = 0;
		int sizeWeight = 0;
		std::string Title = "";
		std::string index = "";
		bool visible = true;
		ImVec2 align = ImVec2(0.5f, 0.5f);
	};

	void UpdateColumnWidth(int width, int column_max)
	{
		int columnSizeWeightSum = 0;
		int columnSizeFixed = 0;
		for (int i = 0; i < column_max; ++i) {
			if (columns[i].sizeWeight != 0)
			{
				columnSizeWeightSum += columns[i].sizeWeight;
			}
			else
			{
				columnSizeFixed += columns[i].size;
			}
		}
		for (int i = 0; i < column_max; ++i) {
			if (columns[i].sizeWeight != 0)
			{
				columns[i].size = ((width - columnSizeFixed) * columns[i].sizeWeight) / columnSizeWeightSum;
			}
		}
	}

	std::vector<Column> columns;
	std::vector<std::vector<std::string> > values;

	int progressColumn = 2;
	float maxValue = 0;
};

typedef std::map<std::string, ImVec4> ColorMapType;
static ColorMapType colorMap;
std::map<std::string, std::string> iconMap;
static std::string Title;
static Table dealerTable;
static Table healerTable;

ImVec4 htmlCodeToImVec4(const std::string hex)
{
	unsigned char dat[256] = { 0, };
	boost::algorithm::unhex(hex, dat);
	return ImVec4((float)dat[0] / 255.0f, (float)dat[1] / 255.0f, (float)dat[2] / 255.0f, 1.0f);
}

const std::string ImVec4TohtmlCode(ImVec4 val)
{
	unsigned char dat[256] = { 0, };
	dat[0] = (unsigned char)std::min(255, std::max(0, (int)(val.x * 255.0f)));
	dat[1] = (unsigned char)std::min(255, std::max(0, (int)(val.y * 255.0f)));
	dat[2] = (unsigned char)std::min(255, std::max(0, (int)(val.z * 255.0f)));
	std::string ret;
	boost::algorithm::hex(dat, dat+3, std::back_inserter(ret));
	return ret;
}

inline static void websocketThread()
{
	auto func = []()
	{
		while (true)
		{
			boost::asio::io_service ios;
			boost::asio::ip::tcp::resolver r{ ios };
			try {
				// localhost only !
				std::string const host = "127.0.0.1";
				boost::asio::ip::tcp::socket sock{ ios };
				auto endpoint = r.resolve(boost::asio::ip::tcp::resolver::query{ host, websocket_port });
				std::string s = endpoint->service_name();
				uint16_t p = endpoint->endpoint().port();
				boost::asio::connect(sock, endpoint);

				beast::websocket::stream<boost::asio::ip::tcp::socket&> ws{ sock };
				std::string host_port = host + ":" + websocket_port;
				ws.handshake(host_port, "/MiniParse");

				while (sock.is_open())
				{

					beast::multi_buffer b;
					beast::websocket::opcode op;
					ws.read(op, b);

					std::string message_str;
					message_str = boost::lexical_cast<std::string>(beast::buffers(b.data()));
					// debug
					//std::cout << beast::buffers(b.data()) << "\n";
					b.consume(b.size());
					if (message_str.size() == 1)
					{
						ws.write(boost::asio::buffer(std::string(".")));
					}
					else
					{
						{
							Json::Value root;
							Json::Reader r;

							if (r.parse(message_str, root))
							{
								std::vector<std::vector<std::string> > _values;
								if (root["type"].asString() == "broadcast" && root["msgtype"].asString() == "CombatData")
								{
									float _maxValue = 0;
									Table& table = dealerTable;
									{
										_values.clear();
										Json::Value combatant = root["msg"]["Combatant"];
										Json::Value encounter = root["msg"]["Encounter"];

										Title = encounter["CurrentZoneName"].asString();

										for (auto i = combatant.begin(); i != combatant.end(); ++i)
										{
											std::vector<std::string> vals;
											for (int j = 0; j < table.columns.size(); ++j)
											{
												vals.push_back((*i)[table.columns[j].index].asString());
											}
											_maxValue = std::max<float>(atof(vals[table.progressColumn].c_str()), _maxValue);

											_values.push_back(vals);
										}

										std::sort(_values.begin(), _values.end(), [](const std::vector<std::string>& vals0, const std::vector<std::string>& vals1)
										{
											return atof(vals0[2].c_str()) > atof(vals1[2].c_str());
										});
									}
									mutex.lock();
									dealerTable.values = _values;
									dealerTable.maxValue = _maxValue;
									mutex.unlock();
								}
							}
						}
					}
				}
			}
			catch (std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
			catch (...)
			{

			}
			Sleep(5000);
		}
	};
	std::thread* t(new std::thread(func));
}

extern "C" int ModInit(ImGuiContext* context)
{
	ImGui::SetCurrentContext(context);
	WCHAR result[MAX_PATH] = {};
	GetWindowsDirectoryW(result, MAX_PATH);
	boost::filesystem::path p = result;

	GetModuleFileNameW(NULL, result, MAX_PATH);
	boost::filesystem::path m = result;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::GetIO().Fonts->AddFontDefault();
	ImFontConfig config;
	config.MergeMode = true;
	ImFontConfig configMerge;
	configMerge.MergeMode = true;
	struct stat buffer;

	static const ImWchar ranges[] =
	{
		0x25A0, 0x25FF,
		0,
	};

	if (boost::filesystem::exists(p / "Fonts" / "gulim.ttc"))
		io.Fonts->AddFontFromFileTTF((p / "Fonts" / "gulim.ttc").string().c_str(), 13.0f, &config, ranges);
	if (boost::filesystem::exists(p / "Fonts" / "ArialUni.ttf"))
		io.Fonts->AddFontFromFileTTF((p / "Fonts" / "ArialUni.ttf").string().c_str(), 15.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	if (boost::filesystem::exists(p / "Fonts" / "gulim.ttc"))
		io.Fonts->AddFontFromFileTTF((p / "Fonts" / "gulim.ttc").string().c_str(), 13.0f, &config, io.Fonts->GetGlyphRangesKorean());
	else if (boost::filesystem::exists(p / "Fonts" / "NanumBarunGothic.ttf"))
		io.Fonts->AddFontFromFileTTF((p / "Fonts" / "NanumBarunGothic.ttf").string().c_str(), 15.0f, &config, io.Fonts->GetGlyphRangesKorean());
	else if (boost::filesystem::exists(m.parent_path() / "NanumBarunGothic.ttf"))
		io.Fonts->AddFontFromFileTTF((m.parent_path() / "NanumBarunGothic.ttf").string().c_str(), 15.0f, &config, io.Fonts->GetGlyphRangesKorean());

	dealerTable.columns.push_back(Table::Column("Job", "Job", 30, 0, ImVec2(0.5f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Name", "name", 0, 1, ImVec2(0.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("DPS", "encdps", 50, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("D%%", "damage%", 40, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Damage", "damage", 50, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Swing", "swings", 40, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Miss", "misses", 40, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("D.CRIT", "crithit%", 40, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Death", "deaths", 40, 0, ImVec2(1.0f, 0.5f)));

	// default color map
	colorMap["Pld"] = htmlCodeToImVec4("7B9AA2");
	colorMap["Gld"] = htmlCodeToImVec4("7B9AA2");

	colorMap["War"] = htmlCodeToImVec4("A91A16");
	colorMap["Mrd"] = htmlCodeToImVec4("A91A16");

	colorMap["Drk"] = htmlCodeToImVec4("682531");

	colorMap["Mnk"] = htmlCodeToImVec4("B38915");
	colorMap["Pgl"] = htmlCodeToImVec4("B38915");

	colorMap["Drg"] = htmlCodeToImVec4("3752D8");
	colorMap["Lnc"] = htmlCodeToImVec4("3752D8");

	colorMap["Nin"] = htmlCodeToImVec4("EE2E48");
	colorMap["Rog"] = htmlCodeToImVec4("EE2E48");

	colorMap["Brd"] = htmlCodeToImVec4("ADC551");
	colorMap["Arc"] = htmlCodeToImVec4("ADC551");

	colorMap["Mch"] = htmlCodeToImVec4("148AA9");

	colorMap["Blm"] = htmlCodeToImVec4("674598");
	colorMap["Thm"] = htmlCodeToImVec4("674598");


	colorMap["Whm"] = htmlCodeToImVec4("BDBDBD");
	colorMap["Cnj"] = htmlCodeToImVec4("BDBDBD");

	colorMap["Smn"] = htmlCodeToImVec4("32670B");
	colorMap["Acn"] = htmlCodeToImVec4("32670B");
	
	colorMap["Sch"] = htmlCodeToImVec4("32307B");

	colorMap["Ast"] = htmlCodeToImVec4("B1561C");
	colorMap["Limit Break"] = htmlCodeToImVec4("FFBB00");
	colorMap["YOU"] = htmlCodeToImVec4("FF5722");

	// default port
	strcpy(websocket_port, "10501");

	{
		GetModuleFileNameW(NULL, result, MAX_PATH);
		boost::filesystem::path m = result;

		Json::Reader r;
		Json::Value setting;
		try {
			std::ifstream fin((m.parent_path() / "mod.json").wstring());
			if (r.parse(fin, setting))
			{
				global_opacity = setting.get("opacity", Json::Int(100)).asInt();
				show_name = setting.get("show_name", true).asBool();
				column_max = setting.get("column_max", Json::Int(3)).asInt();
				strcpy(websocket_port, setting.get("websocket_port", "10501").asCString());
				Json::Value color = setting.get("color_map", Json::Value());
				for (auto i = color.begin(); i != color.end(); ++i)
				{
					colorMap[i.key().asString()] = htmlCodeToImVec4(i->asString());
				}
			}
			fin.close();
		}
		catch (...)
		{
		}
	}
	websocketThread();
	return 0;
}

extern "C" int ModUnInit(ImGuiContext* context)
{
	ImGui::SetCurrentContext(context);

	WCHAR result[MAX_PATH] = {};
	GetModuleFileNameW(NULL, result, MAX_PATH);
	boost::filesystem::path m = result;

	Json::StyledWriter w;
	Json::Value setting;
	setting["opacity"] = global_opacity;
	setting["show_name"] = show_name;
	setting["column_max"] = column_max;
	setting["websocket_port"] = websocket_port;
	Json::Value color;
	for (auto i = colorMap.begin(); i != colorMap.end(); ++i)
	{
		color[i->first] = ImVec4TohtmlCode(i->second);
	}
	setting["color_map"] = color;

	std::ofstream fout((m.parent_path() /"mod.json").wstring());
	fout << w.write(setting);
	fout.close();
	return 0;
}

void RenderTable(Table& table)
{
	// hard coded....
	const ImGuiStyle& style = ImGui::GetStyle();
	int windowWidth = ImGui::GetWindowSize().x - style.ItemInnerSpacing.x * 2.0f;
	dealerTable.UpdateColumnWidth(windowWidth, column_max);

	ImGui::Columns(table.columns.size(), "mixed");
	int offset = 0;
	for (int i = 0; i < column_max; ++i)
	{
		ImGui::SetColumnOffset(i, offset);
		offset += table.columns[i].size;
	}
	for (int i = column_max; i < table.columns.size(); ++i)
	{
		ImGui::SetColumnOffset(i, offset);
	}
	for (int i = 0; i < column_max; ++i)
	{
		if (i == 1)
		{
			if (ImGui::SmallButton("Name")) show_name = !show_name;
		}
		else
		{
			ImGui::Text(table.columns[i].Title.c_str());
		}
		ImGui::NextColumn();
	}
	for (int i = column_max; i < table.columns.size(); ++i)
	{
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::Separator();

	int base = ImGui::GetCursorPosY();
	const int height = 20;
	for (int i = 0; i < table.values.size(); ++i)
	{
		const std::string& jobStr =  table.values[i][0];
		const std::string& progressStr =  table.values[i][2];
		const std::string& nameStr = table.values[i][1];

		if (table.maxValue == 0.0)
			table.maxValue = 1.0;
		float progress = atof(progressStr.c_str()) / table.maxValue;

		ImVec4 progressColor = ImVec4(0, 0, 0, 1);
		ColorMapType::iterator ji;
		if ((ji = colorMap.find(jobStr)) != colorMap.end())
		{
			progressColor = ji->second;
		}
		if ((ji = colorMap.find(nameStr)) != colorMap.end())
		{
			progressColor = ji->second;
		}
		progressColor.w = (float)global_opacity / 100.0f;
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::SetCursorPos(ImVec2(0, base + i * height));
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 pos = ImGui::GetCursorPos();
		pos.x += winpos.x + style.ItemInnerSpacing.x;
		pos.y += winpos.y;
		ImRect bb(pos, ImVec2(pos.x + windowWidth, pos.y + height));
		ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0.5, 0.5, 0.5, 0.0)), true, style.FrameRounding);

		ImRect bb2(pos, ImVec2(pos.x + windowWidth * progress, pos.y + height));
		ImGui::RenderFrame(bb2.Min, bb2.Max, ImGui::GetColorU32(progressColor), true, style.FrameRounding);

		int basex = 0;
		for (int j = 0; j < column_max; ++j)
		{
			ImGui::SetCursorPos(ImVec2(basex + style.ItemInnerSpacing.x, base + i * height));
			ImVec2 winpos = ImGui::GetWindowPos();
			ImVec2 pos = ImGui::GetCursorPos();
			pos.x += winpos.x;
			pos.y += winpos.y;
			ImRect clip, align;
			ImRect bb(pos, ImVec2(pos.x + windowWidth, pos.y + height));

			if (j != 1 || show_name)
			{
				std::string text = table.values[i][j];
				if (iconMap.find(text) != iconMap.end())
				{
					text = iconMap[text];
				}
				//ImGuiCol_Text
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0, 0.0, 0.0, 1.0));
				ImGui::RenderTextClipped(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + table.columns[j].size + 1, pos.y + height + 1),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					table.columns[j].align,
					nullptr);
				ImGui::PopStyleColor();
				ImGui::RenderTextClipped(pos, ImVec2(pos.x + table.columns[j].size, pos.y + height),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					table.columns[j].align,
					nullptr);
			}

			basex += table.columns[j].size;
		}
		ImGui::NewLine();
		ImGui::SetCursorPos(ImVec2(0, base + (i + 1) * height));
	}

	ImGui::Separator();

}

extern "C" int ModRender(ImGuiContext* context)
{
	try {
		ImGui::SetCurrentContext(context);

		{
			int next_column_max = column_max;
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2, 0.2, 0.2, (float)global_opacity / 100.0f));
			ImGui::Begin("AWIO (ActWebSocket ImGui Overlay)", nullptr, ImVec2(300, 500), -1,
				ImGuiWindowFlags_ShowBorders);

			mutex.lock();
			ImGui::Text(Title.c_str());

			if (ImGui::TreeNode("Preferences"))
			{
				if (ImGui::SmallButton("Mode")) next_column_max = column_max == 9 ? 3 : 9;
				ImGui::SliderInt("Opacity", &global_opacity, 0, 100);
				ImGui::InputText("WebSocket Port", websocket_port, 50, ImGuiInputTextFlags_CharsDecimal);
				ImGui::TreePop();
			}

			ImGui::Separator();
			RenderTable(dealerTable);
			column_max = next_column_max;
			mutex.unlock();

			ImGui::Separator();

			ImGui::End();
			ImGui::PopStyleColor();
		}
	}
	catch (std::exception& e)
	{
		ImGui::End();
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
