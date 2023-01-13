#define STB_IMAGE_IMPLEMENTATION

#include "Drawing.h"
#include "client.hpp"




#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*(_ARR)))) 

LPCSTR Drawing::lpWindowName = "ImGui Standalone";
ImVec2 Drawing::vWindowSize = { 500, 500 };
ImGuiWindowFlags Drawing::WindowFlags = /*ImGuiWindowFlags_NoSavedSettings |*/ ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize;
//bool Drawing::bDraw = true;

extern Utils utils;
extern HackSettings hackSettings;
extern Client* g_client;

float wbg = 1.0f;

//#define str(eng,cn) (const char*)u8##cn
//#define str(eng,cn) (const char*)u8##cnshij
#define str(eng,cn) utils.b_chineseOS?(const char*)u8##cn:eng

void drawMinimap();
void drawMenu();
void drawMenu2();
void drawESP();

void Drawing::Active()
{
    hackSettings.guiSettings.b_draw = true;
}

bool Drawing::isActive()
{
    return (hackSettings.guiSettings.b_draw == true);
}


void Drawing::Draw() {
    static bool* b_previousEnableMenu = nullptr;
    if (isActive())
    {

        if (hackSettings.guiSettings.b_debug) {
            ImGui::ShowDemoWindow();
        }


        if (hackSettings.guiSettings.b_enableMinimap) {
            drawMinimap();
        }

        //绘制菜单
        if (hackSettings.guiSettings.b_enableMenu) {
            drawMenu2();
        }

        //ESP
        if (hackSettings.guiSettings.b_enableESP) {
            drawESP();
        }
        else {

        }
    }
}


// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}



bool drawLocalPlayerOnMap(GameMap& map, const ImVec2& mapLeftBottomPointOnScreen) {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    static float circleRadius = 5;

    LocalPlayer* localPlayer = &g_client->localPlayer;

    if (localPlayer->address == NULL) {
        return false;
    }

    PlayerController* playerController = &localPlayer->playerController;

    if (playerController->address == NULL || playerController->b_isLocal == false) {
        return false;
    }

    Vector3* position = &playerController->v3_position;

    Vector2 relativePosition = map.positionInGame_to_relativePositionLeftBottom({ position->x, position->y });
    Vector2 positionOnScreen{ mapLeftBottomPointOnScreen.x + relativePosition.x, mapLeftBottomPointOnScreen.y - relativePosition.y };

    drawList->AddCircleFilled({ positionOnScreen.x,positionOnScreen.y }, circleRadius, ImColor(1.0f, 1.0f, 1.0f));
    drawList->AddText({ positionOnScreen.x, positionOnScreen.y + circleRadius }, ImColor(1.0f, 1.0f, 1.0f), str("You", "你"));
}

/// <summary>
/// 在地图上绘制玩家
/// </summary>
bool drawOtherPlayersOnMap(GameMap& map, const ImVec2& mapLeftBottomPointOnScreen) {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    static float circleRadius = 10;

    PlayerController* playerControllers = g_client->playerControllers;

    PlayerController* ptr = playerControllers;

    for (int i = 0; i < g_client->n_players; (i++, ptr++)) {
        if (ptr->address == NULL) {
            continue;
        }

        //单独处理本地玩家的绘制
        if (ptr->b_isLocal) {
            continue;
        }

        Vector3* position = &ptr->v3_position;

        Vector2 relativePosition = map.positionInGame_to_relativePositionLeftBottom({ position->x, position->y });
        Vector2 positionOnScreen{ mapLeftBottomPointOnScreen.x + relativePosition.x , mapLeftBottomPointOnScreen.y - relativePosition.y };

        drawList->AddCircleFilled({ positionOnScreen.x,positionOnScreen.y }, circleRadius, ImColor(1.0f, 0.0f, 0.0f));
        drawList->AddText({ positionOnScreen.x, positionOnScreen.y + circleRadius }, ImColor(1.0f, 1.0f, 1.0f), ptr->nickname.c_str());
    }
    return true;
}

