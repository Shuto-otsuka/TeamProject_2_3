#include <External/NsightAftermath/Include/GFSDK_Aftermath.h>
#include <GraphicsEngine/Font/Font.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>

namespace SeedCore
{
	Bool Font::Update()
	{
		if (pendingGlyphs_.empty())
		{
			return false;
		}

		const Size startIndex = glyphGeometries_.size();
		for (Uint32 glyphIndex : pendingGlyphs_)
		{
			msdf_atlas::GlyphGeometry geometry;
			if (!geometry.load(msdfFont_, geometryScale_ / static_cast<Double>(ftFace_->units_per_EM), msdfgen::GlyphIndex(glyphIndex)))
			{
				continue;
			}

			geometry.edgeColoring(&msdfgen::edgeColoringInkTrap, 3.0, 0);
			geometry.wrapBox(1.0, pixelRange_, 1.0);

			glyphGeometries_.emplace_back(std::move(geometry));
		}
		pendingGlyphs_.clear();

		const Size addedCount = glyphGeometries_.size() - startIndex;
		if (addedCount == 0)
		{
			return false;
		}

		Size updateStartIndex = startIndex;
		const Atlas::ChangeFlags changeFlags = atlas_.add(glyphGeometries_.data() + startIndex, static_cast<Int>(addedCount), true);
		if (changeFlags & Atlas::REARRANGED)
		{
			atlas_ = Atlas(256);
			atlas_.atlasGenerator().setThreadCount(static_cast<Int>(Max<Uint>(1u, std::thread::hardware_concurrency() / 2)));
			atlas_.add(glyphGeometries_.data(), static_cast<Int>(glyphGeometries_.size()), true);
			updateStartIndex = 0;
		}

		for (Size index = updateStartIndex; index < glyphGeometries_.size(); ++index)
		{
			const msdf_atlas::GlyphGeometry& geometry = glyphGeometries_[index];

			GlyphAtlasEntry entry = {};
			entry.isWhitespace_ = geometry.isWhitespace();

			Double l = 0.0, b = 0.0, r = 0.0, t = 0.0;
			geometry.getQuadAtlasBounds(l, b, r, t);
			entry.atlasLeft_ = static_cast<Float>(l);
			entry.atlasBottom_ = static_cast<Float>(b);
			entry.atlasRight_ = static_cast<Float>(r);
			entry.atlasTop_ = static_cast<Float>(t);

			geometry.getQuadPlaneBounds(l, b, r, t);
			entry.planeLeft_ = static_cast<Float>(l);
			entry.planeBottom_ = static_cast<Float>(b);
			entry.planeRight_ = static_cast<Float>(r);
			entry.planeTop_ = static_cast<Float>(t);

			glyphs_[static_cast<Uint32>(geometry.getIndex())] = entry;
		}

		atlasDirty_ = true;

		return true;
	}

