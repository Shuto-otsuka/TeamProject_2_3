#include <GraphicsEngine/Texture/TextureUnit.h>
#include <GraphicsEngine/Texture/Image.h>
#include <GraphicsEngine/Texture/Texture.h>
#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/TextureStreaming.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Font/Font.h>
#include <GraphicsEngine/Font/FontResource.h>
#include <GraphicsEngine/Movie/Video.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <FoundationEngine/World/ECS/Component/Selected.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	TextureUnit::TextureUnit()
	{
		slots_.resize(unitCapacity_);
	}

	TextureUnit::~TextureUnit() = default;

	void TextureUnit::Gather(const RenderScene& scene)
	{
		unitUpdates_.clear();
		quadUpdates_.clear();
		drawn_.clear();
		World& world = *scene.world_;
		GraphicsResourceSystem& resources = *scene.resource_;
		TextureResource& textureResource = *resources.textureResource_;
		TextureStreaming& textureStreaming = *resources.textureStreaming_;
		FontResource& fontResource = *resources.fontResource_;
		MovieResource& movieResource = *resources.movieResource_;

		for (Uint32 index : selected_)
		{
			selectedFlags_[index] = 0;
		}
		selected_.clear();
		Query<Read<Selected>>(world).ForEach([&](EntityID entityID, const Selected&)
			{
				if (entityID.index_ >= capacity_)
				{
					return;
				}
				if (selectedFlags_.size() <= entityID.index_)
				{
					selectedFlags_.resize(static_cast<Size>(entityID.index_) + 1, 0);
				}
				selectedFlags_[entityID.index_] = 1;
				selected_.push_back(entityID.index_);
			});

		for (Slot& slot : slots_)
		{
			slot.seen_ = false;
		}
		ComponentID boundsComponentID = ComponentRegistry::GetComponentID<Bounds>();

		Query<Read<Active>, Read<Image>> imageQuery(world);
		imageQuery.ForEach([&](EntityID entityID, const Active& active, const Image& image)
			{
				if (!active.active_ || entityID.index_ >= capacity_)
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				Handle<Texture> textureHandle = textureResource.GetHandle(image.textureID_);
				if (!actor || textureHandle.empty())
				{
					return;
				}
				Texture* texture = textureStreaming.Resolve(textureHandle, streamingFrame_);
				if (!texture || !texture->Resource())
				{
					return;
				}

				D3D12_RESOURCE_DESC desc = texture->Resource()->GetDesc();
				Vector2 textureDimension = Vector2(static_cast<Float>(desc.Width), static_cast<Float>(desc.Height));
				Vector2 textureSize = image.textureSize_;
				if (textureSize.x == 0.0f && textureSize.y == 0.0f)
				{
					textureSize = textureDimension;
				}
				Vector2 pivotNormalized = (textureSize.x > 0.0f && textureSize.y > 0.0f) ? Vector2(image.pivot_.x / textureSize.x, image.pivot_.y / textureSize.y) : Vector2(0.5f, 0.5f);

				if (boundsComponentID)
				{
					void* boundsRaw = actor.GetComponent(boundsComponentID);
					if (boundsRaw)
					{
						Bounds* bounds = static_cast<Bounds*>(boundsRaw);
						bounds->center_ = Vector3((0.5f - pivotNormalized.x) * textureSize.x, (pivotNormalized.y - 0.5f) * textureSize.y, 0.0f);
						bounds->extent_ = Vector3(textureSize.x * 0.5f, textureSize.y * 0.5f, 1.0f);
					}
				}

				Uint32 slotIndex = static_cast<Uint32>(TextureKind::Texture) * capacity_ + entityID.index_;
				Slot& slot = slots_[slotIndex];
				slot.seen_ = true;
				Bool billboard = image.viewType_ == Image::ViewType::Billboard;

				Content content{};
				content.display_ = billboard ? TextureDisplay::Billboard : TextureDisplay::Sprite;
				content.textureSize_ = textureSize;
				content.textureDimension_ = textureDimension;
				content.texturePosition_ = image.texturePosition_;
				content.pivot_ = image.pivot_;
				if (!slot.sent_ || std::memcmp(&content, &slot.content_, sizeof(Content)) != 0)
				{
					slot.content_ = content;
					quads_.clear();
					TextureQuadStructuredBuffer quad{};
					quad.rectMin_ = billboard ? Vector2(-pivotNormalized.x, pivotNormalized.y) : Vector2(-image.pivot_.x, -image.pivot_.y);
					quad.rectSize_ = billboard ? Vector2(1.0f, -1.0f) : textureSize;
					quad.uvMin_ = Vector2(image.texturePosition_.x / textureDimension.x, image.texturePosition_.y / textureDimension.y);
					quad.uvMax_ = Vector2((image.texturePosition_.x + textureSize.x) / textureDimension.x, (image.texturePosition_.y + textureSize.y) / textureDimension.y);
					quads_.push_back(quad);
					WriteQuads(slotIndex);
				}

				TextureUnitStructuredBuffer unit{};
				unit.world_ = actor.WorldMatrix();
				unit.color_ = image.color_;
				unit.scrollDirection_ = image.scrollDirection_;
				unit.scrollSpeed_ = image.scrollSpeed_;
				unit.textureIndex_ = texture->ShaderResourceViewIndex();
				unit.kind_ = TextureKind::Texture;
				unit.display_ = content.display_;
				unit.flags_ = (selectedFlags_.size() > entityID.index_ && selectedFlags_[entityID.index_] != 0 ? 2u : 0u) | (billboard && image.faceCamera_ ? 4u : 0u) | (image.motionType_ == Image::MotionType::Dynamic ? 8u : 0u);
				unit.quadOffset_ = slot.quadOffset_;
				unit.quadCount_ = slot.quadCount_;
				slot.unit_ = unit;
				drawn_.push_back(Drawn{ billboard ? 2u : 1u, billboard ? 0 : image.order_, slotIndex });
			});

		Query<Read<Active>, Read<Text>> textQuery(world);
		textQuery.ForEach([&](EntityID entityID, const Active& active, const Text& text)
			{
				if (!active.active_ || entityID.index_ >= capacity_ || text.text_.str().empty())
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				Font* font = fontResource.Resolve(fontResource.GetHandle(text.fontID_));
				if (!actor || !font || !font->HasAtlasTexture())
				{
					return;
				}
				msdfgen::BitmapConstRef<msdf_atlas::byte, 4> bitmap = font->GetAtlasBitmap();
				if (bitmap.width <= 0 || bitmap.height <= 0)
				{
					return;
				}

				Uint32 slotIndex = static_cast<Uint32>(TextureKind::Font) * capacity_ + entityID.index_;
				Slot& slot = slots_[slotIndex];
				slot.seen_ = true;
				Bool billboard = text.viewType_ == Text::ViewType::Billboard;

				Content content{};
				content.display_ = billboard ? TextureDisplay::Billboard : TextureDisplay::Sprite;
				content.textureIndex_ = font->GetAtlasTextureIndex();
				content.textureDimension_ = Vector2(static_cast<Float>(bitmap.width), static_cast<Float>(bitmap.height));
				content.pivot_ = text.pivot_;
				content.fontSize_ = text.fontSize_;
				content.letterSpacing_ = text.letterSpacing_;
				content.lineSpacing_ = text.lineSpacing_;
				content.shadow_ = text.shadowEnable_ ? 1u : 0u;
				content.shadowOffset_ = text.shadowOffset_;
				if (!slot.sent_ || std::memcmp(&content, &slot.content_, sizeof(Content)) != 0 || slot.text_ != text.text_)
				{
					slot.content_ = content;
					slot.text_ = text.text_;
					quads_.clear();
					font->Shape(text.text_.str());

					Float scaleFactor = text.fontSize_ / font->GetFontSize();
					Float lineAdvance = font->GetLineHeight() * scaleFactor * text.lineSpacing_;
					Float inverseWidth = 1.0f / static_cast<Float>(bitmap.width);
					Float inverseHeight = 1.0f / static_cast<Float>(bitmap.height);
					Float unitScale = billboard ? 0.01f : 1.0f;
					Float textBoxMinX = FLT_MAX;
					Float textBoxMaxX = -FLT_MAX;
					Float textBoxMinY = FLT_MAX;
					Float textBoxMaxY = -FLT_MAX;
					Float lineTop = 0.0f;
					std::istringstream stream(text.text_.str());
					std::string line;
					while (std::getline(stream, line))
					{
						Float baseline = lineTop + font->GetAscender() * scaleFactor;
						Vector2 pen = { 0.0f, 0.0f };
						for (const ShapedGlyph& shaped : font->Shape(line))
						{
							const GlyphAtlasEntry* entry = font->FindGlyph(shaped.glyphIndex_);
							if (entry && !entry->isWhitespace_)
							{
								Float glyphLeft = pen.x + shaped.offset_.x * scaleFactor + entry->planeLeft_ * scaleFactor;
								Float glyphTop = baseline - shaped.offset_.y * scaleFactor - entry->planeTop_ * scaleFactor;
								Float glyphWidth = (entry->planeRight_ - entry->planeLeft_) * scaleFactor;
								Float glyphHeight = (entry->planeTop_ - entry->planeBottom_) * scaleFactor;

								textBoxMinX = Min(textBoxMinX, glyphLeft - text.pivot_.x);
								textBoxMaxX = Max(textBoxMaxX, glyphLeft - text.pivot_.x + glyphWidth);
								textBoxMinY = Min(textBoxMinY, text.pivot_.y - glyphTop - glyphHeight);
								textBoxMaxY = Max(textBoxMaxY, text.pivot_.y - glyphTop);

								TextureQuadStructuredBuffer quad{};
								quad.rectMin_ = billboard ? Vector2((glyphLeft - text.pivot_.x) * unitScale, (text.pivot_.y - glyphTop) * unitScale) : Vector2(glyphLeft - text.pivot_.x, glyphTop - text.pivot_.y);
								quad.rectSize_ = billboard ? Vector2(glyphWidth * unitScale, -glyphHeight * unitScale) : Vector2(glyphWidth, glyphHeight);
								quad.uvMin_ = Vector2(entry->atlasLeft_ * inverseWidth, entry->atlasTop_ * inverseHeight);
								quad.uvMax_ = Vector2(entry->atlasRight_ * inverseWidth, entry->atlasBottom_ * inverseHeight);
								if (text.shadowEnable_)
								{
									TextureQuadStructuredBuffer shadow = quad;
									shadow.rectMin_ += billboard ? Vector2(text.shadowOffset_.x * unitScale, -text.shadowOffset_.y * unitScale) : text.shadowOffset_;
									shadow.flags_ = 1;
									quads_.push_back(shadow);
								}
								quads_.push_back(quad);
							}
							pen.x += shaped.advance_.x * scaleFactor + text.letterSpacing_;
							pen.y += shaped.advance_.y * scaleFactor;
						}
						lineTop += lineAdvance;
					}
					WriteQuads(slotIndex);

					if (!billboard && boundsComponentID && textBoxMinX <= textBoxMaxX)
					{
						void* boundsRaw = actor.GetComponent(boundsComponentID);
						if (boundsRaw)
						{
							Bounds* bounds = static_cast<Bounds*>(boundsRaw);
							bounds->center_ = Vector3((textBoxMinX + textBoxMaxX) * 0.5f, (textBoxMinY + textBoxMaxY) * 0.5f, 0.0f);
							bounds->extent_ = Vector3((textBoxMaxX - textBoxMinX) * 0.5f, (textBoxMaxY - textBoxMinY) * 0.5f, 1.0f);
						}
					}
				}

				TextureUnitStructuredBuffer unit{};
				unit.world_ = actor.WorldMatrix();
				unit.color_ = text.color_;
				unit.outlineColor_ = text.outlineColor_;
				unit.glowColor_ = text.glowColor_;
				unit.shadowColor_ = text.shadowColor_;
				unit.unitRange_ = Vector2(font->GetPixelRange() / static_cast<Float>(bitmap.width), font->GetPixelRange() / static_cast<Float>(bitmap.height));
				unit.outlineWidth_ = text.outlineWidth_;
				unit.glowPower_ = text.glowPower_;
				unit.textureIndex_ = font->GetAtlasTextureIndex();
				unit.kind_ = TextureKind::Font;
				unit.display_ = content.display_;
				unit.flags_ = (selectedFlags_.size() > entityID.index_ && selectedFlags_[entityID.index_] != 0 ? 2u : 0u) | (billboard && text.faceCamera_ ? 4u : 0u);
				unit.quadOffset_ = slot.quadOffset_;
				unit.quadCount_ = slot.quadCount_;
				slot.unit_ = unit;
				drawn_.push_back(Drawn{ billboard ? 2u : 1u, billboard ? 0 : text.order_, slotIndex });
			});

		Query<Read<Active>, Read<Movie>> movieQuery(world);
		movieQuery.ForEach([&](EntityID entityID, const Active& active, const Movie& movie)
			{
				if (!active.active_ || entityID.index_ >= capacity_ || movie.movieID_ == 0)
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				Video* video = movieResource.Resolve(movieResource.GetHandle(movie.movieID_));
				if (!actor || !video || !video->HasTexture() || video->GetWidth() <= 0 || video->GetHeight() <= 0)
				{
					return;
				}

				Vector2 textureDimension = Vector2(static_cast<Float>(video->GetWidth()), static_cast<Float>(video->GetHeight()));
				Vector2 size = movie.size_;
				if (size.x <= 0.0f && size.y <= 0.0f)
				{
					size = textureDimension;
				}
				Vector2 pivotNormalized = (size.x > 0.0f && size.y > 0.0f) ? Vector2(movie.pivot_.x / size.x, movie.pivot_.y / size.y) : Vector2(0.5f, 0.5f);
				TextureDisplay display = movie.displayMode_ == Movie::DisplayMode::Fullscreen ? TextureDisplay::Fullscreen : movie.displayMode_ == Movie::DisplayMode::Billboard ? TextureDisplay::Billboard : TextureDisplay::Sprite;

				if (display == TextureDisplay::Sprite && boundsComponentID)
				{
					void* boundsRaw = actor.GetComponent(boundsComponentID);
					if (boundsRaw)
					{
						Bounds* bounds = static_cast<Bounds*>(boundsRaw);
						bounds->center_ = Vector3((0.5f - pivotNormalized.x) * size.x, (pivotNormalized.y - 0.5f) * size.y, 0.0f);
						bounds->extent_ = Vector3(size.x * 0.5f, size.y * 0.5f, 1.0f);
					}
				}

				Uint32 slotIndex = static_cast<Uint32>(TextureKind::Movie) * capacity_ + entityID.index_;
				Slot& slot = slots_[slotIndex];
				slot.seen_ = true;

				Content content{};
				content.display_ = display;
				content.textureSize_ = size;
				content.pivot_ = movie.pivot_;
				if (!slot.sent_ || std::memcmp(&content, &slot.content_, sizeof(Content)) != 0)
				{
					slot.content_ = content;
					quads_.clear();
					TextureQuadStructuredBuffer quad{};
					quad.rectMin_ = display == TextureDisplay::Billboard ? Vector2(-pivotNormalized.x, pivotNormalized.y) : Vector2(-movie.pivot_.x, -movie.pivot_.y);
					quad.rectSize_ = display == TextureDisplay::Billboard ? Vector2(1.0f, -1.0f) : size;
					quads_.push_back(quad);
					WriteQuads(slotIndex);
				}

				TextureUnitStructuredBuffer unit{};
				unit.world_ = actor.WorldMatrix();
				unit.color_ = movie.color_;
				unit.textureIndex_ = video->GetTextureIndex();
				unit.kind_ = TextureKind::Movie;
				unit.display_ = display;
				unit.flags_ = (selectedFlags_.size() > entityID.index_ && selectedFlags_[entityID.index_] != 0 ? 2u : 0u) | (display == TextureDisplay::Billboard && movie.faceCamera_ ? 4u : 0u);
				unit.quadOffset_ = slot.quadOffset_;
				unit.quadCount_ = slot.quadCount_;
				slot.unit_ = unit;
				drawn_.push_back(Drawn{ display == TextureDisplay::Fullscreen ? 0u : display == TextureDisplay::Billboard ? 2u : 1u, display == TextureDisplay::Sprite ? movie.order_ : 0, slotIndex });
			});

		std::ranges::stable_sort(drawn_, [](const Drawn& a, const Drawn& b) { return a.group_ != b.group_ ? a.group_ < b.group_ : a.order_ < b.order_; });

		constants_ = TextureConstantBuffer{};
		Uint32 drawBase = 0;
		for (const Drawn& drawn : drawn_)
		{
			Slot& slot = slots_[drawn.slot_];
			if (drawn.group_ == 2 && constants_.billboardCount_ == 0)
			{
				constants_.billboardBase_ = drawBase;
			}
			slot.unit_.drawBase_ = drawBase;
			slot.unit_.flags_ |= 1u;
			drawBase += slot.quadCount_;
			if (drawn.group_ == 2)
			{
				constants_.billboardCount_ += slot.quadCount_;
			}
			else
			{
				constants_.spriteCount_ += slot.quadCount_;
			}
		}
		if (constants_.billboardCount_ == 0)
		{
			constants_.billboardBase_ = constants_.spriteCount_;
		}
		constants_.quadCount_ = quadEnd_;

		for (Uint32 slotIndex = 0;slotIndex < unitCapacity_;++slotIndex)
		{
			Slot& slot = slots_[slotIndex];
			if (!slot.seen_)
			{
				if (!slot.sent_)
				{
					continue;
				}
				if (slot.quadCapacity_ > 0)
				{
					freeRanges_.push_back({ slot.quadOffset_, slot.quadCapacity_ });
				}
				slot = Slot{};
				TextureUnitUpdate update{};
				update.index_ = slotIndex;
				unitUpdates_.push_back(update);
				continue;
			}

			if (slot.sent_ && std::memcmp(&slot.unit_, &slot.sentUnit_, sizeof(TextureUnitStructuredBuffer)) == 0)
			{
				continue;
			}
			TextureUnitUpdate update{};
			update.index_ = slotIndex;
			update.data_ = slot.unit_;
			unitUpdates_.push_back(update);
			slot.sentUnit_ = slot.unit_;
			slot.sent_ = true;
		}

		textureStreaming.EvictBudget(streamingFrame_);
		++streamingFrame_;
	}

	void TextureUnit::WriteQuads(Uint32 slotIndex)
	{
		Slot& slot = slots_[slotIndex];
		Uint32 count = static_cast<Uint32>(quads_.size());
		if (count > slot.quadCapacity_)
		{
			if (slot.quadCapacity_ > 0)
			{
				freeRanges_.push_back({ slot.quadOffset_, slot.quadCapacity_ });
			}
			slot.quadOffset_ = 0;
			slot.quadCapacity_ = 0;
			auto found = std::ranges::find_if(freeRanges_, [count](const std::pair<Uint32, Uint32>& range) { return range.second >= count; });
			if (found != freeRanges_.end())
			{
				slot.quadOffset_ = found->first;
				slot.quadCapacity_ = count;
				found->first += count;
				found->second -= count;
				if (found->second == 0)
				{
					freeRanges_.erase(found);
				}
			}
			else if (quadEnd_ + count <= quadCapacity_)
			{
				slot.quadOffset_ = quadEnd_;
				slot.quadCapacity_ = count;
				quadEnd_ += count;
			}
		}

		slot.quadCount_ = Min(count, slot.quadCapacity_);
		for (Uint32 quadIndex = 0;quadIndex < slot.quadCount_;++quadIndex)
		{
			TextureQuadUpdate update{};
			update.index_ = slot.quadOffset_ + quadIndex;
			update.data_ = quads_[quadIndex];
			update.data_.unit_ = slotIndex;
			quadUpdates_.push_back(update);
		}
	}

	std::span<const TextureUnitUpdate> TextureUnit::UnitUpdates()const
	{
		return unitUpdates_;
	}

	std::span<const TextureQuadUpdate> TextureUnit::QuadUpdates()const
	{
		return quadUpdates_;
	}

	const TextureConstantBuffer& TextureUnit::Constants()const
	{
		return constants_;
	}
}