void drawMinimap() {
    ImGuiIO& io = ImGui::GetIO();


    //设置小地图初始化大小
    ImGui::SetNextWindowSize({ 500.0f, 400.0f }, ImGuiCond_Once);
    ImGui::Begin(str("Minimap","小地图"));

    GameMap* gameMap = nullptr;


    const char* mapNames[] = {
        str("Ancient Sands", "古代沙地"),
        str("The Basement","地下室"),
        str("Jungle Temple","丛林神殿"),
        str("GooseChapel","鹅教堂"),
        str("Mallard Manor","马拉德庄园"),
        str("Nexus Colony","连结殖民地"),
        str("Black Swan","黑天鹅"),
        str("SS MotherGoose","老妈鹅星球飞船")
    };

    static int selected_map = -1;

    if (ImGui::Button(str("Select map", "选择地图")))
        ImGui::OpenPopup("select_map");
    ImGui::SameLine();
    ImGui::TextUnformatted(selected_map == -1 ? str("<None>", "无") : mapNames[selected_map]);

    if (hackSettings.guiSettings.b_debug) {
        ImGui::SameLine();
        if (ImGui::Button(str("Debug map offsets", "调试地图偏移"))) {
            ImGui::OpenPopup("debug_map_offsets");
        }
    }

    //选择地图图片
    if (ImGui::BeginPopup("select_map"))
    {
        ImGui::Text("Aquarium");
        ImGui::Separator();
        for (int i = 0; i < IM_ARRAYSIZE(mapNames); i++)
            if (ImGui::Selectable(mapNames[i])) {
                //修改当前地图图片
                selected_map = i;
            }
        ImGui::EndPopup();
    }

    if (selected_map < 0) {
        //尚未选择地图
        //Have not selected map
        ImGui::Text(str("You have not selected map", "你还没有选择地图"));
    }
    else {
        //读取地图
        gameMap = &UI::miniMaps.at(selected_map);

        //弹出调试界面
        if (ImGui::BeginPopup("debug_map_offsets"))
        {
            static Vector2 tpPosition = { 0.0f,0.0f };
            if (ImGui::Button(str("TP to: ", "传送到: "))) {
                g_client->teleportTo(tpPosition);
            }
            ImGui::InputFloat("debug_map_tp_X", &tpPosition.x);
            ImGui::InputFloat("debug_map_tp_Y", &tpPosition.y);


            ImGui::Text("Offsets:");
            ImGui::SliderFloat("debug_map_offsets_X", &gameMap->offset.x, -50, 50);
            ImGui::SliderFloat("debug_map_offsets_Y", &gameMap->offset.y, -50, 50);
            ImGui::Text("Scale To Game Position:");

            static float min_scale = 0.01;
            static float max_scale = 1;

            //TODO添加最大最小值输入框
            ImGui::SliderFloat("Scale", &gameMap->scaleToGamePosition, min_scale, max_scale, "%.4f");
            ImGui::InputFloat("min_scale", &min_scale);
            ImGui::InputFloat("max_scale", &max_scale);

            ImGui::EndPopup();
        }

        //ImGui::Text("pointer = %p", gameMap);
        //ImGui::Text("size = %d x %d", gameMap->width, gameMap->height);

        ImVec2 leftTopOfImage = ImGui::GetCursorScreenPos();

        //ImGui::GetForegroundDrawList()->AddCircleFilled({ leftTopOfImage.x,leftTopOfImage.y }, 5, ImColor(1.0f, 0.0f, 0.0f));
        //ImVec2 roomForImage = { view.x, view.y -  };


        //检查长宽是否为0
        if (gameMap->width > 0 && gameMap->height > 0) {
            static bool minimapShowedBefore = false;

            ImVec2 mousePositionLeftBottomOfGamemap;

            //处理显示地图的尺寸问题
            {
                //显示默认大小
                if (!minimapShowedBefore) {
                    minimapShowedBefore = true;
                }
                //根据地图尺寸调整窗口大小
                else {
                    //当前窗口剩余可容纳图片的范围
                    ImVec2 roomSpaceForImage = ImGui::GetContentRegionAvail();
                    //ImGui::Text("roomSpaceForImage. %.1f, %.1f", roomSpaceForImage.x ,roomSpaceForImage.y);

                    if (roomSpaceForImage.x > 10 && roomSpaceForImage.y > 10) {
                        //width / height
                        float roomSpaceRatio = roomSpaceForImage.x / roomSpaceForImage.y;
                        float mapImageRatio = 1.0f * gameMap->width / gameMap->height;

                        //宽大于图片等比缩放所需的长度，
                        //此时按照剩余空间的高和图片的高来缩放
                        if (roomSpaceRatio >= mapImageRatio) {
                            gameMap->scaleToDisplay = roomSpaceForImage.y / gameMap->height;
                        }
                        else {
                            //高大于图片等比缩放所需的长度，
                            //此时按照剩余空间的宽和图片的宽来缩放
                            gameMap->scaleToDisplay = roomSpaceForImage.x / gameMap->width;
                        }
                    }
                }
            }

            //显示游戏地图
            bool gameMapClicked = ImGui::ImageButton((void*)gameMap->texture, ImVec2(gameMap->width * gameMap->scaleToDisplay, gameMap->height * gameMap->scaleToDisplay));

            //记录本次鼠标悬停在游戏地图上这段时间是否有过TP
            static bool hasTPedWhenHoveringOnGameMap = false;
            //记录上一次TP的坐标
            static Vector2 lastTPedPosition;

            //图片最左下角的坐标
            mousePositionLeftBottomOfGamemap = ImGui::GetCursorScreenPos();
            //处理鼠标移动到图片上的逻辑
            if (ImGui::IsItemHovered())
            {
                Vector2 positionInGame = gameMap->relativePositionLeftBottom_to_PositionInGame({
                    io.MousePos.x - mousePositionLeftBottomOfGamemap.x,
                    //因为屏幕坐标Y轴是和游戏内Y轴相反的
                    mousePositionLeftBottomOfGamemap.y - io.MousePos.y
                    });

                //处理点击传送的逻辑
                ImGui::BeginTooltip();


                //地图刚被点击
                if (gameMapClicked) {
                    //TODO 传送玩家
                    g_client->teleportTo(positionInGame);
                    hasTPedWhenHoveringOnGameMap = true;
                    lastTPedPosition = positionInGame;
                }

                //Debug
                if (hackSettings.guiSettings.b_debug) {
                    if (!hasTPedWhenHoveringOnGameMap) {
                        ImGui::Text(str("Click to TP\n(%.1f, %.1f)", "点击传送\n(%.1f, %.1f)\n相对图片左下角(%.1f, %.1f)"),
                            positionInGame.x, positionInGame.y,
                            io.MousePos.x - mousePositionLeftBottomOfGamemap.x, mousePositionLeftBottomOfGamemap.y - io.MousePos.y
                        );
                    }
                    else {
                        //尚未点击
                        ImGui::Text(str("You have been teleported to\n(%.1f, %.1f)", "你已被传送至\n(%.1f, %.1f)"), lastTPedPosition.x, lastTPedPosition.y);
                    }
                }
                //Release
                else {
                    if (!hasTPedWhenHoveringOnGameMap) {
                        ImGui::Text(str("Click to TP\n(%.1f, %.1f)", "点击传送\n(%.1f, %.1f)"),
                            positionInGame.x, positionInGame.y
                        );
                    }
                    else {
                        //尚未点击
                        ImGui::Text(str("You have been teleported to\n(%.1f, %.1f)", "你已被传送至\n(%.1f, %.1f)"), lastTPedPosition.x, lastTPedPosition.y);
                    }
                }

                ImGui::EndTooltip();
            }
            else {
                //游戏地图没有鼠标焦点
                hasTPedWhenHoveringOnGameMap = false;
            }

            //ImGui::GetForegroundDrawList()->AddCircleFilled({ pos.x,pos.y }, 20, ImColor(1.0f, 0.0f, 0.0f));
            //在地图上绘制玩家位置
            drawOtherPlayersOnMap(*gameMap, mousePositionLeftBottomOfGamemap);
            drawLocalPlayerOnMap(*gameMap, mousePositionLeftBottomOfGamemap);
        }
        else {
            //不显示游戏地图
        }
    }

    ImGui::End();
}