	DynamicArray<ShapedGlyph> Font::Shape(const std::string& utf8Text)
	{
		DynamicArray<ShapedGlyph> result;

		if (!hbFont_ || utf8Text.empty())
		{
			return result;
		}

		hb_buffer_t* buffer = hb_buffer_create();
		hb_buffer_add_utf8(buffer, utf8Text.c_str(), static_cast<Int>(utf8Text.size()), 0, static_cast<Int>(utf8Text.size()));
		hb_buffer_guess_segment_properties(buffer);

		hb_shape(hbFont_, buffer, nullptr, 0);

		Uint glyphCount = 0;
		hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &glyphCount);
		hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &glyphCount);

		constexpr Float toPixel = 1.0f / 64.0f;

		result.reserve(glyphCount);
		for (Uint index = 0; index < glyphCount; ++index)
		{
			ShapedGlyph glyph = {};
			glyph.glyphIndex_ = infos[index].codepoint;
			glyph.cluster_ = infos[index].cluster;
			glyph.advance_.x = static_cast<Float>(positions[index].x_advance) * toPixel;
			glyph.advance_.y = static_cast<Float>(positions[index].y_advance) * toPixel;
			glyph.offset_.x = static_cast<Float>(positions[index].x_offset) * toPixel;
			glyph.offset_.y = static_cast<Float>(positions[index].y_offset) * toPixel;
			result.emplace_back(glyph);

			RequestGlyph(glyph.glyphIndex_);
		}

		hb_buffer_destroy(buffer);

		return result;
	}

	const GlyphAtlasEntry* Font::FindGlyph(Uint32 glyphIndex)const
	{
		auto it = glyphs_.find(glyphIndex);
		return it != glyphs_.end() ? &it->second : nullptr;
	}

	msdfgen::BitmapConstRef<msdf_atlas::byte, 4> Font::GetAtlasBitmap()const
	{
		return atlas_.atlasGenerator().atlasStorage();
	}

	Bool Font::AtlasDirty()const
	{
		return atlasDirty_;
	}

	void Font::ClearAtlasDirty()
	{
		atlasDirty_ = false;
	}

	void Font::UploadAtlas(ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* bindlessHeap)
	{
		if (!atlasDirty_)
		{
			return;
		}

		msdfgen::BitmapConstRef<msdf_atlas::byte, 4> bitmap = GetAtlasBitmap();
		if (bitmap.width <= 0 || bitmap.height <= 0)
		{
			return;
		}

		/// [EN] Frames still in flight may sample the current atlas, so every update writes a new texture under a new index and hands the old pair to the heap's deferred release.
		/// [JP] 実行中のフレームが今のアトラスを読んでいる可能性があるため、更新のたびに新しい番号で新しいテクスチャへ書き、古い組はヒープの遅延解放へ渡す。
		if (atlasTextureIndexAllocated_)
		{
			bindlessHeap->Release(atlasTexture_, atlasTextureIndex_);
			atlasTexture_.Reset();
			atlasTextureIndexAllocated_ = false;
		}

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = static_cast<Uint64>(bitmap.width);
		resourceDesc.Height = static_cast<Uint>(bitmap.height);
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		/// [EN] The texture starts in the COMMON state, so the copy queue writes it and the frame reads it without explicit transitions.
		/// [JP] テクスチャは COMMON 状態で始まるので、コピーキューが書き、フレームが読むのに明示的な遷移は要らない。
		if (FAILED(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(atlasTexture_.ReleaseAndGetAddressOf()))))
		{
			return;
		}
#if SC_DEBUG
		atlasTexture_->SetName(L"Font_Atlas");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(atlasTexture_.Get());
#endif

		atlasTextureWidth_ = bitmap.width;
		atlasTextureHeight_ = bitmap.height;
		atlasTextureIndex_ = bindlessHeap->AllocateIndex();
		atlasTextureIndexAllocated_ = true;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(atlasTexture_.Get(), &srvDesc, bindlessHeap->CPUHandle(atlasTextureIndex_));

		lane.GetStream().Upload(device, lane.GetCommandList(), atlasTexture_.Get(), 0, std::as_bytes(std::span(bitmap.pixels, static_cast<Size>(bitmap.width) * bitmap.height * 4)));

		ClearAtlasDirty();
	}

	Uint Font::GetAtlasTextureIndex()const
	{
		return atlasTextureIndex_;
	}

	Bool Font::HasAtlasTexture()const
	{
		return atlasTexture_ != nullptr;
	}

	Float Font::GetPixelRange()const
	{
		return static_cast<Float>(pixelRange_);
	}

	Float Font::GetFontSize()const
	{
		return fontSize_;
	}

	Float Font::GetLineHeight()const 
	{
		return lineHeight_;
	}

	Float Font::GetAscender()const
	{
		return ascender_;
	}

	Float Font::GetDescender()const
	{
		return descender_;
	}

	void Font::RequestGlyph(Uint32 glyphIndex)
	{
		if (glyphs_.contains(glyphIndex))
		{
			return;
		}

		if (std::ranges::find(pendingGlyphs_, glyphIndex) == pendingGlyphs_.end())
		{
			pendingGlyphs_.emplace_back(glyphIndex);
		}
	}
}
