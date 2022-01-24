
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>




#include "apImGui.h"
#include "apHelper.h"
#include "apImGuiMaterialEditor.h"


#include "apRenderer.h"
#include "apResourceManager.h"
#include "apGraphicsDevice_DX12.h"
#include "apGraphics.h"




static inline ImRect ImGui_GetItemRect()
{
    return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}



namespace ap::imgui::material
{

    using namespace ax;

    using ax::Widgets::IconType;

    static const int            s_PinIconSize = 24;
    static ImTextureID          s_HeaderBackground = nullptr;
    //static ImTextureID        s_SampleImage = nullptr;
    static ImTextureID          s_SaveIcon = nullptr;
    static ImTextureID          s_RestoreIcon = nullptr;

    static ap::Resource  s_tex1;
    static ap::Resource  s_tex2;
    static ap::Resource  s_tex3;

 

    static const char* OutputNodeName = "Material Result";
    static const char* OutputNodeBaseColor = "Base Color";
    static const char* OutputNodeOpacity = "Opacity";

    static const char* TextureNodeName = "Texture Sample";

    static const char* ConstantFloatNodeName = "Float";
    static const char* ConstantFloat2NodeName = "Float2";
    static const char* ConstantFloat3NodeName = "Float3";
    static const char* ConstantFloat4NodeName = "Float4";

    static const char* FloatAddNodeName = "Float Add";
    static const char* Float2AddNodeName = "Float2 Add";
    static const char* Float3AddNodeName = "Float3 Add";
    static const char* Float4AddNodeName = "Float4 Add";

    static const char* FloatMulNodeName = "Float Mul";
    static const char* Float2MulNodeName = "Float2 Mul";
    static const char* Float3MulNodeName = "Float3 Mul";
    static const char* Float4MulNodeName = "Float4 Mul";

    static const char* FloatSubtractNodeName = "Float Subtract";
    static const char* Float2SubtractNodeName = "Float2 Subtract";
    static const char* Float3SubtractNodeName = "Float3 Subtract";
    static const char* Float4SubtractNodeName = "Float4 Subtract";


    static int s_NextId = 10000;
    static int GetNextId()
    {
        return s_NextId++;
    }


    void Initialize()
    {
        s_tex1 = ap::resourcemanager::Load("resources\\images\\BlueprintBackground.png");
        s_tex2 = ap::resourcemanager::Load("resources\\images\\ic_save_white_24dp.png");
        s_tex3 = ap::resourcemanager::Load("resources\\images\\ic_restore_white_24dp.png");


    }
    MaterialNodes::MaterialNodes()
        : opened(false)
    {
        
    }
  

   