void drawMenu2() {
    PlayerController* playerController = &g_client->localPlayer.playerController;

    ImGuiStyle& Style = ImGui::GetStyle();
    auto Color = Style.Colors;

    static bool WinPos = true;//用于初始化窗口位置
    int Screen_Width{ GetSystemMetrics(SM_CXSCREEN) };//获取显示器的宽
    int Screen_Heigth{ GetSystemMetrics(SM_CYSCREEN) };//获取显示器的高

    static bool CheckBox_1 = false, CheckBox_2 = true;
    static int InputInt = 0;
    static int Comb = 0;
    static float InputFloat = 0;
    static char InputString[80] = { '?' };

    static int Tab = 0;
    enum Tab
    {
        Info,
        Players,
        Misc,
        README,
        ESP,
        style
    };

    static int Color_ = 0;
    enum Color_
    {
        Red,
        Green,
        Blue,
        Orange
    };

    

    switch (Color_)
    {
    case Color_::Red:
        Style.ChildRounding = 8.0f;
        Style.FrameRounding = 5.0f;


        Color[ImGuiCol_Button] = ImColor(192, 51, 74, 255);
        Color[ImGuiCol_ButtonHovered] = ImColor(212, 71, 94, 255);
        Color[ImGuiCol_ButtonActive] = ImColor(172, 31, 54, 255);

        Color[ImGuiCol_FrameBg] = ImColor(54, 54, 54, 150);
        Color[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42, 150);
        Color[ImGuiCol_FrameBgHovered] = ImColor(100, 100, 100, 150);

        Color[ImGuiCol_CheckMark] = ImColor(192, 51, 74, 255);

        Color[ImGuiCol_SliderGrab] = ImColor(192, 51, 74, 255);
        Color[ImGuiCol_SliderGrabActive] = ImColor(172, 31, 54, 255);

        Color[ImGuiCol_Header] = ImColor(192, 51, 74, 255);
        Color[ImGuiCol_HeaderHovered] = ImColor(212, 71, 94, 255);
        Color[ImGuiCol_HeaderActive] = ImColor(172, 31, 54, 255);

        Color[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, wbg);
        Color[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, wbg);
        Color[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, wbg);
        Color[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, wbg);
        Color[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, wbg);
        Color[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, wbg);
        Color[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, wbg);
        break;
    case Color_::Green:
        Style.ChildRounding = 8.0f;
        Style.FrameRounding = 5.0f;

        Color[ImGuiCol_Button] = ImColor(10, 105, 56, 255);
        Color[ImGuiCol_ButtonHovered] = ImColor(30, 125, 76, 255);
        Color[ImGuiCol_ButtonActive] = ImColor(0, 95, 46, 255);

        Color[ImGuiCol_FrameBg] = ImColor(54, 54, 54, 150);
        Color[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42, 150);
        Color[ImGuiCol_FrameBgHovered] = ImColor(100, 100, 100, 150);

        Color[ImGuiCol_CheckMark] = ImColor(10, 105, 56, 255);

        Color[ImGuiCol_SliderGrab] = ImColor(10, 105, 56, 255);
        Color[ImGuiCol_SliderGrabActive] = ImColor(0, 95, 46, 255);

        Color[ImGuiCol_Header] = ImColor(10, 105, 56, 255);
        Color[ImGuiCol_HeaderHovered] = ImColor(30, 125, 76, 255);
        Color[ImGuiCol_HeaderActive] = ImColor(0, 95, 46, 255);

        Color[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, wbg);
        Color[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, wbg);
        Color[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, wbg);
        Color[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, wbg);
        Color[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, wbg);
        Color[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, wbg);
        Color[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, wbg);

        break;
    case Color_::Blue:
        Style.ChildRounding = 8.0f;
        Style.FrameRounding = 5.0f;

        Color[ImGuiCol_Button] = ImColor(51, 120, 255, 255);
        Color[ImGuiCol_ButtonHovered] = ImColor(71, 140, 255, 255);
        Color[ImGuiCol_ButtonActive] = ImColor(31, 100, 225, 255);

        Color[ImGuiCol_FrameBg] = ImColor(54, 54, 54, 150);
        Color[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42, 150);
        Color[ImGuiCol_FrameBgHovered] = ImColor(100, 100, 100, 150);

        Color[ImGuiCol_CheckMark] = ImColor(51, 120, 255, 255);

        Color[ImGuiCol_SliderGrab] = ImColor(51, 120, 255, 255);
        Color[ImGuiCol_SliderGrabActive] = ImColor(31, 100, 225, 255);

        Color[ImGuiCol_Header] = ImColor(51, 120, 255, 255);
        Color[ImGuiCol_HeaderHovered] = ImColor(71, 140, 255, 255);
        Color[ImGuiCol_HeaderActive] = ImColor(31, 100, 225, 255);

        Color[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, wbg);
        Color[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, wbg);
        Color[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, wbg);
        Color[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, wbg);
        Color[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, wbg);
        Color[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, wbg);
        Color[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, wbg);

        break;
    case Color_::Orange://233,87,33
        Style.ChildRounding = 8.0f;
        Style.FrameRounding = 5.0f;



        Color[ImGuiCol_Button] = ImColor(233, 87, 33, 255);
        Color[ImGuiCol_ButtonHovered] = ImColor(253, 107, 53, 255);
        Color[ImGuiCol_ButtonActive] = ImColor(213, 67, 13, 255);

        Color[ImGuiCol_FrameBg] = ImColor(54, 54, 54, 150);
        Color[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42, 150);
        Color[ImGuiCol_FrameBgHovered] = ImColor(100, 100, 100, 150);

        Color[ImGuiCol_CheckMark] = ImColor(233, 87, 33, 255);

        Color[ImGuiCol_SliderGrab] = ImColor(233, 87, 33, 255);
        Color[ImGuiCol_SliderGrabActive] = ImColor(213, 67, 13, 255);

        Color[ImGuiCol_Header] = ImColor(233, 87, 33, 255);
        Color[ImGuiCol_HeaderHovered] = ImColor(253, 107, 53, 255);
        Color[ImGuiCol_HeaderActive] = ImColor(213, 67, 13, 255);


        Color[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, wbg);
        Color[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, wbg);
        Color[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, wbg);
        Color[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, wbg);
        Color[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, wbg);
        Color[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, wbg);
        break;
    }

    if (WinPos)//初始化窗口
    {
        ImGui::SetNextWindowPos({ float(Screen_Width - 600) / 2,float(Screen_Heigth - 400) / 2 });
        WinPos = false;//初始化完毕
    }

    ImGui::Begin(str("Main", "主菜单"), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);//开始绘制窗口
    ImGui::SetWindowSize({ 600.0f,400.0f });//设置窗口大小


    {
        ImGui::GetWindowDrawList()->AddLine({ ImGui::GetWindowPos().x + 420.0f,ImGui::GetWindowPos().y + 10.0f }, { ImGui::GetWindowPos().x + 420.0f,ImGui::GetWindowPos().y + 390.0f }, ImColor(100, 100, 100, 255));

        ImGui::SetCursorPos({ 430.0f,20.0f });

        ImGui::TextColored(Color[ImGuiCol_Button], str("Goose Goose Duck", "鹅鸭杀助手"));




        ImGui::SetCursorPos({ 430.0f,65.0f });

        ImGui::PushStyleColor(ImGuiCol_Button, Tab == Tab::Info ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        if (ImGui::Button(str("LocalPlayer Info", "本地玩家信息"), { 150.0f,40.0f }))
        {
            Tab = Tab::Info;
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, Tab == Tab::Players ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::SetCursorPos({ 430.0f,115.0f });
        if (ImGui::Button(str("Players Info", "角色信息"), { 150.0f,40.0f }))
        {
            Tab = Tab::Players;
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, Tab == Tab::Misc ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::SetCursorPos({ 430.0f,165.0f });
        if (ImGui::Button(str("Misc", "功能类"), { 150.0f,40.0f }))
        {
            Tab = Tab::Misc;
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, Tab == Tab::ESP ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::SetCursorPos({ 430.0f,215.0f });
        if (ImGui::Button(str("esp", "透视"), { 150.0f,40.0f }))
        {
            Tab = Tab::ESP;
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, Tab == Tab::README ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::SetCursorPos({ 430.0f,265.0f });
        if (ImGui::Button(str("README", "说明"), { 150.0f,40.0f }))
        {
            Tab = Tab::README;
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, Tab == Tab::style ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::SetCursorPos({ 430.0f,315.0f });
        if (ImGui::Button(str("style", "样式"), { 150.0f,40.0f }))
        {
            Tab = Tab::style;
        }
        ImGui::PopStyleColor();

        time_t t = time(0);
        char tmp[32] = { NULL };
        strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M", localtime(&t));

        ImGui::SetCursorPos({ 430.0f,365.0f });
        ImGui::TextColored(Color[ImGuiCol_Button], "%s", tmp);
    }
    ImGui::SetCursorPos({ 10.0f,10.0f });
    ImGui::BeginChild("enble", { 400.0f,380.0f }, true);
    switch (Tab)
    {
    case Tab::Info:
        {
            ImGui::Text(playerController->nickname.c_str());

            float minSpeed = hackSettings.gameOriginalData.f_baseMovementSpeed;
            if (minSpeed <= 0) {
                minSpeed = 5.0f;
            }

            ImGui::SliderFloat(
                str("Movement speed", "移速"),
                &hackSettings.guiSettings.f_baseMovementSpeed,
                minSpeed,
                minSpeed * 2
            );

            ImGui::Text("{%.2f, %.2f}", playerController->v3_position.x, playerController->v3_position.y);
        }
        

        break;
    case Tab::Players:
        {
            if (ImGui::BeginTable("table1", 5,
                ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
            ))
            {
                ImGui::TableSetupColumn(str("Nickname", "昵称"));
                ImGui::TableSetupColumn(str("Role", "角色"));
                ImGui::TableSetupColumn(str("Killed this round", "本轮杀过人"));
                ImGui::TableSetupColumn(str("Death Time", "死亡时间"));
                ImGui::TableSetupColumn(str("Pos", "坐标"));
                //ImGui::TableSetupColumn("Three");
                ImGui::TableHeadersRow();

                PlayerController* player = g_client->playerControllers;
                for (int row = 0; row < g_client->n_players; (row++, player++))
                {
                    //跳过无效玩家和本地玩家
                    if (player->address == NULL || player->b_isLocal) {
                        continue;
                    }
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn(); ImGui::Text(player->nickname.c_str());
                    ImGui::TableNextColumn(); ImGui::Text(player->roleName.c_str());
                    if (player->b_hasKilledThisRound) {
                        ImGui::TableNextColumn(); ImGui::Text(str("Yes", "是"));
                    }
                    else {
                        ImGui::TableNextColumn(); ImGui::Text(str("", ""));
                    }
                    if (player->i_timeOfDeath != 0) {
                        ImGui::TableNextColumn(); ImGui::Text("%d", player->i_timeOfDeath);
                    }
                    else {
                        ImGui::TableNextColumn(); ImGui::Text("");
                    }
                    ImGui::TableNextColumn(); ImGui::Text("(%.1f, %.1f)", player->v3_position.x, player->v3_position.y);

                }
                ImGui::EndTable();
        }
        
            break;
    case Tab::Misc:
        {
            ImGui::Checkbox(str("Remove fog of war", "隐藏战争迷雾"), &hackSettings.disableFogOfWar);
            HelpMarker(
                str("Remove shadows and let you see other players behind walls", "可以透过墙看到和听到其他玩家，隐藏视野阴影")
            );

            ImGui::Checkbox(str("Noclip", "穿墙"), &hackSettings.guiSettings.b_alwaysEnableNoclip);
            HelpMarker(
                str("Walk through anything\nYou can press Left ALT to temporarily enable noclip", "穿墙模式\n长按左ALT键来临时穿墙")
            );
        }
        
        break;
    case Tab::ESP:
        {
            ImGui::Text(str("Button below is just for testing if overlay works", "下面的按钮目前只是为了测试绘制能否正常工作"));
            ImGui::Checkbox(str("Enable ESP", "全局开关"), &hackSettings.guiSettings.b_enableESP);
            HelpMarker(
                str("Create Issue to report bug if you can't see two green lines and yellow rect line", "如果你看不到屏幕上有横竖两条绿线以及环绕整个显示器的黄色矩形的话,请到Issue提交bug")
            );
        }
  
        break;
    case Tab::README:
        {
            ImGui::Text(str("This an open-source project from Liuhaixv", "这是一个来自Liuhaixv的开源项目"));
            ImGui::SameLine();
            if (ImGui::Button(str("Link to project", "查看项目"))) {
                ShellExecute(0, 0, "https://github.com/Liuhaixv/Goose_Goose_Duck_Hack", 0, 0, SW_SHOW);
            }
        }   
        break;
    case Tab::style:
        {
            ImGui::Text(str("Style", "主题颜色"));
            ImGui::SameLine();
            ImGui::Combo(" ", &Color_, str("RED\0GREEN\0BLUE\0ORANGE", "红色\0绿色\0蓝色\0橙色"));
            ImGui::SliderFloat(str("", "背景不透明度"), &wbg, 0.0f, 1.0f);
        }
        break;
        }
        ImGui::EndChild();
    }
}



void drawMenu() {

    bool b_open = true;
    bool* ptr_bOpen = &b_open;
    ImGui::Begin(str("Main", "主菜单"));
    ImGui::SetWindowSize({ 600.0f,400.0f });
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Main menu", tab_bar_flags))
    {
        PlayerController* playerController = &g_client->localPlayer.playerController;
        //菜单3
        if (ImGui::BeginTabItem(str("LocalPlayer Info", "本地玩家信息")))
        {
            ImGui::Text(playerController->nickname.c_str());

            float minSpeed = hackSettings.gameOriginalData.f_baseMovementSpeed;
            if (minSpeed <= 0) {
                minSpeed = 5.0f;
            }

            ImGui::SliderFloat(
                str("Movement speed", "移速"),
                &hackSettings.guiSettings.f_baseMovementSpeed,
                minSpeed,
                minSpeed * 2
            );

            ImGui::Text("{%.2f, %.2f}", playerController->v3_position.x, playerController->v3_position.y);

            ImGui::EndTabItem();
        }

        //菜单1
        if (ImGui::BeginTabItem(str("Players Info", "角色信息")))
        {
            if (ImGui::BeginTable("table1", 5,
                ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
            ))
            {
                ImGui::TableSetupColumn(str("Nickname", "昵称"));
                ImGui::TableSetupColumn(str("Role", "角色"));
                ImGui::TableSetupColumn(str("Killed this round", "本轮杀过人"));
                ImGui::TableSetupColumn(str("Death Time", "死亡时间"));
                ImGui::TableSetupColumn(str("Pos", "坐标"));
                //ImGui::TableSetupColumn("Three");
                ImGui::TableHeadersRow();

                PlayerController* player = g_client->playerControllers;
                for (int row = 0; row < g_client->n_players; (row++, player++))
                {
                    //跳过无效玩家和本地玩家
                    if (player->address == NULL || player->b_isLocal) {
                        continue;
                    }
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn(); ImGui::Text(player->nickname.c_str());
                    ImGui::TableNextColumn(); ImGui::Text(player->roleName.c_str());
                    if (player->b_hasKilledThisRound) {
                        ImGui::TableNextColumn(); ImGui::Text(str("Yes", "是"));
                    }
                    else {
                        ImGui::TableNextColumn(); ImGui::Text(str("", ""));
                    }
                    if (player->i_timeOfDeath != 0) {
                        ImGui::TableNextColumn(); ImGui::Text("%d", player->i_timeOfDeath);
                    }
                    else {
                        ImGui::TableNextColumn(); ImGui::Text("");
                    }
                    ImGui::TableNextColumn(); ImGui::Text("(%.1f, %.1f)", player->v3_position.x, player->v3_position.y);

                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        //菜单2
        if (ImGui::BeginTabItem(str("Misc", "功能类")))
        {
            ImGui::Checkbox(str("Remove fog of war", "隐藏战争迷雾"), &hackSettings.disableFogOfWar);
            HelpMarker(
                str("Remove shadows and let you see other players behind walls", "可以透过墙看到和听到其他玩家，隐藏视野阴影")
            );

            ImGui::Checkbox(str("Noclip", "穿墙"), &hackSettings.guiSettings.b_alwaysEnableNoclip);
            HelpMarker(
                str("Walk through anything\nYou can press Left ALT to temporarily enable noclip", "穿墙模式\n长按左ALT键来临时穿墙")
            );

            ImGui::EndTabItem();
        }

        //菜单2
        if (ImGui::BeginTabItem(str("ESP", "透视")))
        {
            ImGui::Text(str("Button below is just for testing if overlay works", "下面的按钮目前只是为了测试绘制能否正常工作"));
            ImGui::Checkbox(str("Enable ESP", "全局开关"), &hackSettings.guiSettings.b_enableESP);
            HelpMarker(
                str("Create Issue to report bug if you can't see two green lines and yellow rect line", "如果你看不到屏幕上有横竖两条绿线以及环绕整个显示器的黄色矩形的话,请到Issue提交bug")
            );

            ImGui::EndTabItem();
        }

        //菜单2
        if (ImGui::BeginTabItem(str("README", "说明")))
        {
            ImGui::Text(str("This an open-source project from Liuhaixv", "这是一个来自Liuhaixv的开源项目"));
            ImGui::SameLine();
            if (ImGui::Button(str("Link to project", "查看项目"))) {
                ShellExecute(0, 0, "https://github.com/Liuhaixv/Goose_Goose_Duck_Hack", 0, 0, SW_SHOW);
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(str("style", "风格")))
        {


            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

void drawESP() {
    //TODO
    ///*
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList(viewport);

    /*
    drawList->AddCircleFilled(
        { 500,500 },
        30,
        ImColor{ 1.0f, 1.0f, 0.0f }
    );
    */

    drawList->AddRect({ 0, 0 }, { ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y }, ImColor(1.0f, 1.0f, 0.0f), 50.0f, 0, 6.0f);

    ImColor lineColor{ 0.0f,1.0f,0.0f };
    float lineThichness = 4;
    drawList->AddLine({ 0, ImGui::GetIO().DisplaySize.y / 2 }, { ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y / 2 },
        lineColor, lineThichness);
    drawList->AddLine({ ImGui::GetIO().DisplaySize.x / 2, 0 }, { ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y },
        lineColor, lineThichness);
    //*/

    /*
    ImGui::GetForegroundDrawList()->AddCircleFilled(
        { 500,500 },
        30,
        ImColor{ 1.0f, 1.0f, 0.0f }
    );

    ImGui::GetForegroundDrawList()->AddRect({ 0, 0 }, { ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y }, ImColor(1.0f, 1.0f, 0.0f), 50.0f, 0, 3.0f);
    */
}
