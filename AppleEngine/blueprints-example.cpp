
#include "utilities/builders.h"
#include "utilities/widgets.h"

#include <imgui_node_editor.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include "apImGui.h"


#include "apResourceManager.h"
#include "apGraphicsDevice_DX12.h"
#include "apGraphics.h"


using namespace ap::imgui;

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

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

using namespace ax;

using ax::Widgets::IconType;

static ed::EditorContext* m_Editor = nullptr;

//extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vkey);
//extern "C" bool Debug_KeyPress(int vkey)
//{
//    static std::map<int, bool> state;
//    auto lastState = state[vkey];
//    state[vkey] = (GetAsyncKeyState(vkey) & 0x8000) != 0;
//    if (state[vkey] && !lastState)
//        return true;
//    else
//        return false;
//}

enum class PinType
{
    Flow,
    Bool,
    Int,
    Float,
    Float2,
    Float3,
    Float4,
    String,
    Object,
    Function,
    Delegate,
};

enum class PinKind
{
    Output,
    Input
};

enum class NodeType
{
    Blueprint,
    Simple,
    Tree,
    Comment,
    Houdini
};

struct Node;

struct Pin
{
    ed::PinId   ID;
    ::Node*     Node;
    std::string Name;
    PinType     Type;
    PinKind     Kind;
    ImColor     Color = {0,0,0,0};  //temp

    Pin(int id, const char* name, PinType type):
        ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
    {
    }
};

struct Node
{
    ed::NodeId ID;
    std::string Name;
    std::vector<Pin> Inputs;
    std::vector<Pin> Outputs;
    ImColor Color;
    NodeType Type;
    ImVec2 Size;

    XMFLOAT4 data;   //
    ap::Resource texture;

    std::string State;
    std::string SavedState;

    Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)):
        ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
    {
    }
};

struct Link
{
    ed::LinkId ID;

    ed::PinId StartPinID;
    ed::PinId EndPinID;

    ImColor Color;

    Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId):
        ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
    {
    }
};


static const int            s_PinIconSize = 24;
static std::vector<Node>    s_Nodes;
static std::vector<Link>    s_Links;
static ImTextureID          s_HeaderBackground = nullptr;
//static ImTextureID          s_SampleImage = nullptr;
static ImTextureID          s_SaveIcon = nullptr;
static ImTextureID          s_RestoreIcon = nullptr;

struct NodeIdLess
{
    bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
    {
        return lhs.AsPointer() < rhs.AsPointer();
    }
};

static const float          s_TouchTime = 1.0f;
static std::map<ed::NodeId, float, NodeIdLess> s_NodeTouchTime;

static int s_NextId = 10000;
static int GetNextId()
{
    return s_NextId++;
}

//static ed::NodeId GetNextNodeId()
//{
//    return ed::NodeId(GetNextId());
//}

static ed::LinkId GetNextLinkId()
{
    return ed::LinkId(GetNextId());
}

static void TouchNode(ed::NodeId id)
{
    s_NodeTouchTime[id] = s_TouchTime;
}

static float GetTouchProgress(ed::NodeId id)
{
    auto it = s_NodeTouchTime.find(id);
    if (it != s_NodeTouchTime.end() && it->second > 0.0f)
        return (s_TouchTime - it->second) / s_TouchTime;
    else
        return 0.0f;
}

static void UpdateTouch()
{
    const auto deltaTime = ImGui::GetIO().DeltaTime;
    for (auto& entry : s_NodeTouchTime)
    {
        if (entry.second > 0.0f)
            entry.second -= deltaTime;
    }
}

static Node* FindNode(ed::NodeId id)
{
    for (auto& node : s_Nodes)
        if (node.ID == id)
            return &node;

    return nullptr;
}

static Link* FindLink(ed::LinkId id)
{
    for (auto& link : s_Links)
        if (link.ID == id)
            return &link;

    return nullptr;
}