    void MaterialNodes::Initialize()
    {
        ed::Config config;

        config.SettingsFile = ("materialNodes/" + materialName + ".json").c_str();

        config.LoadNodeSettings = [this](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
        {
            auto node = FindNode(nodeId);
            if (!node)
                return 0;

            if (data != nullptr)
                memcpy(data, node->State.data(), node->State.size());
            return node->State.size();
        };

        config.SaveNodeSettings = [this](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
        {
            auto node = FindNode(nodeId);
            if (!node)
                return false;

            node->State.assign(data, size);

            return true;
        };

        nodes.reserve(30);
        links.reserve(30);


        editor = ed::CreateEditor(&config);
        SpawnMaterialResultNode();
        BuildNodes();

        ed::SetCurrentEditor(editor);
        ed::NavigateToContent();

    }

    static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
    {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }


    ImColor GetIconColor(PinType type)
    {
        switch (type)
        {
        default:
        case PinType::Flow:     return ImColor(255, 255, 255);
        case PinType::Bool:     return ImColor(220, 48, 48);
        case PinType::Int:      return ImColor(68, 201, 156);

        case PinType::Float:    return ImColor(147, 226, 74);
        case PinType::Float2:     return ImColor(220, 220, 220);
        case PinType::Float3:     return ImColor(220, 220, 220);
        case PinType::Float4:     return ImColor(220, 220, 220);

        case PinType::String:   return ImColor(124, 21, 153);
        case PinType::Object:   return ImColor(51, 150, 215);
        case PinType::Function: return ImColor(218, 0, 183);
        case PinType::Delegate: return ImColor(255, 48, 48);
        }
    };

    void DrawPinIcon(const Pin& pin, bool connected, int alpha)
    {
        IconType iconType;


        ImColor  color = pin.Color.Value.w ? pin.Color : GetIconColor(pin.Type);
        color.Value.w = alpha / 255.0f;
        switch (pin.Type)
        {
        case PinType::Flow:     iconType = IconType::Flow;   break;
        case PinType::Bool:     iconType = IconType::Circle; break;
        case PinType::Int:      iconType = IconType::Circle; break;

        case PinType::Float:    iconType = IconType::Circle; break;
        case PinType::Float2:     iconType = IconType::Circle; break;
        case PinType::Float3:     iconType = IconType::Circle; break;
        case PinType::Float4:     iconType = IconType::Circle; break;

        case PinType::String:   iconType = IconType::Circle; break;
        case PinType::Object:   iconType = IconType::Circle; break;
        case PinType::Function: iconType = IconType::Circle; break;
        case PinType::Delegate: iconType = IconType::Square; break;
        default:
            return;
        }

        ax::Widgets::Icon(ImVec2(s_PinIconSize, s_PinIconSize), iconType, connected, color, ImColor(32, 32, 32, alpha));
    };


    static const std::string baseTranslatedNodeName = "materialExpression";

    void MaterialNodes::Frame()
    {
        if(opened)
        {
            

            PushID();

            if (!initialized)
            {
                Initialize();
                initialized = true;
            }

            //ed::GetCurrentEditor();
            ed::SetCurrentEditor(editor);
            //ed::NavigateToContent();

            int mipmap = -1;
            uint64_t textureID1 = ap::graphics::GetDevice()->CopyDescriptorToImGui(&s_tex1.GetTexture(), mipmap);
            uint64_t textureID2 = ap::graphics::GetDevice()->CopyDescriptorToImGui(&s_tex2.GetTexture(), mipmap);
            uint64_t textureID3 = ap::graphics::GetDevice()->CopyDescriptorToImGui(&s_tex3.GetTexture(), mipmap);

            s_HeaderBackground = (ImTextureID)textureID1;
            s_SaveIcon = (ImTextureID)textureID2;
            s_RestoreIcon = (ImTextureID)textureID3;
        
            ImGui::Begin("Material Editor", &opened, ImGuiWindowFlags_NoCollapse);
            //ImGui::SetWindowCollapsed(false);


            ImGui::Columns(2);

            ImVec2 childSize = ImVec2(400, 0);

            //ImGui::BeginChild("Selection", childSize);

            

            Node* contextNode = FindNode(contextNodeId);
            if (contextNode && (contextNode->Name.find("Float") != std::string::npos))
            {

                if (contextNode->Type == NodeType::Constant)
                {
                    for (int i = 0; i < contextNode->Outputs.size(); i++)
                    {
                        Pin& pin = contextNode->Outputs[i];
                        switch (pin.Type)
                        {
                        case PinType::Float:
                            ImGui::DragFloat(pin.Name.c_str(), (float*)&pin.data, 0.05f);
                            break;
                        case PinType::Float2:
                            ImGui::DragFloat2(pin.Name.c_str(), (float*)&pin.data, 0.05f);
                            break;
                        case PinType::Float3:
                            ImGui::DragFloat3(pin.Name.c_str(), (float*)&pin.data, 0.05f);
                            break;
                        case PinType::Float4:
                            ImGui::DragFloat4(pin.Name.c_str(), (float*)&pin.data, 0.05f);
                            break;
                        default:
                            break;
                        }
                    }
                }

                for (int i = 0; i < contextNode->Inputs.size(); i++)
                {
                    Pin& pin = contextNode->Inputs[i];
                    switch (pin.Type)
                    {
                    case PinType::Float:
                        ImGui::DragFloat(pin.Name.c_str(), (float*)&pin.data, 0.05f);
                        break;
                    case PinType::Float2:
                        ImGui::DragFloat2(pin.Name.c_str(), (float*)&pin.data, 0.05f);
                        break;
                    case PinType::Float3:
                        ImGui::DragFloat3(pin.Name.c_str(), (float*)&pin.data, 0.05f);
                        break;
                    case PinType::Float4:
                        ImGui::DragFloat4(pin.Name.c_str(), (float*)&pin.data, 0.05f);
                        break;
                    default:
                        break;
                    }

                }


            }



            ImGui::Separator();

            static std::string savedShader;

#if 1
            if (ImGui::Button("Save", ImVec2(100, 20)))
            {


                nodeMap = {};
                translatedNodes = {};
                TranslateNodes();

                std::string shaderTemplate =

                    R"(
#pragma once
#define OBJECTSHADER_LAYOUT_COMMON
#include "../objectHF.hlsli"
//#include "../globals.hlsli"

//test




CBUFFER(MaterialParams, CBSLOT_MATERIALPARAMS)
{

%s

};

float4 main(PixelInput input) : SV_TARGET
{

%s

return float4(BaseColor,Opacity);


}      

)";

                char shaderOutput[3000];

                std::string param1;
                while (!translatedParams.empty())
                {
                    param1 += translatedParams.front();
                    translatedParams.pop();
                }

                std::string param2;
                while (!translatedNodes.empty())
                {
                    param2 += translatedNodes.front();
                    translatedNodes.pop();
                }

                

                sprintf_s(shaderOutput, sizeof(shaderOutput), shaderTemplate.c_str(), param1.c_str(), param2.c_str());

                savedShader = shaderOutput;

                std::ofstream file("../AppleEngine/shaders/materialNodes/"+ materialName+".hlsl", std::ios::binary | std::ios::trunc);
                if (file.is_open())
                {
                    file.write(const_cast<const char*>(shaderOutput), strlen(shaderOutput));
                    file.close();
                }


                ap::renderer::ReloadShaders();

            }
            ImVec2 size = ImGui::GetContentRegionAvail();
            ImGui::InputTextMultiline(GenerateID(), (char*)savedShader.c_str(), savedShader.size(), ImVec2(size.x, size.y - 100), ImGuiInputTextFlags_ReadOnly);

#endif
            ImGui::NextColumn();

            auto& io = ImGui::GetIO();


            ed::Begin("Node editor");
            {
                auto cursorTopLeft = ImGui::GetCursorScreenPos();


                util::BlueprintNodeBuilder builder(s_HeaderBackground, (int)s_tex1.GetTexture().desc.width, (int)s_tex1.GetTexture().desc.height);

                for (auto& node : nodes)
                {
                    if (node.Type != NodeType::Blueprint && node.Type != NodeType::Constant && node.Type != NodeType::Simple)
                        continue;

                    const auto isSimple = node.Type == NodeType::Simple;

                    bool hasOutputDelegates = false;
                    for (auto& output : node.Outputs)
                        if (output.Type == PinType::Delegate)
                            hasOutputDelegates = true;

                    builder.Begin(node.ID);
                    if (!isSimple)
                    {
                        builder.Header(node.Color);
                        ImGui::Spring(0);
                        ImGui::TextUnformatted(node.Name.c_str());
                        ImGui::Spring(1);
                        ImGui::Dummy(ImVec2(0, 28));
                        if (hasOutputDelegates)
                        {
                            ImGui::BeginVertical("delegates", ImVec2(0, 28));
                            ImGui::Spring(1, 0);
                            for (auto& output : node.Outputs)
                            {
                                if (output.Type != PinType::Delegate)
                                    continue;

                                auto alpha = ImGui::GetStyle().Alpha;
                                if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                                    alpha = alpha * (48.0f / 255.0f);

                                ed::BeginPin(output.ID, ed::PinKind::Output);
                                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                                ed::PinPivotSize(ImVec2(0, 0));
                                ImGui::BeginHorizontal(output.ID.AsPointer());
                                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                                if (!output.Name.empty())
                                {
                                    ImGui::TextUnformatted(output.Name.c_str());
                                    ImGui::Spring(0);
                                }
                                DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                                ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                                ImGui::EndHorizontal();
                                ImGui::PopStyleVar();
                                ed::EndPin();

                                //DrawItemRect(ImColor(255, 0, 0));
                            }
                            ImGui::Spring(1, 0);
                            ImGui::EndVertical();
                            ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                        }
                        else
                            ImGui::Spring(0);
                        builder.EndHeader();
                    }

                    if (node.Name == "Texture Sample")
                    {
                        DrawImage(node.texture, ImVec2(85.f, 85.0f), false);
                    }


                    for (auto& input : node.Inputs)
                    {
                        auto alpha = ImGui::GetStyle().Alpha;
                        if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
                            alpha = alpha * (48.0f / 255.0f);

                        builder.Input(input.ID);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                        DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
                        ImGui::Spring(0);
                        if (!input.Name.empty())
                        {
                            ImGui::TextUnformatted(input.Name.c_str());
                            ImGui::Spring(0);
                        }
                        if (input.Type == PinType::Bool)
                        {
                            ImGui::Button("Hello");
                            ImGui::Spring(0);
                        }
                        ImGui::PopStyleVar();
                        builder.EndInput();
                    }

                    if (isSimple)
                    {
                        builder.Middle();

                        ImGui::Spring(1, 0);
                        ImGui::TextUnformatted(node.Name.c_str());
                        ImGui::Spring(1, 0);
                    }

                    for (auto& output : node.Outputs)
                    {
                        if (!isSimple && output.Type == PinType::Delegate)
                            continue;

                        auto alpha = ImGui::GetStyle().Alpha;
                        if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                            alpha = alpha * (48.0f / 255.0f);

                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                        builder.Output(output.ID);
                        if (output.Type == PinType::String)
                        {
                            static char buffer[128] = "Edit Me\nMultiline!";
                            static bool wasActive = false;


                            ImGui::PushItemWidth(100.0f);
                            ImGui::InputText("##edit", buffer, 127);
                            ImGui::PopItemWidth();
                            if (ImGui::IsItemActive() && !wasActive)
                            {
                                ed::EnableShortcuts(false);
                                wasActive = true;
                            }
                            else if (!ImGui::IsItemActive() && wasActive)
                            {
                                ed::EnableShortcuts(true);
                                wasActive = false;
                            }
                            ImGui::Spring(0);
                        }
                        if (!output.Name.empty())
                        {
                            ImGui::Spring(0);
                            ImGui::TextUnformatted(output.Name.c_str());
                        }
                        ImGui::Spring(0);
                        DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                        ImGui::PopStyleVar();
                        builder.EndOutput();
                    }

                    builder.End();
                }

               
               
                for (auto& node : nodes)
                {
                    if (node.Type != NodeType::Comment)
                        continue;

                    const float commentAlpha = 0.75f;

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
                    ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
                    ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
                    ed::BeginNode(node.ID);
                    ImGui::PushID(node.ID.AsPointer());
                    ImGui::BeginVertical("content");
                    ImGui::BeginHorizontal("horizontal");
                    ImGui::Spring(1);
                    ImGui::TextUnformatted(node.Name.c_str());
                    ImGui::Spring(1);
                    ImGui::EndHorizontal();
                    ed::Group(node.Size);
                    ImGui::EndVertical();
                    ImGui::PopID();
                    ed::EndNode();
                    ed::PopStyleColor(2);
                    ImGui::PopStyleVar();

                    if (ed::BeginGroupHint(node.ID))
                    {
                        //auto alpha   = static_cast<int>(commentAlpha * ImGui::GetStyle().Alpha * 255);
                        auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

                        //ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha * ImGui::GetStyle().Alpha);

                        auto min = ed::GetGroupMin();
                        //auto max = ed::GetGroupMax();

                        ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
                        ImGui::BeginGroup();
                        ImGui::TextUnformatted(node.Name.c_str());
                        ImGui::EndGroup();

                        auto drawList = ed::GetHintBackgroundDrawList();

                        auto hintBounds = ImGui_GetItemRect();
                        auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

                        drawList->AddRectFilled(
                            hintFrameBounds.GetTL(),
                            hintFrameBounds.GetBR(),
                            IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

                        drawList->AddRect(
                            hintFrameBounds.GetTL(),
                            hintFrameBounds.GetBR(),
                            IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);

                        //ImGui::PopStyleVar();
                    }
                    ed::EndGroupHint();
                }

                for (auto& link : links)
                    ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

                if (!createNewNode)
                {

                    if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
                    {
                        auto showLabel = [](const char* label, ImColor color)
                        {
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
                            auto size = ImGui::CalcTextSize(label);

                            auto padding = ImGui::GetStyle().FramePadding;
                            auto spacing = ImGui::GetStyle().ItemSpacing;

                            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

                            auto rectMin = ImGui::GetCursorScreenPos() - padding;
                            auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

                            auto drawList = ImGui::GetWindowDrawList();
                            drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
                            ImGui::TextUnformatted(label);
                        };

                        ed::PinId startPinId = 0, endPinId = 0;
                        if (ed::QueryNewLink(&startPinId, &endPinId))
                        {
                            auto startPin = FindPin(startPinId);
                            auto endPin = FindPin(endPinId);

                            newLinkPin = startPin ? startPin : endPin;

                            if (startPin->Kind == PinKind::Input)
                            {
                                std::swap(startPin, endPin);
                                std::swap(startPinId, endPinId);
                            }

                            if (startPin && endPin)
                            {
                                if (endPin == startPin)
                                {
                                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                                }
                                else if (endPin->Kind == startPin->Kind)
                                {
                                    showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
                                    ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                                }
                                //else if (endPin->Node == startPin->Node)
                                //{
                                //    showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
                                //    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                                //}
                                else if (endPin->Type != startPin->Type)
                                {
                                    showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                                    ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                                }
                                else
                                {
                                    showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                                    if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                                    {
                                        links.emplace_back(Link(GetNextId(), startPinId, endPinId));
                                        // links.back().Color = GetIconColor(startPin->Type);
                                    }
                                }
                            }
                        }

                        ed::PinId pinId = 0;
                        if (ed::QueryNewNode(&pinId))
                        {
                            newLinkPin = FindPin(pinId);
                            if (newLinkPin)
                                showLabel("+ Create Node", ImColor(32, 45, 32, 180));

                            if (ed::AcceptNewItem())
                            {
                                createNewNode = true;
                                newNodeLinkPin = FindPin(pinId);
                                newLinkPin = nullptr;
                                ed::Suspend();
                                ImGui::OpenPopup("Create New Node");
                                ed::Resume();
                            }
                        }
                    }
                    else
                        newLinkPin = nullptr;

                    ed::EndCreate();

                    if (ed::BeginDelete())
                    {
                        ed::LinkId linkId = 0;
                        while (ed::QueryDeletedLink(&linkId))
                        {
                            if (ed::AcceptDeletedItem())
                            {
                                auto id = std::find_if(links.begin(), links.end(), [linkId](auto& link) { return link.ID == linkId; });
                                if (id != links.end())
                                    links.erase(id);
                            }
                        }

                        ed::NodeId nodeId = 0;
                        while (ed::QueryDeletedNode(&nodeId))
                        {
                            if (ed::AcceptDeletedItem())
                            {
                                auto id = std::find_if(nodes.begin(), nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
                                if (id != nodes.end())
                                    nodes.erase(id);
                            }
                        }
                    }
                    ed::EndDelete();
                }

                ImGui::SetCursorScreenPos(cursorTopLeft);
            }

# if 1
            auto openPopupPosition = ImGui::GetMousePos();
            ed::Suspend();
            if (ed::ShowNodeContextMenu(&contextNodeId))
                ImGui::OpenPopup("Node Context Menu");
            else if (ed::ShowPinContextMenu(&contextPinId))
                ImGui::OpenPopup("Pin Context Menu");
            else if (ed::ShowLinkContextMenu(&contextLinkId))
                ImGui::OpenPopup("Link Context Menu");
            else if (ed::ShowBackgroundContextMenu())
            {
                ImGui::OpenPopup("Create New Node");
                newNodeLinkPin = nullptr;
            }
            ed::Resume();

            ed::Suspend();
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            if (ImGui::BeginPopup("Node Context Menu"))
            {
                auto node = FindNode(contextNodeId);

                ImGui::TextUnformatted("Node Context Menu");
                ImGui::Separator();
                if (node)
                {
                    ImGui::Text("ID: %p", node->ID.AsPointer());
                    ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
                    ImGui::Text("Inputs: %d", (int)node->Inputs.size());
                    ImGui::Text("Outputs: %d", (int)node->Outputs.size());
                }
                else
                    ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
                ImGui::Separator();
                if (ImGui::MenuItem("Delete"))
                    ed::DeleteNode(contextNodeId);
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Pin Context Menu"))
            {
                auto pin = FindPin(contextPinId);

                ImGui::TextUnformatted("Pin Context Menu");
                ImGui::Separator();
                if (pin)
                {
                    ImGui::Text("ID: %p", pin->ID.AsPointer());
                    if (pin->Node)
                        ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
                    else
                        ImGui::Text("Node: %s", "<none>");
                }
                else
                    ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());

                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Link Context Menu"))
            {
                auto link = FindLink(contextLinkId);

                ImGui::TextUnformatted("Link Context Menu");
                ImGui::Separator();
                if (link)
                {
                    ImGui::Text("ID: %p", link->ID.AsPointer());
                    ImGui::Text("From: %p", link->StartPinID.AsPointer());
                    ImGui::Text("To: %p", link->EndPinID.AsPointer());
                }
                else
                    ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
                ImGui::Separator();
                if (ImGui::MenuItem("Delete"))
                    ed::DeleteLink(contextLinkId);
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Create New Node"))
            {
                auto newNodePostion = openPopupPosition;
                //ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

                //auto drawList = ImGui::GetWindowDrawList();
                //drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

                Node* node = nullptr;
                if (ImGui::MenuItem("Texture Sample"))
                    node = SpawnTextureSampleNode();

                ImGui::Separator();
                if (ImGui::MenuItem("Constant Float3"))
                    node = SpawnConstantFloat3Node();

                ImGui::Separator();
                if (ImGui::MenuItem("Float3 Add"))
                    node = SpawnFloat3AddNode();
                ImGui::Separator();
                if (ImGui::MenuItem("Comment"))
                    node = SpawnComment();

#if 0
                {
                    ImGui::Separator();
                    if (ImGui::MenuItem("Input Action"))
                        node = SpawnInputActionNode();
                    if (ImGui::MenuItem("Output Action"))
                        node = SpawnOutputActionNode();
                    if (ImGui::MenuItem("Branch"))
                        node = SpawnBranchNode();
                    if (ImGui::MenuItem("Do N"))
                        node = SpawnDoNNode();
                    if (ImGui::MenuItem("Set Timer"))
                        node = SpawnSetTimerNode();
                    if (ImGui::MenuItem("Less"))
                        node = SpawnLessNode();
                    if (ImGui::MenuItem("Weird"))
                        node = SpawnWeirdNode();
                    if (ImGui::MenuItem("Trace by Channel"))
                        node = SpawnTraceByChannelNode();
                    if (ImGui::MenuItem("Print String"))
                        node = SpawnPrintStringNode();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Comment"))
                        node = SpawnComment();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Sequence"))
                        node = SpawnTreeSequenceNode();
                    if (ImGui::MenuItem("Move To"))
                        node = SpawnTreeTaskNode();
                    if (ImGui::MenuItem("Random Wait"))
                        node = SpawnTreeTask2Node();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Message"))
                        node = SpawnMessageNode();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Transform"))
                        node = SpawnHoudiniTransformNode();
                    if (ImGui::MenuItem("Group"))
                        node = SpawnHoudiniGroupNode();

                }
#endif

                if (node)
                {
                    BuildNodes();

                    createNewNode = false;

                    ed::SetNodePosition(node->ID, newNodePostion);

                    if (auto startPin = newNodeLinkPin)
                    {
                        auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

                        for (auto& pin : pins)
                        {
                            if (CanCreateLink(startPin, &pin))
                            {
                                auto endPin = &pin;
                                if (startPin->Kind == PinKind::Input)
                                    std::swap(startPin, endPin);

                                links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
                                links.back().Color = GetIconColor(startPin->Type);

                                break;
                            }
                        }
                    }
                }

                ImGui::EndPopup();
            }
            else
                createNewNode = false;
            ImGui::PopStyleVar();
            ed::Resume();
# endif



            ed::End();

            ImGui::Columns(1);

            ImGui::End();

            PopID();

        }

    }

    std::vector<char> MaterialNodes::FillMaterialConstantBuffer() const
    {
        std::vector<char> ret(300);

        uint32_t index = 0;


        for (auto& e : translatedParamData)
        {
            switch (e.first)
            {
            case PinType::Int:
            {
                ap::Resource& res = *((ap::Resource*)&e.second->x);

                int textureIdx = -1;
                if (res.IsValid())
                    textureIdx = ap::graphics::GetDevice()->GetDescriptorIndex(&res.GetTexture(), ap::graphics::SubresourceType::SRV);

                *(int*)&ret.data()[index] = textureIdx;
                index = index + 4;
                break;
            }
            case PinType::Float3:
                *(XMFLOAT3*)&ret.data()[index] = *((XMFLOAT3*)&(e.second->x));
                index = index + 16;
                break;
            default:
                break;
            }


        }

        ret.resize(index);

        return std::move(ret);
    }




    Node* MaterialNodes::SpawnTextureSampleNode()
    {
        nodes.emplace_back(GetNextId(), TextureNodeName, ImColor(128, 195, 248));
        //nodes.back().DataType = PinType::Float3;

        nodes.back().Outputs.emplace_back(GetNextId(), "RGB", PinType::Float3);
        nodes.back().Outputs.emplace_back(GetNextId(), "R", PinType::Float);
        nodes.back().Outputs.back().Color = ImColor(255, 0, 0);
        nodes.back().Outputs.emplace_back(GetNextId(), "G", PinType::Float);
        nodes.back().Outputs.back().Color = ImColor(0, 255, 0);
        nodes.back().Outputs.emplace_back(GetNextId(), "B", PinType::Float);
        nodes.back().Outputs.back().Color = ImColor(0, 0, 255);
        nodes.back().Outputs.emplace_back(GetNextId(), "A", PinType::Float);
        nodes.back().Outputs.back().Color = ImColor(180, 180, 180);



        BuildNode(&nodes.back());

        return &nodes.back();
    }

    Node* MaterialNodes::SpawnMaterialResultNode()
    {
        nodes.emplace_back(GetNextId(), OutputNodeName, ImColor(222, 184, 135));


        nodes.back().Inputs.emplace_back(GetNextId(), OutputNodeBaseColor, PinType::Float3);
        nodes.back().Inputs.emplace_back(GetNextId(), OutputNodeOpacity, PinType::Float);


        for (auto& e : nodes.back().Inputs)
            e.Color = ImColor(220, 220, 220);


        BuildNode(&nodes.back());

        return &nodes.back();
    }

    Node* MaterialNodes::SpawnConstantFloat3Node()
    {
        nodes.emplace_back(GetNextId(), ConstantFloat3NodeName, ImColor(46, 190, 87));
        nodes.back().DataType = PinType::Float3;
        nodes.back().Type = NodeType::Constant;


        nodes.back().Outputs.emplace_back(GetNextId(), " ", PinType::Float3);

        for (auto& e : nodes.back().Outputs)
            e.Color = ImColor(220, 220, 220);


        BuildNode(&nodes.back());

        return &nodes.back();
    }

    Node* MaterialNodes::SpawnFloat3AddNode()
    {
        nodes.emplace_back(GetNextId(), Float3AddNodeName, ImColor(46, 139, 87));
        nodes.back().DataType = PinType::Float3;

        nodes.back().Inputs.emplace_back(GetNextId(), "A", PinType::Float3);
        nodes.back().Inputs.emplace_back(GetNextId(), "B", PinType::Float3);

        for (auto& e : nodes.back().Inputs)
            e.Color = ImColor(220, 220, 220);

        nodes.back().Outputs.emplace_back(GetNextId(), " ", PinType::Float3);

        for (auto& e : nodes.back().Outputs)
            e.Color = ImColor(220, 220, 220);


        BuildNode(&nodes.back());

        return &nodes.back();
    }

    Node* MaterialNodes::SpawnComment()
    {
        nodes.emplace_back(GetNextId(), "Test Comment");
        nodes.back().Type = NodeType::Comment;
        nodes.back().Size = ImVec2(300, 200);

        return &nodes.back();
    }

    Node* MaterialNodes::FindNode(ed::NodeId id)
    {
        for (auto& node : nodes)
            if (node.ID == id)
                return &node;

        return nullptr;
    }

    Link* MaterialNodes::FindLink(ed::LinkId id)
    {

        for (auto& link : links)
            if (link.ID == id)
                return &link;

        return nullptr;
    }

    Link* MaterialNodes::FindLink(const Pin& pin)
    {
        for (auto& link : links)
            if (link.EndPinID == pin.ID)
                return &link;

        return nullptr;
    }

    Pin* MaterialNodes::FindPin(ed::PinId id)
    {
        if (!id)
            return nullptr;

        for (auto& node : nodes)
        {
            for (auto& pin : node.Inputs)
                if (pin.ID == id)
                    return &pin;

            for (auto& pin : node.Outputs)
                if (pin.ID == id)
                    return &pin;
        }

        return nullptr;
    }

    bool MaterialNodes::IsPinLinked(ed::PinId id)
    {
        if (!id)
            return false;

        for (auto& link : links)
            if (link.StartPinID == id || link.EndPinID == id)
                return true;

        return false;
    }

    bool MaterialNodes::CanCreateLink(Pin* a, Pin* b)
    {
        if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node)
            return false;

        return true;
    }

    void MaterialNodes::BuildNode(Node* node)
    {
        for (auto& input : node->Inputs)
        {
            input.Node = node;
            input.Kind = PinKind::Input;
        }

        for (auto& output : node->Outputs)
        {
            output.Node = node;
            output.Kind = PinKind::Output;
        }
    }

    void MaterialNodes::BuildNodes()
    {
        for (auto& node : nodes)
            BuildNode(&node);
    }


    static std::string XMFLOAT4ToString(const XMFLOAT4& data, PinType type)
    {
        std::string str;
        switch (type)
        {
        case PinType::Bool:
            str += data.x > 0 ? "true" : "false";
            break;
        case PinType::Float:
            str += "float(" + std::to_string(data.x) + ")";
            break;
        case PinType::Float2:
            str += "float2(" + std::to_string(data.x) + "," + std::to_string(data.y) + ")";
            break;
        case PinType::Float3:
            str += "float3(" + std::to_string(data.x) + "," + std::to_string(data.y) + "," + std::to_string(data.z) + ")";
            break;
        case PinType::Float4:
            str += "float4(" + std::to_string(data.x) + "," + std::to_string(data.y) + "," + std::to_string(data.z) + "," + std::to_string(data.w) + ")";
            break;
        default:
            break;
        }

        return str;

    }



    void MaterialNodes::TranslateNodes()
    {
        Node* outputNode = nullptr;
        for (auto& node : nodes)
        {
            if (node.Name == OutputNodeName)
            {
                outputNode = &node;
                break;
            }

        }

        translatedParamData = {};

        for (int i = 0; i < nodes.size(); i++)
        {
            Node& node = nodes[i];
            if (node.Name == TextureNodeName)
            {
                //str += node.texture.IsValid() ? std::to_string(ap::graphics::GetDevice()->CopyDescriptorToImGui(&node.texture.GetTexture())) : "-1";
                std::string str = "int texture" + std::to_string(node.ID.Get()) + ";\n";
                translatedParams.push(str);
                translatedParamData.push_back({ PinType::Int, (XMFLOAT4*)&node.texture });
            }
            else if (node.Name == ConstantFloat3NodeName)
            {
                std::string str = "float3 " + baseTranslatedNodeName+  std::to_string(node.ID.Get()) + ";\n";
                str += "float pad" + std::to_string(node.ID.Get()) + ";\n";
                translatedParams.push(str);
                translatedParamData.push_back({ PinType::Float3, &node.Outputs[0].data });

            }


        }




        for (int i = 0; i < outputNode->Inputs.size(); i++)
        {
            TranslateResultNode(outputNode->Inputs[i]);
        }


    }

    void MaterialNodes::TranslateNode(const Node& node)
    {

        if (node.Type == NodeType::Constant)
            return;

        if (nodeMap.count(node.ID.Get()) != 0)
            return;

        std::string translatedNode;



        switch (node.DataType)
        {
        case PinType::Bool:
            translatedNode += "bool ";
            break;
        case PinType::Float:
            translatedNode += "float ";
            break;
        case PinType::Float2:
            translatedNode += "float2 ";
            break;
        case PinType::Float3:
            translatedNode += "float3 ";
            break;
        case PinType::Float4:
            translatedNode += "float4 ";
            break;
        default:
            break;
        }

        translatedNode += baseTranslatedNodeName + std::to_string(node.ID.Get()) + " = ";


        if (node.Name == TextureNodeName)
        {
            translatedNode += "bindless_textures[texture" + std::to_string(node.ID.Get()) + "].Sample(sampler_objectshader,  input.uvsets.xy); \n";
            //translatedNode += baseTranslatedNodeName + std::to_string(node.ID.Get())+".rgb" + " = " + "DEGAMMA(" + baseTranslatedNodeName + std::to_string(node.ID.Get()) + ".rgb)";
        }
        else if (node.Name == Float3AddNodeName)
        {

            std::string a;
            std::string b;
            Link* linkA = FindLink(node.Inputs[0]);
            Link* linkB = FindLink(node.Inputs[1]);
            if (linkA)
                a = baseTranslatedNodeName + std::to_string(FindPin(linkA->StartPinID)->Node->ID.Get()) + ".rgb";
            else
                a = XMFLOAT4ToString(node.Inputs[0].data, node.Inputs[0].Type);
            if (linkB)
                b = baseTranslatedNodeName + std::to_string(FindPin(linkB->StartPinID)->Node->ID.Get()) + ".rgb";
            else
                b = XMFLOAT4ToString(node.Inputs[1].data, node.Inputs[1].Type);

            translatedNode += (a + " + " + b) + ";\n";
        }



        nodeMap.insert({ node.ID.Get(),baseTranslatedNodeName + std::to_string(node.ID.Get()) });

        for (int i = 0; i < node.Inputs.size(); i++)
        {
            Link* link = FindLink(node.Inputs[i]);
            if (link)
                TranslateNode(*(FindPin(link->StartPinID)->Node));

        }

        translatedNodes.push(translatedNode);
    }

    void MaterialNodes::TranslateResultNode(const Pin& pin)
    {

        Link* link = FindLink(pin);

        std::string translatedNode;

        if (pin.Name == OutputNodeBaseColor)
        {
            translatedNode = "\nfloat3 BaseColor";
        }
        else if (pin.Name == OutputNodeOpacity)
        {
            translatedNode = "float Opacity";
        }

        translatedNode += " = ";


        Pin* startPin = nullptr;
        if (link)
        {
            startPin = FindPin(link->StartPinID);

            if (startPin->Node->Type == NodeType::Constant)
            {
                translatedNode += baseTranslatedNodeName + std::to_string(startPin->Node->ID.Get());
            }
            else
                translatedNode += baseTranslatedNodeName + std::to_string(startPin->Node->ID.Get());

            switch (startPin->Type)
            {
            case PinType::Float:
            {
                if (startPin->Name == "R")
                    translatedNode += ".r";
                else if (startPin->Name == "G")
                    translatedNode += ".g";
                else if (startPin->Name == "B")
                    translatedNode += ".b";
                else if (startPin->Name == "A")
                    translatedNode += ".a";
                else
                    translatedNode += ".r";

                break;
            }
            case PinType::Float2:
                translatedNode += ".rg";
                break;
            case PinType::Float3:
                translatedNode += ".rgb";
                break;
            case PinType::Float4:
                translatedNode += ".rgba";
                break;
            default:
                break;
            }

            translatedNode += ";\n";
        }
        else
        {
            translatedNode += "1;\n";
        }


        if (link)
            TranslateNode(*startPin->Node);

        translatedNodes.push(translatedNode);


    }







}