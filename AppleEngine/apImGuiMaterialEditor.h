#pragma once

#include "utilities/builders.h"
#include "utilities/widgets.h"

#include <imgui_node_editor.h>

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <stack>
#include <algorithm>
#include <utility>
#include <fstream>

namespace ap::imgui::material
{
    namespace ed = ax::NodeEditor;
    namespace util = ax::NodeEditor::Utilities;
   

    void Initialize();

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
        Blueprint = 1 << 0,
        Simple = 1 << 1,
        Tree = 1 << 2,
        Comment = 1 << 3,
        Houdini = 1 << 4,

        Constant = 1 << 5,

    };

    struct Node;

    struct Pin
    {
        ed::PinId   ID;
        Node* Node;
        std::string Name;
        PinType     Type;
        PinKind     Kind;
        ImColor     Color = { 0,0,0,0 };  //temp
        DirectX::XMFLOAT4    data = { 0,0,0,0 };

        Pin(int id, const char* name, PinType type) :
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
        PinType DataType = PinType::Float4;
        ImVec2 Size;

        DirectX::XMFLOAT4 data;   //
        ap::Resource texture;

        std::string State;
        std::string SavedState;

        Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
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

        Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
            ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(220, 220, 220)
        {
        }
    };



	class MaterialNodes
	{
    public:
        MaterialNodes();
        void Initialize();

        void Frame();

        std::vector<char> FillMaterialConstantBuffer() const;
    private:
        Node* SpawnTextureSampleNode();
        Node* SpawnMaterialResultNode();
        Node* SpawnConstantFloat3Node();
        Node* SpawnFloat3AddNode();
        Node* SpawnComment();

        Node* FindNode(ed::NodeId id);
        Link* FindLink(ed::LinkId id);
        Link* FindLink(const Pin& pin);
        Pin* FindPin(ed::PinId id);
        bool IsPinLinked(ed::PinId id);
        bool CanCreateLink(Pin* a, Pin* b);
        void BuildNode(Node* node);
        void BuildNodes();

        void TranslateNodes();
        void TranslateNode(const Node& node);
        void TranslateResultNode(const Pin& pin);

	public:
        std::vector<Node>    nodes;
        std::vector<Link>    links;

        std::string materialName;
        
        bool initialized = false;
        bool used = false;
        bool opened = false;
	private:
        ax::NodeEditor::EditorContext* editor;
        
        std::unordered_map<unsigned long long, std::string> nodeMap;
        std::queue<std::string> translatedNodes;
        std::queue<std::string> translatedParams;
        std::vector<std::pair<PinType, XMFLOAT4*>> translatedParamData;


        

        //
        ed::NodeId contextNodeId = 0;
        ed::LinkId contextLinkId = 0;
        ed::PinId  contextPinId = 0;
        bool createNewNode = false;
        Pin* newNodeLinkPin = nullptr;
        Pin* newLinkPin = nullptr;

	};


}


