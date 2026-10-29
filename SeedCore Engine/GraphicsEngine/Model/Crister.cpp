#include <External/NsightAftermath/Include/GFSDK_Aftermath.h>
#include <External/DirectXTex/Include/DirectXTex.h>
#include <GraphicsEngine/Model/Crister.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/Texture/Compression/BC7CompressShader.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	/**
	* [EN]
	* Releases every resident streaming page (pinned included — the model
	* itself is going away) and returns their descriptor indices to
	* bindlessHeap_. Every GPU resource is handed to the deferred-reclaim
	* ring rather than dying with this object, since frames still in
	* flight may be drawing from these exact buffers and textures.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 常駐中のストリーミングページをすべて解放し（モデル自体が消えるため
	* ピン留め込み）、ディスクリプタインデックスを bindlessHeap_ へ返す。
	* GPU リソースはこのオブジェクトと一緒に死なせず、遅延回収リングへ
	* 渡す — インフライトのフレームがまさにこれらのバッファやテクスチャで
	* 描画している可能性があるため。
	*/
	Crister::~Crister()
	{
		/// [EN] Leave the shared streaming registry first, so budget eviction never walks into this Crister while it is being torn down.
		/// [JP] まず共有のストリーミングレジストリから外れ、破棄中のこの Crister を予算による追い出しが辿らないようにする。
		auto found = std::ranges::find(streamingRegistry_, this);
		if (found != streamingRegistry_.end())
		{
			streamingRegistry_.erase(found);
		}

		/// [EN] A Crister that never ran Upload() holds no GPU resources and no descriptors, so there is nothing more to release.
		/// [JP] Upload() を通っていない Crister は GPU リソースもディスクリプタも持たないので、これ以上解放するものは無い。
		if (!bindlessHeap_)
		{
			return;
		}

		/// [EN] Release every resident page (pinned included — the model itself is going away) and give the descriptor indices back to the heap. Every GPU resource is handed to the deferred-reclaim ring rather than dying with this object: ModelLoader::Clear destroys the Crister synchronously (ModelResource::Unload, which the editor's model-conversion "適用" runs mid-session), and the frames still in flight are drawing from exactly these buffers and textures.
		/// [JP] 常駐ページをすべて解放し（モデル自体が消えるためピン留め込み）、ディスクリプタインデックスをヒープへ返す。GPU リソースはこのオブジェクトと一緒に死なせず、遅延回収リングへ渡す: ModelLoader::Clear は Crister を同期的に破棄し(ModelResource::Unload — エディタのモデル変換「適用」が実行中に呼ぶ)、インフライトのフレームはまさにこれらのバッファとテクスチャで描画している最中だから。
		for (StreamingGeometry& page : streamingGeometry_)
		{
			if (!page.resident_)
			{
				continue;
			}
			bindlessHeap_->Release(std::move(page.meshletResource_), page.meshletBufferIndex_);
			bindlessHeap_->Release(std::move(page.meshletBoundResource_), page.meshletBoundBufferIndex_);
			bindlessHeap_->Release(std::move(page.vertexIndicesResource_), page.vertexIndicesBufferIndex_);
			bindlessHeap_->Release(std::move(page.primitiveIndicesResource_), page.primitiveIndicesBufferIndex_);

			/// [EN] A page that reads the shared pool has no vertex buffer of its own; the pool is released once below.
			/// [JP] 共有プールを読むページは自前の頂点バッファを持たない。プールは下で1度だけ解放する。
			if (page.ownsVertices_)
			{
				bindlessHeap_->Release(std::move(page.vertexResource_), page.vertexBufferIndex_);
			}

			/// [EN] Take the page out of the geometry total so the budget stays accurate for the remaining Crister.
			/// [JP] 残りの Crister にとって予算が正しいままになるよう、ページをジオメトリの合計から外す。
			totalResidentGeometryBytes_ -= page.sizeBytes_;
			page.resident_ = false;
		}

		/// [EN] Release the shared LOD 0 vertex pool, pinned or not.
		/// [JP] ピン留めかどうかに関わらず、共有 LOD 0 頂点プールを解放する。
		if (poolResident_)
		{
			bindlessHeap_->Release(std::move(poolResource_), poolBufferIndex_);
			totalResidentGeometryBytes_ -= poolSizeBytes_;
			poolResident_ = false;
		}

		/// [EN] Release both resident mips of every texture, the pinned fallback included.
		/// [JP] 全テクスチャの常駐中の2つのミップを、ピン留めのフォールバックも含めて解放する。
		for (StreamingTexture& streamingTexture : streamingTextures_)
		{
			if (streamingTexture.pinnedMip_.resource_)
			{
				totalResidentTextureBytes_ -= streamingTexture.pinnedMip_.sizeBytes_;
				bindlessHeap_->Release(std::move(streamingTexture.pinnedMip_.resource_), streamingTexture.pinnedMip_.bindlessIndex_);
			}
			if (streamingTexture.currentMip_.resource_)
			{
				totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
				bindlessHeap_->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
			}
		}

		/// [EN] The RT proxy geometry (vertex/position/index/skin), its morph pools, and the raster-side morph buffers outlive this object the same way the streaming pages above do: a reflection/GI TLAS instance built over this Crister can linger for a few frames after the model is unloaded (see RaytracingRenderer's pendingBlasEviction_ deferral), and its closest-hit shader fetches vertex attributes from exactly these buffers. Hand them to the deferred-reclaim ring and return their bindless SRV slots instead of letting the ComPtr members free them synchronously here.
		/// [JP] RT プロキシジオメトリ（頂点/位置/インデックス/スキン）、そのモーフプール、およびラスタ側のモーフバッファは、上の streamingページと同じくこのオブジェクトより長生きする: この Crister の上に構築された反射/GI の TLAS インスタンスは、モデルのアンロード後も数フレーム残り得る（RaytracingRenderer のpendingBlasEviction_ 遅延参照）。そのヒットシェーダはまさにこれらのバッファから頂点属性を読む。ComPtr メンバにここで同期的に解放させず、遅延回収リングへ渡し、bindless SRVスロットを返す。
		bindlessHeap_->Release(std::move(vertexResource_), vertexBufferIndex_);
		bindlessHeap_->Release(std::move(skinVertexResource_), skinVertexBufferIndex_);
		bindlessHeap_->Release(std::move(indexResource_), indexBufferIndex_);
		bindlessHeap_->Release(std::move(positionResource_), positionBufferIndex_);
		bindlessHeap_->Release(std::move(raytracingSkinVertexResource_), raytracingSkinVertexBufferIndex_);
		bindlessHeap_->Release(std::move(raytracingMorphDeltaResource_), raytracingMorphDeltaBufferIndex_);
		bindlessHeap_->Release(std::move(morphDeltaResource_), morphDeltaBufferIndex_);
		bindlessHeap_->Release(std::move(vertexMorphSourceResource_), vertexMorphSourceBufferIndex_);
	}

	/**
	* [EN]
	* Computes the quantisation AABB and bakes vertices_ (and, if
	* skinned, skin attributes) into compressedVertices_ /
	* compressedSkinVertices_, then frees vertices_. Must run after
	* BuildMeshlets (LOD vertex duplication must already be final)
	* and before serialising to the .crister cache.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 量子化 AABB を計算し、vertices_（スキンがあればスキン属性も）を
	* compressedVertices_ / compressedSkinVertices_ へ焼き込んで
	* vertices_ を解放する。BuildMeshlets の後（LOD 頂点複製が確定済み）、
	* .crister キャッシュへのシリアライズ前に実行すること。
	*/
	void Crister::BakeMesh()
	{
		/// [EN] Compute the dequantisation AABBs, then quantise every vertex into the 16-byte GPU format (and skinning attributes, if skinned). Extents are clamped away from zero so flat axes still dequantise to the shared min value. Must run after BuildMeshlets, since QEM appends fresh vertex copies per LOD.
		/// [JP] 逆量子化 AABB を計算し、全頂点を 16 バイトの GPU フォーマット（とスキンがあればスキニング属性）へ量子化する。extent はゼロから離してクランプし、平坦な軸でも共通の min 値へ正しく逆量子化されるようにする。QEM が LOD ごとに新頂点を追加するため、BuildMeshlets の後に実行すること。
		{
			Vector3 positionMax = Vector3(0, 0, 0);
			Vector2 texcoordMax = Vector2(0, 0);
			positionMin_ = Vector3(0, 0, 0);
			texcoordMin_ = Vector2(0, 0);

			/// [EN] Grow the position and texcoord bounds from the first vertex over every vertex; with no vertices the AABB stays at the origin.
			/// [JP] 最初の頂点から始めて、全頂点で位置とテクスチャ座標の境界を広げる。頂点が無ければ AABB は原点のまま。
			if (!vertices_.empty())
			{
				positionMin_ = vertices_[0].position_;
				positionMax = vertices_[0].position_;
				texcoordMin_ = vertices_[0].texcoord_;
				texcoordMax = vertices_[0].texcoord_;
				for (const Vertex& vertex : vertices_)
				{
					positionMin_ = Vector3::Min(positionMin_, vertex.position_);
					positionMax = Vector3::Max(positionMax, vertex.position_);
					texcoordMin_ = Vector2::Min(texcoordMin_, vertex.texcoord_);
					texcoordMax = Vector2::Max(texcoordMax, vertex.texcoord_);
				}
			}

			/// [EN] The extent divides the position during quantisation, so a flat axis is kept slightly above zero.
			/// [JP] 量子化では大きさで位置を割るので、平坦な軸もわずかに 0 より大きく保つ。
			positionExtent_ = Vector3::Max(positionMax - positionMin_, Vector3(1e-6f, 1e-6f, 1e-6f));
			texcoordExtent_ = Vector2::Max(texcoordMax - texcoordMin_, Vector2(1e-6f, 1e-6f));
		}

		/// [EN] Quantise every vertex into the 16-byte GPU format against the AABB just computed.
		/// [JP] 求めた AABB に対して、全頂点を 16 バイトの GPU フォーマットへ量子化する。
		compressedVertices_.resize(vertices_.size());
		for (Size vertexIndex = 0; vertexIndex < vertices_.size(); vertexIndex++)
		{
			compressedVertices_[vertexIndex] = EncodeVertex(vertices_[vertexIndex], positionMin_, positionExtent_, texcoordMin_, texcoordExtent_);
		}

		/// [EN] Skinned models also get one CompressedSkinVertex per vertex; static models skip this array entirely.
		/// [JP] スキン付きモデルは頂点ごとに CompressedSkinVertex も持つ。静的モデルはこの配列ごと持たない。
		if (!skins_.empty())
		{
			compressedSkinVertices_.resize(vertices_.size());
			for (Size vertexIndex = 0; vertexIndex < vertices_.size(); vertexIndex++)
			{
				const Vertex& vertex = vertices_[vertexIndex];
				CompressedSkinVertex& skin = compressedSkinVertices_[vertexIndex];

				/// [EN] Pack the four joint indices as 16 bits each, two per word.
				/// [JP] 4つのジョイントインデックスを 16bit ずつ、1ワードに2つ詰める。
				skin.jointsXY_ = (vertex.joints_.x & 0xFFFF) | ((vertex.joints_.y & 0xFFFF) << 16);
				skin.jointsZW_ = (vertex.joints_.z & 0xFFFF) | ((vertex.joints_.w & 0xFFFF) << 16);

				/// [EN] Pack the four weights as 8-bit UNORM; the shader renormalises them because the rounded values rarely sum to exactly 1.
				/// [JP] 4つの重みを 8bit UNORM で詰める。丸めた値の合計はちょうど 1 になることが少ないので、シェーダーで正規化し直す。
				skin.weights_ =
					QuantizeUnorm8(vertex.weights_.x) |
					(QuantizeUnorm8(vertex.weights_.y) << 8) |
					(QuantizeUnorm8(vertex.weights_.z) << 16) |
					(QuantizeUnorm8(vertex.weights_.w) << 24);
			}
		}

		/// [EN] The full-precision vertices are no longer needed; release their memory rather than only emptying them.
		/// [JP] フル精度の頂点はもう要らないので、空にするだけでなくメモリも返す。
		vertices_.clear();
		vertices_.shrink_to_fit();
	}

	/**
	* [EN]
	* Downsamples oversized textures, dilates transparent-texel color
	* (before compression, since BC7 blocks cannot be edited per texel
	* afterwards), then BC7-compresses a full mip chain into each
	* Bitmap's cacheData_. Must run before serialising to the .crister
	* cache. The BC7 encoding runs on the GPU (BC7CompressCS.hlsl, mode
	* 6 only) as a one-shot compute dispatch on cmdQueue, with the root
	* signature and PSO owned by bc7Shader. The shader binds its input
	* and output as root SRV/UAV, so unlike Upload() this needs no
	* BindlessHeap.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 大きすぎるテクスチャをダウンサンプルし、透明テクセルの色を
	* dilation する（圧縮後は BC7 ブロックをテクセル単位で編集できない
	* ため、圧縮前に行う）。その後フルミップチェーンを BC7 圧縮して各
	* Bitmap の cacheData_ へ焼き込む。.crister キャッシュへの
	* シリアライズ前に実行すること。BC7 エンコードは GPU で行い
	* (BC7CompressCS.hlsl、mode 6 のみ)、cmdQueue 上の一発実行の
	* コンピュートディスパッチとして、bc7Shader が持つルートシグネチャと
	* PSO で動かす。シェーダーは入出力をルート SRV/UAV でバインドするので、
	* Upload() と違い BindlessHeap は要らない。
	*/
	void Crister::BakeBitmap(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader)
	{
		/// [EN] Shrinks an RGBA8 image by an integer box filter until it fits maxDimension; used only for images beyond D3D12's 2D texture limit of 16384.
		/// [JP] RGBA8 画像を整数倍率のボックスフィルタで縮小する（D3D12 の 2Dテクスチャ最大寸法 16384 を超える場合のみ呼ばれる）。
		auto downsampleRgbaToFit = [](DynamicArray<Uchar>& rgba, Int& width, Int& height, Int maxDimension)
		{
			/// [EN] The shrink factor is the smallest integer that brings the longer side within maxDimension.
			/// [JP] 縮小倍率は、長い方の辺を maxDimension 以内に収める最小の整数。
			Int largest = width > height ? width : height;
			Int factor = (largest + maxDimension - 1) / maxDimension;
			if (factor <= 1)
			{
				return;
			}

			Int newWidth = width / factor;
			Int newHeight = height / factor;
			DynamicArray<Uchar> shrunk(static_cast<Size>(newWidth) * newHeight * 4);

			/// [EN] Each destination texel is the plain average of the factor x factor source texels it covers.
			/// [JP] 縮小後の各テクセルは、それが覆う factor x factor 個の元テクセルの単純平均。
			for (Int y = 0; y < newHeight; y++)
			{
				for (Int x = 0; x < newWidth; x++)
				{
					Uint32 sum[4] = { 0, 0, 0, 0 };
					for (Int sampleY = 0; sampleY < factor; sampleY++)
					{
						for (Int sampleX = 0; sampleX < factor; sampleX++)
						{
							Size source = (static_cast<Size>(y) * factor + sampleY) * width + (static_cast<Size>(x) * factor + sampleX);
							for (Int component = 0; component < 4; component++)
							{
								sum[component] += rgba[source * 4 + component];
							}
						}
					}

					Size destination = static_cast<Size>(y) * newWidth + x;
					Uint32 sampleCount = static_cast<Uint32>(factor) * factor;
					for (Int component = 0; component < 4; component++)
					{
						shrunk[destination * 4 + component] = static_cast<Uchar>(sum[component] / sampleCount);
					}
				}
			}

			/// [EN] Hand the shrunk image and its new size back to the caller.
			/// [JP] 縮小した画像と新しいサイズを呼び出し側へ返す。
			rgba = std::move(shrunk);
			width = newWidth;
			height = newHeight;
		};

		/// [EN] Repeatedly fills the RGB of transparent (alpha 0) texels with the color of neighbouring opaque texels (dilation / edge padding), leaving alpha untouched. Transparent texels are often black, and compressing to BC7 and building mips without this bleeds that black into the cutout edges as a dark fringe. BC7 blocks cannot be edited per texel, so this runs before compression.
		/// [JP] 透明(α=0)テクセルのRGBを、隣接する不透明テクセルの色で反復的に埋める（dilation / edge padding）。αは変えない。透明部のRGBは黒のことが多く、これを埋めずに BC7 圧縮・ミップ生成すると、その黒がカットアウトの縁へ滲んで黒フチになる。色だけ隣から埋めておけば防げる。BC7 圧縮後はテクセル単位で触れなくなるため、圧縮前のこの時点でやる必要がある。
		auto dilateTransparentColor = [](Uchar* pixels, Int width, Int height)
		{
			const Size w = static_cast<Size>(width);
			const Size h = static_cast<Size>(height);
			const Size rowPitch = w * 4;

			auto Texel = [&](Size x, Size y) -> Uchar*
			{
				return pixels + y * rowPitch + x * 4;
			};

			/// [EN] A texel counts as known once its color is meaningful: opaque texels from the start, filled ones after each pass.
			/// [JP] 色に意味があるテクセルを known とする。最初は不透明なテクセルで、各パスの後に埋めたものが加わる。
			DynamicArray<Uint8> known(w * h);
			for (Size y = 0; y < h; y++)
			{
				for (Size x = 0; x < w; x++)
				{
					known[y * w + x] = Texel(x, y)[3] > 0 ? 1 : 0;
				}
			}

			Bool changed = true;
			Uint pass = 0;

			/// [EN] Each pass grows the colored region by one texel; stop when nothing changes, or after at most 64 passes so a mostly transparent image stays cheap.
			/// [JP] 1回のパスで色の付いた領域が1テクセル広がる。何も変わらなくなるか、ほぼ透明な画像でも重くならないよう最大 64 回で止める。
			while (changed && pass < 64)
			{
				changed = false;
				pass++;

				/// [EN] Texels filled in this pass become known only after the pass, so color does not spread more than one texel per pass.
				/// [JP] このパスで埋めた分はパス終了後に known 化（同一パス内で滲ませない）。
				DynamicArray<Size> filled;
				for (Size y = 0; y < h; y++)
				{
					for (Size x = 0; x < w; x++)
					{
						if (known[y * w + x])
						{
							continue;
						}

						Uint accumR = 0;
						Uint accumG = 0;
						Uint accumB = 0;
						Uint count = 0;

						/// [EN] Average the colors of the known texels among the eight neighbours.
						/// [JP] 周りの8テクセルのうち、known なものの色を平均する。
						for (Int dy = -1; dy <= 1; dy++)
						{
							for (Int dx = -1; dx <= 1; dx++)
							{
								if (dx == 0 && dy == 0)
								{
									continue;
								}
								const Int nx = static_cast<Int>(x) + dx;
								const Int ny = static_cast<Int>(y) + dy;
								if (nx < 0 || ny < 0 || nx >= width || ny >= height)
								{
									continue;
								}
								if (!known[static_cast<Size>(ny) * w + static_cast<Size>(nx)])
								{
									continue;
								}
								const Uchar* neighbor = Texel(static_cast<Size>(nx), static_cast<Size>(ny));
								accumR += neighbor[0];
								accumG += neighbor[1];
								accumB += neighbor[2];
								count++;
							}
						}

						/// [EN] Only the RGB is written; alpha stays 0 so the texel is still cut out.
						/// [JP] 書くのは RGB だけ。アルファは 0 のままなので、テクセルは切り抜かれたまま。
						if (count > 0)
						{
							Uchar* destination = Texel(x, y);
							destination[0] = static_cast<Uchar>(accumR / count);
							destination[1] = static_cast<Uchar>(accumG / count);
							destination[2] = static_cast<Uchar>(accumB / count);
							filled.push_back(y * w + x);
							changed = true;
						}
					}
				}

				for (const Size index : filled)
				{
					known[index] = 1;
				}
			}
		};

		/// [EN] BC7-compresses one mip on the GPU and reads the blocks back into outputBlocks, fully synchronously: a one-shot command list and fence are created and the calling thread blocks until the GPU is done. The root signature and PSO come from bc7Shader, which Graphics builds once at startup.
		/// [JP] 1ミップぶんをGPUでBC7圧縮してoutputBlocksへ読み戻す。完全同期:専用の一発コマンドリストとフェンスを作り、GPUが終わるまで呼び出しスレッドをブロックする。ルートシグネチャ/PSOはbc7Shader(Graphics所有、起動時に一度だけ構築)のものを使う。
		auto compressBC7 = [device, cmdQueue, &bc7Shader](const DirectX::Image& image, DynamicArray<Uchar>& outputBlocks)
		{
			Uint32 width = static_cast<Uint32>(image.width);
			Uint32 height = static_cast<Uint32>(image.height);

			/// [EN] BC7 encodes 4x4 texel blocks; a partial block at the edge still takes a whole block.
			/// [JP] BC7 は 4x4 テクセルのブロック単位で符号化する。端の欠けたブロックも1ブロックぶんを使う。
			Uint32 blockCountX = (width + 3) / 4;
			Uint32 blockCountY = (height + 3) / 4;
			Uint32 blockCount = blockCountX * blockCountY;

			/// [EN] Pack the image row by row into one RGBA8 word per texel, dropping the row padding, as the shader expects.
			/// [JP] シェーダーが期待する形に合わせ、行の余白を除いて、テクセルごとに RGBA8 を1ワードへ詰める。
			DynamicArray<Uint32> packedPixels(static_cast<Size>(width) * height);
			for (Size y = 0; y < height; y++)
			{
				const Uchar* row = image.pixels + y * image.rowPitch;
				for (Size x = 0; x < width; x++)
				{
					const Uchar* texel = row + x * 4;
					packedPixels[y * width + x] = texel[0] | (texel[1] << 8) | (texel[2] << 16) | (texel[3] << 24);
				}
			}

			HRESULT hr{ S_OK };

			/// [EN] Input: an UPLOAD-heap buffer the CPU writes the packed texels into.
			/// [JP] 入力: CPU が詰めたテクセルを書き込む UPLOAD ヒープのバッファ。
			D3D12_HEAP_PROPERTIES uploadHeapProperties{};
			uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

			D3D12_RESOURCE_DESC inputDesc{};
			inputDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			inputDesc.Width = static_cast<UINT64>(packedPixels.size()) * sizeof(Uint32);
			inputDesc.Height = 1;
			inputDesc.DepthOrArraySize = 1;
			inputDesc.MipLevels = 1;
			inputDesc.Format = DXGI_FORMAT_UNKNOWN;
			inputDesc.SampleDesc.Count = 1;
			inputDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			Microsoft::WRL::ComPtr<ID3D12Resource> inputBuffer;
			hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &inputDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(inputBuffer.GetAddressOf()));
			SC_HR_CHECK(hr, "入力バッファの生成に失敗しました");
#if SC_DEBUG
			inputBuffer->SetName(L"Crister_CompressInput");
			GFSDK_Aftermath_DX12_UpdateResourceInfo(inputBuffer.Get());
#endif

			/// [EN] Copy the texels in; the CPU never reads this buffer, so the read range is empty.
			/// [JP] テクセルをコピーする。CPU はこのバッファを読まないので、読み取り範囲は空。
			void* mappedInput = nullptr;
			D3D12_RANGE noRead{ 0, 0 };
			hr = inputBuffer->Map(0, &noRead, &mappedInput);
			SC_HR_CHECK(hr, "入力バッファのMapに失敗しました");
			memcpy(mappedInput, packedPixels.data(), packedPixels.size() * sizeof(Uint32));
			inputBuffer->Unmap(0, nullptr);

			/// [EN] Output: a DEFAULT-heap UAV buffer of 16 bytes per block that the shader writes.
			/// [JP] 出力: シェーダーが書き込む、ブロックあたり 16 バイトの DEFAULT ヒープの UAV バッファ。
			D3D12_HEAP_PROPERTIES defaultHeapProperties{};
			defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

			D3D12_RESOURCE_DESC outputDesc{};
			outputDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			outputDesc.Width = static_cast<UINT64>(blockCount) * 16;
			outputDesc.Height = 1;
			outputDesc.DepthOrArraySize = 1;
			outputDesc.MipLevels = 1;
			outputDesc.Format = DXGI_FORMAT_UNKNOWN;
			outputDesc.SampleDesc.Count = 1;
			outputDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			outputDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			Microsoft::WRL::ComPtr<ID3D12Resource> outputBuffer;
			hr = device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &outputDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(outputBuffer.GetAddressOf()));
			SC_HR_CHECK(hr, "出力バッファの生成に失敗しました");
