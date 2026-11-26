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
	void CristerCompressor::BakeBitmap(ID3D12Device* device, D3D12CommandQueue* cmdQueue, GraphicsProcessor& processor, std::span<Bitmap> bitmaps)const
	{
		auto downsampleRgbaToFit = [](DynamicArray<Uchar>& rgba, Int& width, Int& height, Int maxDimension)
		{
			Int largest = width > height ? width : height;
			Int factor = (largest + maxDimension - 1) / maxDimension;
			if (factor <= 1)
			{
				return;
			}

			Int newWidth = width / factor;
			Int newHeight = height / factor;
			DynamicArray<Uchar> shrunk(static_cast<Size>(newWidth) * newHeight * 4);

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

			rgba = std::move(shrunk);
			width = newWidth;
			height = newHeight;
		};

		auto dilateTransparentColor = [](Uchar* pixels, Int width, Int height)
		{
			const Size w = static_cast<Size>(width);
			const Size h = static_cast<Size>(height);
			const Size rowPitch = w * 4;

			auto Texel = [&](Size x, Size y) -> Uchar*
			{
				return pixels + y * rowPitch + x * 4;
			};

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

			while (changed && pass < 64)
			{
				changed = false;
				pass++;

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

		auto compressBC7 = [device, cmdQueue, &processor](const DirectX::Image& image, DynamicArray<Uchar>& outputBlocks)
		{
			Uint32 width = static_cast<Uint32>(image.width);
			Uint32 height = static_cast<Uint32>(image.height);

			Uint32 blockCountX = (width + 3) / 4;
			Uint32 blockCountY = (height + 3) / 4;
			Uint32 blockCount = blockCountX * blockCountY;

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

			void* mappedInput = nullptr;
			D3D12_RANGE noRead{ 0, 0 };
			hr = inputBuffer->Map(0, &noRead, &mappedInput);
			SC_HR_CHECK(hr, "入力バッファのMapに失敗しました");
			memcpy(mappedInput, packedPixels.data(), packedPixels.size() * sizeof(Uint32));
			inputBuffer->Unmap(0, nullptr);

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

			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAllocator.GetAddressOf()));
			SC_HR_CHECK(hr, "コマンドアロケーターの生成に失敗しました");

			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
			hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(cmdList.GetAddressOf()));
			SC_HR_CHECK(hr, "コマンドリストの生成に失敗しました");

			cmdList->SetComputeRootSignature(processor.BC7().GetRootSignature());
			cmdList->SetPipelineState(processor.BC7().GetPipelineState());

			Uint32 constants[4] = { width, height, blockCountX, blockCountY };
			cmdList->SetComputeRoot32BitConstants(0, 4, constants, 0);
			cmdList->SetComputeRootShaderResourceView(1, inputBuffer->GetGPUVirtualAddress());
			cmdList->SetComputeRootUnorderedAccessView(2, outputBuffer->GetGPUVirtualAddress());

			cmdList->Dispatch((blockCountX + 7) / 8, (blockCountY + 7) / 8, 1);

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
				auto queueLock = cmdQueue->AcquireLock();
				cmdQueue->GetCommandQueue()->ExecuteCommandLists(1, lists);
				hr = cmdQueue->GetCommandQueue()->Signal(fence.Get(), 1);
				SC_HR_CHECK(hr, "コマンドキューのSignalに失敗しました");
			}

			if (fence->GetCompletedValue() < 1)
			{
				HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				fence->SetEventOnCompletion(1, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}

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
			if (texture.width_ <= 0 || texture.height_ <= 0 || texture.cacheData_.empty())
			{
				continue;
			}

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

			DynamicArray<Uchar> rgba(pixelCount * 4);

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

			Int textureWidth = texture.width_;
			Int textureHeight = texture.height_;
			if (textureWidth > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || textureHeight > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
			{
				SC_LOG_WARNING("テクスチャ {} が D3D12 の最大寸法(16384)を超えています({}x{})。上限内に縮小して読み込みます。", texture.name_, textureWidth, textureHeight);
				downsampleRgbaToFit(rgba, textureWidth, textureHeight, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
			}

			dilateTransparentColor(rgba.data(), textureWidth, textureHeight);

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

			texture.width_ = textureWidth;
			texture.height_ = textureHeight;
			texture.mipCount_ = static_cast<Int>(mipChain.GetMetadata().mipLevels);
			texture.cacheData_.clear();

			for (Size mip = 0; mip < mipChain.GetImageCount(); mip++)
			{
				DynamicArray<Uchar> mipBlocks;
				compressBC7(*mipChain.GetImage(mip, 0, 0), mipBlocks);
				texture.cacheData_.insert(texture.cacheData_.end(), mipBlocks.begin(), mipBlocks.end());
			}
		}
	}
}