static Pin* FindPin(ed::PinId id)
{
    if (!id)
        return nullptr;

    for (auto& node : s_Nodes)
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

static bool IsPinLinked(ed::PinId id)
{
    if (!id)
        return false;

    for (auto& link : s_Links)
        if (link.StartPinID == id || link.EndPinID == id)
            return true;

    return false;
}

static bool CanCreateLink(Pin* a, Pin* b)
{
    if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node)
        return false;

    return true;
}



static void BuildNode(Node* node)
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


static Node* SpawnTextureSampleNode()
{
    s_Nodes.emplace_back(GetNextId(), "Texture Sample", ImColor(128, 195, 248));

    s_Nodes.back().Outputs.emplace_back(GetNextId(), "RGB", PinType::Float3);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "R", PinType::Float);
    s_Nodes.back().Outputs.back().Color = ImColor(255, 0, 0);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "G", PinType::Float);
    s_Nodes.back().Outputs.back().Color = ImColor(0, 255, 0);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "B", PinType::Float);
    s_Nodes.back().Outputs.back().Color = ImColor(0, 0, 255);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "A", PinType::Float);
    s_Nodes.back().Outputs.back().Color = ImColor(150, 150, 150);



    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnInputActionNode()
{
    s_Nodes.emplace_back(GetNextId(), "InputAction Fire", ImColor(255, 128, 128));
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Delegate);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "Pressed", PinType::Flow);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "Released", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}



