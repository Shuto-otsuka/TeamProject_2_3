#include <External/NsightAftermath/Include/GFSDK_Aftermath.h>
#include <External/DirectXTex/Include/DirectXTex.h>
#include <GraphicsEngine/Model/Crister/CristerCompressor.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/Graphics/GraphicsProcessor.h>
#include <GraphicsEngine/Texture/Compression/BC7CompressShader.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	/**
	* [EN]
	* Downsamples oversized textures, dilates transparent-texel color
	* (before compression, since BC7 blocks cannot be edited per texel
	* afterwards), then BC7-compresses a full mip chain into each
	* Bitmap's cacheData_. Must run before serialising to the .crister
	* cache. The BC7 encoding runs on the GPU (BC7CompressCS.hlsl, mode
	* 6 only) as a one-shot compute dispatch on cmdQueue, with the root
	* signature and PSO of the processor's BC7 shader. The shader binds its input
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
	* コンピュートディスパッチとして、processor の BC7 シェーダーの
	* ルートシグネチャと PSO で動かす。シェーダーは入出力をルート
	* SRV/UAV でバインドするので、Upload() と違い BindlessHeap は要らない。
	*/
	void CristerCompressor::BakeBitmap(ID3D12Device* device, D3D12CommandQueue* cmdQueue, GraphicsProcessor& processor, std::span<Bitmap> bitmaps)const
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

		/// [EN] BC7-compresses one mip on the GPU and reads the blocks back into outputBlocks, fully synchronously: a one-shot command list and fence are created and the calling thread blocks until the GPU is done. The root signature and PSO come from the processor's BC7 shader, which Graphics builds once at startup.
		/// [JP] 1ミップぶんをGPUでBC7圧縮してoutputBlocksへ読み戻す。完全同期:専用の一発コマンドリストとフェンスを作り、GPUが終わるまで呼び出しスレッドをブロックする。ルートシグネチャ/PSOはprocessorのBC7シェーダー(Graphics所有、起動時に一度だけ構築)のものを使う。
		auto compressBC7 = [device, cmdQueue, &processor](const DirectX::Image& image, DynamicArray<Uchar>& outputBlocks)
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

			cmdList->SetComputeRootSignature(processor.BC7().GetRootSignature());
			cmdList->SetPipelineState(processor.BC7().GetPipelineState());

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

		for (Bitmap& texture : bitmaps)
		{
			/// [EN] An image with no texels has nothing to bake.
			/// [JP] テクセルの無い画像には焼くものが無い。
			if (texture.width_ <= 0 || texture.height_ <= 0 || texture.cacheData_.empty())
			{
				continue;
			}

			/// [EN] Only images decoded to 8 or 16 bits per channel, with 1 to 4 channels and as many bytes as that layout needs, can be baked. Anything else is dropped (no data, no mips), so the residency never mistakes undecoded pixels for BC7 blocks.
			/// [JP] 焼けるのは、チャンネルあたり 8bit か 16bit で 1〜4 チャンネル、その形に必要なだけのバイトがある画像だけ。それ以外は捨て(データもミップも無し)、デコードしたままのピクセルを常駐管理が BC7 のブロックと取り違えないようにする。
			Size pixelCount = static_cast<Size>(texture.width_) * static_cast<Size>(texture.height_);
			Size bytesPerComponent = static_cast<Size>(texture.bits_ / 8);
			Bool supportedLayout = (texture.bits_ == 8 || texture.bits_ == 16) && texture.component_ >= 1 && texture.component_ <= 4;
			if (!supportedLayout || texture.cacheData_.size() < pixelCount * static_cast<Size>(texture.component_) * bytesPerComponent)
			{
				SC_LOG_WARNING("テクスチャ {} の形式({} チャンネル, {}bit)には対応していません。このテクスチャ抜きで続行します。", texture.name_, texture.component_, texture.bits_);
				texture.cacheData_.clear();
				texture.mipCount_ = 0;
				continue;
			}

			/// [EN] Always produce a mutable RGBA8 buffer (expanding when the decoded image has fewer than 4 components, and narrowing 16-bit channels to 8 bits) so it can be dilated in place.
			/// [JP] dilation で書き換えるため、常に可変の RGBA8 バッファを作る（4 コンポーネント未満なら展開し、16bit のチャンネルは 8bit へ狭める。8bit の 4 コンポーネントならコピー）。
			DynamicArray<Uchar> rgba(pixelCount * 4);

			/// [EN] Missing channels are filled with 0, and a missing alpha with 255 so the texel is opaque. A 16-bit channel is little-endian and is rounded to the nearest 8-bit value.
			/// [JP] 足りないチャンネルは 0 で埋め、アルファが無ければ 255 にして不透明にする。16bit のチャンネルはリトルエンディアンで、最も近い 8bit の値へ丸める。
			if (texture.component_ == 4 && bytesPerComponent == 1)
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
							Size source = (pixel * static_cast<Size>(texture.component_) + static_cast<Size>(component)) * bytesPerComponent;
							if (bytesPerComponent == 2)
							{
								Uint32 wide = static_cast<Uint32>(texture.cacheData_[source]) | (static_cast<Uint32>(texture.cacheData_[source + 1]) << 8);
								rgba[pixel * 4 + component] = static_cast<Uchar>((wide * 255 + 32767) / 65535);
							}
							else
							{
								rgba[pixel * 4 + component] = texture.cacheData_[source];
							}
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
				texture.cacheData_.clear();
				texture.mipCount_ = 0;
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
}