#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Command/History.h>
#include <FoundationEngine/Resource/Scene/SceneSystem.h>
#include <Editor/Editor/Context/ApplicationContext.h>
#include <Editor/Editor/Context/WorldContext.h>
#include <Editor/Editor/Context/GraphicsContext.h>
#include <Editor/Editor/Context/GuizmoContext.h>
#include <Editor/Editor/Context/SceneContext.h>
#include <Editor/Editor/Context/SelectionContext.h>
#include <Editor/Editor/Context/ViewContext.h>
#include <Editor/Editor/Context/ConfigContext.h>
#include <Editor/Editor/Context/PreviewContext.h>
#include <Editor/Editor/Context/PanelContext.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>

namespace SeedCore
{
	struct EditorContext
	{
		ApplicationContext application_;

		WorldContext world_;

		GraphicsContext graphics_;

		GuizmoContext guizmo_;

		SceneContext scene_;

		SelectionContext selection_;

		ViewContext view_;

		ConfigContext config_;

		PreviewContext preview_;

		PanelContext panel_;
	};
}
