#pragma once

namespace ax
{
	namespace NodeEditor
	{
		struct EditorContext;
	}
}

namespace SeedCore
{
	enum class NodePinKind :Uint8
	{
		Input,
		Output,
	};

	enum class NodePinPlacement :Uint8
	{
		Side,
		Whole,
	};

	enum class NodeLinkShape :Uint8
	{
		Curve,
		Straight,
	};

	enum class NodeTarget :Uint8
	{
		Background,
		Node,
		Pin,
		Link,
	};

	struct NodePin
	{
		Uint32 node_ = 0;

		Uint32 index_ = 0;
	};

	class NodeCanvas :public NonTransferable
	{
	public:
		explicit NodeCanvas(const Vector4& padding = { 8.0f, 8.0f, 8.0f, 8.0f }, NodeLinkShape connecting = NodeLinkShape::Curve);

		~NodeCanvas();

	public:
		void Begin(const Char* id);

		void End();

	public:
		void BeginNode(Uint32 nodeID, const Color& background = { 0.125f, 0.125f, 0.125f, 0.784f }, const Color& border = { 1.0f, 1.0f, 1.0f, 0.376f });

		void EndNode();

		void NodeHeader(const Char* title, const Color& color);

	public:
		void BeginPin(const NodePin& pin, NodePinKind kind, NodePinPlacement placement);

		void EndPin();

	public:
		void Link(Uint32 linkID, const NodePin& from, const NodePin& to, NodeLinkShape shape, const Color& color, const Vector2& fromAnchor = Vector2::Zero, const Vector2& toAnchor = Vector2::Zero);

		void Guide(const NodePin& from, const Vector2& to, const Color& color, const Vector2& fromAnchor = Vector2::Zero);

	public:
		Bool BeginConnect();

		void EndConnect();

		Bool QueryConnectLink(NodePin& from, NodePin& to);

		Bool QueryConnectLink(NodePin& from, NodePin& to, Vector2& fromAnchor, Vector2& toAnchor);

		Bool QueryConnectNode(NodePin& pin);

		Bool AcceptConnect();

		void RejectConnect();

	public:
		Bool BeginRemove();

		void EndRemove();

		Bool QueryRemoveLink(Uint32& linkID);

		Bool QueryRemoveNode(Uint32& nodeID);

		Bool AcceptRemove();

		void RejectRemove();

	public:
		Bool BeginMenu(NodeTarget target);

		void EndMenu();

		void Tooltip(const Char* text);

		NODISCARD Uint32 MenuNode()const;

		NODISCARD NodePin MenuPin()const;

		NODISCARD Uint32 MenuLink()const;

		NODISCARD Vector2 MenuPosition()const;

	public:
		void PlaceNode(Uint32 nodeID, const Vector2& position);

		NODISCARD Vector2 NodePosition(Uint32 nodeID)const;

		NODISCARD Vector2 NodeSize(Uint32 nodeID)const;

	public:
		void SelectNode(Uint32 nodeID, Bool append = false);

		void SelectLink(Uint32 linkID, Bool append = false);

		void Deselect();

		NODISCARD Bool Reselected()const;

		NODISCARD DynamicArray<Uint32> SelectedNodes()const;

		NODISCARD DynamicArray<Uint32> SelectedLinks()const;

	public:
		NODISCARD Uint32 HoveredNode()const;

		NODISCARD NodePin HoveredPin()const;

		NODISCARD Uint32 HoveredLink()const;

		NODISCARD Uint32 DoubleClickNode()const;

	public:
		void FitAll();

		void FitSelection();

		void CenterNode(Uint32 nodeID);

		NODISCARD Vector2 ViewCenter()const;

		NODISCARD Vector2 ScreenToCanvas(const Vector2& screen)const;

		NODISCARD Vector2 CanvasToScreen(const Vector2& canvas)const;

		NODISCARD Float Zoom()const;

	private:
		struct Header
		{
			Uint32 node_ = 0;

			Bool active_ = false;

			Float bottom_ = 0.0f;

			Color color_ = Color::Clear;
		};

		struct Menu
		{
			NodeTarget target_ = NodeTarget::Background;

			Uint32 node_ = 0;

			NodePin pin_;

			Uint32 link_ = 0;

			Vector2 position_ = Vector2::Zero;

			Bool connecting_ = false;
		};

		struct Pin
		{
			NodePinKind kind_ = NodePinKind::Input;

			NodePinPlacement placement_ = NodePinPlacement::Side;
		};

		struct Remove
		{
			Bool pressed_ = false;

			Bool straight_ = false;

			Size index_ = 0;
		};

		struct View
		{
			Vector2 min_ = Vector2::Zero;

			Vector2 max_ = Vector2::Zero;
		};

		struct Straight
		{
			Uint32 link_ = 0;

			NodePin from_;

			NodePin to_;

			Vector2 fromAnchor_ = Vector2::Zero;

			Vector2 toAnchor_ = Vector2::Zero;

			Color color_ = Color::Clear;

			Vector2 start_ = Vector2::Zero;

			Vector2 finish_ = Vector2::Zero;

			Bool selected_ = false;

			Bool alive_ = false;
		};

		struct Draft
		{
			NodePin from_;

			Vector2 fromAnchor_ = Vector2::Zero;

			Vector2 toAnchor_ = Vector2::Zero;

			Color color_ = Color::Clear;
		};

	private:
		ax::NodeEditor::EditorContext* context_ = nullptr;

		Header header_;

		Menu menu_;

		Pin pin_;

		Remove remove_;

		View view_;

		Bool reselected_ = false;

		DynamicArray<Straight> straights_;

		DynamicArray<Draft> drafts_;
	};
}