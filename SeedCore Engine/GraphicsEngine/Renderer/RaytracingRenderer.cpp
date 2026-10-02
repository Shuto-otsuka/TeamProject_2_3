#include <GraphicsEngine/Renderer/RaytracingRenderer.h>

#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/World.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/Model/Crister.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Renderer/ModelRenderer.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates every pass renderer, sharing the engine-wide root signature and
	* pipeline-state / raytracing-state caches with them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全パスのレンダラーを生成し、エンジン共通のルートシグネチャと
	* パイプラインステート/レイトレーシングステートのキャッシュを共有させる。
	*/
	RaytracingRenderer::RaytracingRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject, RaytracingStateObject& raytracingStateObject) : skinBlendShader_(rootSignature, pipelineStateObject), morphBlendShader_(rootSignature, pipelineStateObject)
	{
		shadowRenderer_ = MakePtr<ShadowRenderer>(rootSignature, pipelineStateObject);
		ambientOcclusionRenderer_ = MakePtr<AmbientOcclusionRenderer>(rootSignature, pipelineStateObject);
		subsurfaceScatteringRenderer_ = MakePtr<SubsurfaceScatteringRenderer>(rootSignature, pipelineStateObject);
		reflectionRenderer_ = MakePtr<ReflectionRenderer>(rootSignature, raytracingStateObject, pipelineStateObject);
		refractionRenderer_ = MakePtr<RefractionRenderer>(rootSignature, raytracingStateObject);
		globalIlluminationRenderer_ = MakePtr<GlobalIlluminationRenderer>(rootSignature, raytracingStateObject, pipelineStateObject);
		volumetricCloudScapesRenderer_ = MakePtr<VolumetricCloudScapesRenderer>(rootSignature, pipelineStateObject);
		volumetricStarRenderer_ = MakePtr<VolumetricStarRenderer>(rootSignature, pipelineStateObject);
		weatherParticleRenderer_ = MakePtr<WeatherParticleRenderer>(rootSignature, pipelineStateObject);
		volumetricLightRenderer_ = MakePtr<VolumetricLightRenderer>(rootSignature, pipelineStateObject);
	}

	/**
	* [EN]
	* Reserves one TLAS bindless slot per frame-ring slot, creates every pass
	* at width x height, and compiles the morph and skin blend shaders.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームリングスロットごとに TLAS の bindless スロットを 1 つ確保し、
	* 全パスを width x height で生成し、モーフとスキンのブレンドシェーダを
	* コンパイルする。
	*/
	void RaytracingRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
	{
		bindlessHeap_ = bindlessHeap;
		shaderResourceIndicesSystem_ = &shaderResourceIndicesSystem;

		/// [EN] One TLAS view per frame-ring slot, so writing this frame's view never touches the one an in-flight frame still reads.
		/// [JP] フレームリングスロットごとに TLAS のビューを 1 つ持ち、今フレームのビューを書いても実行中のフレームが読むビューには触れないようにする。
		for (Uint frame = 0; frame < FrameRing::frameCount; frame++)
		{
			tlasBindlessIndices_[frame] = bindlessHeap->AllocateIndex();
		}

		shadowRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);
		ambientOcclusionRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);
		subsurfaceScatteringRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);
		reflectionRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);
		refractionRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);
		globalIlluminationRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);
		volumetricCloudScapesRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);
		volumetricStarRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);
		weatherParticleRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem);
		volumetricLightRenderer_->Create(device, bindlessHeap, shaderCache, constantIndicesSystem, shaderResourceIndicesSystem, unorderedAccessIndicesSystem, width, height);

		skinBlendShader_.Create(shaderCache, device);
		morphBlendShader_.Create(shaderCache, device);
	}

	/**
	* [EN]
	* Resizes every screen-sized pass to the new native resolution.
	* VolumetricLight is left alone because its froxel volume is a fixed grid,
	* and the BLAS/TLAS are left alone because they are sized by geometry, not
	* by the screen.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 画面サイズに依存する全パスを新しいネイティブ解像度にリサイズする。
	* VolumetricLight は froxel ボリュームが固定グリッドのため対象外。
	* BLAS/TLAS も画面ではなくジオメトリでサイズが決まるため対象外。
	*/
	void RaytracingRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
	{
		shadowRenderer_->Resize(device, width, height);
		ambientOcclusionRenderer_->Resize(device, width, height);
		subsurfaceScatteringRenderer_->Resize(device, width, height);
		reflectionRenderer_->Resize(device, width, height);
		refractionRenderer_->Resize(device, width, height);
		globalIlluminationRenderer_->Resize(device, width, height);
		volumetricCloudScapesRenderer_->Resize(device, width, height);
		volumetricStarRenderer_->Resize(device, width, height);
	}

	/**
	* [EN]
	* Collects one PendingInstance per active Mesh actor, queues meshes whose
	* static BLAS is not cached yet, and prunes the caches of meshes and
	* entities that are gone. Records no GPU work.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 有効な Mesh アクターごとに PendingInstance を 1 つ集め、静的 BLAS が
	* 未キャッシュのメッシュを積み、消えたメッシュやエンティティの
	* キャッシュを刈る。GPU の処理は記録しない。
	*/
	void RaytracingRenderer::Gather(LoaderSystem& loaderSystem, ModelResource& modelResource, World& world, const ModelRenderer& modelRenderer)
	{
		pendingInstances_.clear();
		pendingBlasBuilds_.clear();

		/// [EN] Meshes and entities seen this frame. Caches keyed by Crister* or EntityID are pruned against these, because a freed address or ID can be reused by an unrelated new object that would otherwise pick up a stale BLAS or material table.
		/// [JP] 今フレーム見えたメッシュとエンティティ。Crister* や EntityID をキーとするキャッシュはこれで刈る。解放されたアドレスや ID は無関係な新しいオブジェクトに再利用され得るため、残しておくと古い BLAS やマテリアルテーブルを拾ってしまう。
		std::unordered_set<const Crister*> liveCristers;
		std::unordered_set<EntityID> liveEntities;

		Query<Read<Active>, Read<Mesh>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, const Mesh& mesh)
			{
				if (!active.active_)
				{
					return;
				}

				/// [EN] Skip actors whose mesh is not loaded yet or has no triangles; a BLAS cannot be built from them.
				/// [JP] メッシュが未ロード、または三角形を持たないアクターは BLAS を構築できないため飛ばす。
				Handle<Crister> cristerHandle = modelResource.GetHandle(mesh.meshID_);
				if (cristerHandle.empty())
				{
					return;
				}

				Crister* crister = modelResource.Resolve(loaderSystem, cristerHandle);
				if (!crister || crister->IndexCount() == 0)
				{
					return;
				}

				Actor actor = world.GetActor(entityID);
				if (!actor)
				{
					return;
				}

				PendingInstance instance{};
				instance.crister_ = crister;
				instance.worldMatrix_ = actor.WorldMatrix();
				instance.entityID_ = entityID;

				/// [EN] A skinned mesh with a pose written by ModelRenderer this frame takes the skinned-BLAS path.
				/// [JP] 今フレーム ModelRenderer がポーズを書いたスキンメッシュは、スキン済み BLAS の経路を通る。
				const Animator* animator = actor.GetComponent<Animator>();
				if (animator && crister->ProxySkinned())
				{
					instance.hasSkeletalPose_ = modelRenderer.TryGetAnimatedBoneOffset(entityID, instance.boneOffset_);
				}

				/// [EN] Animated morph weights are sampled per Node. Each morphed SubMesh takes the weights of the first Node that references its mesh, since in glTF every Node referencing a mesh shares one weight array for it.
				/// [JP] アニメーションのモーフウェイトは Node ごとにサンプリングされる。glTF ではメッシュを参照する全 Node がそのメッシュのウェイト配列を 1 つ共有するため、モーフのある各 SubMesh は、そのメッシュを参照する最初の Node のウェイトを使う。
				if (animator)
				{
					const DynamicArray<SubMesh>& subMeshes = crister->SubMeshes();
					const DynamicArray<Node>& nodes = crister->Nodes();
					instance.morphWeights_.resize(subMeshes.size());

					for (Size subMeshIndex = 0; subMeshIndex < subMeshes.size(); subMeshIndex++)
					{
						const SubMesh& subMesh = subMeshes[subMeshIndex];
						if (subMesh.morphs_.empty())
						{
							continue;
						}

						auto owner = std::ranges::find_if(nodes, [&](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
						if (owner == nodes.end())
						{
							continue;
						}

						DynamicArray<Float> weights;
						if (modelRenderer.TryGetAnimatedMorphWeights(entityID, static_cast<Int>(std::distance(nodes.begin(), owner)), weights) && !weights.empty())
						{
							instance.morphWeights_[subMeshIndex] = std::move(weights);
							instance.hasMorphWeights_ = true;
						}
					}
				}

				/// [EN] Everything except a skinned instance needs the static BLAS, either as its shape or as the fallback of a morphed one.
				/// [JP] スキン付き以外のインスタンスは全て静的 BLAS を必要とする。形状そのものとして、またはモーフ付きのフォールバックとして使う。
				if (!instance.hasSkeletalPose_ && !blasCache_.contains(crister) && std::ranges::find(pendingBlasBuilds_, crister) == pendingBlasBuilds_.end())
				{
					pendingBlasBuilds_.push_back(crister);
				}

				liveCristers.insert(crister);
				liveEntities.insert(entityID);
				pendingInstances_.push_back(std::move(instance));
			});

		/// [EN] Static BLAS and material tables are shared by the whole frame ring, so a mesh that disappears is erased only after frameCount frames without being seen, once no in-flight frame can still trace it.
		/// [JP] 静的 BLAS とマテリアルテーブルはフレームリング全体で共有するため、見えなくなったメッシュは frameCount フレーム見えないままになってから、つまり実行中のフレームがもうトレースしなくなってから消去する。
		for (const Crister* crister : liveCristers)
		{
			pendingBlasEviction_.erase(crister);
		}

		for (auto& [crister, framesRemaining] : pendingBlasEviction_)
		{
			if (framesRemaining > 0)
			{
				framesRemaining--;
			}
		}

		std::erase_if(pendingBlasEviction_, [&](const auto& entry)
			{
				if (entry.second > 0)
				{
					return false;
				}
				blasCache_.erase(entry.first);
				materialTableCache_.erase(entry.first);
				return true;
			});

		for (const auto& [crister, blas] : blasCache_)
		{
			if (!liveCristers.contains(crister) && !pendingBlasEviction_.contains(crister))
			{
				pendingBlasEviction_[crister] = FrameRing::frameCount;
			}
		}

		/// [EN] Per-entity caches are already per frame-ring slot, so this slot's entries can be erased as soon as their entity is gone.
		/// [JP] エンティティごとのキャッシュは既にフレームリングスロットごとなので、このスロットのエントリはエンティティが消えた時点で消去できる。
		Uint frameIndex = FrameRing::Index();
		auto isStale = [&](const auto& entry) { return !liveEntities.contains(entry.first); };
		std::erase_if(skinnedBlasCache_[frameIndex], isStale);
		std::erase_if(morphedBlasCache_[frameIndex], isStale);

		/// [EN] Position buffers hand their bindless views and resource to the heap by hand. DispatchBuffers and weight buffers defer their own release, so they are simply dropped once their entity is gone.
		/// [JP] 位置バッファは bindless のビューとリソースを自分でヒープへ渡す。DispatchBuffer とウェイトバッファは自身で解放を遅延させるため、エンティティが消えたら捨てるだけでよい。
		auto releaseStalePositions = [&](std::unordered_map<EntityID, SkinnedPositionBuffer>& buffers)
			{
				std::erase_if(buffers, [&](const auto& entry)
					{
						if (!isStale(entry))
						{
							return false;
						}
						bindlessHeap_->Release(entry.second.resource_, { entry.second.shaderResourceViewIndex_, entry.second.unorderedAccessViewIndex_ });
						return true;
					});
			};
		releaseStalePositions(skinnedPositionBuffers_[frameIndex]);
		releaseStalePositions(morphedPositionBuffers_[frameIndex]);
		std::erase_if(skinBlendDispatchBuffers_, isStale);
		std::erase_if(morphBlendBuffers_, isStale);
	}

	/**
	* [EN]
	* Records this frame's BLAS/TLAS work, publishes the TLAS index, then
	* prepares every pass. The BLAS/TLAS work runs only while the device is
	* alive and some pass that traces the TLAS is on; otherwise the TLAS index
	* stays 0xFFFFFFFF and every such pass falls back to its clear.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 今フレームの BLAS/TLAS の処理を記録し、TLAS のインデックスを公開して
	* から全パスを準備する。BLAS/TLAS の処理はデバイスが生きていて、かつ
	* TLAS をトレースするパスが 1 つでも有効なときだけ行う。そうでなければ
	* TLAS のインデックスは 0xFFFFFFFF のままで、該当するパスは全て
	* フォールバックのクリアになる。
	*/
	void RaytracingRenderer::Build(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, ID3D12Device* device, const ModelRenderer& modelRenderer, Float deltaTime, Float nightFactor, const Vector3& cameraPosition, Float totalTime, const WeatherGpuState& weather)
	{
		/// [EN] After device removal every D3D12 call fails, so the acceleration structures are skipped and the real removal reason is logged once.
		/// [JP] デバイス削除後は全ての D3D12 呼び出しが失敗するため、加速構造は省き、本当の削除理由を 1 度だけログに出す。
		HRESULT deviceRemovedReason = device->GetDeviceRemovedReason();
		if (FAILED(deviceRemovedReason) && !deviceRemovedLogged_)
		{
			_com_error removedError(deviceRemovedReason);
			SC_LOG_ERROR("GPU デバイスが削除されているため、レイトレーシングを停止しました。理由: {:#010x} ({})", static_cast<Uint32>(deviceRemovedReason), ConvertToCharString(removedError.ErrorMessage()));
			deviceRemovedLogged_ = true;
		}

		/// [EN] Clouds and stars march their own volumes without the TLAS, so they do not count here.
		/// [JP] 雲と星は TLAS を使わず自前のボリュームをマーチするため、ここでは数えない。
		Bool tlasRequired = shadowEnabled_ || ambientOcclusionEnabled_ || subsurfaceScatteringEnabled_ || reflectionEnabled_ || refractionEnabled_ || globalIlluminationEnabled_ || volumetricLightEnabled_;

		/// [EN] Set once this frame's TLAS is built and its view written; until then the shaders see no TLAS.
		/// [JP] 今フレームの TLAS を構築し、ビューを書き込んだら立てる。それまでシェーダには TLAS が無いように見える。
		Bool tlasBuilt = false;

		if (SUCCEEDED(deviceRemovedReason) && tlasRequired)
		{
			/// [EN] The device is always created as ID3D12Device5 and the command list as ID3D12GraphicsCommandList6, so the DXR build APIs need no capability check.
			/// [JP] デバイスは常に ID3D12Device5、コマンドリストは ID3D12GraphicsCommandList6 として生成されるため、DXR の構築 API に能力チェックは不要。
			ID3D12Device5* device5 = static_cast<ID3D12Device5*>(device);
			ID3D12GraphicsCommandList4* commandList4 = cmdList->Get();
			Uint frameIndex = FrameRing::Index();

			/// [EN] Creates a plain linear buffer (one row-major row, no format, no multisampling) of size bytes on heapType, starting in state.
			/// [JP] heapType 上に size バイトのただの線形バッファ(フォーマット無し、マルチサンプル無しの行優先 1 行)を state で生成する。
			auto createBuffer = [&](Uint64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, const Wchar* name)
				{
					D3D12_HEAP_PROPERTIES heapProperties{};
					heapProperties.Type = heapType;

					D3D12_RESOURCE_DESC resourceDesc{};
					resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					resourceDesc.Width = size;
					resourceDesc.Height = 1;
					resourceDesc.DepthOrArraySize = 1;
					resourceDesc.MipLevels = 1;
					resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
					resourceDesc.SampleDesc.Count = 1;
					resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					resourceDesc.Flags = flags;

					SC_HR_CHECK(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, state, nullptr, IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())), "レイトレーシング用バッファの生成に失敗しました");
#ifdef _DEBUG
					resource->SetName(name);
					GFSDK_Aftermath_DX12_UpdateResourceInfo(resource.Get());
#endif
				};

			/// [EN] Uploads count elements of stride bytes from data into a new UPLOAD-heap buffer, and writes a structured-buffer view of it into a new bindless slot. An empty read range tells the driver the CPU never reads the mapping back.
			/// [JP] data から stride バイトの要素 count 個を新しい UPLOAD ヒープのバッファへアップロードし、その構造化バッファのビューを新しい bindless スロットへ書き込む。読み取り範囲を空にし、CPU がマップを読み返さないことをドライバに伝える。
			auto upload = [&](const void* data, Uint32 count, Uint32 stride, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Uint32& viewIndex, const Wchar* name)
				{
					createBuffer(static_cast<Uint64>(count) * stride, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, resource, name);

					void* mapped = nullptr;
					D3D12_RANGE readRange{};
					SC_HR_CHECK(resource->Map(0, &readRange, &mapped), "マテリアルテーブルのMapに失敗しました");
					memcpy(mapped, data, static_cast<Size>(count) * stride);
					resource->Unmap(0, nullptr);

					viewIndex = bindlessHeap_->AllocateIndex();
					D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
					shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
					shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					shaderResourceViewDesc.Buffer.NumElements = count;
					shaderResourceViewDesc.Buffer.StructureByteStride = stride;
					device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(viewIndex));
				};

			/// [EN] Builds blas from crister's triangles with positions read from vertexBuffer (the base, morphed or skinned positions). The mesh is opaque when every material is, which lets traversal skip any-hit.
			/// [JP] crister の三角形と、vertexBuffer から読む位置(ベース、モーフ済み、スキン済みのいずれか)から blas を構築する。全マテリアルが不透明ならメッシュも不透明にし、走査で any-hit を省けるようにする。
			auto buildBlas = [&](BottomLevelAccelerationStructure& blas, const Crister* crister, D3D12_GPU_VIRTUAL_ADDRESS vertexBuffer)
				{
					BottomLevelGeometryDesc geometryDesc{};
					geometryDesc.vertexBuffer_ = vertexBuffer;
					geometryDesc.vertexCount_ = crister->VertexCount();
					geometryDesc.vertexStride_ = sizeof(Vector3);
					geometryDesc.vertexFormat_ = DXGI_FORMAT_R32G32B32_FLOAT;
					geometryDesc.indexBuffer_ = crister->IndexBufferAddress();
					geometryDesc.indexCount_ = crister->IndexCount();
					geometryDesc.indexFormat_ = DXGI_FORMAT_R32_UINT;
					geometryDesc.opaque_ = std::ranges::all_of(crister->Surfaces(), [](const Surface& material) { return material.alphaMode_ == 0; });
					return blas.Build(device5, commandList4, &geometryDesc, 1);
				};

			/// [EN] Grows buffer to hold vertexCount positions, as a GPU-only buffer compute shaders write through a UAV, and points its bindless SRV and UAV at it. Each frame-ring slot has its own buffer and the slot's previous frame has finished, so the views are rewritten in place.
			/// [JP] buffer を vertexCount 個の位置が入るまで拡張する。コンピュートシェーダが UAV 経由で書き込む GPU 専用バッファで、bindless の SRV と UAV をそこへ向ける。フレームリングスロットごとに別のバッファで、そのスロットの前のフレームは終わっているため、ビューはその場で書き換える。
			auto reservePositions = [&](SkinnedPositionBuffer& buffer, Uint32 vertexCount, const Wchar* name)
				{
					if (buffer.capacity_ >= vertexCount)
					{
						return;
					}
					createBuffer(sizeof(Vector3) * static_cast<Uint64>(vertexCount), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, buffer.resource_, name);
					buffer.capacity_ = vertexCount;
					buffer.state_ = D3D12_RESOURCE_STATE_COMMON;

					if (buffer.shaderResourceViewIndex_ == SC_INVALID)
					{
						buffer.shaderResourceViewIndex_ = bindlessHeap_->AllocateIndex();
						buffer.unorderedAccessViewIndex_ = bindlessHeap_->AllocateIndex();
					}

					D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
					shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
					shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					shaderResourceViewDesc.Buffer.NumElements = vertexCount;
					shaderResourceViewDesc.Buffer.StructureByteStride = sizeof(Vector3);
					device->CreateShaderResourceView(buffer.resource_.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(buffer.shaderResourceViewIndex_));

					D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
					unorderedAccessViewDesc.Format = DXGI_FORMAT_UNKNOWN;
					unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					unorderedAccessViewDesc.Buffer.NumElements = vertexCount;
					unorderedAccessViewDesc.Buffer.StructureByteStride = sizeof(Vector3);
					device->CreateUnorderedAccessView(buffer.resource_.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap_->CPUHandle(buffer.unorderedAccessViewIndex_));
				};

			/// [EN] Moves buffer to after, recording the transition from the state it was left in. A buffer already in after needs no barrier.
			/// [JP] buffer を after へ遷移させる。置かれていた状態からの遷移として記録する。既に after にあるバッファにはバリアは要らない。
			auto transition = [&](SkinnedPositionBuffer& buffer, D3D12_RESOURCE_STATES after)
				{
					if (buffer.state_ == after)
					{
						return;
					}
					D3D12_RESOURCE_BARRIER barrier{};
					barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barrier.Transition.pResource = buffer.resource_.Get();
					barrier.Transition.StateBefore = buffer.state_;
					barrier.Transition.StateAfter = after;
					barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					commandList4->ResourceBarrier(1, &barrier);
					buffer.state_ = after;
				};

			/// [EN] Static BLAS for the meshes Gather queued; built once and cached until the mesh is evicted.
			/// [JP] Gather が積んだメッシュの静的 BLAS。1 度だけ構築し、メッシュが破棄されるまでキャッシュする。
			for (const Crister* crister : pendingBlasBuilds_)
			{
				ResourcePtr<BottomLevelAccelerationStructure> blas = MakePtr<BottomLevelAccelerationStructure>();
				if (buildBlas(*blas, crister, crister->PositionBufferAddress()))
				{
					blasCache_.emplace(crister, std::move(blas));
				}
				else if (!blasBuildFailureLogged_)
				{
					SC_LOG_WARNING("BLAS の構築に失敗しました。DXR(レイトレーシング Tier 1.0 以上)非対応ハードウェアの可能性があります。影は常に照射(1.0)として扱われます。");
					blasBuildFailureLogged_ = true;
				}
			}
			pendingBlasBuilds_.clear();

			/// [EN] Material tables. Every instance, skinned or not, resolves its hit material through its mesh's table, so each mesh in the scene needs one. A table is also rebuilt when texture streaming has moved a material's texture to another bindless slot (MaterialsDirty).
			/// [JP] マテリアルテーブル。スキンの有無に関わらず、全インスタンスはメッシュのテーブルでヒットしたマテリアルを解決するため、シーンの各メッシュにテーブルが要る。テクスチャストリーミングでマテリアルのテクスチャが別の bindless スロットへ移ったとき(MaterialsDirty)も再構築する。
			std::unordered_set<const Crister*> visitedCristers;
			for (const PendingInstance& pending : pendingInstances_)
			{
				const Crister* crister = pending.crister_;
				if (!visitedCristers.insert(crister).second || (materialTableCache_.contains(crister) && !crister->MaterialsDirty()))
				{
					continue;
				}
				crister->ClearMaterialsDirty();

				/// [EN] The subset of each glTF material the ray-traced passes shade with: base color, transmission/volume for refraction, and the alpha mode for alpha testing. A mesh without materials gets one default (white) material.
				/// [JP] レイトレーシングのパスがシェーディングに使う glTF マテリアルの一部。ベースカラー、屈折用の透過/ボリューム、アルファテスト用のアルファモード。マテリアルの無いメッシュにはデフォルト(白)のマテリアルを 1 つ与える。
				const DynamicArray<Surface>& surfaces = crister->Surfaces();
				DynamicArray<ReflectionMaterialData> materials(Max(surfaces.size(), static_cast<Size>(1)));
				for (Size materialIndex = 0; materialIndex < surfaces.size(); materialIndex++)
				{
					const Surface& surface = surfaces[materialIndex];
					ReflectionMaterialData& material = materials[materialIndex];
					material.baseColor_[0] = surface.baseColor_.R();
					material.baseColor_[1] = surface.baseColor_.G();
					material.baseColor_[2] = surface.baseColor_.B();
					material.baseColorAlpha_ = surface.baseColor_.A();
					material.baseColorTextureIndex_ = static_cast<Uint32>(crister->TextureBindlessIndex(surface.baseColorTextureIndex_));
					material.ior_ = surface.khr_.ior_.ior_;
					material.transmissionFactor_ = surface.khr_.transmission_.transmissionFactor_;
					material.volumeAttenuationColor_[0] = surface.khr_.volume_.attenuationColor_[0];
					material.volumeAttenuationColor_[1] = surface.khr_.volume_.attenuationColor_[1];
					material.volumeAttenuationColor_[2] = surface.khr_.volume_.attenuationColor_[2];
					material.volumeAttenuationDistance_ = surface.khr_.volume_.attenuationDistance_;
					material.thicknessFactor_ = surface.khr_.volume_.thicknessFactor_;
					material.thicknessTextureIndex_ = static_cast<Uint32>(crister->TextureBindlessIndex(surface.khr_.volume_.thicknessTextureIndex_));
					material.alphaMode_ = static_cast<Uint32>(surface.alphaMode_);
					material.alphaCutoff_ = surface.alphaCutoff_;
				}

				/// [EN] Each SubMesh writes its surfaceIndex_ over its triangle range in the RT proxy, which already covers every node placement of that SubMesh. A triangle no SubMesh covers stays on material 0.
				/// [JP] 各 SubMesh は RT プロキシ内の自分の三角形範囲へ surfaceIndex_ を書き込む。この範囲はその SubMesh の全ノード配置を含む。どの SubMesh にも属さない三角形はマテリアル 0 のまま。
				Uint32 triangleCount = Max(crister->IndexCount() / 3, 1u);
				DynamicArray<Uint32> triangleMaterialIndices(triangleCount, 0);
				for (const SubMesh& subMesh : crister->SubMeshes())
				{
					Uint32 triangleEnd = Min(subMesh.raytracingTriangleOffset_ + subMesh.raytracingTriangleCount_, triangleCount);
					for (Uint32 triangleIndex = subMesh.raytracingTriangleOffset_; triangleIndex < triangleEnd; triangleIndex++)
					{
						triangleMaterialIndices[triangleIndex] = subMesh.surfaceIndex_;
					}
				}

				MaterialTable& table = materialTableCache_[crister];
				upload(materials.data(), static_cast<Uint32>(materials.size()), sizeof(ReflectionMaterialData), table.materialsResource_, table.materialsShaderResourceViewIndex_, L"Raytracing_MaterialsTable");
				upload(triangleMaterialIndices.data(), triangleCount, sizeof(Uint32), table.triangleMaterialIndexResource_, table.triangleMaterialIndexShaderResourceViewIndex_, L"Raytracing_TriangleMaterialIndexTable");
			}

			/// [EN] The morph and skin blend passes run on the shared root signature like every other pass. Each dispatch only swaps the pipeline and the per-dispatch root constant (root parameter 3, the bindless index of its DispatchBuffer).
			/// [JP] モーフとスキンのブレンドパスは、他のパスと同じく共有のルートシグネチャで動く。ディスパッチごとに差し替えるのは、パイプラインとディスパッチごとのルート定数(ルートパラメータ 3、DispatchBuffer の bindless インデックス)だけ。
			ID3D12DescriptorHeap* heaps[] = { heap };
			commandList4->SetDescriptorHeaps(_countof(heaps), heaps);
			commandList4->SetComputeRootSignature(skinBlendShader_.GetRootSignature());
			RootSignature::BindCompute(commandList4, addresses);

			/// [EN] Morph pass. Morph composes before skin: the blended buffer starts as a copy of the base positions, then MorphBlendCS overwrites the vertex range of each SubMesh with weights this frame. A morph-only instance builds its BLAS from it right away; a skinned one reads it as its skin input below.
			/// [JP] モーフのパス。モーフはスキンより前に合成する。ブレンド用バッファはベース位置のコピーから始め、今フレームウェイトのある各 SubMesh の頂点範囲を MorphBlendCS が上書きする。モーフのみのインスタンスはそこからすぐ BLAS を構築し、スキン付きのものは下でスキンの入力として読む。
			for (const PendingInstance& pending : pendingInstances_)
			{
				const Crister* crister = pending.crister_;
				if (!pending.hasMorphWeights_ || crister->VertexCount() == 0)
				{
					continue;
				}

				SkinnedPositionBuffer& blendedBuffer = morphedPositionBuffers_[frameIndex][pending.entityID_];
				reservePositions(blendedBuffer, crister->VertexCount(), L"MorphedPositionBuffer");

				transition(blendedBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
				crister->CopyMorph(commandList4, blendedBuffer.resource_.Get());
				transition(blendedBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				const DynamicArray<SubMesh>& subMeshes = crister->SubMeshes();
				DynamicArray<MorphBlendBuffer>& blendBuffers = morphBlendBuffers_[pending.entityID_];
				if (blendBuffers.size() < subMeshes.size())
				{
					blendBuffers.resize(subMeshes.size());
				}

				for (Size subMeshIndex = 0; subMeshIndex < pending.morphWeights_.size(); subMeshIndex++)
				{
					const DynamicArray<Float>& weights = pending.morphWeights_[subMeshIndex];
					const SubMesh& subMesh = subMeshes[subMeshIndex];
					if (weights.empty() || subMesh.raytracingMorphDeltaOffset_ == 0xFFFFFFFFu || subMesh.raytracingVertexCount_ == 0)
					{
						continue;
					}

					/// [EN] The SubMesh's morph target count never changes, so its weight and DispatchBuffer are created on first use and only rewritten after that.
					/// [JP] SubMesh のモーフターゲット数は変わらないため、ウェイトと DispatchBuffer は初回に生成し、以降は書き換えるだけ。
					Uint32 targetCount = static_cast<Uint32>(subMesh.morphs_.size());
					MorphBlendBuffer& blendBuffer = blendBuffers[subMeshIndex];
					if (!blendBuffer.weights_)
					{
						blendBuffer.weights_ = MakePtr<ReadOnlyStructuredBuffer<Float>>(device, bindlessHeap_, targetCount);
						blendBuffer.dispatchBuffer_ = MakePtr<StaticConstantBuffer<MorphBlendDispatchBuffer>>(device, bindlessHeap_);
					}
					blendBuffer.weights_->Update(weights.data(), static_cast<Uint>(Min(weights.size(), static_cast<Size>(targetCount))));
					blendBuffer.dispatchBuffer_->Update(MorphBlendDispatchBuffer{ subMesh.raytracingVertexOffset_, subMesh.raytracingVertexCount_, targetCount, subMesh.raytracingMorphDeltaOffset_, crister->PositionBufferIndex(), crister->ProxyMorphDeltaBufferIndex(), blendBuffer.weights_->Index(), blendedBuffer.unorderedAccessViewIndex_ });

					Uint dispatchBufferIndex = blendBuffer.dispatchBuffer_->Index();
					commandList4->SetPipelineState(morphBlendShader_.GetPipelineState());
					commandList4->SetComputeRoot32BitConstants(3, 1, &dispatchBufferIndex, 0);
					commandList4->Dispatch((subMesh.raytracingVertexCount_ + 63) / 64, 1, 1);
				}

				/// [EN] The blended positions are read next, as BLAS vertices or as the skin input; the transition also waits for the dispatches above.
				/// [JP] ブレンド済み位置は次に BLAS の頂点かスキンの入力として読まれる。この遷移は上のディスパッチの完了待ちも兼ねる。
				transition(blendedBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				if (pending.hasSkeletalPose_)
				{
					continue;
				}

				ResourcePtr<BottomLevelAccelerationStructure>& morphedBlas = morphedBlasCache_[frameIndex][pending.entityID_];
				if (!morphedBlas)
				{
					morphedBlas = MakePtr<BottomLevelAccelerationStructure>();
				}
				if (!buildBlas(*morphedBlas, crister, blendedBuffer.resource_->GetGPUVirtualAddress()) && !morphedBlasBuildFailureLogged_)
				{
					SC_LOG_WARNING("モーフ付き BLAS の構築に失敗しました。対象アクターはモーフ無しの静止形状で描画されます。");
					morphedBlasBuildFailureLogged_ = true;
				}
			}

			/// [EN] Skin pass. SkinBlendCS skins the positions with the bone matrices ModelRenderer wrote this frame, and the entity's BLAS is rebuilt from the result.
			/// [JP] スキンのパス。今フレーム ModelRenderer が書いたボーン行列で SkinBlendCS が位置をスキニングし、その結果からエンティティの BLAS を再構築する。
			for (const PendingInstance& pending : pendingInstances_)
			{
				if (!pending.hasSkeletalPose_)
				{
					continue;
				}

				const Crister* crister = pending.crister_;
				Uint32 vertexCount = crister->VertexCount();

				SkinnedPositionBuffer& skinnedBuffer = skinnedPositionBuffers_[frameIndex][pending.entityID_];
				reservePositions(skinnedBuffer, vertexCount, L"SkinnedPositionBuffer");
				transition(skinnedBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				/// [EN] Skin the morph-blended positions when the morph pass produced them, the base positions otherwise.
				/// [JP] モーフのパスがブレンド済み位置を作っていればそれを、無ければベース位置をスキニングする。
				Uint32 inputPositionIndex = crister->PositionBufferIndex();
				auto morphed = morphedPositionBuffers_[frameIndex].find(pending.entityID_);
				if (pending.hasMorphWeights_ && morphed != morphedPositionBuffers_[frameIndex].end())
				{
					inputPositionIndex = morphed->second.shaderResourceViewIndex_;
				}

				ResourcePtr<StaticConstantBuffer<SkinBlendDispatchBuffer>>& dispatchBuffer = skinBlendDispatchBuffers_[pending.entityID_];
				if (!dispatchBuffer)
				{
					dispatchBuffer = MakePtr<StaticConstantBuffer<SkinBlendDispatchBuffer>>(device, bindlessHeap_);
				}
				dispatchBuffer->Update(SkinBlendDispatchBuffer{ vertexCount, pending.boneOffset_, inputPositionIndex, crister->ProxySkinVertexBufferIndex(), modelRenderer.BoneMatrixBufferIndex(), skinnedBuffer.unorderedAccessViewIndex_, 0, 0 });

				Uint dispatchBufferIndex = dispatchBuffer->Index();
				commandList4->SetPipelineState(skinBlendShader_.GetPipelineState());
				commandList4->SetComputeRoot32BitConstants(3, 1, &dispatchBufferIndex, 0);
				commandList4->Dispatch((vertexCount + 63) / 64, 1, 1);

				/// [EN] The skinned positions are read next as BLAS vertices; the transition also waits for the dispatch.
				/// [JP] スキン済み位置は次に BLAS の頂点として読まれる。この遷移はディスパッチの完了待ちも兼ねる。
				transition(skinnedBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				ResourcePtr<BottomLevelAccelerationStructure>& skinnedBlas = skinnedBlasCache_[frameIndex][pending.entityID_];
				if (!skinnedBlas)
				{
					skinnedBlas = MakePtr<BottomLevelAccelerationStructure>();
				}
				if (!buildBlas(*skinnedBlas, crister, skinnedBuffer.resource_->GetGPUVirtualAddress()) && !skinnedBlasBuildFailureLogged_)
				{
					SC_LOG_WARNING("スキン付き BLAS の構築に失敗しました。対象アクターは TLAS から除外されます。");
					skinnedBlasBuildFailureLogged_ = true;
				}
			}

			/// [EN] Address of the BLAS cached under key, or 0 when there is none or it has never built successfully.
			/// [JP] key でキャッシュされた BLAS のアドレス。無いか、1 度も構築に成功していなければ 0。
			auto blasAddress = [](const auto& cache, const auto& key) -> D3D12_GPU_VIRTUAL_ADDRESS
				{
					auto found = cache.find(key);
					return found != cache.end() ? found->second->Address() : 0;
				};

			/// [EN] TLAS instances, and the instance table the closest-hit shaders index with InstanceID(), filled in the same order so instanceID_ is the position in both.
			/// [JP] TLAS インスタンスと、closest-hit シェーダが InstanceID() で引くインスタンステーブル。同じ順序で詰め、instanceID_ が両方での位置になるようにする。
			DynamicArray<TopLevelInstanceDesc> instanceDescs;
			DynamicArray<ReflectionInstanceData> reflectionInstances;
			instanceDescs.reserve(pendingInstances_.size());
			reflectionInstances.reserve(pendingInstances_.size());

			for (const PendingInstance& pending : pendingInstances_)
			{
				if (instanceDescs.size() >= maxInstances_)
				{
					if (!instanceLimitLogged_)
					{
						SC_LOG_WARNING("レイトレーシングのインスタンス数が上限({})に達したため、超過分を TLAS から除外しました。", maxInstances_);
						instanceLimitLogged_ = true;
					}
					break;
				}

				/// [EN] A skinned instance uses only its skinned BLAS. A morphed one falls back to the static bind-pose BLAS when its morphed BLAS is missing. An instance with no usable BLAS is dropped, because a null BLAS address breaks the TLAS build.
				/// [JP] スキン付きのインスタンスはスキン済み BLAS だけを使う。モーフ付きはモーフ済み BLAS が無ければ静的なバインドポーズの BLAS にフォールバックする。使える BLAS が無いインスタンスは除外する。null の BLAS アドレスは TLAS の構築を壊すため。
				D3D12_GPU_VIRTUAL_ADDRESS bottomLevelAddress = 0;
				if (pending.hasSkeletalPose_)
				{
					bottomLevelAddress = blasAddress(skinnedBlasCache_[frameIndex], pending.entityID_);
				}
				else
				{
					if (pending.hasMorphWeights_)
					{
						bottomLevelAddress = blasAddress(morphedBlasCache_[frameIndex], pending.entityID_);
					}
					if (bottomLevelAddress == 0)
					{
						bottomLevelAddress = blasAddress(blasCache_, pending.crister_);
					}
				}
				if (bottomLevelAddress == 0)
				{
					continue;
				}

				/// [EN] Traversal inverts the instance transform to bring rays into BLAS space, so a non-finite matrix makes traversal never terminate. Such instances are dropped.
				/// [JP] 走査はレイを BLAS 空間へ移すためにインスタンス変換の逆行列を取るので、非有限の行列では走査が終わらない。そうしたインスタンスは除外する。
				const Float* matrixBegin = &pending.worldMatrix_.m[0][0];
				if (!std::all_of(matrixBegin, matrixBegin + 16, [](Float value) { return std::isfinite(value); }))
				{
					if (!degenerateInstanceLogged_)
					{
						SC_LOG_WARNING("ワールド行列が非有限なアクターを TLAS から除外しました。アニメーション/IK が NaN を出力している可能性があります。");
						degenerateInstanceLogged_ = true;
					}
					continue;
				}

				/// [EN] Closest-hit shaders re-fetch the hit triangle through the RT-proxy vertex/index views, so an instance whose proxy is still streaming in (views unallocated) is dropped; reading an unallocated descriptor from a hit shader hangs DispatchRays.
				/// [JP] closest-hit シェーダは RT プロキシの頂点/インデックスのビュー経由でヒットした三角形を引き直すため、プロキシがストリーミング中(ビュー未確保)のインスタンスは除外する。ヒットシェーダが未確保のディスクリプタを読むと DispatchRays がハングする。
				if (pending.crister_->VertexBufferIndex() == 0xFFFFFFFF || pending.crister_->IndexBufferIndex() == 0xFFFFFFFF)
				{
					if (!rtProxyNotReadyLogged_)
					{
						SC_LOG_WARNING("RT プロキシ(頂点/インデックスバッファ)が未構築の Crister を TLAS から除外しました。ストリーミング中の可能性があります。");
						rtProxyNotReadyLogged_ = true;
					}
					continue;
				}

				TopLevelInstanceDesc instanceDesc{};
				instanceDesc.bottomLevelAddress_ = bottomLevelAddress;
				instanceDesc.instanceMask_ = 0xFF;
				instanceDesc.instanceID_ = static_cast<Uint32>(instanceDescs.size());
				instanceDesc.hitGroupIndex_ = 0;

				/// [EN] The engine's matrix is row-vector with the translation in row 3 (p' = p * M); DXR's 3x4 is column-vector with the translation in column 3 (p' = M * p). Transpose the upper 3x3 and move the translation row into the last column.
				/// [JP] エンジンの行列は行ベクトルで並進は行 3 (p' = p * M)、DXR の 3x4 は列ベクトルで並進は列 3 (p' = M * p)。左上 3x3 を転置し、並進の行を最後の列へ移す。
				for (Int rowIndex = 0; rowIndex < 3; rowIndex++)
				{
					for (Int columnIndex = 0; columnIndex < 3; columnIndex++)
					{
						instanceDesc.transform_[rowIndex][columnIndex] = pending.worldMatrix_.m[columnIndex][rowIndex];
					}
					instanceDesc.transform_[rowIndex][3] = pending.worldMatrix_.m[3][rowIndex];
				}
				instanceDescs.push_back(instanceDesc);

				/// [EN] The instance's RT-proxy views, its material tables, and the UV quantization range that decodes the compressed vertex UVs (the same range the raster path gets through ModelRenderer).
				/// [JP] インスタンスの RT プロキシのビュー、マテリアルテーブル、圧縮された頂点 UV を復元する UV 量子化範囲(ラスタ経路が ModelRenderer 経由で受け取るものと同じ)。
				ReflectionInstanceData reflectionInstance{};
				reflectionInstance.vertexBufferIndex_ = pending.crister_->VertexBufferIndex();
				reflectionInstance.indexBufferIndex_ = pending.crister_->IndexBufferIndex();

				const MaterialTable& materialTable = materialTableCache_.at(pending.crister_);
				reflectionInstance.materialDataIndex_ = materialTable.materialsShaderResourceViewIndex_;
				reflectionInstance.triangleMaterialIndexBufferIndex_ = materialTable.triangleMaterialIndexShaderResourceViewIndex_;

				Vector2 texcoordMin = pending.crister_->TexcoordMin();
				Vector2 texcoordExtent = pending.crister_->TexcoordExtent();
				reflectionInstance.texcoordMin_[0] = texcoordMin.x;
				reflectionInstance.texcoordMin_[1] = texcoordMin.y;
				reflectionInstance.texcoordExtent_[0] = texcoordExtent.x;
				reflectionInstance.texcoordExtent_[1] = texcoordExtent.y;
				reflectionInstances.push_back(reflectionInstance);
			}

			if (!instanceDescs.empty())
			{
				reflectionRenderer_->UpdateInstanceTable(reflectionInstances.data(), static_cast<Uint32>(reflectionInstances.size()));

				if (tlas_.Build(device5, commandList4, instanceDescs.data(), static_cast<Uint32>(instanceDescs.size())))
				{
					/// [EN] An acceleration-structure view takes no resource; its location comes from the view desc.
					/// [JP] 加速構造のビューはリソースを取らず、位置はビューの desc から与える。
					D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
					shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
					shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
					shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					shaderResourceViewDesc.RaytracingAccelerationStructure.Location = tlas_.Address();
					device->CreateShaderResourceView(nullptr, &shaderResourceViewDesc, bindlessHeap_->CPUHandle(tlasBindlessIndices_[frameIndex]));
					tlasBuilt = true;
				}
				else if (!tlasBuildFailureLogged_)
				{
					SC_LOG_WARNING("TLAS の構築に失敗しました。影は常に照射(1.0)として扱われます。");
					tlasBuildFailureLogged_ = true;
				}
			}
		}

		/// [EN] Published every frame, so the shaders see a consistent 0xFFFFFFFF when no TLAS was built.
		/// [JP] 毎フレーム公開し、TLAS を構築しなかったときはシェーダに一貫して 0xFFFFFFFF が見えるようにする。
		shaderResourceIndicesSystem_->SetTLASIndex(tlasBuilt ? tlasBindlessIndices_[FrameRing::Index()] : 0xFFFFFFFF);

		/// [EN] A pass that traces scene geometry runs only when it is on and a TLAS exists. Clouds, stars and volumetric light depend only on their own switch.
		/// [JP] シーンのジオメトリをトレースするパスは、有効かつ TLAS があるときだけ実行する。雲、星、ボリュメトリックライトは自分のスイッチだけで決まる。
		shadowRenderer_->Prepare(shadowSettings_, tlasBuilt && shadowEnabled_);
		ambientOcclusionRenderer_->Prepare(ambientOcclusionSettings_, tlasBuilt && ambientOcclusionEnabled_);
		subsurfaceScatteringRenderer_->Prepare(subsurfaceScatteringSettings_, tlasBuilt && subsurfaceScatteringEnabled_);
		reflectionRenderer_->Prepare(reflectionSettings_, tlasBuilt && reflectionEnabled_);
		refractionRenderer_->Prepare(refractionSettings_, tlasBuilt && refractionEnabled_);
		globalIlluminationRenderer_->Prepare(globalIlluminationSettings_, tlasBuilt && globalIlluminationEnabled_);
		volumetricCloudScapesRenderer_->Prepare(volumetricCloudScapesSettings_, volumetricCloudScapesEnabled_);
		volumetricStarRenderer_->Prepare(volumetricStarSettings_, volumetricStarEnabled_, deltaTime, nightFactor);
		volumetricLightRenderer_->Prepare(volumetricLightSettings_, volumetricLightEnabled_);

		/// [EN] Rain and snow drift with the cloud wind, scaled from the cloud's scroll speed to particle speed.
		/// [JP] 雨と雪は雲の風に流される。雲のスクロール速度をパーティクルの速度へ拡大して使う。
		Vector3 wind = Vector3(volumetricCloudScapesSettings_.windSpeed_ * 10.0f, 0.0f, 0.0f);
		weatherParticleRenderer_->PrepareFrame(cameraPosition, deltaTime, totalTime, wind, weather.rainEnabled_, weather.rain_, volumetricCloudScapesSettings_.rain_, weather.snowEnabled_, weather.snow_, weather.snowIntensity_);
	}

	/**
	* [EN]
	* Forwards to the pass renderer that type selects. Passes without
	* per-view history take no view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* type が選ぶパスのレンダラーへ転送する。ビューごとの履歴を持たない
	* パスには view を渡さない。
	*/
	void RaytracingRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingType type, RaytracingView view)
	{
		switch (type)
		{
		case RaytracingType::Shadow:
		{
			shadowRenderer_->Dispatch(cmdList, heap, addresses, view);
		}
		break;
		case RaytracingType::AmbientOcclusion:
		{
			ambientOcclusionRenderer_->Dispatch(cmdList, heap, addresses, view);
		}
		break;
		case RaytracingType::SubsurfaceScattering:
		{
			subsurfaceScatteringRenderer_->Dispatch(cmdList, heap, addresses);
		}
		break;
		case RaytracingType::Reflection:
		{
			reflectionRenderer_->Dispatch(cmdList, heap, addresses, view);
		}
		break;
		case RaytracingType::Refraction:
		{
			refractionRenderer_->Dispatch(cmdList, heap, addresses);
		}
		break;
		case RaytracingType::GlobalIllumination:
		{
			globalIlluminationRenderer_->Dispatch(cmdList, heap, addresses, view);
		}
		break;
		case RaytracingType::VolumetricCloudScapes:
		{
			volumetricCloudScapesRenderer_->Dispatch(cmdList, heap, addresses);
		}
		break;
		case RaytracingType::VolumetricStar:
		{
			volumetricStarRenderer_->Dispatch(cmdList, heap, addresses);
		}
		break;
		case RaytracingType::VolumetricLight:
		{
			volumetricLightRenderer_->Dispatch(cmdList, heap, addresses, view);
		}
		break;
		}
	}

	/**
	* [EN]
	* Records the rain/snow particle simulation. It advances one shared
	* world-space simulation, so it runs once per frame, not once per view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 雨/雪パーティクルのシミュレーションを記録する。ワールド空間で共有する
	* シミュレーションを 1 つ進めるため、ビューごとではなくフレームに 1 回
	* 実行する。
	*/
	void RaytracingRenderer::SimulateWeatherParticles(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		weatherParticleRenderer_->Simulate(cmdList, heap, addresses);
	}

	/**
	* [EN]
	* Draws the rain/snow particles for one view from its own camera.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1 つのビューについて、そのカメラから雨/雪パーティクルを描画する。
	*/
	void RaytracingRenderer::DrawWeatherParticles(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, GeometryBuffer* geometryBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		weatherParticleRenderer_->Draw(cmdList, frameBuffer, geometryBuffer, heap, addresses);
	}

	/**
	* [EN]
	* Copies each pass's tuning values and on/off switch. When the sun light
	* is on, its angular radius also sizes the sun disc of the procedural sky,
	* so the disc matches the light that casts the shadows.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各パスの調整値とオン/オフを写す。太陽光が有効なときは、その視半径で
	* プロシージャル空の太陽ディスクの大きさも決め、ディスクと影を落とす
	* 光を一致させる。
	*/
	void RaytracingRenderer::SetRaytracingSettings(const RaytracingContext& settings)
	{
		shadowSettings_ = settings.shadow_;
		shadowEnabled_ = settings.shadowEnabled_;
		ambientOcclusionSettings_ = settings.ambientOcclusion_;
		ambientOcclusionEnabled_ = settings.ambientOcclusionEnabled_;
		subsurfaceScatteringSettings_ = settings.subsurfaceScattering_;
		subsurfaceScatteringEnabled_ = settings.subsurfaceScatteringEnabled_;
		reflectionSettings_ = settings.reflection_;
		reflectionEnabled_ = settings.reflectionEnabled_;
		refractionSettings_ = settings.refraction_;
		refractionEnabled_ = settings.refractionEnabled_;
		globalIlluminationSettings_ = settings.globalIllumination_;
		globalIlluminationEnabled_ = settings.globalIlluminationEnabled_;
		volumetricCloudScapesSettings_ = settings.volumetricCloudScapes_;
		volumetricCloudScapesEnabled_ = settings.volumetricCloudScapesEnabled_;
		volumetricStarSettings_ = settings.volumetricStar_;
		volumetricStarEnabled_ = settings.volumetricStarEnabled_;
		volumetricLightSettings_ = settings.volumetricLight_;
		volumetricLightEnabled_ = settings.volumetricLightEnabled_;

		if (settings.sunLightEnabled_)
		{
			volumetricCloudScapesSettings_.sunSize_ = settings.sunLight_.angularRadius_;
		}
	}
}