#if SC_DEBUG
			outputBuffer->SetName(L"Crister_CompressOutput");
			GFSDK_Aftermath_DX12_UpdateResourceInfo(outputBuffer.Get());
#endif

			/// [EN] Readback: a READBACK-heap copy of the output the CPU can map.
			/// [JP] リードバック: CPU がマップできる、出力の READBACK ヒープのコピー先。
			D3D12_HEAP_PROPERTIES readbackHeapProperties{};
			readbackHeapProperties.Type = D3D12_HEAP_TYPE_READBACK;

			D3D12_RESOURCE_DESC readbackDesc = outputDesc;
			readbackDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer;
			hr = device->CreateCommittedResource(&readbackHeapProperties, D3D12_HEAP_FLAG_NONE, &readbackDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(readbackBuffer.GetAddressOf()));
			SC_HR_CHECK(hr, "リードバックバッファの生成に失敗しました");
#if SC_DEBUG
			readbackBuffer->SetName(L"Crister_CompressReadback");
			GFSDK_Aftermath_DX12_UpdateResourceInfo(readbackBuffer.Get());
#endif

			/// [EN] A one-shot allocator and command list for this mip alone.
			/// [JP] このミップのためだけの、一発実行のアロケーターとコマンドリスト。
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAllocator.GetAddressOf()));
			SC_HR_CHECK(hr, "コマンドアロケーターの生成に失敗しました");

			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
			hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(cmdList.GetAddressOf()));
			SC_HR_CHECK(hr, "コマンドリストの生成に失敗しました");

			cmdList->SetComputeRootSignature(bc7Shader.GetRootSignature());
			cmdList->SetPipelineState(bc7Shader.GetPipelineState());

			/// [EN] Root parameters: the image and block-grid size as constants, the input as a root SRV and the output as a root UAV.
			/// [JP] ルートパラメーター: 画像とブロック格子のサイズを定数で、入力をルート SRV、出力をルート UAV で渡す。
			Uint32 constants[4] = { width, height, blockCountX, blockCountY };
			cmdList->SetComputeRoot32BitConstants(0, 4, constants, 0);
			cmdList->SetComputeRootShaderResourceView(1, inputBuffer->GetGPUVirtualAddress());
			cmdList->SetComputeRootUnorderedAccessView(2, outputBuffer->GetGPUVirtualAddress());

			/// [EN] One thread per block, in 8x8 thread groups.
			/// [JP] ブロック1つにつき1スレッド、8x8 のスレッドグループで実行する。
			cmdList->Dispatch((blockCountX + 7) / 8, (blockCountY + 7) / 8, 1);

			/// [EN] Once the shader is done, move the output to copy-source and copy it into the readback buffer.
			/// [JP] シェーダーが終わったら、出力をコピー元の状態へ移し、リードバックバッファへコピーする。
			D3D12_RESOURCE_BARRIER toCopySource{};
			toCopySource.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toCopySource.Transition.pResource = outputBuffer.Get();
			toCopySource.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			toCopySource.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
			toCopySource.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			cmdList->ResourceBarrier(1, &toCopySource);

			cmdList->CopyResource(readbackBuffer.Get(), outputBuffer.Get());

			hr = cmdList->Close();
			SC_HR_CHECK(hr, "コマンドリストのCloseに失敗しました");

			ID3D12CommandList* lists[] = { cmdList.Get() };
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
			SC_HR_CHECK(hr, "フェンスの生成に失敗しました");
			{
				/// [EN] Lock only around the submission itself (queue-touching, fast) - not the wait below, which just blocks this thread on a fence event and never touches the queue.
				/// [JP] 提出そのもの(キューに触れる、高速な部分)だけをロックする - 下の待機はこのスレッドをフェンスイベントで止めるだけでキューには一切触れない。
				auto queueLock = cmdQueue->AcquireLock();
				cmdQueue->GetCommandQueue()->ExecuteCommandLists(1, lists);
				hr = cmdQueue->GetCommandQueue()->Signal(fence.Get(), 1);
				SC_HR_CHECK(hr, "コマンドキューのSignalに失敗しました");
			}

			/// [EN] Block this thread until the GPU has finished the copy.
			/// [JP] GPU がコピーを終えるまで、このスレッドを止める。
			if (fence->GetCompletedValue() < 1)
			{
				HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				fence->SetEventOnCompletion(1, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}

			/// [EN] Read the finished blocks back; the CPU writes nothing, so the written range is empty.
			/// [JP] 出来上がったブロックを読み戻す。CPU は何も書かないので、書き込み範囲は空。
			outputBlocks.resize(static_cast<Size>(blockCount) * 16);
			void* mappedReadback = nullptr;
			D3D12_RANGE readRange{ 0, outputBlocks.size() };
			hr = readbackBuffer->Map(0, &readRange, &mappedReadback);
			SC_HR_CHECK(hr, "リードバックバッファのMapに失敗しました");
			memcpy(outputBlocks.data(), mappedReadback, outputBlocks.size());
			D3D12_RANGE noWrite{ 0, 0 };
			readbackBuffer->Unmap(0, &noWrite);
		};

		for (Bitmap& texture : bitmaps_)
		{
			/// [EN] Only images decoded to 8 bits per channel can be baked; anything else is left as it is.
			/// [JP] 焼けるのはチャンネルあたり 8bit にデコードされた画像だけ。それ以外はそのままにする。
			if (texture.width_ <= 0 || texture.height_ <= 0 || texture.cacheData_.empty() || texture.bits_ != 8)
			{
				continue;
			}

			/// [EN] Always produce a mutable RGBA8 buffer (expanding when the decoded image has fewer than 4 components) so it can be dilated in place.
			/// [JP] dilation で書き換えるため、常に可変の RGBA8 バッファを作る（4 コンポーネント未満なら展開、4 ならコピー）。
			Size pixelCount = static_cast<Size>(texture.width_) * static_cast<Size>(texture.height_);
			DynamicArray<Uchar> rgba(pixelCount * 4);

			/// [EN] Missing channels are filled with 0, and a missing alpha with 255 so the texel is opaque.
			/// [JP] 足りないチャンネルは 0 で埋め、アルファが無ければ 255 にして不透明にする。
			if (texture.component_ == 4)
			{
				memcpy(rgba.data(), texture.cacheData_.data(), pixelCount * 4);
			}
			else
			{
				for (Size pixel = 0; pixel < pixelCount; pixel++)
				{
					for (Int component = 0; component < 4; component++)
					{
						if (component < texture.component_)
						{
							rgba[pixel * 4 + component] = texture.cacheData_[pixel * texture.component_ + component];
						}
						else
						{
							rgba[pixel * 4 + component] = component == 3 ? 255 : 0;
						}
					}
				}
			}

			/// [EN] Creating a 2D texture larger than D3D12's limit of 16384 fails, so such images are box-shrunk on the CPU first.
			/// [JP] D3D12 の 2D テクスチャ最大寸法(16384)を超える画像は作成自体が失敗するため、先に CPU 側でボックス縮小して収める。
			Int textureWidth = texture.width_;
			Int textureHeight = texture.height_;
			if (textureWidth > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || textureHeight > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
			{
				SC_LOG_WARNING("テクスチャ {} が D3D12 の最大寸法(16384)を超えています({}x{})。上限内に縮小して読み込みます。", texture.name_, textureWidth, textureHeight);
				downsampleRgbaToFit(rgba, textureWidth, textureHeight, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
			}

			/// [EN] Fill transparent-texel colors before mip generation to kill cutout halos.
			/// [JP] ミップ生成前に透明テクセルの色を埋め、カットアウトの黒フチを防ぐ。
			dilateTransparentColor(rgba.data(), textureWidth, textureHeight);

			/// [EN] Describe the RGBA8 buffer as a DirectXTex image and build its full box-filtered mip chain.
			/// [JP] RGBA8 のバッファを DirectXTex の画像として記述し、ボックスフィルタのフルミップチェーンを作る。
			DirectX::Image sourceImage{};
			sourceImage.width = static_cast<Size>(textureWidth);
			sourceImage.height = static_cast<Size>(textureHeight);
			sourceImage.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sourceImage.rowPitch = static_cast<Size>(textureWidth) * 4;
			sourceImage.slicePitch = sourceImage.rowPitch * static_cast<Size>(textureHeight);
			sourceImage.pixels = rgba.data();

			HRESULT hr{ S_OK };
			DirectX::ScratchImage mipChain;
			hr = DirectX::GenerateMipMaps(sourceImage, DirectX::TEX_FILTER_BOX, 0, mipChain);
			if (FAILED(hr))
			{
				SC_LOG_WARNING("テクスチャ {} のミップ生成に失敗しました(HRESULT=0x{:08X})。このテクスチャ抜きで続行します。", texture.name_, static_cast<Uint32>(hr));
				continue;
			}

			/// [EN] Each mip is BC7-compressed on the GPU through compressBC7.
			/// [JP] ミップごとに compressBC7 で GPU で BC7 圧縮する。
			texture.width_ = textureWidth;
			texture.height_ = textureHeight;
			texture.mipCount_ = static_cast<Int>(mipChain.GetMetadata().mipLevels);
			texture.cacheData_.clear();

			/// [EN] cacheData_ now holds the BC7 blocks of every mip back to back, mip 0 first.
			/// [JP] cacheData_ には全ミップの BC7 ブロックを、ミップ 0 から続けて入れる。
			for (Size mip = 0; mip < mipChain.GetImageCount(); mip++)
			{
				DynamicArray<Uchar> mipBlocks;
				compressBC7(*mipChain.GetImage(mip, 0, 0), mipBlocks);
				texture.cacheData_.insert(texture.cacheData_.end(), mipBlocks.begin(), mipBlocks.end());
			}
		}
	}

	/**
	* [EN]
	* Flattens this Crister's triangles into a CPU-side position/index
	* pair, for MeshCollisionLoader to bake into a ".collision" file.
	* Reads only the CPU-resident arrays (compressedVertices_/
	* meshlets_/vertexIndices_/primitiveIndices_/clusters_), so it is
	* unaffected by geometry streaming residency and needs no GPU
	* readback. Positions are dequantised with the same math as the
	* shader decode. Returns false when there is nothing to extract.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Crister の三角形を CPU 側の位置/インデックス対へ展開する。
	* MeshCollisionLoader がこれを ".collision" ファイルへ焼き込む。
	* CPU 常駐配列 (compressedVertices_/meshlets_/vertexIndices_/
	* primitiveIndices_/clusters_) しか読まないため、ジオメトリ
	* ストリーミングの常駐状態に影響されず、GPU リードバックも不要。
	* 位置はシェーダデコードと同じ計算で逆量子化する。抽出対象が
	* 無ければ false を返す。
	*/
	Bool Crister::BakeCollision(MeshCollisionDetail detail, DynamicArray<Vector3>& outPositions, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outPositions.clear();
		outIndices.clear();

		if (compressedVertices_.empty() || subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Maps a global (pre-dedup) vertex index to its position in outPositions, so triangles sharing a vertex reuse the same compact index instead of duplicating the position.
		/// [JP] グローバル（重複排除前）の頂点インデックスを outPositions 内の位置へ対応付ける。頂点を共有する三角形が同じコンパクトインデックスを再利用し、位置が重複しないようにする。
		std::unordered_map<Uint32, Uint32> remap;

		for (const SubMesh& subMesh : subMeshes_)
		{
			/// [EN] A SubMesh without clusters has no triangles.
			/// [JP] クラスタを持たない SubMesh には三角形が無い。
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] Exact takes the SubMesh's first (LOD 0, most detailed) cluster; Proxy takes its last (coarsest) cluster.
			/// [JP] Exact は SubMesh の最初のクラスタ（LOD 0、最も詳細）、Proxy は最後のクラスタ（最も粗い）を取る。
			Uint32 clusterIndex = detail == MeshCollisionDetail::Exact ? subMesh.clusterOffset_ : subMesh.clusterOffset_ + subMesh.clusterCount_ - 1;

			if (clusterIndex >= clusters_.size())
			{
				continue;
			}

			/// [EN] Walk every meshlet in the chosen cluster, then every triangle corner in each meshlet, resolving each corner through vertexIndices_/primitiveIndices_ to a global vertex index.
			/// [JP] 選んだクラスタ内の全メシュレットを走査し、各メシュレット内の全三角形の各頂点を、vertexIndices_/primitiveIndices_ 経由でグローバル頂点インデックスへ解決する。
			const Cluster& cluster = clusters_[clusterIndex];

			/// [EN] A static SubMesh is placed once per node that references its mesh, as it is drawn; a skinned one, or one no node references, is taken as is.
			/// [JP] 静的な SubMesh は描画と同じく、そのメッシュを参照するノードごとに1回ずつ配置する。スキン付き、またはどのノードからも参照されないものはそのまま使う。
			DynamicArray<Matrix> placements;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				for (const Node& node : nodes_)
				{
					if (node.mesh_ == subMesh.meshIndex_)
					{
						placements.push_back(node.globalTransform_);
					}
				}
			}
			if (placements.empty())
			{
				placements.push_back(Matrix::Identity);
			}

			/// [EN] Each placement is a separate copy of the triangles, so vertices are deduplicated within one placement only.
			/// [JP] 配置ごとに三角形の別のコピーになるので、頂点の重複排除は1つの配置の中だけで行う。
			for (const Matrix& placement : placements)
			{
				remap.clear();

				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							Uint32 globalIndex = vertexIndices_[meshlet.vertexOffset_ + primitiveIndices_[byteOffset + corner]];
							auto found = remap.find(globalIndex);
							if (found == remap.end())
							{
								/// [EN] First time this global vertex is seen: decode its position and append it, remembering the compact index for later corners that share it.
								/// [JP] このグローバル頂点を初めて見た場合: 位置をデコードして追加し、後で同じ頂点を共有する角のためにコンパクトインデックスを記憶する。
								Uint32 compactIndex = static_cast<Uint32>(outPositions.size());
								remap[globalIndex] = compactIndex;
								outPositions.push_back(Vector3::Transform(DecodePosition(compressedVertices_[globalIndex]), placement));
								outIndices.push_back(compactIndex);
							}
							else
							{
								/// [EN] Already emitted: reuse its compact index instead of pushing a duplicate position.
								/// [JP] 既に出力済み: 位置を重複追加せず、そのコンパクトインデックスを再利用する。
								outIndices.push_back(found->second);
							}
						}
					}
				}
			}
		}

		/// [EN] Succeeds only when at least one triangle came out.
		/// [JP] 三角形が1つ以上出たときだけ成功とする。
		return outIndices.size() >= 3;
	}

	/**
	* [EN]
	* Inverse of BakeMesh: decodes compressedVertices_ / compressedSkinVertices_
	* back into vertices_ and rebuilds vertexIndices_ / subMeshes_ as a flat
	* per-SubMesh triangle list from the LOD 0 meshlets. For a ".crister"-only
	* asset (no source glTF/FBX to re-parse) this is how ModelExporter gets an
	* uncompressed, non-meshlet mesh to export.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* BakeMesh の逆: compressedVertices_ / compressedSkinVertices_ を
	* vertices_ へデコードし直し、LOD 0 メシュレットから vertexIndices_ /
	* subMeshes_ を SubMesh ごとのフラットな三角形リストへ再構築する。
	* ".crister" 単体アセット（再解析できるソース glTF/FBX が無い）を
	* ModelExporter が書き出すための、非圧縮・非メシュレットのメッシュを
	* 得る手段。
	*/
	void Crister::Reconstruct()
	{
		/// [EN] Nothing to do when full-precision vertices already exist, or when there is nothing to decode.
		/// [JP] フル精度の頂点が既にあるか、デコードするものが無ければ何もしない。
		if (!vertices_.empty() || compressedVertices_.empty())
		{
			return;
		}

		/// [EN] Decode every compressed vertex, with its skin attributes when present, back into vertices_ at the same global index.
		/// [JP] 全ての圧縮頂点を、スキン属性があればそれも含めて、同じグローバルインデックスのまま vertices_ へデコードし直す。
		vertices_.reserve(compressedVertices_.size());
		for (Size vertexIndex = 0; vertexIndex < compressedVertices_.size(); vertexIndex++)
		{
			Vertex vertex = DecodeVertex(compressedVertices_[vertexIndex]);
			if (vertexIndex < compressedSkinVertices_.size())
			{
				DecodeSkin(compressedSkinVertices_[vertexIndex], vertex.joints_, vertex.weights_);
			}
			vertices_.push_back(vertex);
		}

		/// [EN] Rebuild a flat triangle list per SubMesh from its LOD 0 meshlets, resolving each corner to its global vertex index.
		/// [JP] SubMesh ごとに LOD 0 の meshlet からフラットな三角形リストを作り直し、各角をグローバル頂点インデックスへ解決する。
		DynamicArray<Uint32> flatIndices;
		for (SubMesh& subMesh : subMeshes_)
		{
			Uint32 indexStart = static_cast<Uint32>(flatIndices.size());
			if (subMesh.clusterCount_ > 0 && subMesh.clusterOffset_ < clusters_.size())
			{
				const Cluster& cluster = clusters_[subMesh.clusterOffset_];
				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							flatIndices.push_back(vertexIndices_[meshlet.vertexOffset_ + primitiveIndices_[byteOffset + corner]]);
						}
					}
				}
			}

			/// [EN] Every SubMesh now indexes the whole vertex array directly, and morphs are dropped since the exported mesh has no LOD remap to keep them aligned.
			/// [JP] 各 SubMesh は頂点配列全体を直接指すようにする。書き出すメッシュには揃えを保つ LOD の対応が無いので、モーフは捨てる。
			subMesh.vertexOffset_ = 0;
			subMesh.vertexCount_ = static_cast<Uint32>(vertices_.size());
			subMesh.indexOffset_ = indexStart;
			subMesh.indexCount_ = static_cast<Uint32>(flatIndices.size()) - indexStart;
			subMesh.morphs_.clear();
		}

		/// [EN] vertexIndices_ now holds the flat triangle list instead of meshlet-local indices.
		/// [JP] vertexIndices_ は meshlet ローカルなインデックスではなく、フラットな三角形リストを持つようになる。
		vertexIndices_ = std::move(flatIndices);
	}

	/**
	* [EN]
	* Reads this Crister's LOD 0 geometry into a CPU-side full-vertex
	* (position/normal/tangent/texcoord)/index pair addressed by the
	* SAME global vertex index space compressedVertices_/vertexIndices_
	* already use — decoded from compressedVertices_ (there is no
	* surviving full-precision source once BakeMesh() has run). Unlike
	* BakeCollision (which compacts/dedups into its own local index
	* space and is baked once into the cached ".collision" asset), this
	* is not part of the bake/cache pipeline — nothing is written to
	* disk, and Softbody calls it directly off the already-loaded
	* Crister once, to seed both the physics simulation (positions) and
	* the render-side SoftbodyMesh (normal_/tangent_/texcoord_, which
	* stay at their bind pose — the simulation only ever overwrites
	* position_ each frame; see SoftbodyMesh). outVertices is a 1:1,
	* unfiltered copy of compressedVertices_ and outIndices reference it
	* directly, so a physical simulation can run on the exact same
	* vertex ordering the mesh shader reads and write simulated vertex i
	* straight back into the render vertex buffer at index i with no
	* remap step. Reads only the CPU-resident arrays, same as
	* BakeCollision. Returns false when there is nothing to extract.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Crister の LOD 0 ジオメトリを、compressedVertices_/
	* vertexIndices_ が既に使っているのと同じグローバル頂点インデックス
	* 空間で、CPU 側のフル頂点（位置/法線/接線/UV）/インデックス対へ
	* 読み出す — compressedVertices_ からのデコードになる（BakeMesh()
	* 実行後はフル精度のソースが残っていないため）。BakeCollision
	* （独自のローカルインデックス空間へ圧縮・重複排除し、".collision"
	* アセットとして一度だけ焼き込む）と異なり、これはベイク/キャッシュ
	* パイプラインの一部ではない — ディスクには何も書き込まず、
	* Softbody が一度だけロード済みの Crister に対して直接呼び出し、
	* 物理シミュレーション（位置）とレンダー側の SoftbodyMesh
	* （normal_/tangent_/texcoord_ — バインドポーズのまま使い回す。
	* シミュレーションは毎フレーム position_ だけを上書きする。
	* SoftbodyMesh 参照）の両方の種にする。outVertices は
	* compressedVertices_ の 1:1 な無加工コピーで、outIndices はそれを
	* 直接参照する。これにより物理シミュレーションがメッシュシェーダと
	* 全く同じ頂点順序で動作でき、シミュレート後の頂点 i をそのまま
	* レンダー用頂点バッファのインデックス i へ書き戻せる（リマップ
	* 不要）。BakeCollision と同じく CPU 常駐配列のみを読む。抽出対象が
	* 無ければ false を返す。
	*/
	Bool Crister::SoftbodyFinestVertices(DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outVertices.clear();
		outIndices.clear();

		if (compressedVertices_.empty() || subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Unlike BakeCollision, no remap: outVertices is a 1:1 copy of compressedVertices_ (decoded), so a global vertex index can be used directly as an index into it.
		/// [JP] BakeCollision と違いリマップは行わない: outVertices はcompressedVertices_ の 1:1 コピー（デコード済み）なので、グローバル頂点インデックスをそのままインデックスとして使える。
		outVertices.reserve(compressedVertices_.size());
		for (const CompressedVertex& compressed : compressedVertices_)
		{
			outVertices.push_back(DecodeVertex(compressed));
		}

		/// [EN] Tracks which vertices have already been moved to their node placement, so a vertex shared by several triangles is transformed once.
		/// [JP] どの頂点をノード配置へ移し終えたかを記録し、複数の三角形が共有する頂点を1回だけ変換する。
		DynamicArray<Bool> placedVertices(outVertices.size(), false);
		for (const SubMesh& subMesh : subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] LOD 0 is always the SubMesh's first cluster.
			/// [JP] LOD 0 は常に SubMesh の最初のクラスタ。
			Uint32 clusterIndex = subMesh.clusterOffset_;
			if (clusterIndex >= clusters_.size())
			{
				continue;
			}

			/// [EN] A static SubMesh takes the transform of the first node that references its mesh; a skinned one stays in bind pose.
			/// [JP] 静的な SubMesh は、そのメッシュを参照する最初のノードの変換を使う。スキン付きのものはバインドポーズのまま。
			Matrix placement = Matrix::Identity;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				auto placementNode = std::ranges::find_if(nodes_, [&subMesh](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
				if (placementNode != nodes_.end())
				{
					placement = placementNode->globalTransform_;
				}
			}
			Bool placed = placement != Matrix::Identity;

			/// [EN] Normals go through the inverse transpose so non-uniform scale keeps them perpendicular, and a mirroring placement flips the tangent handedness.
			/// [JP] 法線は逆転置行列で変換し、非一様スケールでも面に垂直なままにする。鏡映する配置では接線の利き手を反転する。
			Matrix normalPlacement = placement.Invert().Transpose();
			Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

			const Cluster& cluster = clusters_[clusterIndex];
			for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
			{
				const MeshletDesc& meshlet = meshlets_[meshletIndex];
				for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
				{
					Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
					for (Int corner = 0; corner < 3; corner++)
					{
						Uint32 globalIndex = vertexIndices_[meshlet.vertexOffset_ + primitiveIndices_[byteOffset + corner]];

						/// [EN] The index is the global vertex index itself, since outVertices mirrors compressedVertices_.
						/// [JP] outVertices は compressedVertices_ と同じ並びなので、インデックスはグローバル頂点インデックスそのもの。
						outIndices.push_back(globalIndex);

						/// [EN] Move position, normal and tangent into the placement the first time the vertex is reached.
						/// [JP] その頂点に初めて来たときに、位置・法線・接線を配置へ移す。
						if (placed && !placedVertices[globalIndex])
						{
							placedVertices[globalIndex] = true;
							Vertex& vertex = outVertices[globalIndex];
							vertex.position_ = Vector3::Transform(vertex.position_, placement);
							vertex.normal_ = Vector3::TransformNormal(vertex.normal_, normalPlacement);
							vertex.normal_.Normalize();
							Vector3 tangent = Vector3::TransformNormal(Vector3(vertex.tangent_.x, vertex.tangent_.y, vertex.tangent_.z), placement);
							tangent.Normalize();
							vertex.tangent_ = Vector4(tangent.x, tangent.y, tangent.z, vertex.tangent_.w * tangentSign);
						}
					}
				}
			}
		}

		/// [EN] Succeeds only when at least one triangle came out.
		/// [JP] 三角形が1つ以上出たときだけ成功とする。
		return outIndices.size() >= 3;
	}

	/**
	* [EN]
	* Reads each SubMesh's coarsest cluster (the same range as
	* BakeCollision's Proxy detail) into a CPU-side
	* full-vertex/index pair, compacted/deduped into its own local
	* index space (unlike SoftbodyFinestVertices, which keeps the full LOD 0
	* global index space uncompacted). For a Softbody to stay real-time
	* on a render-resolution mesh (thousands of vertices, one PBD edge/
	* shear/bend constraint per edge — far past what Jolt's soft body
	* solver is meant for), simulation runs on this much smaller proxy
	* mesh instead; SoftbodyMesh binds each render vertex (from
	* SoftbodyFinestVertices) to its nearest proxy vertices at build time and
	* blends their simulated displacement onto the full-resolution mesh
	* every frame, so the mesh shader still renders full detail. Not
	* part of the bake/cache pipeline — called live off the
	* already-loaded Crister. Returns false when there is nothing to
	* extract.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各 SubMesh の最粗クラスタ（BakeCollision の Proxy 詳細度と同じ
	* 範囲）を、CPU 側のフル頂点/インデックス対
	* へ、自身のローカルインデックス空間に圧縮・重複排除して読み出す
	* （LOD 0 のグローバルインデックス空間を無加工のまま保つ
	* SoftbodyFinestVertices とは異なる）。Softbody がレンダー解像度の
	* メッシュ（数千頂点、辺ごとに PBD の辺/シア/曲げ拘束1つ —
	* Jolt のソフトボディソルバーが想定する規模をはるかに超える）で
	* リアルタイムを保つため、シミュレーションはこのずっと小さい
	* プロキシメッシュ上で行う。SoftbodyMesh がビルド時に各レンダー
	* 頂点（SoftbodyFinestVertices 由来）を最も近いプロキシ頂点群へ束縛し、
	* 毎フレームそのシミュレート済み変位をブレンドしてフル解像度
	* メッシュへ適用するので、メッシュシェーダは引き続きフル
	* ディテールで描画する。ベイク/キャッシュパイプラインの一部では
	* なく、ロード済みの Crister に対してその都度呼び出す。抽出対象が
	* 無ければ false を返す。
	*/
	Bool Crister::SoftbodyCoarsestVertices(DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const
	{
		/// [EN] Start from empty outputs; a model without vertices or SubMeshes has nothing to extract.
		/// [JP] 出力を空から始める。頂点か SubMesh の無いモデルには抽出するものが無い。
		outVertices.clear();
		outIndices.clear();

		if (compressedVertices_.empty() || subMeshes_.empty())
		{
			return false;
		}

		/// [EN] Maps a QUANTISED bind-pose position to its slot in outVertices — unlike BakeCollision's remap (keyed by original global vertex index), this welds any two corners that land on the same position even if they came from different global indices (e.g. UV-seam-duplicated vertices). LOD simplification of the coarsest cluster can otherwise leave two distinct global indices at (numerically) the same position; feeding both into Jolt as separately connected vertices produces a zero-length edge, which SoftBodySharedSettings::CreateConstraints (its Dijkstra-based CalculateClosestKinematic) cannot handle. Quantised to a 1e-4 world-unit grid: fine enough that legitimately distinct vertices never collide, coarse enough to catch float round-trip noise between originally-identical positions.
		/// [JP] 量子化したバインドポーズ位置を outVertices 内のスロットへ対応付ける — BakeCollision のリマップ（元のグローバル頂点インデックスでキー）と異なり、これは元のグローバルインデックスが違っていても同じ位置に落ちる2つの角を溶接する（UV継ぎ目で複製された頂点など）。最粗クラスタの LOD簡略化は、（数値的に）同じ位置に異なる2つのグローバルインデックスを残すことがある。両方を別々に接続された頂点として Jolt に渡すと長さ0の辺ができ、SoftBodySharedSettings::CreateConstraints（Dijkstra ベースの CalculateClosestKinematic）はそれを扱えない。1e-4ワールド単位グリッドへ量子化: 正当に別々の頂点が衝突しない程度に細かく、元々同一だった位置間の浮動小数往復誤差を拾える程度に粗い。
		auto quantisedPositionKey = [](const Vector3& position) -> Uint64
		{
			constexpr Float gridScale = 10000.0f;
			Int64 x = static_cast<Int64>(std::lround(position.x * gridScale));
			Int64 y = static_cast<Int64>(std::lround(position.y * gridScale));
			Int64 z = static_cast<Int64>(std::lround(position.z * gridScale));

			/// [EN] Combine the three grid coordinates with large primes into one spatial hash key.
			/// [JP] 3つの格子座標を大きな素数で組み合わせ、1つの空間ハッシュのキーにする。
			Uint64 hash = static_cast<Uint64>(x) * 73856093ull;
			hash ^= static_cast<Uint64>(y) * 19349663ull;
			hash ^= static_cast<Uint64>(z) * 83492791ull;
			return hash;
		};

		std::unordered_map<Uint64, Uint32> remap;

		for (const SubMesh& subMesh : subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] Coarsest cluster, the same range as BakeCollision's Proxy detail.
			/// [JP] 最粗クラスタ。BakeCollision の Proxy 詳細度と同じ範囲。
			Uint32 clusterIndex = subMesh.clusterOffset_ + subMesh.clusterCount_ - 1;
			if (clusterIndex >= clusters_.size())
			{
				continue;
			}

			/// [EN] A static SubMesh takes the transform of the first node that references its mesh; a skinned one stays in bind pose.
			/// [JP] 静的な SubMesh は、そのメッシュを参照する最初のノードの変換を使う。スキン付きのものはバインドポーズのまま。
			Matrix placement = Matrix::Identity;
			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				auto placementNode = std::ranges::find_if(nodes_, [&subMesh](const Node& node) { return node.mesh_ == subMesh.meshIndex_; });
				if (placementNode != nodes_.end())
				{
					placement = placementNode->globalTransform_;
				}
			}
			Bool placed = placement != Matrix::Identity;

			/// [EN] Normals go through the inverse transpose so non-uniform scale keeps them perpendicular, and a mirroring placement flips the tangent handedness.
			/// [JP] 法線は逆転置行列で変換し、非一様スケールでも面に垂直なままにする。鏡映する配置では接線の利き手を反転する。
			Matrix normalPlacement = placement.Invert().Transpose();
			Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

			const Cluster& cluster = clusters_[clusterIndex];
			for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
			{
				const MeshletDesc& meshlet = meshlets_[meshletIndex];
				for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
				{
					Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
					for (Int corner = 0; corner < 3; corner++)
					{
						Uint32 globalIndex = vertexIndices_[meshlet.vertexOffset_ + primitiveIndices_[byteOffset + corner]];
						const CompressedVertex& compressed = compressedVertices_[globalIndex];

						Vertex vertex;

						/// [EN] Only the position is decoded up front; it alone decides whether this corner welds onto an existing vertex.
						/// [JP] 先にデコードするのは位置だけ。既存の頂点へ溶接するかは位置だけで決まる。
						vertex.position_ = DecodePosition(compressed);
						if (placed)
						{
							vertex.position_ = Vector3::Transform(vertex.position_, placement);
						}

						Uint64 key = quantisedPositionKey(vertex.position_);
						auto found = remap.find(key);
						if (found == remap.end())
						{
							/// [EN] A new position: decode the remaining attributes, place them like the position, and append the vertex.
							/// [JP] 新しい位置なら、残りの属性をデコードして位置と同じように配置し、頂点を追加する。
							Uint32 compactIndex = static_cast<Uint32>(outVertices.size());
							remap[key] = compactIndex;

							vertex.normal_ = DecodeOctahedralNormal(compressed.normal_);
							vertex.tangent_ = DecodeTangent(compressed);
							vertex.texcoord_ = DecodeTexcoord(compressed);
							if (placed)
							{
								vertex.normal_ = Vector3::TransformNormal(vertex.normal_, normalPlacement);
								vertex.normal_.Normalize();
								Vector3 tangent = Vector3::TransformNormal(Vector3(vertex.tangent_.x, vertex.tangent_.y, vertex.tangent_.z), placement);
								tangent.Normalize();
								vertex.tangent_ = Vector4(tangent.x, tangent.y, tangent.z, vertex.tangent_.w * tangentSign);
							}
							outVertices.push_back(vertex);

							outIndices.push_back(compactIndex);
						}
						else
						{
							/// [EN] A position already emitted: weld onto that vertex.
							/// [JP] 既に出力した位置なら、その頂点へ溶接する。
							outIndices.push_back(found->second);
						}
					}
				}
			}
		}

		/// [EN] Succeeds only when at least one triangle came out.
		/// [JP] 三角形が1つ以上出たときだけ成功とする。
		return outIndices.size() >= 3;
	}

	/**
	* [EN]
	* Quantises a single full-precision Vertex into the 16-byte GPU
	* format, given the position/texcoord dequantisation AABBs to
	* quantise against. Extracted out of BakeMesh()'s per-vertex loop
	* body so SoftbodyMesh can re-quantise a Softbody's deformed
	* vertices every frame against a freshly computed AABB (the mesh
	* moves, so a fixed bake-time AABB would clip) using the exact same
	* math BakeMesh() bakes into the cache with — must stay in sync with
	* Model.hlsli's DecodeModelVertex, same as BakeMesh().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フル精度の Vertex 1 つを、指定された位置/UV の逆量子化 AABB に
	* 対して 16 バイトの GPU フォーマットへ量子化する。BakeMesh() の
	* 頂点ループ本体から切り出したもの — SoftbodyMesh が Softbody の
	* 変形後頂点を毎フレーム、その都度計算した AABB（メッシュが動くため
	* 焼き込み時固定の AABB ではクリップしてしまう）に対して、
	* BakeMesh() がキャッシュへ焼き込むのと全く同じ計算式で再量子化
	* できるようにする — BakeMesh() と同様、Model.hlsli の
	* DecodeModelVertex と同期を保つこと。
	*/
	CompressedVertex Crister::EncodeVertex(const Vertex& vertex, const Vector3& positionMin, const Vector3& positionExtent, const Vector2& texcoordMin, const Vector2& texcoordExtent)
	{
		/// [EN] Position and texcoord map into [0,1] within their AABBs and are stored as 16-bit UNORM.
		/// [JP] 位置とテクスチャ座標は、それぞれの AABB の中で [0,1] へ写し、16bit UNORM で持つ。
		Uint32 quantizedX = QuantizeUnorm16((vertex.position_.x - positionMin.x) / positionExtent.x);
		Uint32 quantizedY = QuantizeUnorm16((vertex.position_.y - positionMin.y) / positionExtent.y);
		Uint32 quantizedZ = QuantizeUnorm16((vertex.position_.z - positionMin.z) / positionExtent.z);
		Uint32 quantizedU = QuantizeUnorm16((vertex.texcoord_.x - texcoordMin.x) / texcoordExtent.x);
		Uint32 quantizedV = QuantizeUnorm16((vertex.texcoord_.y - texcoordMin.y) / texcoordExtent.y);

		/// [EN] Tangent is 8+7-bit octahedral plus a 1-bit handedness sign; error in the tangent-space rotation of a normal map is harder to see than error in the normal itself, so the coarser precision is enough.
		/// [JP] タンジェントは 8+7bit octahedral + 利き手符号 1bit。法線マップの接空間回転誤差は法線本体より知覚されにくいため粗くて足りる。
		Uint32 tangentOctahedral = EncodeOctahedralNormal(Vector3(vertex.tangent_.x, vertex.tangent_.y, vertex.tangent_.z));

		/// [EN] Requantise the 16-bit octahedral components to 8 and 7 bits with rounding, leaving the top bit for the sign.
		/// [JP] 16bit の octahedral 成分を丸めながら 8bit と 7bit に量子化し直し、最上位の1bit を符号に空ける。
		Uint32 tangentOctX8 = ((tangentOctahedral & 0xFFFF) * 255 + 32767) / 65535;
		Uint32 tangentOctY7 = ((tangentOctahedral >> 16) * 127 + 32767) / 65535;
		Uint32 tangentSign = vertex.tangent_.w >= 0.0f ? 1u : 0u;
		Uint32 quantizedTangent = tangentOctX8 | (tangentOctY7 << 8) | (tangentSign << 15);

		/// [EN] Lay the fields out exactly as DecodeModelVertex in Model.hlsli reads them.
		/// [JP] Model.hlsli の DecodeModelVertex が読むのと全く同じ配置で詰める。
		CompressedVertex compressed;
		compressed.positionXY_ = quantizedX | (quantizedY << 16);
		compressed.positionZTexU_ = quantizedZ | (quantizedU << 16);
		compressed.texVTangent_ = quantizedV | (quantizedTangent << 16);
		compressed.normal_ = EncodeOctahedralNormal(vertex.normal_);
		return compressed;
	}

	/**
	* [EN]
	* Inverse of EncodeVertex: dequantises a CompressedVertex back into a
	* full-precision Vertex, against this Crister's own quantisation AABBs
	* (positionMin_/positionExtent_/texcoordMin_/texcoordExtent_) — unlike
	* EncodeVertex, not static, since every call site decodes this
	* Crister's own compressedVertices_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* EncodeVertex の逆。CompressedVertex を、この Crister 自身の量子化
	* AABB（positionMin_/positionExtent_/texcoordMin_/texcoordExtent_）に
	* 対してフル精度の Vertex へ逆量子化する。EncodeVertex と異なり
	* static ではない — どの呼び出し元もこの Crister 自身の
	* compressedVertices_ をデコードするため。
	*/
	Vertex Crister::DecodeVertex(const CompressedVertex& compressed)const
	{
		/// [EN] Decode each field separately; the skin attributes live in a different array and are not part of this.
		/// [JP] 各フィールドを個別にデコードする。スキン属性は別の配列にあり、ここには含まない。
		Vertex vertex;
		vertex.position_ = DecodePosition(compressed);
		vertex.normal_ = DecodeOctahedralNormal(compressed.normal_);
		vertex.tangent_ = DecodeTangent(compressed);
		vertex.texcoord_ = DecodeTexcoord(compressed);
		return vertex;
	}

	/**
	* [EN]
	* Packs a unit direction into the 16+16-bit octahedral encoding
	* (Shader/Normal.hlsli::OctNormalEncode's CPU-side counterpart).
	* Static for the same reason as EncodeVertex: SoftbodyMesh calls this
	* indirectly via EncodeVertex with its own bounds, not this Crister's.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単位方向ベクトルを 16+16bit のオクタヘドラル符号化へ詰める
	* （Shader/Normal.hlsli::OctNormalEncode の CPU 側対応）。EncodeVertex
	* と同じ理由で static — SoftbodyMesh が EncodeVertex 経由で、この
	* Crister とは別の自身の境界を使って間接的に呼ぶため。
	*/
	Uint32 Crister::EncodeOctahedralNormal(Vector3 direction)
	{
		/// [EN] Project the direction onto the octahedron |x| + |y| + |z| = 1; a zero vector falls back to +Z.
		/// [JP] 方向を八面体 |x| + |y| + |z| = 1 へ投影する。零ベクトルは +Z にする。
		Float sum = std::abs(direction.x) + std::abs(direction.y) + std::abs(direction.z);
		if (sum < 1e-8f)
		{
			direction = Vector3(0, 0, 1);
			sum = 1.0f;
		}

		Float x = direction.x / sum;
		Float y = direction.y / sum;

		/// [EN] Fold the lower hemisphere over the diagonals so the whole sphere fits in the [-1,1] square.
		/// [JP] 下半球を対角線で折り返し、球全体を [-1,1] の正方形に収める。
		if (direction.z < 0.0f)
		{
			Float foldedX = (1.0f - std::abs(y)) * (x >= 0.0f ? 1.0f : -1.0f);
			Float foldedY = (1.0f - std::abs(x)) * (y >= 0.0f ? 1.0f : -1.0f);
			x = foldedX;
			y = foldedY;
		}

		/// [EN] Remap [-1,1] to [0,1] and pack x and y as 16 bits each.
		/// [JP] [-1,1] を [0,1] へ写し、x と y を 16bit ずつ詰める。
		return QuantizeUnorm16(x * 0.5f + 0.5f) | (QuantizeUnorm16(y * 0.5f + 0.5f) << 16);
	}

	/**
	* [EN]
	* Inverse of EncodeOctahedralNormal (Shader/Normal.hlsli::OctNormalDecode's
	* CPU-side counterpart). Not static, matching DecodeVertex.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* EncodeOctahedralNormal の逆（Shader/Normal.hlsli::OctNormalDecode の
	* CPU 側対応）。DecodeVertex と同じく static ではない。
	*/
	Vector3 Crister::DecodeOctahedralNormal(Uint32 packed)const
	{
		/// [EN] Unpack x and y back to [-1,1] and rebuild z from the octahedron's |x| + |y| + |z| = 1.
		/// [JP] x と y を [-1,1] へ戻し、八面体の |x| + |y| + |z| = 1 から z を作り直す。
		Float ex = static_cast<Float>(packed & 0xFFFF) / 65535.0f * 2.0f - 1.0f;
		Float ey = static_cast<Float>(packed >> 16) / 65535.0f * 2.0f - 1.0f;

		Vector3 n(ex, ey, 1.0f - std::abs(ex) - std::abs(ey));

		/// [EN] A negative z means the point was folded from the lower hemisphere; unfold it.
		/// [JP] z が負なら下半球から折り返した点なので、元に戻す。
		if (n.z < 0.0f)
		{
			Float nx = (1.0f - std::abs(n.y)) * (n.x >= 0.0f ? 1.0f : -1.0f);
			Float ny = (1.0f - std::abs(n.x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
			n.x = nx;
			n.y = ny;
		}

		/// [EN] The octahedron point is not unit length; normalise it back onto the sphere.
		/// [JP] 八面体上の点は長さ 1 ではないので、正規化して球面へ戻す。
		n.Normalize();
		return n;
	}

	/**
	* [EN]
	* Re-converts an already-baked Crister (no source glTF required) from
	* one axis convention to another, in place: decodes every quantised
	* vertex/skin back to full precision, applies deltaBasis to
	* positions/normals/tangents/node transforms/skin inverse-bind
	* matrices/light positions-directions/meshlet bounds, optionally
	* reverses triangle winding, recomputes the quantisation AABBs from
	* the transformed data, re-quantises via BakeMesh(), and
	* re-serialises the result to cristerPath. Does NOT touch GPU
	* resources or bindless indices — the caller must force a reload
	* (Unload then Load) afterward. Returns false if this Crister has no
	* compressed vertex data to convert (e.g. textures-only/degenerate asset).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 既に焼き込み済みの Crister（ソース glTF 不要）を、ある軸コンベンション
	* から別のものへその場で再変換する: 量子化済みの頂点/スキンをフル精度へ
	* デコードし、位置/法線/タンジェント/ノードトランスフォーム/スキン
	* 逆バインド行列/ライト位置・向き/メシュレット境界へ deltaBasis を
	* 適用、必要なら三角形の巻き順を反転、変換後データから量子化 AABB を
	* 再計算し、BakeMesh() で再量子化して cristerPath へ再シリアライズ
	* する。GPU リソースや bindless インデックスは触らない —
	* 呼び出し側が後で強制リロード（Unload → Load）すること。この
	* Crister に変換対象の量子化済み頂点データが無い場合（テクスチャのみ
	* 等の縮退アセット）は false を返す。
	*/
	Bool Crister::ApplyAxisConversion(const Matrix& deltaBasis, Bool flipWinding, const std::filesystem::path& cristerPath)
	{
		if (compressedVertices_.empty())
		{
			return false;
		}

		/// [EN] A basis with a negative determinant mirrors space, which flips the handedness of every tangent frame.
		/// [JP] 行列式が負の基底は空間を鏡映するので、全ての接線フレームの利き手が反転する。
		Float determinant = deltaBasis.Determinant();
		Bool isMirror = determinant < 0.0f;
		Float tangentSign = isMirror ? -1.0f : 1.0f;

		/// [EN] Decode every quantised vertex back to full precision, then transform position/normal/tangent in place. Texcoord is unaffected by an axis-convention change.
		/// [JP] 量子化済みの全頂点をフル精度へデコードし、位置/法線/タンジェントをその場で変換する。テクスチャ座標は軸コンベンション変更の影響を受けない。
		DynamicArray<Vertex> transformedVertices(compressedVertices_.size());
		for (Size vertexIndex = 0; vertexIndex < compressedVertices_.size(); vertexIndex++)
		{
			const CompressedVertex& compressed = compressedVertices_[vertexIndex];
			Vertex& vertex = transformedVertices[vertexIndex];

			Vertex decoded = DecodeVertex(compressed);
			vertex.position_ = Vector3::Transform(decoded.position_, deltaBasis);
			vertex.texcoord_ = decoded.texcoord_;
			vertex.normal_ = Vector3::Transform(decoded.normal_, deltaBasis);

			Vector3 tangentXyz = Vector3::Transform(Vector3(decoded.tangent_.x, decoded.tangent_.y, decoded.tangent_.z), deltaBasis);
			vertex.tangent_ = Vector4(tangentXyz.x, tangentXyz.y, tangentXyz.z, decoded.tangent_.w * tangentSign);

			/// [EN] Skin attributes do not depend on the axes; they are only decoded so BakeMesh() can re-encode them.
			/// [JP] スキン属性は軸に依存しない。BakeMesh() で符号化し直せるようデコードするだけ。
			if (!compressedSkinVertices_.empty())
			{
				DecodeSkin(compressedSkinVertices_[vertexIndex], vertex.joints_, vertex.weights_);
			}
		}

		/// [EN] Convert every node's local rotation and translation, then rebuild the global transforms from them.
		/// [JP] 全ノードのローカル回転と平行移動を変換し、そこからグローバル変換を作り直す。
		for (Node& node : nodes_)
		{
			ConvertRotationByBasis(node.rotation_, deltaBasis);
			ConvertPositionByBasis(node.translation_, deltaBasis);
		}
		CumulateTransforms();

		/// [EN] Inverse bind matrices live in the same model space as the vertices, so they change basis too.
		/// [JP] 逆バインド行列は頂点と同じモデル空間にあるので、これも基底を変える。
		for (Skin& skin : skins_)
		{
			for (Matrix& inverseBindMatrix : skin.inverseBindMatrices_)
			{
				ConvertMatrixByBasis(inverseBindMatrix, deltaBasis);
			}
		}

		/// [EN] Lights move with the model: position and direction change basis, and the direction is renormalised.
		/// [JP] ライトもモデルと一緒に動く。位置と向きの基底を変え、向きは正規化し直す。
		for (PunctualLight& light : lights_)
		{
			ConvertPositionByBasis(light.position_, deltaBasis);
			ConvertPositionByBasis(light.direction_, deltaBasis);
			light.direction_.Normalize();
		}

		/// [EN] Meshlet culling bounds keep matching the geometry: the sphere center and the normal cone axis change basis.
		/// [JP] meshlet のカリング用の境界もジオメトリに合わせる。球の中心と法線コーンの軸の基底を変える。
		for (MeshletBound& bound : meshletBounds_)
		{
			ConvertPositionByBasis(bound.center_, deltaBasis);
			ConvertPositionByBasis(bound.coneAxis_, deltaBasis);
		}

		/// [EN] Swapping two corners of every triangle reverses its winding, keeping front faces outward after a mirror.
		/// [JP] 全三角形の2つの角を入れ替えると巻き順が逆になり、鏡映の後も表面が外を向いたままになる。
		if (flipWinding)
		{
			Size triangleCount = primitiveIndices_.size() / 3;
			for (Size triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
			{
				std::swap(primitiveIndices_[triangleIndex * 3 + 1], primitiveIndices_[triangleIndex * 3 + 2]);
			}
		}

		/// [EN] Reuse BakeMesh()'s existing quantisation pipeline verbatim: it recomputes positionMin_/positionExtent_/texcoordMin_/texcoordExtent_ from vertices_ and re-populates compressedVertices_/compressedSkinVertices_.
		/// [JP] BakeMesh() の既存の量子化パイプラインをそのまま再利用する: vertices_ から positionMin_/positionExtent_/texcoordMin_/texcoordExtent_ を再計算し、compressedVertices_/compressedSkinVertices_ を再構築する。
		vertices_ = std::move(transformedVertices);
		BakeMesh();

		/// [EN] Write the converted model back over its .crister cache.
		/// [JP] 変換したモデルを .crister キャッシュへ上書きする。
		BinaryOutputArchive archive;
		Serialize(archive);
		if (!archive.Write(String(cristerPath.string())))
		{
			return false;
		}

		return true;
	}

	/**
	* [EN]
	* Applies a global position/rotation(euler degrees)/scale/pivot
	* transform to this Crister's node hierarchy and light
	* positions-directions, then re-serialises to cristerPath. Vertices,
	* skin inverse-bind matrices and meshlet bounds stay in mesh-local
	* space: static SubMeshes are placed by their node's global transform
	* at draw time (SubMeshPlacement) and skinned ones through their
	* joints, so both pick the transform up from the nodes.
	* scale/pivot/rotation compose about pivot first, position is a
	* separate world-space offset applied after. Only root-level nodes
	* (stages_[defaultStage_].nodes_) have their local transform
	* updated - CumulateTransforms() then propagates to every
	* descendant, since post-multiplying the whole transform onto just
	* the root telescopes correctly through the local-transform chain
	* (node.globalTransform_ = local * parentGlobal). Returns false if
	* this Crister has no compressed vertex data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グローバルな位置/回転(オイラー角、度)/スケール/ピボット変換を
	* この Crister のノード階層とライト位置・向きへ適用し、その後
	* cristerPath へ再シリアライズする。頂点・スキン逆バインド行列・
	* メシュレット境界はメッシュローカル空間のまま: 静的 SubMesh は描画時に
	* ノードのグローバル変換で配置され(SubMeshPlacement)、スキン付きは
	* ジョイント経由なので、どちらもノードから変換を受け取る。
	* スケール/ピボット/回転はまずピボットを中心に合成し、position は
	* その後に適用する独立したワールド空間オフセット。ローカル
	* トランスフォームを更新するのはルートノード
	* (stages_[defaultStage_].nodes_)のみ — CumulateTransforms() が
	* 全子孫へ伝播する。ルートだけに変換全体を後乗せすれば、ローカル
	* トランスフォームの連鎖(node.globalTransform_ = local *
	* parentGlobal)を通じて正しく telescope するため。この Crister に
	* 変換対象の量子化済み頂点データが無い場合は false を返す。
	*/
	Bool Crister::ApplyTransformConversion(Vector3 position, Vector3 rotation, Vector3 scale, Vector3 pivot, const std::filesystem::path& cristerPath)
	{
		if (compressedVertices_.empty())
		{
			return false;
		}

		/// [EN] Away-from-zero clamp: a zero/near-zero axis would make linearBasis singular, and the root nodes' S/R/T could no longer be decomposed back out of the composed transform.
		/// [JP] ゼロから離す方向へのクランプ: 軸が 0/0 近傍だと linearBasis が特異になり、合成した変換からルートノードの S/R/T を分解し直せなくなる。
		Vector3 clampedScale(
			std::abs(scale.x) < 0.0001f ? std::copysign(0.0001f, scale.x) : scale.x,
			std::abs(scale.y) < 0.0001f ? std::copysign(0.0001f, scale.y) : scale.y,
			std::abs(scale.z) < 0.0001f ? std::copysign(0.0001f, scale.z) : scale.z);

		/// [EN] Scale then rotate (Euler degrees, as yaw Y / pitch X / roll Z) about the pivot, then offset by position in world space.
		/// [JP] ピボットを中心にスケールしてから回転し(オイラー角は度、ヨー Y / ピッチ X / ロール Z)、その後ワールド空間で position だけずらす。
		Matrix linearBasis = Matrix::CreateScale(clampedScale.x, clampedScale.y, clampedScale.z) * Matrix::CreateFromYawPitchRoll(ToRadians(rotation.y), ToRadians(rotation.x), ToRadians(rotation.z));
		Matrix fullTransform = Matrix::CreateTranslation(-pivot) * linearBasis * Matrix::CreateTranslation(pivot + position);

		/// [EN] Apply the transform to the root nodes of the default stage only; every descendant inherits it through its parent.
		/// [JP] 変換はデフォルトステージのルートノードにだけかける。子孫は全て親を通して受け継ぐ。
		if (defaultStage_ >= 0 && static_cast<Size>(defaultStage_) < stages_.size())
		{
			for (Int rootNodeIndex : stages_[defaultStage_].nodes_)
			{
				if (rootNodeIndex < 0 || static_cast<Size>(rootNodeIndex) >= nodes_.size())
				{
					continue;
				}

				Node& node = nodes_[rootNodeIndex];
				Matrix localScale = Matrix::CreateScale(node.scale_.x, node.scale_.y, node.scale_.z);
				Matrix localRotation = Matrix::CreateFromQuaternion(node.rotation_);
				Matrix localTranslation = Matrix::CreateTranslation(node.translation_.x, node.translation_.y, node.translation_.z);

				/// [EN] Post-multiply the root's local S/R/T by the transform and decompose the result back into S/R/T.
				/// [JP] ルートのローカル S/R/T に変換を後ろから掛け、結果を S/R/T へ分解し直す。
				Matrix newLocal = localScale * localRotation * localTranslation * fullTransform;

				newLocal.Decompose(node.scale_, node.rotation_, node.translation_);
			}
		}

		/// [EN] Propagate the new root transforms down to every node's global transform.
		/// [JP] 新しいルートの変換を、全ノードのグローバル変換へ伝える。
		CumulateTransforms();

		/// [EN] Lights are stored resolved, so they take the transform directly: the full transform for the position, only the linear part for the direction.
		/// [JP] ライトは解決済みの値で持つので、変換を直接かける。位置には全体の変換を、向きには線形部分だけをかける。
		for (PunctualLight& light : lights_)
		{
			light.position_ = Vector3::Transform(light.position_, fullTransform);
			light.direction_ = Vector3::TransformNormal(light.direction_, linearBasis);
			light.direction_.Normalize();
		}

		/// [EN] Write the converted model back over its .crister cache.
		/// [JP] 変換したモデルを .crister キャッシュへ上書きする。
		BinaryOutputArchive archive;
		Serialize(archive);
		if (!archive.Write(String(cristerPath.string())))
		{
			return false;
		}

		return true;
	}

	/**
	* [EN]
	* Creates every GPU resource this Crister needs to draw: derives each
	* Cluster's streaming page ranges from its meshlet slice and pins the
	* pages that must never evict (coarsest cluster per SubMesh, skinned
	* LOD 0, the vertex pool they reference), uploads skin attributes for
	* the pool range, flattens each SubMesh's LOD 0 cluster, once per node
	* placement, into RT proxy geometry (compressed vertices + decoded
	* float3 positions + flat 32-bit triangle indices, deduplicated),
	* resolves SubMeshPlacement from the nodes, sets up per-texture
	* streaming state from BakeBitmap()'s BC7 mip chains (clamped to the
	* leading run of mip levels legal as a standalone single-mip BC7
	* resource), then registers with the shared streaming bookkeeping and
	* brings the pinned geometry pages/texture mips resident so the model
	* is immediately drawable at its fallback LOD/resolution. Finer
	* pages/mips stream in later on demand (MakeClusterResident/
	* MakeTextureMipResident, driven by ModelRenderer::Gather).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Crister の描画に必要な全 GPU リソースを作成する: 各 Cluster の
	* ストリーミングページ範囲を meshlet スライスから導出し、追い出しては
	* いけないページ（SubMesh ごとの最粗クラスタ、スキンド LOD 0、それらが
	* 参照する頂点プール）をピン留めする。プール範囲分のスキニング属性を
	* アップロードし、各 SubMesh の LOD 0 クラスタをノード配置ごとに RT
	* プロキシジオメトリ（圧縮頂点 + デコード済み float3 位置 + 重複排除
	* 済みフラット 32bit 三角形インデックス）へ展開し、ノードから
	* SubMeshPlacement を解決する。BakeBitmap() が焼いた BC7 ミップ
	* チェーンから（「単一ミップの独立した BC7 リソース」として合法な
	* 先頭のミップ範囲へ切り詰めた上で）テクスチャごとのストリーミング
	* 状態を準備し、最後に共有ストリーミング管理へ登録してピン留め
	* ジオメトリページ/テクスチャミップを常駐させ、フォールバック
	* LOD/解像度で即座に描画可能にする。より細かいページ/ミップは後で
	* オンデマンドにストリームインする（MakeClusterResident/
	* MakeTextureMipResident、ModelRenderer::Gather から駆動）。
	*/
	void Crister::Upload(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* heap)
	{
		HRESULT hr{ S_OK };

		/// [EN] Every static buffer created below is queued on one upload batch and submitted together near the end.
		/// [JP] 以下で作る静的バッファは全て1つのアップロードバッチに積み、終わり近くでまとめて投入する。
		DirectX::ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();

		/// [EN] Capture upload context so cluster pages can stream in and out after load (these engine objects outlive every Crister).
		/// [JP] ロード後にクラスタページを出し入れできるようアップロードコンテキストを保持する（全 Crister より長寿命のオブジェクト）。
		device_ = device;
		uploadQueue_ = cmdQueue;
		bindlessHeap_ = heap;

		/// [EN] Resolve each SubMesh's placements: the global transform of every node that references its mesh, or a single identity for a skinned or unreferenced SubMesh.
		/// [JP] 各 SubMesh の配置を解決する。そのメッシュを参照する全ノードのグローバル変換で、スキン付きか参照されない SubMesh は単位行列1つ。
		subMeshPlacement_.assign(subMeshes_.size(), DynamicArray<Matrix>{});
		for (Size subMeshIndex = 0; subMeshIndex < subMeshes_.size(); subMeshIndex++)
		{
			const SubMesh& subMesh = subMeshes_[subMeshIndex];
			DynamicArray<Matrix>& placements = subMeshPlacement_[subMeshIndex];

			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				for (const Node& node : nodes_)
				{
					if (node.mesh_ == subMesh.meshIndex_)
					{
						placements.push_back(node.globalTransform_);
					}
				}
			}

			if (placements.empty())
			{
				placements.push_back(Matrix::Identity);
			}
		}

		/// [EN] Derive each cluster's page ranges from its meshlet slice. buildMeshletsFromIndices appends sequentially, so a cluster's meshlets occupy contiguous slices of vertexIndices_ and primitiveIndices_; LOD>=1 clusters also own a contiguous vertex range (QEM appends fresh vertex copies per LOD). LOD 0 clusters reference the original vertex pool instead.
		/// [JP] 各クラスタのページレンジを meshlet 列から導出する。buildMeshletsFromIndices は逐次追加なので、クラスタの meshlet はvertexIndices_ / primitiveIndices_ の連続スライスを占有する。LOD>=1 クラスタは専用の連続頂点範囲も所有する（QEM が LOD ごとに新頂点を追加）。LOD 0 クラスタは元の頂点プールを参照する。
		streamingGeometry_.assign(clusters_.size(), StreamingGeometry{});
		poolVertexEnd_ = 0;
		for (Size clusterIndex = 0; clusterIndex < clusters_.size(); clusterIndex++)
		{
			const Cluster& cluster = clusters_[clusterIndex];
			StreamingGeometry& page = streamingGeometry_[clusterIndex];
			if (cluster.meshletCount_ == 0)
			{
				continue;
			}

			/// [EN] The page's index and primitive slices run from the cluster's first meshlet to the end of its last one.
			/// [JP] ページのインデックスとプリミティブのスライスは、クラスタの最初の meshlet から最後の meshlet の終わりまで。
			const MeshletDesc& first = meshlets_[cluster.meshletOffset_];
			const MeshletDesc& last = meshlets_[cluster.meshletOffset_ + cluster.meshletCount_ - 1];
			page.vertexIndexBegin_ = first.vertexOffset_;
			page.vertexIndexEnd_ = last.vertexOffset_ + last.vertexCount_;
			page.primitiveBegin_ = first.triangleOffset_;
			page.primitiveEnd_ = last.triangleOffset_ + last.triangleCount_ * 3;
			page.ownsVertices_ = cluster.lodLevel_ >= 1;

			/// [EN] Find the range of global vertices the page's indices touch.
			/// [JP] ページのインデックスが触れるグローバル頂点の範囲を求める。
			Uint32 minVertex = 0xFFFFFFFF;
			Uint32 maxVertex = 0;
			for (Uint32 index = page.vertexIndexBegin_; index < page.vertexIndexEnd_; index++)
			{
				minVertex = Min(minVertex, vertexIndices_[index]);
				maxVertex = Max(maxVertex, vertexIndices_[index]);
			}

			/// [EN] An LOD >= 1 page owns exactly that range; an LOD 0 page reads the pool, which grows to cover every vertex an LOD 0 page touches.
			/// [JP] LOD >= 1 のページはちょうどその範囲を持つ。LOD 0 のページはプールを読み、プールは LOD 0 のページが触れる全ての頂点を含むまで広がる。
			if (page.ownsVertices_)
			{
				page.vertexBegin_ = minVertex;
				page.vertexEnd_ = maxVertex + 1;
			}
			else
			{
				page.vertexBegin_ = 0;
				page.vertexEnd_ = maxVertex + 1;
				poolVertexEnd_ = Max(poolVertexEnd_, maxVertex + 1);
			}
		}

		/// [EN] Pin the pages that must never leave VRAM: the coarsest cluster of every SubMesh (guaranteed fallback LOD) and, for skinned SubMeshes, LOD 0 (their only drawable LOD). The vertex pool is pinned when any pinned LOD 0 cluster references it.
		/// [JP] VRAM から出してはいけないページをピン留めする: 各 SubMesh の最粗クラスタ（フォールバック LOD の保証）と、スキンド SubMesh のLOD 0（唯一描画可能な LOD）。ピン留めされた LOD 0 クラスタがあれば頂点プールもピン留めする。
		for (const SubMesh& subMesh : subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}
			streamingGeometry_[subMesh.clusterOffset_ + subMesh.clusterCount_ - 1].pinned_ = true;
			if (subMesh.skinIndex_ >= 0)
			{
				streamingGeometry_[subMesh.clusterOffset_].pinned_ = true;
			}
		}

		/// [EN] The pool is pinned as soon as any pinned page reads it.
		/// [JP] ピン留めされたページが1つでもプールを読むなら、プールもピン留めする。
		for (Size clusterIndex = 0; clusterIndex < streamingGeometry_.size(); clusterIndex++)
		{
			if (streamingGeometry_[clusterIndex].pinned_ && !streamingGeometry_[clusterIndex].ownsVertices_)
			{
				poolPinned_ = true;
			}
		}

		/// [EN] Skinning attributes for the pool range (skinned SubMeshes are LOD 0 only, so pool-sized coverage is sufficient). Always resident when skins exist — skinned LOD 0 is pinned anyway.
		/// [JP] プール範囲分のスキニング属性（スキンド SubMesh は LOD 0 のみなのでプールサイズで足りる）。スキンがあれば常に常駐 — どのみちスキンド LOD 0 はピン留めされている。
		if (!skins_.empty() && poolVertexEnd_ > 0)
		{
			hr = CreateStaticBufferUnbounded(device, resourceUpload, compressedSkinVertices_.data(), poolVertexEnd_, sizeof(CompressedSkinVertex), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, skinVertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "スキンバーテックスバッファの生成に失敗しました");
			skinVertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, skinVertexResource_.Get(), poolVertexEnd_, sizeof(CompressedSkinVertex));
		}

		/// [EN] RT proxy geometry: flatten each SubMesh's cluster (see below for which), once per node placement, into a compact vertex set + 32-bit triangle list. Positions are decoded on the CPU with the same math as the shader decode.
		/// [JP] RT プロキシジオメトリ: 各 SubMesh のクラスタ(どれを使うかは下を参照)を、ノード配置ごとにコンパクトな頂点集合 + 32bit 三角形リストへ展開する。位置はシェーダーのデコードと同じ計算で CPU でデコードする。
		DynamicArray<Uint32> flatTriangleIndices;
		DynamicArray<CompressedVertex> raytracingVertices;
		DynamicArray<Vector3> raytracingPositions;
		DynamicArray<CompressedSkinVertex> raytracingSkinVertices;
		DynamicArray<Vector3> raytracingMorphDeltas;

		/// [EN] Per-vertex skin attributes for the RT proxy are built only for a model with exactly one skin, the case the proxy skin blend handles.
		/// [JP] RT プロキシの頂点ごとのスキン属性は、スキンがちょうど1つのモデルにだけ作る。プロキシのスキンブレンドが扱うのはその場合。
		Bool buildRaytracingSkinVertices = skins_.size() == 1 && !compressedSkinVertices_.empty();
		std::unordered_map<Uint32, Uint32> raytracingRemap;
		for (Size subMeshIndex = 0; subMeshIndex < subMeshes_.size(); subMeshIndex++)
		{
			SubMesh& subMesh = subMeshes_[subMeshIndex];

			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			/// [EN] RT uses LOD 0 (full detail): reflections show the mesh at its best quality, at the cost of keeping the RT flat-index / position buffers fully resident. Rasterisation still streams.
			/// [JP] RT は LOD 0（フル詳細）を使う: 反射は最高品質のメッシュで描かれる。代償として RT のフラットインデックス/位置バッファは常に全量常駐する。ラスタ側のストリーミングは従来どおり。
			const Cluster& cluster = clusters_[subMesh.clusterOffset_];

			/// [EN] Encounter order for this SubMesh's compact vertices is contiguous ascending indices into raytracingVertices (globalIndex never collides across SubMeshes, since each owns a disjoint range of vertices_) — so [raytracingVertexOffset_, raytracingVertexOffset_ + raytracingVertexCount_) below is valid, and submeshMorphDeltas[target] fills in that same order.
			/// [JP] この SubMesh のコンパクト頂点の出現順は raytracingVertices への連続した昇順インデックスになる(globalIndex は SubMesh間で衝突しない、各 SubMesh が vertices_ の互いに素な範囲を持つため) — そのため下の [raytracingVertexOffset_, raytracingVertexOffset_ + raytracingVertexCount_) は正しく、submeshMorphDeltas[target] も同じ順序で埋まる。
			subMesh.raytracingVertexOffset_ = static_cast<Uint32>(raytracingVertices.size());
			subMesh.raytracingTriangleOffset_ = static_cast<Uint32>(flatTriangleIndices.size() / 3);
			DynamicArray<DynamicArray<Vector3>> submeshMorphDeltas(subMesh.morphs_.size());

			const DynamicArray<Matrix>& placements = subMeshPlacement_[subMeshIndex];

			/// [EN] A morphing SubMesh is flattened for its first placement only, because its morph deltas are laid out over a single copy of its vertices.
			/// [JP] モーフする SubMesh は最初の配置ぶんだけ展開する。モーフデルタはその頂点の1コピーぶんに対して並んでいるため。
			Size placementCount = subMesh.morphs_.empty() ? placements.size() : 1;
			for (Size placementIndex = 0; placementIndex < placementCount; placementIndex++)
			{
				const Matrix& placement = placements[placementIndex];
				Bool placed = placement != Matrix::Identity;
				Matrix normalPlacement = placement.Invert().Transpose();
				Float tangentSign = placement.Determinant() < 0.0f ? -1.0f : 1.0f;

				/// [EN] Each placement is its own copy of the vertices, so deduplication restarts per placement.
				/// [JP] 配置ごとに頂点の別のコピーになるので、重複排除は配置ごとにやり直す。
				raytracingRemap.clear();

				for (Uint32 meshletIndex = cluster.meshletOffset_; meshletIndex < cluster.meshletOffset_ + cluster.meshletCount_; meshletIndex++)
				{
					const MeshletDesc& meshlet = meshlets_[meshletIndex];
					for (Uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount_; triangleIndex++)
					{
						Uint32 byteOffset = meshlet.triangleOffset_ + triangleIndex * 3;
						for (Int corner = 0; corner < 3; corner++)
						{
							Uint32 globalIndex = vertexIndices_[meshlet.vertexOffset_ + primitiveIndices_[byteOffset + corner]];
							auto found = raytracingRemap.find(globalIndex);
							if (found == raytracingRemap.end())
							{
								/// [EN] First time this vertex appears in this placement: give it the next compact index.
								/// [JP] この配置でこの頂点が初めて出たので、次のコンパクトインデックスを割り当てる。
								Uint32 compactIndex = static_cast<Uint32>(raytracingVertices.size());
								raytracingRemap[globalIndex] = compactIndex;

								const CompressedVertex& compressed = compressedVertices_[globalIndex];

								/// [EN] A source asset with a degenerate/non-finite quantisation AABB (positionMin_/positionExtent_) decodes to a NaN/Inf position here even though the compressed bit pattern itself is well-formed. That NaN reaches the BLAS's position buffer, producing a NaN bounding box the traversal hardware can never cull against - every ray descends into it instead of skipping it, which is a no-page-fault DispatchRays hang rather than a crash. Falling back to the origin keeps this mesh's triangle count/indexing intact (so BLAS build still succeeds) while giving the BVH a finite, if wrong-looking, box.
								/// [JP] 量子化 AABB(positionMin_/positionExtent_)が退化/非有限なソースアセットは、圧縮ビットパターン自体は正常でもここで NaN/Inf の位置へデコードされる。その NaN は BLAS の位置バッファへそのまま渡り、走査ハードウェアが絶対にカリングできない NaN境界ボックスを生む - 全てのレイがスキップされずそこへ降りていくため、クラッシュではなくページフォルト無しの DispatchRays ハングになる。原点へフォールバックすれば、このメッシュの三角形数/インデックス構造は保ったまま(BLAS 構築を成功させたまま)、見た目はおかしくとも有限な境界ボックスを BVH に与えられる。
								Vector3 position = DecodePosition(compressed);
								if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
								{
									if (!degenerateRaytracingPositionLogged_)
									{
										SC_LOG_WARNING("RT プロキシの頂点位置が非有限です。ソースアセットの量子化 AABB が壊れている可能性があります。");
										degenerateRaytracingPositionLogged_ = true;
									}
									position = Vector3::Zero;
								}

								/// [EN] A placed copy is decoded, moved into the placement (normals by the inverse transpose, tangent handedness flipped under a mirror) and re-encoded; an unplaced one is copied as is.
								/// [JP] 配置するコピーはデコードして配置へ移し(法線は逆転置行列で、鏡映では接線の利き手を反転)、符号化し直す。配置しないものはそのままコピーする。
								if (placed)
								{
									Vertex placedVertex = DecodeVertex(compressed);
									placedVertex.position_ = Vector3::Transform(position, placement);
									placedVertex.normal_ = Vector3::TransformNormal(placedVertex.normal_, normalPlacement);
									placedVertex.normal_.Normalize();
									Vector3 placedTangent = Vector3::TransformNormal(Vector3(placedVertex.tangent_.x, placedVertex.tangent_.y, placedVertex.tangent_.z), placement);
									placedTangent.Normalize();
									placedVertex.tangent_ = Vector4(placedTangent.x, placedTangent.y, placedTangent.z, placedVertex.tangent_.w * tangentSign);
									position = placedVertex.position_;
									raytracingVertices.push_back(EncodeVertex(placedVertex, positionMin_, positionExtent_, texcoordMin_, texcoordExtent_));
								}
								else
								{
									raytracingVertices.push_back(compressed);
								}

								/// [EN] The float3 position goes to the BLAS buffer, and the corner references the new compact vertex.
								/// [JP] float3 の位置は BLAS 用のバッファへ入れ、角は新しいコンパクト頂点を指す。
								raytracingPositions.push_back(position);
								flatTriangleIndices.push_back(compactIndex);

								/// [EN] Keep the skin attributes index-aligned with the proxy vertices; unskinned SubMeshes get an empty entry.
								/// [JP] スキン属性をプロキシ頂点とインデックスを揃えて並べる。スキンの無い SubMesh には空のエントリを入れる。
								if (buildRaytracingSkinVertices)
								{
									if (subMesh.skinIndex_ >= 0 && globalIndex < compressedSkinVertices_.size())
									{
										raytracingSkinVertices.push_back(compressedSkinVertices_[globalIndex]);
									}
									else
									{
										raytracingSkinVertices.push_back(CompressedSkinVertex{});
									}
								}

								/// [EN] Gather this vertex's delta for every target in the same compact order, moved into the placement like a direction.
								/// [JP] この頂点の全ターゲットのデルタを同じコンパクトな順で集め、方向と同じように配置へ移す。
								if (!subMesh.morphs_.empty() && globalIndex >= subMesh.vertexOffset_)
								{
									Uint32 localVertexIndex = globalIndex - subMesh.vertexOffset_;
									for (Size targetIndex = 0; targetIndex < subMesh.morphs_.size(); targetIndex++)
									{
										const DynamicArray<Vector3>& deltas = subMesh.morphs_[targetIndex].positionDeltas_;
										Vector3 delta = localVertexIndex < deltas.size() ? deltas[localVertexIndex] : Vector3::Zero;
										submeshMorphDeltas[targetIndex].push_back(placed ? Vector3::TransformNormal(delta, placement) : delta);
									}
								}
							}
							else
							{
								/// [EN] Vertex already emitted in this placement: reuse its compact index.
								/// [JP] この配置で既に出力した頂点なので、そのコンパクトインデックスを使い回す。
								flatTriangleIndices.push_back(found->second);
							}
						}
					}
				}
			}

			/// [EN] Record how many proxy vertices and triangles this SubMesh produced.
			/// [JP] この SubMesh が出したプロキシの頂点と三角形の数を記録する。
			subMesh.raytracingVertexCount_ = static_cast<Uint32>(raytracingVertices.size()) - subMesh.raytracingVertexOffset_;
			subMesh.raytracingTriangleCount_ = static_cast<Uint32>(flatTriangleIndices.size() / 3) - subMesh.raytracingTriangleOffset_;

			/// [EN] Append this SubMesh's deltas to the proxy pool, one target after another.
			/// [JP] この SubMesh のデルタを、ターゲットごとに続けてプロキシのプールへ追加する。
			if (!subMesh.morphs_.empty())
			{
				subMesh.raytracingMorphDeltaOffset_ = static_cast<Uint32>(raytracingMorphDeltas.size());
				for (const DynamicArray<Vector3>& targetDeltas : submeshMorphDeltas)
				{
					raytracingMorphDeltas.insert(raytracingMorphDeltas.end(), targetDeltas.begin(), targetDeltas.end());
				}
			}
		}

		/// [EN] Bounds of the placed geometry, from the proxy positions; without a proxy they fall back to the quantisation AABB.
		/// [JP] 配置後のジオメトリの境界をプロキシの位置から求める。プロキシが無ければ量子化の AABB にする。
		placedPositionMin_ = positionMin_;
		placedPositionExtent_ = positionExtent_;
		if (!raytracingPositions.empty())
		{
			Vector3 placedMax = raytracingPositions.front();
			placedPositionMin_ = raytracingPositions.front();
			for (const Vector3& position : raytracingPositions)
			{
				placedPositionMin_ = Vector3::Min(placedPositionMin_, position);
				placedMax = Vector3::Max(placedMax, position);
			}
			placedPositionExtent_ = placedMax - placedPositionMin_;
		}

		/// [EN] Create the RT proxy buffers and their SRVs: compressed vertices, float3 positions, triangle indices, and skin and morph data when present.
		/// [JP] RT プロキシのバッファと SRV を作る。圧縮頂点、float3 の位置、三角形インデックス、あればスキンとモーフのデータ。
		if (!flatTriangleIndices.empty())
		{
			hr = CreateStaticBufferUnbounded(device, resourceUpload, raytracingVertices.data(), raytracingVertices.size(), sizeof(CompressedVertex), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, vertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用バーテックスバッファの生成に失敗しました");
			vertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, vertexResource_.Get(), static_cast<Uint>(raytracingVertices.size()), sizeof(CompressedVertex));
			proxyVertexCount_ = static_cast<Uint32>(raytracingVertices.size());

			hr = CreateStaticBufferUnbounded(device, resourceUpload, raytracingPositions.data(), raytracingPositions.size(), sizeof(Vector3), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, positionResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用ポジションバッファの生成に失敗しました");
			positionBufferIndex_ = CreateStructuredShaderResourceView(device, heap, positionResource_.Get(), static_cast<Uint>(raytracingPositions.size()), sizeof(Vector3));

			hr = CreateStaticBufferUnbounded(device, resourceUpload, flatTriangleIndices.data(), flatTriangleIndices.size(), sizeof(Uint32), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, indexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用インデックスバッファの生成に失敗しました");
			triangleIndexCount_ = static_cast<Uint32>(flatTriangleIndices.size());
			indexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, indexResource_.Get(), triangleIndexCount_, sizeof(Uint32));

			/// [EN] The proxy skin buffer is created only when it lines up one to one with the proxy vertices.
			/// [JP] プロキシのスキンバッファは、プロキシ頂点と 1 対 1 で揃っているときだけ作る。
			if (buildRaytracingSkinVertices && raytracingSkinVertices.size() == raytracingVertices.size())
			{
				hr = CreateStaticBufferUnbounded(device, resourceUpload, raytracingSkinVertices.data(), raytracingSkinVertices.size(), sizeof(CompressedSkinVertex), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, raytracingSkinVertexResource_.ReleaseAndGetAddressOf());
				SC_HR_CHECK(hr, "レイトレーシング用スキンバーテックスバッファの生成に失敗しました");
				raytracingSkinVertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, raytracingSkinVertexResource_.Get(), static_cast<Uint>(raytracingSkinVertices.size()), sizeof(CompressedSkinVertex));
			}

			if (!raytracingMorphDeltas.empty())
			{
				hr = CreateStaticBufferUnbounded(device, resourceUpload, raytracingMorphDeltas.data(), raytracingMorphDeltas.size(), sizeof(Vector3), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, raytracingMorphDeltaResource_.ReleaseAndGetAddressOf());
				SC_HR_CHECK(hr, "レイトレーシング用モーフデルタバッファの生成に失敗しました");
				raytracingMorphDeltaBufferIndex_ = CreateStructuredShaderResourceView(device, heap, raytracingMorphDeltaResource_.Get(), static_cast<Uint>(raytracingMorphDeltas.size()), sizeof(Vector3));
			}
		}

		/// [EN] Raster morph support: unlike the RT proxy's morph delta pool above, this is baked straight from Morph::positionDeltas_ (no compaction/remap — every SubMesh's original vertexCount_ deltas per target, back to back) since the raster path already has its own per-vertex remap (vertexMorphSource_) to resolve any streamed LOD's vertex back to this buffer's indexing. See the raster morph blend in the model mesh shaders and Model/Material/MaterialResolveCS.hlsl/Model/Transparent/ModelTransparentPS.hlsl.
		/// [JP] ラスタのモーフ対応: 上の RT プロキシ用モーフデルタプールと違い、Morph::positionDeltas_ からそのまま焼き込む(圧縮/リマップ無し — 各 SubMesh のオリジナル vertexCount_ 個のデルタをターゲットごとに連続で)。ラスタ経路はストリームされたどの LOD の頂点もこのバッファの番号付けへ逆引きする独自の頂点リマップ(vertexMorphSource_)を既に持つため。モデル用メッシュシェーダーと Model/Material/MaterialResolveCS.hlsl/Model/Transparent/ModelTransparentPS.hlsl のラスタのモーフブレンド参照。
		DynamicArray<Vector3> morphDeltas;
		for (SubMesh& subMesh : subMeshes_)
		{
			if (subMesh.morphs_.empty())
			{
				continue;
			}

			/// [EN] Each morphing SubMesh appends its targets' deltas back to back and records where they start.
			/// [JP] モーフする各 SubMesh は、ターゲットのデルタを続けて追加し、その先頭を記録する。
			subMesh.morphDeltaOffset_ = static_cast<Uint32>(morphDeltas.size());
			for (const Morph& morph : subMesh.morphs_)
			{
				morphDeltas.insert(morphDeltas.end(), morph.positionDeltas_.begin(), morph.positionDeltas_.end());
			}
		}

		/// [EN] Create the raster delta pool together with the vertex-to-original map the shaders resolve LOD vertices through.
		/// [JP] ラスタ用のデルタプールを、シェーダーが LOD の頂点を解決するのに使う元頂点への対応と一緒に作る。
		if (!morphDeltas.empty())
		{
			hr = CreateStaticBufferUnbounded(device, resourceUpload, morphDeltas.data(), morphDeltas.size(), sizeof(Vector3), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, morphDeltaResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "ラスタ用モーフデルタバッファの生成に失敗しました");
			morphDeltaBufferIndex_ = CreateStructuredShaderResourceView(device, heap, morphDeltaResource_.Get(), static_cast<Uint>(morphDeltas.size()), sizeof(Vector3));

			hr = CreateStaticBufferUnbounded(device, resourceUpload, vertexMorphSource_.data(), vertexMorphSource_.size(), sizeof(Uint32), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, vertexMorphSourceResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "頂点モーフソースバッファの生成に失敗しました");
			vertexMorphSourceBufferIndex_ = CreateStructuredShaderResourceView(device, heap, vertexMorphSourceResource_.Get(), static_cast<Uint>(vertexMorphSource_.size()), sizeof(Uint32));
		}

		/// [EN] Set up per-texture streaming state (see StreamingTexture) from the BC7-compressed bitmaps baked by BakeBitmap(). No GPU work here — MakeTextureMipResident() below uploads mips on demand, starting with each texture's pinned coarsest mip.
		/// [JP] BakeBitmap() が焼いた BC7 圧縮ビットマップから、テクスチャごとのストリーミング状態を準備する（StreamingTexture 参照）。ここではGPU 作業を行わない — 下の MakeTextureMipResident() が各テクスチャのピン留め最粗ミップから始めてオンデマンドでアップロードする。
		streamingTextures_.assign(bitmaps_.size(), StreamingTexture{});
		for (Size textureIndex = 0; textureIndex < bitmaps_.size(); textureIndex++)
		{
			const Bitmap& bitmap = bitmaps_[textureIndex];
			if (bitmap.width_ <= 0 || bitmap.height_ <= 0 || bitmap.cacheData_.empty() || bitmap.mipCount_ <= 0)
			{
				continue;
			}

			/// [EN] Only mips that are legal as a STANDALONE single-mip BC7 resource can be streamed: D3D12 requires a block-compressed resource with MipLevels == 1 to have both dimensions a non-zero multiple of 4 (sub-4 sizes such as the 2x2/1x1 tail BakeBitmap generates are only legal as inner levels of a longer chain). Streaming the whole chain would therefore fail to create the coarsest levels and leave the material with no SRV at all. Clamp mipCount_ to the leading run of legal mips, so the pinned fallback is 4x4 rather than a flat 1x1 — and never sub-4.
			/// [JP] 「単一ミップの独立した BC7 リソース」として合法なミップだけをストリーミング対象にする: D3D12 はブロック圧縮リソースのMipLevels == 1 のとき、幅・高さの両方が 0 でない 4 の倍数であることを要求する(BakeBitmap が生成する末尾の 2x2/1x1 のような4 未満の寸法は、より長いチェーンの内側の段としてのみ合法)。チェーン全体をストリーミング対象にすると最粗段の生成に失敗し、マテリアルに SRV が 1 つも無い状態になってしまう。合法なミップが続く範囲で mipCount_ を打ち切り、ピン留めフォールバックが単色の 1x1 ではなく 4x4 になるようにする(4 未満は作らない)。
			Uint32 streamableMipCount = 0;
			Int mipWidth = bitmap.width_;
			Int mipHeight = bitmap.height_;
			while (streamableMipCount < static_cast<Uint32>(bitmap.mipCount_) && mipWidth >= 4 && mipHeight >= 4 && mipWidth % 4 == 0 && mipHeight % 4 == 0)
			{
				streamableMipCount++;
				mipWidth /= 2;
				mipHeight /= 2;
			}
			if (streamableMipCount == 0)
			{
				SC_LOG_WARNING("テクスチャ {} の寸法({}x{})が BC7 の単一ミップリソースとして不正なため、ストリーミング対象外にします。", textureIndex, bitmap.width_, bitmap.height_);
				continue;
			}

			StreamingTexture& streamingTexture = streamingTextures_[textureIndex];
			streamingTexture.mipCount_ = streamableMipCount;

			/// [EN] topResidentMip_ = mipCount_ means nothing is resident yet; the pinned mip is uploaded below.
			/// [JP] topResidentMip_ = mipCount_ はまだ何も常駐していないことを表す。ピン留めのミップは下でアップロードする。
			streamingTexture.topResidentMip_ = streamingTexture.mipCount_;
			streamingTexture.valid_ = true;
		}

		/// [EN] Submit the batch under the queue lock and wait for it, so every buffer above is ready before the model is drawn.
		/// [JP] キューのロックの下でバッチを投入して完了を待ち、モデルを描画する前に上の全バッファが揃っているようにする。
		std::future<void> uploadFinished;
		{
			auto queueLock = cmdQueue->AcquireLock();
			uploadFinished = resourceUpload.End(cmdQueue->GetCommandQueue());
		}
		uploadFinished.wait();

		/// [EN] Register with the streaming bookkeeping, then bring the pinned geometry pages and texture mips in so the model is immediately drawable at its fallback LOD/resolution. Finer pages/mips stream in on demand from ModelRenderer::Gather.
		/// [JP] ストリーミング管理へ登録し、ピン留めジオメトリページとテクスチャミップをアップロードしてフォールバック LOD/解像度で即描画可能にする。より細かいページ/ミップは ModelRenderer::Gather からオンデマンドで常駐する。
		if (!std::ranges::contains(streamingRegistry_, this))
		{
			streamingRegistry_.push_back(this);
		}
		for (Size clusterIndex = 0; clusterIndex < streamingGeometry_.size(); clusterIndex++)
		{
			if (streamingGeometry_[clusterIndex].pinned_)
			{
				MakeClusterResident(static_cast<Uint32>(clusterIndex));
			}
		}

		/// [EN] Upload each streamable texture's coarsest mip, which stays pinned.
		/// [JP] ストリーミングする各テクスチャの最も粗いミップをアップロードする。これはピン留めのまま残る。
		for (Size textureIndex = 0; textureIndex < streamingTextures_.size(); textureIndex++)
		{
			if (streamingTextures_[textureIndex].valid_)
			{
				MakeTextureMipResident(static_cast<Uint32>(textureIndex), streamingTextures_[textureIndex].mipCount_ - 1);
			}
		}
	}

	/**
	* [EN]
	* Returns every Stage (root-node list + name) parsed from the source
	* glTF's scenes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソース glTF の scenes から解析された、全 Stage（ルートノード一覧 +
	* 名前）を返す。
	*/
	const DynamicArray<Stage>& Crister::Stages()const
	{
		return stages_; 
	}

	/**
	* [EN]
	* Returns every Node in the flattened node hierarchy, including their
	* local S/R/T and cumulated globalTransform_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 平坦化されたノード階層の全 Node を返す。各ノードのローカル
	* S/R/T と、累積済みの globalTransform_ を含む。
	*/
	const DynamicArray<Node>& Crister::Nodes()const
	{
		return nodes_;
	}

	/**
	* [EN]
	* Returns every KHR_lights_punctual point/spot light resolved from
	* the source glTF, in world space.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソース glTF から解決された、全 KHR_lights_punctual
	* ポイント/スポットライトをワールド空間で返す。
	*/
	const DynamicArray<PunctualLight>& Crister::Lights()const
	{
		return lights_;
	}

	/**
	* [EN]
	* Returns every SubMesh (material + cluster range + optional skin
	* index) this Crister is split into.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Crister が分割されている全 SubMesh（マテリアル + クラスタ
	* 範囲 + 任意のスキンインデックス）を返す。
	*/
	const DynamicArray<SubMesh>& Crister::SubMeshes()const
	{
		return subMeshes_;
	}

	/**
	* [EN]
	* Returns every Surface (PBR factors + KHR_materials_* extensions +
	* texture indices) parsed from the source glTF.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソース glTF から解析された、全 Surface（PBR ファクタ +
	* KHR_materials_* 拡張 + テクスチャインデックス）を返す。
	*/
	const DynamicArray<Surface>& Crister::Surfaces()const
	{
		return surfaces_;
	}

	/**
	* [EN]
	* Returns every Skin (joint list + inverse-bind matrices) parsed
	* from the source glTF.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソース glTF から解析された、全 Skin（ジョイント一覧 + 逆バインド
	* 行列）を返す。
	*/
	const DynamicArray<Skin>& Crister::Skins()const
	{
		return skins_;
	}

	/**
	* [EN]
	* Returns every Cluster (one LOD level's meshlet range within a
	* SubMesh) across all SubMeshes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全 SubMesh を通した、全 Cluster（SubMesh 内の1 LOD レベルぶんの
	* meshlet 範囲）を返す。
	*/
	const DynamicArray<Cluster>& Crister::Clusters()const
	{
		return clusters_;
	}

	/**
	* [EN]
	* Returns the index into Stages() of the stage rendered by default
	* when no stage is explicitly selected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 明示的にステージが選択されていない時にデフォルトで描画される、
	* Stages() 内のインデックスを返す。
	*/
	Int Crister::DefaultStage()const
	{
		return defaultStage_;
	}

	/**
	* [EN]
	* Linear search for the first Node whose name_ matches name, or -1
	* if none does. glTF node names are not guaranteed unique, so this
	* returns the first match in nodes_ order.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name_ が name と一致する最初の Node を線形探索する、無ければ -1。
	* glTF のノード名は一意性が保証されないため、nodes_ の順で最初に
	* 見つかったものを返す。
	*/
	Int Crister::FindNodeIndex(const std::string& name)const
	{
		for (Size nodeIndex = 0; nodeIndex < nodes_.size(); nodeIndex++)
		{
			if (nodes_[nodeIndex].name_ == name)
			{
				return static_cast<Int>(nodeIndex);
			}
		}
		return -1;
	}

	/**
	* [EN]
	* Minimum corner of the dequantisation AABB for CompressedVertex
	* positions (see the struct comment). Passed to the shaders through
	* ModelStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex 位置の逆量子化 AABB の最小コーナー（構造体コメント
	* 参照）。ModelStructuredBuffer 経由でシェーダに渡す。
	*/
	Vector3 Crister::PositionMin()const
	{
		return positionMin_;
	}

	/**
	* [EN]
	* Extent (max - min) of the dequantisation AABB for CompressedVertex
	* positions. Passed to the shaders through ModelStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex 位置の逆量子化 AABB の大きさ（max - min）。
	* ModelStructuredBuffer 経由でシェーダに渡す。
	*/
	Vector3 Crister::PositionExtent()const
	{
		return positionExtent_;
	}

	/**
	* [EN]
	* Minimum corner of the model-space bounds of the geometry after
	* every SubMesh is placed by its nodes (SubMeshPlacement), taken
	* from the RT proxy positions in Upload(). Equals PositionMin()
	* when there is no proxy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 各 SubMesh をノードで配置した(SubMeshPlacement)後のジオメトリの、
	* モデル空間での境界の最小コーナー。Upload() で RT プロキシの位置から
	* 求める。プロキシが無ければ PositionMin() と同じ。
	*/
	Vector3 Crister::PlacedPositionMin()const
	{
		return placedPositionMin_;
	}

	/**
	* [EN]
	* Extent (max - min) of the same placed model-space bounds. Equals
	* PositionExtent() when there is no proxy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 同じ配置後のモデル空間での境界の大きさ（max - min）。プロキシが
	* 無ければ PositionExtent() と同じ。
	*/
	Vector3 Crister::PlacedPositionExtent()const
	{
		return placedPositionExtent_;
	}

	/**
	* [EN]
	* Minimum corner of the dequantisation AABB for CompressedVertex
	* texcoords. Passed to the shaders through ModelStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex テクスチャ座標の逆量子化 AABB の最小コーナー。
	* ModelStructuredBuffer 経由でシェーダに渡す。
	*/
	Vector2 Crister::TexcoordMin()const
	{
		return texcoordMin_;
	}

	/**
	* [EN]
	* Extent (max - min) of the dequantisation AABB for CompressedVertex
	* texcoords. Passed to the shaders through ModelStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex テクスチャ座標の逆量子化 AABB の大きさ
	* （max - min）。ModelStructuredBuffer 経由でシェーダに渡す。
	*/
	Vector2 Crister::TexcoordExtent()const
	{
		return texcoordExtent_;
	}

	/**
	* [EN]
	* GPU address of the RT proxy's dedicated float3 position buffer
	* (stride = sizeof(Vector3)) for BLAS construction, decoded on the
	* CPU from the quantised CompressedVertex data with the same math
	* the mesh shader uses — so BLAS positions match the rasterized ones.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* BLAS 構築用の、RT プロキシ専用 float3 位置バッファ
	* （stride = sizeof(Vector3)）の GPU アドレス。量子化済み
	* CompressedVertex からメッシュシェーダと同じ計算で CPU デコード
	* するため、BLAS の位置はラスタライズ結果と一致する。
	*/
	D3D12_GPU_VIRTUAL_ADDRESS Crister::PositionBufferAddress()const
	{
		return positionResource_ ? positionResource_->GetGPUVirtualAddress() : 0;
	}

	/**
	* [EN]
	* Bindless SRV of the same float3 position buffer, read by the morph
	* and skin blend passes as their base positions.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 同じ float3 位置バッファの bindless SRV。モーフとスキンのブレンド
	* パスがベース位置として読む。
	*/
	Uint Crister::PositionBufferIndex()const
	{
		return positionBufferIndex_;
	}

	/**
	* [EN]
	* Vertex count of the RT proxy's compact position/vertex buffers
	* (positionResource_/vertexResource_) PositionBufferAddress() points
	* to, i.e. the size a morph-blend scratch position buffer must be
	* allocated to.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* PositionBufferAddress() が指す RT プロキシのコンパクトな位置/
	* 頂点バッファ(positionResource_/vertexResource_)の頂点数。
	* モーフブレンド用の一時位置バッファを確保すべきサイズでもある。
	*/
	Uint32 Crister::VertexCount()const
	{
		return proxyVertexCount_;
	}

	/**
	* [EN]
	* GPU address of the flat (non-meshlet) 32-bit triangle index buffer
	* for BLAS construction, unpacked once at Upload() time from
	* primitiveIndices_/vertexIndices_ in the same order the mesh shader
	* draws them — so BLAS geometry matches the rasterized geometry
	* exactly (no re-derived winding).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* BLAS 構築用の、フラット（非meshlet）32bit 三角形インデックス
	* バッファの GPU アドレス。primitiveIndices_/vertexIndices_ から
	* Upload() 時に1度だけ、メッシュシェーダが描くのと同じ順序で展開する
	* — BLAS のジオメトリはラスタライズされるジオメトリと完全に一致する
	* （巻き順を再導出しない）。
	*/
	D3D12_GPU_VIRTUAL_ADDRESS Crister::IndexBufferAddress()const
	{
		return indexResource_ ? indexResource_->GetGPUVirtualAddress() : 0;
	}

	/**
	* [EN]
	* Triangle count of the flat index buffer IndexBufferAddress() points to.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* IndexBufferAddress() が指すフラットインデックスバッファの三角形数。
	*/
	Uint32 Crister::IndexCount()const
	{
		return triangleIndexCount_;
	}

	/**
	* [EN]
	* Maps a glTF image index (as stored in Surface) to the bindless
	* heap index of the uploaded GPU texture. Returns 0xFFFFFFFF when
	* not present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF の image インデックス（Surface に格納されている値）を、
	* アップロード済み GPU テクスチャの bindless ヒープインデックスに
	* 変換する。無ければ 0xFFFFFFFF。
	*/
	Uint Crister::TextureBindlessIndex(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return SC_INVALID;
		}
		const StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		/// [EN] A texture that does not stream, or has nothing resident yet, has no SRV to give.
		/// [JP] ストリーミングしないテクスチャや、まだ何も常駐していないテクスチャには返せる SRV が無い。
		if (!streamingTexture.valid_ || streamingTexture.topResidentMip_ >= streamingTexture.mipCount_)
		{
			return SC_INVALID;
		}

		/// [EN] Return the finest resident mip: the pinned one until something finer has streamed in, then currentMip_.
		/// [JP] 常駐中で最も細かいミップを返す。より細かいものがストリームインするまではピン留めのもの、その後は currentMip_。
		Bool isPinnedMip = streamingTexture.topResidentMip_ == streamingTexture.mipCount_ - 1;
		return isPinnedMip ? streamingTexture.pinnedMip_.bindlessIndex_ : streamingTexture.currentMip_.bindlessIndex_;
	}

	/**
	* [EN]
	* Whether any texture's resident bindless index has changed (via
	* MakeTextureMipResident/EvictTextureMip) since the last
	* ClearMaterialsDirty() - RaytracingRenderer checks this to know
	* whether its cached reflection/GI material table (which baked
	* TextureBindlessIndex() at build time) needs rebuilding.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回の ClearMaterialsDirty() 以降に、いずれかのテクスチャの常駐
	* バインドレスインデックスが(MakeTextureMipResident/EvictTextureMip
	* 経由で)変わったか。RaytracingRenderer がこれを見て、キャッシュ済みの
	* 反射/GI マテリアルテーブル(構築時に TextureBindlessIndex() を
	* 焼き込んでいる)を再構築すべきか判断する。
	*/
	Bool Crister::MaterialsDirty()const
	{
		return materialsDirty_;
	}

	/**
	* [EN]
	* Clears the flag MaterialsDirty() reports, after the caller has
	* rebuilt whatever depended on the stale bindless indices.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* MaterialsDirty() が報告するフラグをクリアする。呼び出し側が、
	* 古いバインドレスインデックスに依存していたものを再構築した後に呼ぶ。
	*/
	void Crister::ClearMaterialsDirty()const
	{
		materialsDirty_ = false;
	}

	/**
	* [EN]
	* Bindless SRV of the RT proxy CompressedVertex buffer, built from
	* each SubMesh's LOD 0 cluster so reflections show the mesh at full
	* detail. The proxy stays resident independently of raster
	* streaming.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* RT プロキシ CompressedVertex バッファの bindless SRV。反射に
	* メッシュをフル詳細で映すため、各 SubMesh の LOD 0 クラスタから
	* 構築する。プロキシはラスタ側のストリーミングとは関係なく常駐する。
	*/
	Uint Crister::VertexBufferIndex()const
	{
		return vertexBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV over the flat 32-bit triangle index buffer, for
	* raytracing closesthit shaders to re-fetch the hit triangle's
	* vertices (PrimitiveIndex() * 3 + 0/1/2 -> vertex index).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フラット 32bit 三角形インデックスバッファの bindless SRV。
	* レイトレの closesthit がヒット三角形の頂点を引き直すのに使う
	* (PrimitiveIndex() * 3 + 0/1/2 → 頂点インデックス)。
	*/
	Uint Crister::IndexBufferIndex()const
	{
		return indexBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV of the CompressedSkinVertex buffer, or 0xFFFFFFFF
	* when this Crister has no skins.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedSkinVertex バッファの bindless SRV。スキンが無い
	* Crister では 0xFFFFFFFF。
	*/
	Uint Crister::SkinVertexBufferIndex()const
	{
		return skinVertexBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV of morphDeltaResource_ (raster-side flat morph
	* target delta pool, target-major per SubMesh — see
	* SubMesh::morphDeltaOffset_), or 0xFFFFFFFF when no SubMesh has
	* morphs_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* morphDeltaResource_(ラスタ側のフラットなモーフターゲットデルタ
	* プール、SubMesh ごとのターゲット主順 — SubMesh::morphDeltaOffset_
	* 参照)の bindless SRV。どの SubMesh も morphs_ を持たなければ
	* 0xFFFFFFFF。
	*/
	Uint Crister::MorphDeltaBufferIndex()const
	{
		return morphDeltaBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV of vertexMorphSourceResource_ (per-vertex remap to
	* the original vertex morphDeltaResource_ is indexed by — see
	* vertexMorphSource_'s comment), or 0xFFFFFFFF when no SubMesh has
	* morphs_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* vertexMorphSourceResource_(morphDeltaResource_ がインデックスする
	* オリジナル頂点への、頂点ごとの逆引き — vertexMorphSource_ の
	* コメント参照)の bindless SRV。どの SubMesh も morphs_ を持たなければ
	* 0xFFFFFFFF。
	*/
	Uint Crister::VertexMorphSourceBufferIndex()const
	{
		return vertexMorphSourceBufferIndex_;
	}

	/**
	* [EN]
	* Whether this Crister has RT-side skinning data (skinVertexResource_/
	* raytracingSkinVertexResource_ populated), i.e. skins_ is non-empty and the
	* RT proxy build found at least one skinned SubMesh.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Crister が RT 側のスキニングデータを持つか
	* (skinVertexResource_/raytracingSkinVertexResource_ が構築済みか)。
	* skins_ が空でなく、RT プロキシ構築時にスキンド SubMesh を
	* 1つ以上見つけた場合に true。
	*/
	Bool Crister::ProxySkinned()const
	{
		return raytracingSkinVertexResource_ != nullptr;
	}

	/**
	* [EN]
	* Bindless SRV of the RT proxy's skin vertex pool
	* (raytracingSkinVertexResource_), or SC_INVALID when ProxySkinned is
	* false.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* RT プロキシのスキン頂点プール (raytracingSkinVertexResource_) の
	* bindless SRV。ProxySkinned が false なら SC_INVALID。
	*/
	Uint Crister::ProxySkinVertexBufferIndex()const
	{
		return raytracingSkinVertexBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV of the RT proxy's morph delta pool
	* (raytracingMorphDeltaResource_), or SC_INVALID when no SubMesh has
	* morphs_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* RT プロキシのモーフデルタプール (raytracingMorphDeltaResource_) の
	* bindless SRV。どの SubMesh も morphs_ を持たなければ SC_INVALID。
	*/
	Uint Crister::ProxyMorphDeltaBufferIndex()const
	{
		return raytracingMorphDeltaBufferIndex_;
	}

	/**
	* [EN]
	* Returns the model-space placements of one SubMesh: the
	* globalTransform_ of every Node whose mesh_ references the
	* SubMesh's meshIndex_, so a mesh instanced by several nodes is
	* drawn once per node. Skinned SubMeshes, and SubMeshes no node
	* references, get a single identity placement. Resolved in Upload();
	* an out-of-range index also yields a single identity placement.
	* Renderers draw each SubMesh with placement * actor world.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つの SubMesh のモデル空間配置を返す: その SubMesh の meshIndex_
	* を mesh_ で参照する全 Node の globalTransform_。複数ノードから
	* インスタンスされるメッシュはノードごとに1回ずつ描かれる。スキン
	* 付き SubMesh と、どのノードからも参照されない SubMesh は単位行列
	* 1つになる。Upload() で解決し、範囲外のインデックスも単位行列
	* 1つを返す。レンダラーは各 SubMesh を 配置 * アクターワールド で
	* 描く。
	*/
	const DynamicArray<Matrix>& Crister::SubMeshPlacement(Size subMeshIndex)const
	{
		/// [EN] Fallback for an index Upload() has not resolved: one identity placement, i.e. draw at the actor world as-is.
		/// [JP] Upload() が解決していないインデックス用のフォールバック: 単位行列1つ、つまりアクターワールドのまま描く。
		static const DynamicArray<Matrix> identityPlacement = { Matrix::Identity };
		return subMeshIndex < subMeshPlacement_.size() ? subMeshPlacement_[subMeshIndex] : identityPlacement;
	}

	/**
	* [EN]
	* Copies the RT proxy's base (bind-pose) positions
	* (positionResource_, VertexCount() * sizeof(Vector3) bytes) into
	* destination, which the caller must have already transitioned to
	* D3D12_RESOURCE_STATE_COPY_DEST. Used to seed a per-instance morph
	* blend scratch buffer before MorphBlendCS overwrites only the
	* vertex ranges of SubMeshes with active morph weights this frame —
	* every other vertex must keep its base position unchanged.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* RT プロキシのベース(バインドポーズ)位置(positionResource_、
	* VertexCount() * sizeof(Vector3) バイト)を destination へコピー
	* する。呼び出し側は destination を事前に
	* D3D12_RESOURCE_STATE_COPY_DEST へ遷移させておくこと。今フレーム
	* 有効なモーフウェイトを持つ SubMesh の頂点範囲だけを MorphBlendCS が
	* 上書きする前の、インスタンスごとのモーフブレンド用一時バッファの
	* 種として使う — それ以外の頂点はベース位置のまま保つ必要がある。
	*/
	void Crister::CopyMorph(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* destination)const
	{
		/// [EN] Nothing to copy without a proxy, a destination, or any proxy vertices.
		/// [JP] プロキシか書き込み先、またはプロキシ頂点が無ければコピーするものは無い。
		if (!positionResource_ || !destination || proxyVertexCount_ == 0)
		{
			return;
		}

		/// [EN] Copy every base position, one float3 per proxy vertex.
		/// [JP] 全てのベース位置を、プロキシ頂点1つにつき float3 1つぶんコピーする。
		cmdList->CopyBufferRegion(destination, 0, positionResource_.Get(), 0, static_cast<UINT64>(proxyVertexCount_) * sizeof(Vector3));
	}

	/**
	* [EN]
	* Synchronously uploads the cluster's page (and the vertex pool if
	* the cluster is LOD 0). No-op when already resident.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタのページを同期アップロードする（LOD 0 なら頂点プールも）。
	* 常駐済みなら何もしない。
	*/
	void Crister::MakeClusterResident(Uint32 clusterIndex)
	{
		StreamingGeometry& page = streamingGeometry_[clusterIndex];
		const Cluster& cluster = clusters_[clusterIndex];

		/// [EN] Nothing to do for a page already in VRAM, an empty cluster, or a Crister that has not run Upload().
		/// [JP] 既に VRAM にあるページ、空のクラスタ、Upload() を通っていない Crister では何もしない。
		if (page.resident_ || cluster.meshletCount_ == 0 || !device_)
		{
			return;
		}

		/// [EN] A page that reads the shared pool needs the pool resident first, and counts as one more reference keeping it there.
		/// [JP] 共有プールを読むページは、先にプールを常駐させ、プールを残しておく参照の1つとして数える。
		if (!page.ownsVertices_)
		{
			MakePoolResident();
			poolResidentReferences_++;
		}

		/// [EN] Rebase the cluster's meshlets to page-local offsets. The mesh shaders are untouched: ModelStructuredBuffer simply points at the page's buffers and a page-local meshlet offset.
		/// [JP] クラスタの meshlet をページローカルオフセットへリベースする。メッシュシェーダは無変更: ModelStructuredBuffer がページのバッファとページローカルの meshlet オフセットを指すだけ。
		DynamicArray<MeshletDesc> localMeshlets(cluster.meshletCount_);
		DynamicArray<MeshletBound> localBounds(cluster.meshletCount_);
		for (Uint32 meshletIndex = 0; meshletIndex < cluster.meshletCount_; meshletIndex++)
		{
			MeshletDesc meshlet = meshlets_[cluster.meshletOffset_ + meshletIndex];
			meshlet.vertexOffset_ -= page.vertexIndexBegin_;
			meshlet.triangleOffset_ -= page.primitiveBegin_;
			localMeshlets[meshletIndex] = meshlet;
			localBounds[meshletIndex] = meshletBounds_[cluster.meshletOffset_ + meshletIndex];
		}

		/// [EN] Copy the page's vertex indices; for a page with its own vertices, rebase them so they index its local vertex slice.
		/// [JP] ページの頂点インデックスをコピーする。自前の頂点を持つページでは、ローカルな頂点スライスを指すようにリベースする。
		DynamicArray<Uint32> localVertexIndices(page.vertexIndexEnd_ - page.vertexIndexBegin_);
		for (Uint32 index = page.vertexIndexBegin_; index < page.vertexIndexEnd_; index++)
		{
			Uint32 value = vertexIndices_[index];
			localVertexIndices[index - page.vertexIndexBegin_] = page.ownsVertices_ ? value - page.vertexBegin_ : value;
		}

		/// [EN] The primitive bytes are read through a raw buffer, which addresses whole 32-bit words, so the copy is padded to a multiple of 4 bytes.
		/// [JP] プリミティブのバイト列は 32bit 単位でアクセスする raw バッファとして読むので、コピーを4バイトの倍数まで詰め物する。
		Uint32 primitiveSize = page.primitiveEnd_ - page.primitiveBegin_;
		Uint32 alignedSize = (primitiveSize + 3) & ~3u;
		DynamicArray<Uint8> localPrimitives(alignedSize, 0);
		memcpy(localPrimitives.data(), primitiveIndices_.data() + page.primitiveBegin_, primitiveSize);

		/// [EN] A page that owns vertices carries its own copy of its vertex range.
		/// [JP] 頂点を持つページは、自分の頂点範囲のコピーを持つ。
		DynamicArray<CompressedVertex> localVertices;
		if (page.ownsVertices_)
		{
			localVertices.resize(page.vertexEnd_ - page.vertexBegin_);
			for (Uint32 vertexIndex = page.vertexBegin_; vertexIndex < page.vertexEnd_; vertexIndex++)
			{
				localVertices[vertexIndex - page.vertexBegin_] = compressedVertices_[vertexIndex];
			}
		}

		/// [EN] Create the page's buffers and SRVs on one upload batch.
		/// [JP] ページのバッファと SRV を、1つのアップロードバッチで作る。
		DirectX::ResourceUploadBatch resourceUpload(device_);
		resourceUpload.Begin();
		HRESULT hr = CreateStaticBufferUnbounded(device_, resourceUpload, localMeshlets.data(), localMeshlets.size(), sizeof(MeshletDesc), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, page.meshletResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "メッシュレットバッファの生成に失敗しました");
		page.meshletBufferIndex_ = CreateStructuredShaderResourceView(device_, bindlessHeap_, page.meshletResource_.Get(), cluster.meshletCount_, sizeof(MeshletDesc));

		hr = CreateStaticBufferUnbounded(device_, resourceUpload, localBounds.data(), localBounds.size(), sizeof(MeshletBound), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, page.meshletBoundResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "メッシュレットバウンドバッファの生成に失敗しました");
		page.meshletBoundBufferIndex_ = CreateStructuredShaderResourceView(device_, bindlessHeap_, page.meshletBoundResource_.Get(), cluster.meshletCount_, sizeof(MeshletBound));

		hr = CreateStaticBufferUnbounded(device_, resourceUpload, localVertexIndices.data(), localVertexIndices.size(), sizeof(Uint32), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, page.vertexIndicesResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "頂点インデックスバッファの生成に失敗しました");
		page.vertexIndicesBufferIndex_ = CreateStructuredShaderResourceView(device_, bindlessHeap_, page.vertexIndicesResource_.Get(), static_cast<Uint>(localVertexIndices.size()), sizeof(Uint32));

		hr = CreateStaticBufferUnbounded(device_, resourceUpload, localPrimitives.data(), alignedSize, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, page.primitiveIndicesResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "プリミティブインデックスバッファの生成に失敗しました");
		{
			/// [EN] The primitive indices get a raw (ByteAddressBuffer) SRV over 32-bit words instead of a structured one.
			/// [JP] プリミティブインデックスには構造化バッファではなく、32bit ワード単位の raw (ByteAddressBuffer) の SRV を作る。
			Uint index = bindlessHeap_->AllocateIndex();

			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Buffer.FirstElement = 0;
			shaderResourceViewDesc.Buffer.NumElements = alignedSize / 4;
			shaderResourceViewDesc.Buffer.StructureByteStride = 0;
			shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			device_->CreateShaderResourceView(page.primitiveIndicesResource_.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(index));

			page.primitiveIndicesBufferIndex_ = index;
		}

		if (page.ownsVertices_)
		{
			hr = CreateStaticBufferUnbounded(device_, resourceUpload, localVertices.data(), localVertices.size(), sizeof(CompressedVertex), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, page.vertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "頂点バッファの生成に失敗しました");
			page.vertexBufferIndex_ = CreateStructuredShaderResourceView(device_, bindlessHeap_, page.vertexResource_.Get(), static_cast<Uint>(localVertices.size()), sizeof(CompressedVertex));
		}

		/// [EN] Submit under the queue lock and wait, so the page is drawable as soon as this returns.
		/// [JP] キューのロックの下で投入して待ち、戻った時点でページを描画できるようにする。
		std::future<void> finished;
		{
			auto queueLock = uploadQueue_->AcquireLock();
			finished = resourceUpload.End(uploadQueue_->GetCommandQueue());
		}
		finished.wait();

		/// [EN] Count the page's VRAM against the geometry budget.
		/// [JP] ページの VRAM をジオメトリの予算に数える。
		page.sizeBytes_ =
			static_cast<Uint64>(cluster.meshletCount_) * (sizeof(MeshletDesc) + sizeof(MeshletBound)) +
			localVertexIndices.size() * sizeof(Uint32) +
			alignedSize +
			localVertices.size() * sizeof(CompressedVertex);
		totalResidentGeometryBytes_ += page.sizeBytes_;
		page.resident_ = true;
	}

	/**
	* [EN]
	* Texture mip counterpart to MakeClusterResident: streams a mip level
	* instead of a cluster (see StreamingTexture below). targetMip is
	* uploaded directly rather than one level at a time, so a texture
	* reaches the resolution the camera wants in a single synchronous
	* upload instead of one fence wait per level.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* MakeClusterResident のテクスチャミップ版の対。クラスタではなく
	* ミップレベルをストリーミングする（下記 StreamingTexture 参照）。
	* targetMip は1段ずつではなく直接アップロードするので、テクスチャは
	* 段ごとにフェンスを待つことなく、1回の同期アップロードでカメラが
	* 求める解像度に届く。
	*/
	void Crister::MakeTextureMipResident(Uint32 textureIndex, Uint32 targetMip)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}

		StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		/// [EN] A texture that does not stream, or a Crister that has not run Upload(), cannot take a mip.
		/// [JP] ストリーミングしないテクスチャや、Upload() を通っていない Crister にはミップを入れられない。
		if (!streamingTexture.valid_ || !device_)
		{
			return;
		}

		/// [EN] Nothing to do when an equally fine or finer mip is already resident.
		/// [JP] 既に同等以上に細かいミップが常駐しているなら何もしない。
		Uint32 mipIndex = Min(targetMip, streamingTexture.mipCount_ - 1);
		if (mipIndex >= streamingTexture.topResidentMip_)
		{
			return;
		}

		/// [EN] Find the mip's size, pitches and position within the baked BC7 data.
		/// [JP] 焼き込んだ BC7 データの中での、ミップのサイズ・ピッチ・位置を求める。
		const Bitmap& bitmap = bitmaps_[textureIndex];

		Uint32 mipWidth = 0;
		Uint32 mipHeight = 0;
		Uint64 rowPitch = 0;
		Uint64 slicePitch = 0;
		Uint64 byteOffset = 0;
		ComputeTextureMipLayout(bitmap, mipIndex, mipWidth, mipHeight, rowPitch, slicePitch, byteOffset);

		/// [EN] A single-mip BC7 texture at exactly this mip's resolution.
		/// [JP] ちょうどこのミップの解像度を持つ、単一ミップの BC7 テクスチャ。
		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = mipWidth;
		textureDesc.Height = mipHeight;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_BC7_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		Microsoft::WRL::ComPtr<ID3D12Resource> newResource;
		HRESULT hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(newResource.ReleaseAndGetAddressOf()));

		/// [EN] A failure on one mip is not worth taking the whole model down: warn and carry on without this mip (topResidentMip_ does not advance).
		/// [JP] ミップ1枚の失敗はモデル全体を巻き込んでクラッシュさせるほどではない。警告を出してこのミップ抜きで続行する(topResidentMip_ は進めない)。
		if (FAILED(hr))
		{
			SC_LOG_WARNING("テクスチャ {} のミップ {} のアップロードに失敗しました(HRESULT=0x{:08X}, {}x{})。このミップ抜きで続行します。", textureIndex, mipIndex, static_cast<Uint32>(hr), mipWidth, mipHeight);
			return;
		}
#if SC_DEBUG
		newResource->SetName(L"Crister_TextureMip");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(newResource.Get());
#endif

		/// [EN] Upload the mip's blocks straight from cacheData_, move it to a shader-readable state, and wait for the upload to finish.
		/// [JP] ミップのブロックを cacheData_ から直接アップロードし、シェーダーが読める状態へ移して、アップロードの完了を待つ。
		D3D12_SUBRESOURCE_DATA subresource{};
		subresource.pData = bitmap.cacheData_.data() + byteOffset;
		subresource.RowPitch = static_cast<LONG_PTR>(rowPitch);
		subresource.SlicePitch = static_cast<LONG_PTR>(slicePitch);

		DirectX::ResourceUploadBatch resourceUpload(device_);
		resourceUpload.Begin();
		resourceUpload.Upload(newResource.Get(), 0, &subresource, 1);
		resourceUpload.Transition(newResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		std::future<void> finished;
		{
			auto queueLock = uploadQueue_->AcquireLock();
			finished = resourceUpload.End(uploadQueue_->GetCommandQueue());
		}
		finished.wait();

		/// [EN] Give the new mip its own bindless SRV.
		/// [JP] 新しいミップに専用の bindless SRV を作る。
		Uint newIndex = bindlessHeap_->AllocateIndex();
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_BC7_UNORM;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		device_->CreateShaderResourceView(newResource.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(newIndex));

		/// [EN] Intermediate mips are not kept: only the first upload of the pinned (coarsest) mip goes to pinnedMip_; every other upload replaces currentMip_, releasing the old one first.
		/// [JP] 中間ミップは常駐させ続けない: ピン留めミップ(最粗)を初めてアップロードする1回だけ pinnedMip_ へ、それ以外は currentMip_ を置き換える(古い currentMip_ があれば解放してから差し替える)。
		if (mipIndex == streamingTexture.mipCount_ - 1)
		{
			streamingTexture.pinnedMip_.resource_ = newResource;
			streamingTexture.pinnedMip_.bindlessIndex_ = newIndex;
			streamingTexture.pinnedMip_.sizeBytes_ = slicePitch;
		}
		else
		{
			if (streamingTexture.currentMip_.resource_)
			{
				/// [EN] The mip being replaced is the one the in-flight frames are still sampling - this upgrade is driven by worldScale, so it fires on the very frame the Actor's Scale changes. Dropping the last reference here would destroy the texture out from under the GPU.
				/// [JP] 置き換えられるミップは、インフライトのフレームがまだサンプリングしている当のもの — この昇格は worldScale で駆動されるため、Actor の Scale を変えたまさにそのフレームで発火する。ここで最後の参照を落とすと、GPU が使っている最中にテクスチャを破棄することになる。
				bindlessHeap_->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
				totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
			}
			streamingTexture.currentMip_.resource_ = newResource;
			streamingTexture.currentMip_.bindlessIndex_ = newIndex;
			streamingTexture.currentMip_.sizeBytes_ = slicePitch;
		}

		/// [EN] Count the mip against the texture budget, record it as the finest resident one, and tell the RT material table that indices changed.
		/// [JP] ミップをテクスチャの予算に数え、常駐中で最も細かいものとして記録し、RT のマテリアルテーブルにインデックスが変わったことを知らせる。
		totalResidentTextureBytes_ += slicePitch;
		streamingTexture.topResidentMip_ = mipIndex;
		materialsDirty_ = true;
	}

	/**
	* [EN]
	* Synchronously uploads the shared LOD 0 vertex pool page. No-op when
	* already resident, when poolVertexEnd_ is 0 (no LOD 0 clusters), or
	* when no device is bound yet.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有 LOD 0 頂点プールページを同期アップロードする。常駐済み、
	* poolVertexEnd_ が 0（LOD 0 クラスタが無い）、またはまだ device が
	* 束縛されていない場合は何もしない。
	*/
	void Crister::MakePoolResident()
	{
		/// [EN] Nothing to do when the pool is already in VRAM, no LOD 0 cluster needs it, or Upload() has not run.
		/// [JP] プールが既に VRAM にあるか、どの LOD 0 クラスタも必要としないか、Upload() を通っていなければ何もしない。
		if (poolResident_ || poolVertexEnd_ == 0 || !device_)
		{
			return;
		}

		/// [EN] The pool is the leading [0, poolVertexEnd_) range of compressedVertices_, uploaded as is.
		/// [JP] プールは compressedVertices_ の先頭 [0, poolVertexEnd_) の範囲で、そのままアップロードする。
		DirectX::ResourceUploadBatch resourceUpload(device_);
		resourceUpload.Begin();
		HRESULT hr = CreateStaticBufferUnbounded(device_, resourceUpload, compressedVertices_.data(), poolVertexEnd_, sizeof(CompressedVertex), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, poolResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "頂点プールバッファの生成に失敗しました");
		poolBufferIndex_ = CreateStructuredShaderResourceView(device_, bindlessHeap_, poolResource_.Get(), poolVertexEnd_, sizeof(CompressedVertex));
		std::future<void> finished;
		{
			auto queueLock = uploadQueue_->AcquireLock();
			finished = resourceUpload.End(uploadQueue_->GetCommandQueue());
		}
		finished.wait();

		/// [EN] Count the pool against the geometry budget.
		/// [JP] プールをジオメトリの予算に数える。
		poolSizeBytes_ = static_cast<Uint64>(poolVertexEnd_) * sizeof(CompressedVertex);
		totalResidentGeometryBytes_ += poolSizeBytes_;
		poolResident_ = true;
	}

	/**
	* [EN]
	* Frees the cluster page's GPU resources and bindless indices
	* (deferred release, since in-flight frames may still reference
	* them). No-op if not resident or pinned.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページの GPU リソースと bindless インデックスを解放する
	* （インフライトのフレームがまだ参照している可能性があるため遅延
	* 解放）。常駐していないかピン留め済みなら何もしない。
	*/
	void Crister::EvictCluster(Uint32 clusterIndex)
	{
		StreamingGeometry& page = streamingGeometry_[clusterIndex];
		if (!page.resident_ || page.pinned_)
		{
			return;
		}

		/// [EN] Hand the page's buffers to the deferred-reclaim ring and return their descriptors.
		/// [JP] ページのバッファを遅延回収リングへ渡し、ディスクリプタを返す。
		bindlessHeap_->Release(std::move(page.meshletResource_), page.meshletBufferIndex_);
		bindlessHeap_->Release(std::move(page.meshletBoundResource_), page.meshletBoundBufferIndex_);
		bindlessHeap_->Release(std::move(page.vertexIndicesResource_), page.vertexIndicesBufferIndex_);
		bindlessHeap_->Release(std::move(page.primitiveIndicesResource_), page.primitiveIndicesBufferIndex_);
		if (page.ownsVertices_)
		{
			bindlessHeap_->Release(std::move(page.vertexResource_), page.vertexBufferIndex_);
		}
		else
		{
			/// [EN] A page that read the pool drops its reference, which may let EvictPool() free it.
			/// [JP] プールを読んでいたページは参照を外す。これで EvictPool() がプールを解放できるようになることがある。
			poolResidentReferences_--;
		}

		/// [EN] Take the page out of the geometry total.
		/// [JP] ページをジオメトリの合計から外す。
		totalResidentGeometryBytes_ -= page.sizeBytes_;
		page.sizeBytes_ = 0;
		page.resident_ = false;
	}

	/**
	* [EN]
	* Frees the texture's current (non-pinned) mip resource, falling
	* back to the pinned coarsest mip. No-op if not valid or already at
	* the pinned mip.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* テクスチャの現在の（ピン留めでない）ミップリソースを解放し、
	* ピン留めされた最粗ミップへ戻す。無効、または既にピン留めミップの
	* 場合は何もしない。
	*/
	void Crister::EvictTextureMip(Uint32 textureIndex)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}

		StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		/// [EN] Only currentMip_ can be evicted; a texture already down to its pinned mip has nothing to give back.
		/// [JP] 追い出せるのは currentMip_ だけ。既にピン留めのミップだけになっているテクスチャには返すものが無い。
		if (!streamingTexture.valid_ || streamingTexture.topResidentMip_ >= streamingTexture.mipCount_ - 1)
		{
			return;
		}

		/// [EN] Release currentMip_, take it out of the texture total, and fall back to the pinned mip.
		/// [JP] currentMip_ を解放してテクスチャの合計から外し、ピン留めのミップへ戻す。
		bindlessHeap_->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
		streamingTexture.currentMip_.bindlessIndex_ = SC_INVALID;
		totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
		streamingTexture.currentMip_.sizeBytes_ = 0;
		streamingTexture.topResidentMip_ = streamingTexture.mipCount_ - 1;

		/// [EN] The texture's SRV changed, so the RT material table must be rebuilt.
		/// [JP] テクスチャの SRV が変わったので、RT のマテリアルテーブルを作り直す必要がある。
		materialsDirty_ = true;
	}

	/**
	* [EN]
	* Frees the shared LOD 0 vertex pool page's GPU resource. No-op if
	* not resident, pinned, or still referenced by a resident cluster
	* page (poolResidentReferences_ > 0).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有 LOD 0 頂点プールページの GPU リソースを解放する。常駐していない、
	* ピン留め済み、または常駐中のクラスタページから参照されている場合
	* （poolResidentReferences_ > 0）は何もしない。
	*/
	void Crister::EvictPool()
	{
		/// [EN] The pool may go only when it is resident, not pinned, and no resident page reads it.
		/// [JP] プールを追い出せるのは、常駐していて、ピン留めでなく、どの常駐ページも読んでいないときだけ。
		if (!poolResident_ || poolPinned_ || poolResidentReferences_ > 0)
		{
			return;
		}
		bindlessHeap_->Release(std::move(poolResource_), poolBufferIndex_);
		totalResidentGeometryBytes_ -= poolSizeBytes_;
		poolSizeBytes_ = 0;
		poolResident_ = false;
	}

	/**
	* [EN]
	* Marks the cluster page (and its pool, if the cluster doesn't own
	* its own vertices) as used this frame so the eviction age guard
	* keeps it alive while the GPU may still reference it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページ（頂点を自前で持たないクラスタならプールも）を
	* 今フレーム使用中として記録し、GPU がまだ参照しうる間は追い出しの
	* 経過フレームガードで生存させる。
	*/
	void Crister::TouchCluster(Uint32 clusterIndex, Uint64 frame)
	{
		streamingGeometry_[clusterIndex].lastUsedFrame_ = frame;

		/// [EN] Drawing a page that reads the pool also counts as using the pool.
		/// [JP] プールを読むページを描画することは、プールを使ったことにもなる。
		if (!streamingGeometry_[clusterIndex].ownsVertices_)
		{
			poolLastUsedFrame_ = frame;
		}
	}

	/**
	* [EN]
	* Marks the texture's currently-resident mip as used this frame so
	* the eviction age guard keeps it alive while the GPU may still
	* reference it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* テクスチャの現在常駐中のミップを今フレーム使用中として記録し、
	* GPU がまだ参照しうる間は追い出しの経過フレームガードで生存させる。
	*/
	void Crister::TouchTexture(Uint32 textureIndex, Uint64 frame)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}
		streamingTextures_[textureIndex].lastUsedFrame_ = frame;
	}

	/**
	* [EN]
	* Whether the cluster's page is currently GPU-resident.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタのページが現在 GPU に常駐しているか。
	*/
	Bool Crister::ClusterResident(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].resident_;
	}

	/**
	* [EN]
	* Whether the texture has streamed all the way in to mip 0 (its finest).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* テクスチャがミップ 0（最も細かい）まで完全にストリームインしているか。
	*/
	Bool Crister::TextureResident(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return false;
		}

		/// [EN] Fully resident means mip 0, the finest, is the one in VRAM.
		/// [JP] 完全に常駐しているとは、最も細かいミップ 0 が VRAM にあること。
		return streamingTextures_[textureIndex].topResidentMip_ == 0;
	}

	/**
	* [EN]
	* Evicts unpinned cluster pages (oldest first) across every live
	* Crister, then unpinned/unreferenced vertex pools, until total
	* resident geometry fits geometryBudgetBytes_. Pages must be unused
	* for evictAgeFrames_ frames before eviction so in-flight GPU work
	* never loses a buffer it references.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 生存中の全 Crister を対象に、未ピンのクラスタページを古い順に、
	* 続いて未ピン/未参照の頂点プールを追い出し、常駐ジオメトリ合計を
	* geometryBudgetBytes_ 以内に収める。インフライトの GPU 作業が
	* 参照中のバッファを失わないよう、evictAgeFrames_ フレーム未使用の
	* ページのみ追い出す。
	*/
	void Crister::EvictClusterBudget(Uint64 currentFrame)
	{
		/// [EN] Nothing to do while the geometry fits its budget.
		/// [JP] ジオメトリが予算内に収まっている間は何もしない。
		if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
		{
			return;
		}

		/// [EN] A page that may be evicted, with the frame it was last used.
		/// [JP] 追い出してよいページと、それが最後に使われたフレーム。
		struct Candidate
		{
			/// [EN] Crister the page belongs to.
			/// [JP] ページが属する Crister。
			Crister* crister_;

			/// [EN] Index of the page's cluster in that Crister.
			/// [JP] その Crister での、ページのクラスタのインデックス。
			Uint32 clusterIndex_;

			/// [EN] Last frame the page was drawn from.
			/// [JP] ページが最後に描画に使われたフレーム。
			Uint64 lastUsedFrame_;
		};

		/// [EN] Collect every resident, unpinned page across all Crister that has gone unused for at least evictAgeFrames_.
		/// [JP] 全 Crister から、常駐中でピン留めでなく、evictAgeFrames_ 以上使われていないページを集める。
		DynamicArray<Candidate> candidates;
		for (Crister* crister : streamingRegistry_)
		{
			for (Size clusterIndex = 0; clusterIndex < crister->streamingGeometry_.size(); clusterIndex++)
			{
				const StreamingGeometry& page = crister->streamingGeometry_[clusterIndex];
				if (page.resident_ && !page.pinned_ && page.lastUsedFrame_ + evictAgeFrames_ <= currentFrame)
				{
					candidates.push_back({ crister, static_cast<Uint32>(clusterIndex), page.lastUsedFrame_ });
				}
			}
		}

		/// [EN] Least recently used first.
		/// [JP] 最後に使われたのが古い順に並べる。
		std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b)
		{
			return a.lastUsedFrame_ < b.lastUsedFrame_;
		});

		/// [EN] Evict pages until the total is back within the budget.
		/// [JP] 合計が予算内に戻るまでページを追い出す。
		for (const Candidate& candidate : candidates)
		{
			if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
			{
				break;
			}
			candidate.crister_->EvictCluster(candidate.clusterIndex_);
		}

		/// [EN] Still over budget: free vertex pools that have gone unused long enough and that no resident page reads any more.
		/// [JP] まだ予算を超えていれば、十分長く使われておらず、もうどの常駐ページも読んでいない頂点プールを解放する。
		for (Crister* crister : streamingRegistry_)
		{
			if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
			{
				break;
			}
			if (crister->poolLastUsedFrame_ + evictAgeFrames_ <= currentFrame)
			{
				crister->EvictPool();
			}
		}
	}

	/**
	* [EN]
	* Texture counterpart to EvictClusterBudget: evicts unpinned
	* resident texture mips (oldest first) across every live Crister
	* until total resident texture memory fits textureBudgetBytes_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* EvictClusterBudget のテクスチャ版の対: 生存中の全 Crister を対象に、
	* 未ピンの常駐ミップを古い順に追い出し、常駐テクスチャメモリ合計を
	* textureBudgetBytes_ 以内に収める。
	*/
	void Crister::EvictTextureBudget(Uint64 currentFrame)
	{
		/// [EN] Nothing to do while the textures fit their budget.
		/// [JP] テクスチャが予算内に収まっている間は何もしない。
		if (totalResidentTextureBytes_ <= textureBudgetBytes_)
		{
			return;
		}

		/// [EN] A texture whose currentMip_ may be evicted, with the frame it was last sampled.
		/// [JP] currentMip_ を追い出してよいテクスチャと、それが最後にサンプルされたフレーム。
		struct Candidate
		{
			/// [EN] Crister the texture belongs to.
			/// [JP] テクスチャが属する Crister。
			Crister* crister_;

			/// [EN] Index of the texture in that Crister.
			/// [JP] その Crister でのテクスチャのインデックス。
			Uint32 textureIndex_;

			/// [EN] Last frame the texture was sampled.
			/// [JP] テクスチャが最後にサンプルされたフレーム。
			Uint64 lastUsedFrame_;
		};

		/// [EN] Collect every texture across all Crister holding a mip finer than its pinned one that has gone unused for at least evictAgeFrames_.
		/// [JP] 全 Crister から、ピン留めより細かいミップを持ち、evictAgeFrames_ 以上使われていないテクスチャを集める。
		DynamicArray<Candidate> candidates;
		for (Crister* crister : streamingRegistry_)
		{
			for (Size textureIndex = 0; textureIndex < crister->streamingTextures_.size(); textureIndex++)
			{
				const StreamingTexture& streamingTexture = crister->streamingTextures_[textureIndex];
				if (streamingTexture.valid_ && streamingTexture.topResidentMip_ < streamingTexture.mipCount_ - 1 && streamingTexture.lastUsedFrame_ + evictAgeFrames_ <= currentFrame)
				{
					candidates.push_back({ crister, static_cast<Uint32>(textureIndex), streamingTexture.lastUsedFrame_ });
				}
			}
		}

		/// [EN] Least recently used first.
		/// [JP] 最後に使われたのが古い順に並べる。
		std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b)
		{
			return a.lastUsedFrame_ < b.lastUsedFrame_;
		});

		/// [EN] Drop textures back to their pinned mip until the total is within the budget.
		/// [JP] 合計が予算内に戻るまで、テクスチャをピン留めのミップへ戻していく。
		for (const Candidate& candidate : candidates)
		{
			if (totalResidentTextureBytes_ <= textureBudgetBytes_)
			{
				break;
			}
			candidate.crister_->EvictTextureMip(candidate.textureIndex_);
		}
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's vertex buffer — its own page
	* if it owns vertices, otherwise the shared LOD 0 pool.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページの頂点バッファの bindless SRV — 頂点を自前で持てば
	* 自身のページ、そうでなければ共有 LOD 0 プール。
	*/
	Uint Crister::ClusterVertexBufferIndex(Uint32 clusterIndex)const
	{
		const StreamingGeometry& page = streamingGeometry_[clusterIndex];

		/// [EN] A page with its own vertices reads them; any other page reads the shared pool.
		/// [JP] 自前の頂点を持つページはそれを読み、それ以外のページは共有プールを読む。
		return page.ownsVertices_ ? page.vertexBufferIndex_ : poolBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's meshlet buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページの meshlet バッファの bindless SRV。
	*/
	Uint Crister::ClusterMeshletBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].meshletBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's meshlet bound buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページの meshlet バウンドバッファの bindless SRV。
	*/
	Uint Crister::ClusterMeshletBoundBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].meshletBoundBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's vertex indices buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページの頂点インデックスバッファの bindless SRV。
	*/
	Uint Crister::ClusterVertexIndicesBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].vertexIndicesBufferIndex_;
	}

	/**
	* [EN]
	* Bindless SRV of the cluster page's primitive indices buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クラスタページのプリミティブインデックスバッファの bindless SRV。
	*/
	Uint Crister::ClusterPrimitiveIndicesBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].primitiveIndicesBufferIndex_;
	}

	/**
	* [EN]
	* Whether this cluster page owns its own vertex slice
	* (streamingGeometry_[clusterIndex].ownsVertices_) rather than
	* referencing the shared LOD 0 pool. Own-page vertex indices are
	* rebased to page-local numbering by MakeClusterResident, so they
	* are NOT valid indices into Crister::vertexMorphSource_/
	* morphDeltaResource_ (which use the crister-wide numbering the
	* shared pool preserves) — callers populating a raster morph
	* instance must check this and leave morph fields zeroed
	* (ModelStructuredBuffer::morphTargetCount_ == 0) for any cluster where
	* this returns true.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このクラスタページが共有 LOD 0 プールを参照するのではなく、
	* 自前の頂点スライスを持つか
	* (streamingGeometry_[clusterIndex].ownsVertices_)。自前ページの
	* 頂点インデックスは MakeClusterResident によってページローカルな
	* 番号へリベースされるため、Crister::vertexMorphSource_/
	* morphDeltaResource_(共有プールが保つ Crister 全体の番号付けを使う)
	* への有効なインデックスでは【ない】— ラスタのモーフ用インスタンス
	* を組み立てる側はこれを確認し、true が返るクラスタでは
	* モーフフィールドをゼロのまま
	* (ModelStructuredBuffer::morphTargetCount_ == 0)にすること。
	*/
	Bool Crister::StandaloneVertices(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].ownsVertices_;
	}

	/**
	* [EN]
	* Finest mip level currently GPU-resident, or 0 if textureIndex is
	* out of range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在 GPU に常駐している最も細かいミップ段。textureIndex が範囲外
	* なら 0。
	*/
	Uint32 Crister::TextureFinestMip(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return 0;
		}
		return streamingTextures_[textureIndex].topResidentMip_;
	}

	/**
	* [EN]
	* Picks the mip a material texture needs so that one texel covers
	* about one screen pixel: texels per world unit (the SubMesh's baked
	* texcoordDensity * texture resolution) versus screen pixels per
	* world unit (worldScale * pixelsPerUnit). texcoordDensity 0 falls
	* back to assuming the texture spans the whole model once.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1 テクセルが画面 1 ピクセル程度になるミップを選ぶ: 1 ワールド単位
	* あたりのテクセル数(SubMesh に焼いた texcoordDensity × テクスチャ
	* 解像度)と、1 ワールド単位あたりの画面ピクセル数(worldScale ×
	* pixelsPerUnit)を比べる。texcoordDensity が 0 のときはテクスチャが
	* モデル全体に 1 回貼られている前提にフォールバックする。
	*/
	Uint32 Crister::TextureDesiredMip(Uint32 textureIndex, Float texcoordDensity, Float worldScale, Float pixelsPerUnit)const
	{
		/// [EN] A texture that does not stream has no mip to choose.
		/// [JP] ストリーミングしないテクスチャには選ぶミップが無い。
		if (textureIndex >= streamingTextures_.size() || !streamingTextures_[textureIndex].valid_)
		{
			return 0;
		}

		/// [EN] With no camera or no projection yet, pixelsPerUnit is 0 and there is nothing to decide by, so the finest mip is requested (falling back to the coarsest would turn every texture into its average color).
		/// [JP] カメラが無い/射影が未設定だと pixelsPerUnit が 0 になる。その場合は判断材料が無いので最も細かいミップを要求する(最粗へ倒すと全テクスチャが平均色になってしまう)。
		Float screenPixelsPerWorldUnit = worldScale * pixelsPerUnit;
		if (!(screenPixelsPerWorldUnit > 0.0f))
		{
			return 0;
		}

		/// [EN] texelsPerWorldUnit: how many mip-0 texels one (mesh-local) world unit of surface spans. With the baked texcoordDensity this is exact on average for the SubMesh - a UV atlas that packs many islands into one texture gets a proportionally higher density than "the texture spans the model once" would give. max(width, height) keeps non-square textures on the sharp side. texelsPerScreenPixel is then how many mip-0 texels crowd into one screen pixel; each doubling costs exactly one mip level (rounded down, toward sharp).
		/// [JP] texelsPerWorldUnit: 表面の(メッシュローカル)1 ワールド単位がmip 0 の何テクセルに相当するか。焼き込んだ texcoordDensity を使うと SubMesh 平均で正確になる — 多数の UV アイランドを 1 枚に詰めたアトラスは、「モデルに 1 回貼られる」前提より相応に密度が高くなる。非正方形テクスチャは max(width, height) で鮮明側に倒す。texelsPerScreenPixel は画面 1 ピクセルに詰め込まれる mip 0 のテクセル数で、2 倍になるごとにちょうど 1 ミップ粗くできる(切り捨てて鮮明側に倒す)。
		const Bitmap& bitmap = bitmaps_[textureIndex];
		Float textureResolution = static_cast<Float>(Max(bitmap.width_, bitmap.height_));

		Float texelsPerWorldUnit = 0.0f;

		/// [EN] Use the SubMesh's baked density; without it, assume the texture spans the model's largest dimension once.
		/// [JP] SubMesh に焼き込んだ密度を使う。無ければ、テクスチャがモデルの最も大きい寸法に1回貼られているとみなす。
		if (texcoordDensity > 0.0f)
		{
			texelsPerWorldUnit = texcoordDensity * textureResolution;
		}
		else
		{
			Vector3 extent = positionExtent_;
			Float modelSize = Max(Max(extent.x, extent.y), extent.z);
			texelsPerWorldUnit = textureResolution / Max(modelSize, 1e-6f);
		}

		/// [EN] While a screen pixel covers at most one texel, mip 0 is needed; beyond that, each doubling of texels per pixel allows one coarser mip.
		/// [JP] 画面1ピクセルが覆うテクセルが1つ以下ならミップ 0 が要る。それを超えると、ピクセルあたりのテクセルが2倍になるごとに1段粗いミップでよい。
		Uint32 desiredMip = 0;
		Float texelsPerScreenPixel = texelsPerWorldUnit / screenPixelsPerWorldUnit;
		if (texelsPerScreenPixel > 1.0f)
		{
			desiredMip = static_cast<Uint32>(std::log2(texelsPerScreenPixel));
		}

		/// [EN] Never ask for a mip coarser than the coarsest one that streams.
		/// [JP] ストリーミングできる最も粗いミップより粗いものは求めない。
		Uint32 mipCount = streamingTextures_[textureIndex].mipCount_;
		return mipCount == 0 ? 0 : Min(desiredMip, mipCount - 1);
	}

	/**
	* [EN]
	* Walks stages_[defaultStage_]'s node tree depth-first and
	* recomputes every Node::globalTransform_ from local S/R/T.
	* Duplicates ModelLoader::CumulateTransforms' logic so
	* ApplyAxisConversion can recompute global transforms without a
	* ModelLoader/glTF re-parse.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* stages_[defaultStage_]のノードツリーを深さ優先で走査し、ローカル
	* S/R/T から全 Node::globalTransform_ を再計算する。
	* ModelLoader::CumulateTransforms のロジックを複製したもの。
	* ApplyAxisConversion が ModelLoader/glTF 再解析無しにグローバル
	* トランスフォームを再計算できるようにする。
	*/
	void Crister::CumulateTransforms()
	{
		/// [EN] Without a valid default stage there is no root to start from.
		/// [JP] 正しいデフォルトステージが無ければ、始めるルートが無い。
		if (defaultStage_ < 0 || static_cast<Size>(defaultStage_) >= stages_.size())
		{
			return;
		}

		/// [EN] The top of the stack is the global transform of the parent of the node being visited.
		/// [JP] スタックの一番上が、訪れているノードの親のグローバル変換。
		std::stack<Matrix> parentGlobalTransforms;
		std::function<void(Int)> traverse = [&](Int nodeIndex)
			{
				if (nodeIndex < 0 || static_cast<Size>(nodeIndex) >= nodes_.size())
				{
					return;
				}

				Node& node = nodes_[nodeIndex];
				Matrix scale = Matrix::CreateScale(node.scale_.x, node.scale_.y, node.scale_.z);
				Matrix rotation = Matrix::CreateFromQuaternion(node.rotation_);
				Matrix translation = Matrix::CreateTranslation(node.translation_.x, node.translation_.y, node.translation_.z);

				/// [EN] Local S/R/T in row-vector order, then onto the parent's global transform.
				/// [JP] ローカルの S/R/T を行ベクトルの順で掛け、親のグローバル変換に重ねる。
				Matrix localTransform = scale * rotation * translation;
				node.globalTransform_ = localTransform * parentGlobalTransforms.top();

				/// [EN] Each child sees this node's global transform as its parent's.
				/// [JP] 各子は、このノードのグローバル変換を親のものとして受け取る。
				for (Int childIndex : node.children_)
				{
					parentGlobalTransforms.push(node.globalTransform_);
					traverse(childIndex);
					parentGlobalTransforms.pop();
				}
			};

		/// [EN] Every root starts under the identity.
		/// [JP] 各ルートは単位行列の下から始める。
		for (Int nodeIndex : stages_[defaultStage_].nodes_)
		{
			parentGlobalTransforms.push(Matrix::Identity);
			traverse(nodeIndex);
			parentGlobalTransforms.pop();
		}
	}

	/**
	* [EN]
	* Allocates a bindless heap index and creates a StructuredBuffer SRV
	* for resource at that index.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* bindless ヒープインデックスを1つ確保し、resource に対する
	* StructuredBuffer SRV をそのインデックスへ作成する。
	*/
	Uint Crister::CreateStructuredShaderResourceView(ID3D12Device* device, BindlessHeap* heap, ID3D12Resource* resource, Uint elementCount, Uint stride)
	{
		/// [EN] Take a free slot in the bindless heap and describe the whole buffer as elementCount structured elements of stride bytes.
		/// [JP] bindless ヒープの空きスロットを取り、バッファ全体を stride バイトの構造化要素 elementCount 個として記述する。
		Uint index = heap->AllocateIndex();

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Buffer.FirstElement = 0;
		shaderResourceViewDesc.Buffer.NumElements = elementCount;
		shaderResourceViewDesc.Buffer.StructureByteStride = stride;
		shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		device->CreateShaderResourceView(resource, &shaderResourceViewDesc, heap->CPUHandle(index));

		return index;
	}

	/**
	* [EN]
	* Creates a DEFAULT-heap buffer filled with data, the same way
	* DirectX::CreateStaticBuffer does but with no size cap.
	* CreateStaticBuffer rejects anything above ~128MB, because
	* DirectXTK12 treats D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM
	* as a flat byte limit; D3D12 buffers themselves are bounded only by
	* GPU memory, and a high-poly mesh's flat 32-bit triangle index
	* buffer easily goes past 128MB.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* data を詰めた DEFAULT ヒープのバッファを、DirectX::CreateStaticBuffer
	* と同じ手順で、ただしサイズの上限なしで作る。CreateStaticBuffer は
	* DirectXTK12 が D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM
	* を単純なバイト上限として扱うため、約128MB を超えるものを拒否する。
	* D3D12 のバッファ自体の上限は GPU メモリだけで、ハイポリメッシュの
	* フラットな 32bit 三角形インデックスバッファは 128MB を簡単に超える。
	*/
	HRESULT Crister::CreateStaticBufferUnbounded(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const void* data, Uint64 count, Uint64 stride, D3D12_RESOURCE_STATES afterState, ID3D12Resource** outResource)
	{
		/// [EN] A plain DEFAULT-heap buffer of the full size, with no cap.
		/// [JP] 上限なしで、全体のサイズを持つ素の DEFAULT ヒープのバッファ。
		const Uint64 sizeInBytes = count * stride;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(resource.GetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
#if SC_DEBUG
		resource->SetName(L"Crister_StaticBufferUnbounded");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(resource.Get());
#endif

		/// [EN] Queue the data upload and the move to afterState on the caller's batch; the buffer is filled when the batch is submitted.
		/// [JP] データのアップロードと afterState への移行を呼び出し側のバッチに積む。バッファはバッチを投入したときに埋まる。
		D3D12_SUBRESOURCE_DATA subresourceData{ data, 0, 0 };
		resourceUpload.Upload(resource.Get(), 0, &subresourceData, 1);
		resourceUpload.Transition(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, afterState);

		/// [EN] Hand ownership of the reference to the caller.
		/// [JP] 参照の所有権を呼び出し側へ渡す。
		*outResource = resource.Detach();
		return S_OK;
	}

	/**
	* [EN]
	* Texture counterpart to CreateStaticBufferUnbounded's byte-layout
	* role: resolves mip dimensions/row-pitch/slice-pitch/byte-offset
	* within Bitmap::cacheData_ for a given mip index, from the standard
	* BC block layout (16 bytes per 4x4 texel block, mips concatenated
	* in order with no stored per-mip offsets).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CreateStaticBufferUnbounded のバイトレイアウト計算に相当する
	* テクスチャ版: Bitmap::cacheData_ 内での指定ミップの寸法/
	* row-pitch/slice-pitch/バイトオフセットを、標準的な BC
	* ブロックレイアウト（4x4 テクセルブロックあたり 16 バイト、
	* ミップは順に連結・オフセット未保存）から解決する。
	*/
	void Crister::ComputeTextureMipLayout(const Bitmap& bitmap, Uint32 mipIndex, Uint32& outWidth, Uint32& outHeight, Uint64& outRowPitch, Uint64& outSlicePitch, Uint64& outByteOffset)
	{
		Uint64 offset = 0;
		Int mipWidth = bitmap.width_;
		Int mipHeight = bitmap.height_;

		/// [EN] Skip over every finer mip: each takes one 16-byte block per 4x4 texels, and the size halves per level down to 1.
		/// [JP] それより細かい全ミップを飛ばす。各ミップは 4x4 テクセルにつき 16 バイトのブロックを1つ使い、サイズは段ごとに半分になって最小 1 まで下がる。
		for (Uint32 mip = 0; mip < mipIndex; mip++)
		{
			Uint64 blockWidth = Max<Uint64>(1, (static_cast<Uint64>(mipWidth) + 3) / 4);
			Uint64 blockHeight = Max<Uint64>(1, (static_cast<Uint64>(mipHeight) + 3) / 4);
			offset += blockWidth * 16 * blockHeight;
			mipWidth = Max(1, mipWidth / 2);
			mipHeight = Max(1, mipHeight / 2);
		}

		/// [EN] The requested mip: its size, one row of blocks as the row pitch, all block rows as the slice pitch.
		/// [JP] 求めるミップ。サイズと、ブロック1行を行ピッチ、ブロックの全行をスライスピッチとする。
		Uint64 blockWidth = Max<Uint64>(1, (static_cast<Uint64>(mipWidth) + 3) / 4);
		Uint64 blockHeight = Max<Uint64>(1, (static_cast<Uint64>(mipHeight) + 3) / 4);
		outWidth = static_cast<Uint32>(mipWidth);
		outHeight = static_cast<Uint32>(mipHeight);
		outRowPitch = blockWidth * 16;
		outSlicePitch = outRowPitch * blockHeight;
		outByteOffset = offset;
	}

	/**
	* [EN]
	* Dequantises just the position field of a CompressedVertex, against
	* this Crister's own positionMin_/positionExtent_. A separate function
	* from DecodeVertex because some callers only need position (deduping
	* by position, or building a position-only buffer) and would otherwise
	* pay for decoding normal/tangent/texcoord they never use.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex の position フィールドだけを、この Crister 自身の
	* positionMin_/positionExtent_ に対して逆量子化する。DecodeVertex とは
	* 別関数にしてあるのは、呼び出し元の一部が position だけを必要とする
	* （position で重複排除する、position だけのバッファを作る）ため —
	* そうしないと使わない normal/tangent/texcoord のデコードまで払うことになる。
	*/
	Vector3 Crister::DecodePosition(const CompressedVertex& compressed)const
	{
		/// [EN] Each 16-bit UNORM maps back across the AABB extent from its minimum corner.
		/// [JP] 各 16bit UNORM を、AABB の最小コーナーから大きさの範囲へ戻す。
		return positionMin_ + Vector3(
			static_cast<Float>(compressed.positionXY_ & 0xFFFF) / 65535.0f * positionExtent_.x,
			static_cast<Float>(compressed.positionXY_ >> 16) / 65535.0f * positionExtent_.y,
			static_cast<Float>(compressed.positionZTexU_ & 0xFFFF) / 65535.0f * positionExtent_.z);
	}

	/**
	* [EN]
	* Dequantises just the texcoord field of a CompressedVertex, against
	* this Crister's own texcoordMin_/texcoordExtent_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex の texcoord フィールドだけを、この Crister 自身の
	* texcoordMin_/texcoordExtent_ に対して逆量子化する。
	*/
	Vector2 Crister::DecodeTexcoord(const CompressedVertex& compressed)const
	{
		/// [EN] u sits in the upper half of positionZTexU_ and v in the lower half of texVTangent_.
		/// [JP] u は positionZTexU_ の上位16bit、v は texVTangent_ の下位16bit にある。
		return texcoordMin_ + Vector2(
			static_cast<Float>(compressed.positionZTexU_ >> 16) / 65535.0f * texcoordExtent_.x,
			static_cast<Float>(compressed.texVTangent_ & 0xFFFF) / 65535.0f * texcoordExtent_.y);
	}

	/**
	* [EN]
	* Dequantises just the tangent field (xyz direction + handedness sign
	* in .w) of a CompressedVertex.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex の tangent フィールド（xyz 方向 + .w の利き手符号）
	* だけを逆量子化する。
	*/
	Vector4 Crister::DecodeTangent(const CompressedVertex& compressed)const
	{
		/// [EN] The tangent is the upper 16 bits: octahedral x in 8 bits, y in 7 bits and the handedness sign on top.
		/// [JP] 接線は上位16bit。octahedral の x が 8bit、y が 7bit、最上位が利き手の符号。
		Uint32 tangentBits = compressed.texVTangent_ >> 16;

		/// [EN] Widen both components back to 16 bits so the normal decoder can be reused.
		/// [JP] 法線のデコードを使い回せるよう、両成分を 16bit へ広げ直す。
		Uint32 octX16 = static_cast<Uint32>(static_cast<Float>(tangentBits & 0xFF) / 255.0f * 65535.0f + 0.5f);
		Uint32 octY16 = static_cast<Uint32>(static_cast<Float>((tangentBits >> 8) & 0x7F) / 127.0f * 65535.0f + 0.5f);
		Float sign = (tangentBits >> 15) ? 1.0f : -1.0f;

		Vector3 xyz = DecodeOctahedralNormal(octX16 | (octY16 << 16));
		return Vector4(xyz.x, xyz.y, xyz.z, sign);
	}

	/**
	* [EN]
	* Dequantises the joint indices and renormalised weights of a
	* CompressedSkinVertex. Weights are renormalised because four 8-bit
	* UNORM values rarely sum to exactly one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedSkinVertex のジョイントインデックスと再正規化済み
	* ウェイトを逆量子化する。4つの 8bit UNORM 値は合計がちょうど1に
	* なることが稀なため再正規化する。
	*/
	void Crister::DecodeSkin(const CompressedSkinVertex& compressed, XmUint4& outJoints, Vector4& outWeights)const
	{
		/// [EN] Unpack the four 16-bit joint indices.
		/// [JP] 16bit のジョイントインデックスを4つ取り出す。
		outJoints.x = compressed.jointsXY_ & 0xFFFF;
		outJoints.y = compressed.jointsXY_ >> 16;
		outJoints.z = compressed.jointsZW_ & 0xFFFF;
		outJoints.w = compressed.jointsZW_ >> 16;

		outWeights = Vector4(static_cast<Float>(compressed.weights_ & 0xFF), static_cast<Float>((compressed.weights_ >> 8) & 0xFF), static_cast<Float>((compressed.weights_ >> 16) & 0xFF), static_cast<Float>(compressed.weights_ >> 24)) / 255.0f;

		/// [EN] Dividing by the sum both undoes the 8-bit scale and makes the weights add up to 1.
		/// [JP] 合計で割ることで、8bit の倍率を戻すと同時に重みの合計を 1 にする。
		Float weightSum = outWeights.x + outWeights.y + outWeights.z + outWeights.w;
		outWeights /= Max(weightSum, 1e-6f);
	}

	/**
	* [EN]
	* Change-of-basis for a position: transforms vector in place by basis.
	* Duplicates ModelLoader's ConvertPositionByBasis (private to
	* ModelLoader) so ApplyAxisConversion can re-convert without a
	* ModelLoader/glTF re-parse.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 位置の基底変換。vector を basis でその場変換する。ModelLoader の
	* ConvertPositionByBasis（ModelLoader 限定 private）を複製し、
	* ApplyAxisConversion が ModelLoader/glTF 再解析無しに再変換できる
	* ようにする。
	*/
	void Crister::ConvertPositionByBasis(Vector3& vector, const Matrix& basis)const
	{
		vector = Vector3::Transform(vector, basis);
	}

	/**
	* [EN]
	* Change-of-basis for a rotation: transforms quaternion in place by basis.
	* Duplicates ModelLoader's ConvertRotationByBasis (private to ModelLoader).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 回転の基底変換。quaternion を basis でその場変換する。ModelLoader の
	* ConvertRotationByBasis（ModelLoader 限定 private）を複製する。
	*/
	void Crister::ConvertRotationByBasis(Quaternion& quaternion, const Matrix& basis)const
	{
		/// [EN] Conjugate the rotation by the orthonormal basis (inverse = transpose) so it rotates the same way in the new axes.
		/// [JP] 正規直交な基底(逆行列 = 転置)で回転を挟み、新しい軸でも同じように回るようにする。
		Matrix rotation = Matrix::CreateFromQuaternion(quaternion);
		Matrix converted = basis.Transpose() * rotation * basis;
		quaternion = Quaternion::CreateFromRotationMatrix(converted);
	}

	/**
	* [EN]
	* Change-of-basis for a matrix: transforms matrix in place by basis.
	* Duplicates ModelLoader's ConvertMatrixByBasis (private to ModelLoader).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 行列の基底変換。matrix を basis でその場変換する。ModelLoader の
	* ConvertMatrixByBasis（ModelLoader 限定 private）を複製する。
	*/
	void Crister::ConvertMatrixByBasis(Matrix& matrix, const Matrix& basis)const
	{
		/// [EN] Conjugate by the orthonormal basis, the same change of basis the rotations get.
		/// [JP] 回転と同じ基底変換として、正規直交な基底で挟む。
		matrix = basis.Transpose() * matrix * basis;
	}

	/**
	* [EN]
	* Quantises a [0,1] float to a 16-bit UNORM, clamping out-of-range
	* input first. Shared by EncodeVertex (position/texcoord) and
	* EncodeOctahedralNormal.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* [0,1] の float を 16bit UNORM へ量子化する。範囲外の入力は先に
	* クランプする。EncodeVertex（position/texcoord）と
	* EncodeOctahedralNormal が共有する。
	*/
	Uint32 Crister::QuantizeUnorm16(Float value01)
	{
		/// [EN] Clamp to [0,1], then round to the nearest of 65536 steps.
		/// [JP] [0,1] にクランプし、65536 段階の最も近い値へ丸める。
		Float clamped = value01 < 0.0f ? 0.0f : (value01 > 1.0f ? 1.0f : value01);
		return static_cast<Uint32>(clamped * 65535.0f + 0.5f);
	}

	/**
	* [EN]
	* Quantises a [0,1] float to an 8-bit UNORM, clamping out-of-range
	* input first. Used by BakeMesh for skin weights.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* [0,1] の float を 8bit UNORM へ量子化する。範囲外の入力は先に
	* クランプする。BakeMesh がスキンウェイトに使う。
	*/
	Uint32 Crister::QuantizeUnorm8(Float value01)
	{
		/// [EN] Clamp to [0,1], then round to the nearest of 256 steps.
		/// [JP] [0,1] にクランプし、256 段階の最も近い値へ丸める。
		Float clamped = value01 < 0.0f ? 0.0f : (value01 > 1.0f ? 1.0f : value01);
		return static_cast<Uint32>(clamped * 255.0f + 0.5f);
	}

}
