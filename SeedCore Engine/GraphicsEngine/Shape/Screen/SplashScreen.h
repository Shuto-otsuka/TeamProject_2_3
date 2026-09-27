#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/System/SplashSystem.h>

namespace SeedCore
{
	class TextureLoader;
	class BindlessHeap;
	class D3D12CommandQueue;

	class SplashScreen
	{
	public:
		SplashScreen() = default;
		~SplashScreen() = default;

		void Initialize(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* bindlessHeap);

		void Finalize();

		void Draw(ID3D12GraphicsCommandList6* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle, Float screenWidth, Float screenHeight, SplashPhase phase, Float alpha);

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> dayResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> nightResource_;

		Uint dayTextureIndex_ = 0;
		Uint nightTextureIndex_ = 0;

		/// [EN] The warning/fiction disclaimer screens - each a single
		///      centered/letterboxed image drawn through the same t0 slot and
		///      texture_aspect_ path as the day/night logo (see Draw()'s phase
		///      selection). Runtime/Logo/Warning.sub.logo and Fiction.sub.logo.
		/// [JP] 警告/フィクション免責画面 - どちらも day/night ロゴと同じ
		///      t0スロット・texture_aspect_の経路で描画される単一の
		///      中央寄せ/レターボックス画像（Draw()のフェーズ選択参照）。
		///      Runtime/Logo/Warning.sub.logo と Fiction.sub.logo。
		Microsoft::WRL::ComPtr<ID3D12Resource> warningResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> fictionResource_;

		Uint warningTextureIndex_ = 0;
		Uint fictionTextureIndex_ = 0;

		/// [EN] The CRI ADX2 middleware attribution logo, shown after Fiction and
		///      before the engine's own day/night logo (always on, unlike
		///      warning/fiction - middleware attribution isn't optional). Drawn
		///      through the same t0 slot/path as the others. Runtime/Logo/CriWare.logo.
		/// [JP] CRI ADX2 ミドルウェアのクレジットロゴ。Fiction の後、エンジン
		///      自体の昼夜ロゴの前に表示される（warning/fictionと違い常時表示 -
		///      ミドルウェアのクレジット表記は任意ではないため）。他と同じ
		///      t0スロット/経路で描画される。Runtime/Logo/CriWare.logo。
		Microsoft::WRL::ComPtr<ID3D12Resource> criLogoResource_;

		Uint criLogoTextureIndex_ = 0;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

		Bool initialized_ = false;

		BindlessHeap* bindlessHeap_ = nullptr;
	};
}