static Node* SpawnBranchNode()
{
    s_Nodes.emplace_back(GetNextId(), "Branch");
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "True", PinType::Flow);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "False", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnDoNNode()
{
    s_Nodes.emplace_back(GetNextId(), "Do N");
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Enter", PinType::Flow);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "N", PinType::Int);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Reset", PinType::Flow);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "Exit", PinType::Flow);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "Counter", PinType::Int);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnOutputActionNode()
{
    s_Nodes.emplace_back(GetNextId(), "OutputAction");
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Sample", PinType::Float);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Event", PinType::Delegate);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnPrintStringNode()
{
    s_Nodes.emplace_back(GetNextId(), "Print String");
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "In String", PinType::String);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnMessageNode()
{
    s_Nodes.emplace_back(GetNextId(), "", ImColor(128, 195, 248));
    s_Nodes.back().Type = NodeType::Simple;
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "Message", PinType::String);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnSetTimerNode()
{
    s_Nodes.emplace_back(GetNextId(), "Set Timer", ImColor(128, 195, 248));
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Object", PinType::Object);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Function Name", PinType::Function);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Time", PinType::Float);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Looping", PinType::Bool);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnLessNode()
{
    s_Nodes.emplace_back(GetNextId(), "<", ImColor(128, 195, 248));
    s_Nodes.back().Type = NodeType::Simple;
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnWeirdNode()
{
    s_Nodes.emplace_back(GetNextId(), "o.O", ImColor(128, 195, 248));
    s_Nodes.back().Type = NodeType::Simple;
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnTraceByChannelNode()
{
    s_Nodes.emplace_back(GetNextId(), "Single Line Trace by Channel", ImColor(255, 128, 64));
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Start", PinType::Flow);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "End", PinType::Int);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Channel", PinType::Float);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Complex", PinType::Bool);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Actors to Ignore", PinType::Int);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Draw Debug Type", PinType::Bool);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "Ignore Self", PinType::Bool);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "Out Hit", PinType::Float);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Bool);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnTreeSequenceNode()
{
    s_Nodes.emplace_back(GetNextId(), "Sequence");
    s_Nodes.back().Type = NodeType::Tree;
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnTreeTaskNode()
{
    s_Nodes.emplace_back(GetNextId(), "Move To");
    s_Nodes.back().Type = NodeType::Tree;
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnTreeTask2Node()
{
    s_Nodes.emplace_back(GetNextId(), "Random Wait");
    s_Nodes.back().Type = NodeType::Tree;
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnComment()
{
    s_Nodes.emplace_back(GetNextId(), "Test Comment");
    s_Nodes.back().Type = NodeType::Comment;
    s_Nodes.back().Size = ImVec2(300, 200);

    return &s_Nodes.back();
}

static Node* SpawnHoudiniTransformNode()
{
    s_Nodes.emplace_back(GetNextId(), "Transform");
    s_Nodes.back().Type = NodeType::Houdini;
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

static Node* SpawnHoudiniGroupNode()
{
    s_Nodes.emplace_back(GetNextId(), "Group");
    s_Nodes.back().Type = NodeType::Houdini;
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
    s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

    BuildNode(&s_Nodes.back());

    return &s_Nodes.back();
}

void BuildNodes()
{
    for (auto& node : s_Nodes)
        BuildNode(&node);
}

const char* Application_GetName()
{
    return "Blueprints";
}


ap::Resource  g_tex1;
ap::Resource  g_tex2;
ap::Resource  g_tex3;


void Application_Initialize()
{
    g_tex1 = ap::resourcemanager::Load("resources\\images\\BlueprintBackground.png");
    g_tex2 = ap::resourcemanager::Load("resources\\images\\ic_save_white_24dp.png");
    g_tex3 = ap::resourcemanager::Load("resources\\images\\ic_restore_white_24dp.png");



    ed::Config config;

    config.SettingsFile = "Blueprints.json";

    config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
    {
        auto node = FindNode(nodeId);
        if (!node)
            return 0;

        if (data != nullptr)
            memcpy(data, node->State.data(), node->State.size());
        return node->State.size();
    };

    config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
    {
        auto node = FindNode(nodeId);
        if (!node)
            return false;

        node->State.assign(data, size);

        TouchNode(nodeId);

        return true;
    };

    m_Editor = ed::CreateEditor(&config);
    ed::SetCurrentEditor(m_Editor);

  
    ed::NavigateToContent();

    BuildNodes();

 
 
    //auto& io = ImGui::GetIO();
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
        case PinType::Bool:     return ImColor(220,  48,  48);
        case PinType::Int:      return ImColor( 68, 201, 156);
  
        case PinType::Float:    return ImColor(147, 226, 74);
        case PinType::Float2:     return ImColor(200, 200, 200);
        case PinType::Float3:     return ImColor(200, 200, 200);
        case PinType::Float4:     return ImColor(200, 200, 200);

        case PinType::String:   return ImColor(124,  21, 153);
        case PinType::Object:   return ImColor( 51, 150, 215);
        case PinType::Function: return ImColor(218,   0, 183);
        case PinType::Delegate: return ImColor(255,  48,  48);
    }
};


void DrawPinIcon(const Pin& pin, bool connected, int alpha)
{
    IconType iconType;


    ImColor  color = pin.Color.Value.w ? pin.Color :  GetIconColor(pin.Type);
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



void Application_Frame()
{
    
    
    int mipmap = -1;
    uint64_t textureID1 = ap::graphics::GetDevice()->CopyDescriptorToImGui(&g_tex1.GetTexture(), mipmap);
    uint64_t textureID2 = ap::graphics::GetDevice()->CopyDescriptorToImGui(&g_tex2.GetTexture(), mipmap);
    uint64_t textureID3 = ap::graphics::GetDevice()->CopyDescriptorToImGui(&g_tex3.GetTexture(), mipmap);


    s_HeaderBackground = (ImTextureID)textureID1;
    s_SaveIcon = (ImTextureID)textureID2;
    s_RestoreIcon = (ImTextureID)textureID3;


    ImGui::Begin("Material");

   /* if (ImGui::IsWindowDocked() && !ImGui::IsWindowFocused())
    {
        ImGui::End();
        return;
    }*/

    UpdateTouch();

    auto& io = ImGui::GetIO();

    ed::SetCurrentEditor(m_Editor);

    static ed::NodeId contextNodeId      = 0;
    static ed::LinkId contextLinkId      = 0;
    static ed::PinId  contextPinId       = 0;
    static bool createNewNode  = false;
    static Pin* newNodeLinkPin = nullptr;
    static Pin* newLinkPin     = nullptr;

  
    
    
    

    ed::Begin("Node editor");
    {
        auto cursorTopLeft = ImGui::GetCursorScreenPos();
       

        util::BlueprintNodeBuilder builder(s_HeaderBackground, (int)g_tex1.GetTexture().desc.width, (int)g_tex1.GetTexture().desc.height);

        for (auto& node : s_Nodes)
        {
            if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
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

        for (auto& node : s_Nodes)
        {
            if (node.Type != NodeType::Tree)
                continue;

            const float rounding = 5.0f;
            const float padding  = 12.0f;

            const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

            ed::PushStyleColor(ed::StyleColor_NodeBg,        ImColor(128, 128, 128, 200));
            ed::PushStyleColor(ed::StyleColor_NodeBorder,    ImColor( 32,  32,  32, 200));
            ed::PushStyleColor(ed::StyleColor_PinRect,       ImColor( 60, 180, 255, 150));
            ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor( 60, 180, 255, 150));

            ed::PushStyleVar(ed::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
            ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
            ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
            ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
            ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
            ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
            ed::PushStyleVar(ed::StyleVar_PinRadius, 5.0f);
            ed::BeginNode(node.ID);

            ImGui::BeginVertical(node.ID.AsPointer());
            ImGui::BeginHorizontal("inputs");
            ImGui::Spring(0, padding * 2);

            ImRect inputsRect;
            int inputAlpha = 200;
            if (!node.Inputs.empty())
            {
                    auto& pin = node.Inputs[0];
                    ImGui::Dummy(ImVec2(0, padding));
                    ImGui::Spring(1, 0);
                    inputsRect = ImGui_GetItemRect();

                    ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                    ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
                    ed::PushStyleVar(ed::StyleVar_PinCorners, 12);
                    ed::BeginPin(pin.ID, ed::PinKind::Input);
                    ed::PinPivotRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar(3);

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
            }
            else
                ImGui::Dummy(ImVec2(0, padding));

            ImGui::Spring(0, padding * 2);
            ImGui::EndHorizontal();

            ImGui::BeginHorizontal("content_frame");
            ImGui::Spring(1, padding);

            ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
            ImGui::Dummy(ImVec2(160, 0));
            ImGui::Spring(1);
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::Spring(1);
            ImGui::EndVertical();
            auto contentRect = ImGui_GetItemRect();

            ImGui::Spring(1, padding);
            ImGui::EndHorizontal();

            ImGui::BeginHorizontal("outputs");
            ImGui::Spring(0, padding * 2);

            ImRect outputsRect;
            int outputAlpha = 200;
            if (!node.Outputs.empty())
            {
                auto& pin = node.Outputs[0];
                ImGui::Dummy(ImVec2(0, padding));
                ImGui::Spring(1, 0);
                outputsRect = ImGui_GetItemRect();

                ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
                ed::BeginPin(pin.ID, ed::PinKind::Output);
                ed::PinPivotRect(outputsRect.GetTL(), outputsRect.GetBR());
                ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                ed::EndPin();
                ed::PopStyleVar();

                if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                    outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
            }
            else
                ImGui::Dummy(ImVec2(0, padding));

            ImGui::Spring(0, padding * 2);
            ImGui::EndHorizontal();

            ImGui::EndVertical();

            ed::EndNode();
            ed::PopStyleVar(7);
            ed::PopStyleColor(4);

            auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

            //const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
            //const auto unitSize    = 1.0f / fringeScale;

            //const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
            //{
            //    if ((col >> 24) == 0)
            //        return;
            //    drawList->PathRect(a, b, rounding, rounding_corners);
            //    drawList->PathStroke(col, true, thickness);
            //};

            drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
            //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
            //ImGui::PopStyleVar();
            drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
            //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
            //ImGui::PopStyleVar();
            drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
            //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
            drawList->AddRect(
                contentRect.GetTL(),
                contentRect.GetBR(),
                IM_COL32(48, 128, 255, 100), 0.0f);
            //ImGui::PopStyleVar();
        }

        for (auto& node : s_Nodes)
        {
            if (node.Type != NodeType::Houdini)
                continue;

            const float rounding = 10.0f;
            const float padding  = 12.0f;


            ed::PushStyleColor(ed::StyleColor_NodeBg,        ImColor(229, 229, 229, 200));
            ed::PushStyleColor(ed::StyleColor_NodeBorder,    ImColor(125, 125, 125, 200));
            ed::PushStyleColor(ed::StyleColor_PinRect,       ImColor(229, 229, 229, 60));
            ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

            const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

            ed::PushStyleVar(ed::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
            ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
            ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
            ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
            ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
            ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
            ed::PushStyleVar(ed::StyleVar_PinRadius, 6.0f);
            ed::BeginNode(node.ID);

            ImGui::BeginVertical(node.ID.AsPointer());
            if (!node.Inputs.empty())
            {
                ImGui::BeginHorizontal("inputs");
                ImGui::Spring(1, 0);

                ImRect inputsRect;
                int inputAlpha = 200;
                for (auto& pin : node.Inputs)
                {
                    ImGui::Dummy(ImVec2(padding, padding));
                    inputsRect = ImGui_GetItemRect();
                    ImGui::Spring(1, 0);
                    inputsRect.Min.y -= padding;
                    inputsRect.Max.y -= padding;

                    //ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                    //ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
                    ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
                    ed::BeginPin(pin.ID, ed::PinKind::Input);
                    ed::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
                    ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::EndPin();
                    //ed::PopStyleVar(3);
                    ed::PopStyleVar(1);

                    auto drawList = ImGui::GetWindowDrawList();
                    drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 15);
                    drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 15);

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                }

                //ImGui::Spring(1, 0);
                ImGui::EndHorizontal();
            }

            ImGui::BeginHorizontal("content_frame");
            ImGui::Spring(1, padding);

            ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
            ImGui::Dummy(ImVec2(160, 0));
            ImGui::Spring(1);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::PopStyleColor();
            ImGui::Spring(1);
            ImGui::EndVertical();
            auto contentRect = ImGui_GetItemRect();

            ImGui::Spring(1, padding);
            ImGui::EndHorizontal();

            if (!node.Outputs.empty())
            {
                ImGui::BeginHorizontal("outputs");
                ImGui::Spring(1, 0);

                ImRect outputsRect;
                int outputAlpha = 200;
                for (auto& pin : node.Outputs)
                {
                    ImGui::Dummy(ImVec2(padding, padding));
                    outputsRect = ImGui_GetItemRect();
                    ImGui::Spring(1, 0);
                    outputsRect.Min.y += padding;
                    outputsRect.Max.y += padding;

                    ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
                    ed::BeginPin(pin.ID, ed::PinKind::Output);
                    ed::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
                    ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar();

                    auto drawList = ImGui::GetWindowDrawList();
                    drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 15);
                    drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 15);


                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                }

                ImGui::EndHorizontal();
            }

            ImGui::EndVertical();

            ed::EndNode();
            ed::PopStyleVar(7);
            ed::PopStyleColor(4);

            auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

           
        }

        for (auto& node : s_Nodes)
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

                auto hintBounds      = ImGui_GetItemRect();
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

        for (auto& link : s_Links)
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
                    auto endPin   = FindPin(endPinId);

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
                                s_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
                                s_Links.back().Color = GetIconColor(startPin->Type);
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
                        createNewNode  = true;
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
                        auto id = std::find_if(s_Links.begin(), s_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
                        if (id != s_Links.end())
                            s_Links.erase(id);
                    }
                }

                ed::NodeId nodeId = 0;
                while (ed::QueryDeletedNode(&nodeId))
                {
                    if (ed::AcceptDeletedItem())
                    {
                        auto id = std::find_if(s_Nodes.begin(), s_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
                        if (id != s_Nodes.end())
                            s_Nodes.erase(id);
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

                        s_Links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
                        s_Links.back().Color = GetIconColor(startPin->Type);

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

    ImGui::End();
    
}

