#include <GraphicsEngine/Shape/Primitive/PrimitiveMesh.h>

namespace SeedCore
{
	void PrimitiveMesh::Build()
	{
		DynamicArray<Uint32> triangles;
		DynamicArray<Meshlet::TriangleRange> triangleRanges;
		DynamicArray<ShapeKind> rangeKinds;

		DynamicArray<PrimitiveVertex> shapeVertices;
		DynamicArray<Uint32> shapeTriangles;

		auto appendShape = [&](ShapeKind kind)
			{
				Uint32 vertexBase = static_cast<Uint32>(vertices_.size());
				vertices_.insert(vertices_.end(), shapeVertices.begin(), shapeVertices.end());

				Meshlet::TriangleRange triangleRange{};
				triangleRange.triangleOffset_ = static_cast<Uint32>(triangles.size() / 3);
				triangleRange.triangleCount_ = static_cast<Uint32>(shapeTriangles.size() / 3);
				triangleRanges.push_back(triangleRange);
				rangeKinds.push_back(kind);

				for (Uint32 vertexIndex : shapeTriangles)
				{
					triangles.push_back(vertexBase + vertexIndex);
				}

				shapeVertices.clear();
				shapeTriangles.clear();
			};

		CreateBoxShape(shapeVertices, shapeTriangles);
		appendShape(ShapeKind::Box);

		CreateSphereShape(shapeVertices, shapeTriangles);
		appendShape(ShapeKind::Sphere);

		CreateCylinderShape(shapeVertices, shapeTriangles);
		appendShape(ShapeKind::Cylinder);

		CreateConeShape(shapeVertices, shapeTriangles);
		appendShape(ShapeKind::Cone);

		Meshlet meshlet(triangles, triangleRanges);
		meshlets_.assign(meshlet.Meshlets().begin(), meshlet.Meshlets().end());

		vertexIndices_.assign(meshlet.VertexIndices().begin(), meshlet.VertexIndices().end());
	
		primitiveIndices_.assign(meshlet.PrimitiveIndices().begin(), meshlet.PrimitiveIndices().end());

		meshletRanges_.assign(static_cast<Size>(ShapeKind::Arrow) + 1, Meshlet::MeshletRange{});
		for (Size rangeIndex = 0;rangeIndex < rangeKinds.size();++rangeIndex)
		{
			meshletRanges_[static_cast<Size>(rangeKinds[rangeIndex])] = meshlet.MeshletRanges()[rangeIndex];
		}

		/// [EN] One culling bound per meshlet in unit-mesh space: the sphere is centered on the average vertex position, and the normal cone is built from the vertex normals, which are exact for the unit meshes.
		/// [JP] 単位メッシュの空間で、メッシュレットごとにカリング用の範囲を作る。球の中心は頂点の位置の平均。法線コーンは、単位メッシュでは正確な頂点の法線から作る。
		meshletBounds_.resize(meshlets_.size());
		for (Size meshletIndex = 0; meshletIndex < meshlets_.size(); ++meshletIndex)
		{
			const MeshletDesc& meshletDesc = meshlets_[meshletIndex];
			MeshletBound& bound = meshletBounds_[meshletIndex];

			Vector3 center = Vector3::Zero;
			Vector3 axis = Vector3::Zero;
			for (Uint32 vertexIndex = 0; vertexIndex < meshletDesc.vertexCount_; ++vertexIndex)
			{
				const PrimitiveVertex& vertex = vertices_[vertexIndices_[meshletDesc.vertexOffset_ + vertexIndex]];
				center += vertex.position_;
				axis += vertex.normal_;
			}
			center /= static_cast<Float>(meshletDesc.vertexCount_);

			Float radius = 0.0f;
			for (Uint32 vertexIndex = 0; vertexIndex < meshletDesc.vertexCount_; ++vertexIndex)
			{
				radius = Max(radius, (vertices_[vertexIndices_[meshletDesc.vertexOffset_ + vertexIndex]].position_ - center).Length());
			}
			bound.center_ = center;
			bound.radius_ = radius;

			/// [EN] The cone cutoff is the smallest cosine between any vertex normal and the average axis; when the normals cancel out (a closed shape in one meshlet) there is no useful cone, and 0 turns the cone test off.
			/// [JP] コーンの境界は、各頂点の法線と平均の軸のなす角の cos の最小値。法線が打ち消し合う（閉じた形が 1 つのメッシュレットに入っている）ときは使えるコーンが無いので、0 にしてコーンの判定を使わない。
			Float axisLength = axis.Length();
			if (axisLength > 1e-6f)
			{
				axis /= axisLength;
				Float minDot = 1.0f;
				for (Uint32 vertexIndex = 0; vertexIndex < meshletDesc.vertexCount_; ++vertexIndex)
				{
					minDot = Min(minDot, vertices_[vertexIndices_[meshletDesc.vertexOffset_ + vertexIndex]].normal_.Dot(axis));
				}
				bound.coneAxis_ = axis;
				bound.coneCutoff_ = minDot;
			}
			else
			{
				bound.coneAxis_ = Vector3(0.0f, 0.0f, 1.0f);
				bound.coneCutoff_ = 0.0f;
			}
		}

		for (const Meshlet::MeshletRange& meshletRange : meshletRanges_)
		{
			SC_ASSERT(meshletRange.meshletCount_ <= maxMeshletsPerShape_);
		}
	}

	std::span<const PrimitiveVertex> PrimitiveMesh::Vertices()const
	{
		return vertices_;
	}

	std::span<const MeshletDesc> PrimitiveMesh::Meshlets()const
	{
		return meshlets_;
	}

	std::span<const MeshletBound> PrimitiveMesh::MeshletBounds()const
	{
		return meshletBounds_;
	}

	std::span<const Meshlet::MeshletRange> PrimitiveMesh::MeshletRanges()const
	{
		return meshletRanges_;
	}

	std::span<const Uint32> PrimitiveMesh::VertexIndices()const
	{
		return vertexIndices_;
	}

	std::span<const Uint8> PrimitiveMesh::PrimitiveIndices()const
	{
		return primitiveIndices_;
	}

	void PrimitiveMesh::CreateBoxShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles)
	{
		const Vector3 normals[6] =
		{
			{ 1.0f, 0.0f, 0.0f },{ -1.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f },{  0.0f,-1.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f },{  0.0f, 0.0f,-1.0f }
		};
		const Vector3 rights[6] =
		{
			{ 0.0f, 0.0f,-1.0f },{  0.0f, 0.0f, 1.0f },
			{ 1.0f, 0.0f, 0.0f },{  1.0f, 0.0f, 0.0f },
			{ 1.0f, 0.0f, 0.0f },{ -1.0f, 0.0f, 0.0f }
		};
		const Vector3 ups[6] =
		{
			{ 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f,-1.0f },{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f }
		};
		const Vector2 corners[4] =
		{
			{ -1.0f,-1.0f },{  1.0f,-1.0f },
			{  1.0f, 1.0f },{ -1.0f, 1.0f }
		};

		for (Uint32 faceIndex = 0;faceIndex < 6;++faceIndex)
		{
			Uint32 baseIndex = static_cast<Uint32>(vertices.size());
			for (const Vector2& corner : corners)
			{
				PrimitiveVertex vertex{};
				vertex.position_ = normals[faceIndex] + rights[faceIndex] * corner.x + ups[faceIndex] * corner.y;
				vertex.normal_ = normals[faceIndex];
				vertex.texcoord_ = Vector2(corner.x * 0.5f + 0.5f, 0.5f - corner.y * 0.5f);
				vertex.cap_ = 0.0f;
				vertices.push_back(vertex);
			}
			triangles.insert(triangles.end(), { baseIndex + 2,baseIndex + 1,baseIndex,baseIndex + 3,baseIndex + 2,baseIndex });
		}
	}

	/**
	* [EN]
	* Builds the unit sphere: radius 1, centered on the origin, 16 rings
	* from the top pole (+Y) to the bottom pole and 32 segments around Y.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単位の球を作る。半径 1 で原点が中心。上の極（+Y）から下の極まで
	* 16 段、Y 軸まわりに 32 分割。
	*/
	void PrimitiveMesh::CreateSphereShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles)
	{
		/// [EN] A grid of 17 rows (both poles included) by 33 columns: the seam column is doubled so the texture wraps around exactly once, so each row is 33 vertices apart.
		/// [JP] 17 行（両極を含む）× 33 列の格子。テクスチャがちょうど一周で巻くよう継ぎ目の列は二重にするので、行は 33 頂点ずつ離れる。
		Uint32 baseIndex = static_cast<Uint32>(vertices.size());
		for (Uint32 ringIndex = 0;ringIndex <= 16;++ringIndex)
		{
			Float theta = static_cast<Float>(ringIndex) / 16.0f * Pi<Float>::Value;
			Float sinTheta = Sin(theta);
			Float cosTheta = Cos(theta);

			for (Uint32 segmentIndex = 0;segmentIndex <= 32;++segmentIndex)
			{
				Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

				PrimitiveVertex vertex{};
				vertex.position_ = Vector3(sinTheta * Cos(phi), cosTheta, sinTheta * Sin(phi));
				vertex.normal_ = vertex.position_;
				vertex.texcoord_ = Vector2(static_cast<Float>(segmentIndex) / 32.0f, static_cast<Float>(ringIndex) / 16.0f);
				vertex.cap_ = 0.0f;
				vertices.push_back(vertex);

				if (ringIndex == 16 || segmentIndex == 32)
				{
					continue;
				}

				Uint32 a = baseIndex + ringIndex * 33 + segmentIndex;
				Uint32 b = a + 1;
				Uint32 c = a + 33;
				Uint32 d = c + 1;

				/// [EN] The rows next to the poles collapse to a point on one side, so each of their quads keeps only the triangle that is not degenerate.
				/// [JP] 極の隣の行は片側が1点に縮むので、その四角形は潰れていない方の三角形だけを残す。
				if (ringIndex != 15)
				{
					triangles.insert(triangles.end(), { a,c,d });
				}
				if (ringIndex != 0)
				{
					triangles.insert(triangles.end(), { a,d,b });
				}
			}
		}
	}

	/**
	* [EN]
	* Builds the unit cylinder: radius 1 around Y, from y = -1 to y = 1,
	* with 32 segments around Y and a flat cap at each end.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単位の円柱を作る。Y 軸まわりに半径 1、y = -1 から y = 1 まで。
	* Y 軸まわりに 32 分割し、両端に平らなフタを付ける。
	*/
	void PrimitiveMesh::CreateCylinderShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles)
	{
		/// [EN] Side: a top row and a bottom row of 33 vertices, the seam doubled as on the sphere. The normals point straight out from the axis, so the side shades smoothly.
		/// [JP] 側面。上の行と下の行に 33 頂点ずつ。継ぎ目は球と同じく二重にする。法線は軸から真横に向けるので、側面はなめらかに陰影が付く。
		Uint32 sideBase = static_cast<Uint32>(vertices.size());
		for (Uint32 rowIndex = 0;rowIndex < 2;++rowIndex)
		{
			Float y = rowIndex == 0 ? 1.0f : -1.0f;
			for (Uint32 segmentIndex = 0;segmentIndex <= 32;++segmentIndex)
			{
				Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

				PrimitiveVertex vertex{};
				vertex.position_ = Vector3(Cos(phi), y, Sin(phi));
				vertex.normal_ = Vector3(Cos(phi), 0.0f, Sin(phi));
				vertex.texcoord_ = Vector2(static_cast<Float>(segmentIndex) / 32.0f, static_cast<Float>(rowIndex));
				vertex.cap_ = 0.0f;
				vertices.push_back(vertex);
			}
		}

		/// [EN] Same winding as the sphere's quads, with the top row in place of the upper ring.
		/// [JP] 球の四角形と同じ巻き順。上の段の代わりに上の行を使う。
		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			Uint32 a = sideBase + segmentIndex;
			Uint32 b = a + 1;
			Uint32 c = a + 33;
			Uint32 d = c + 1;
			triangles.insert(triangles.end(), { a,c,d,a,d,b });
		}

		/// [EN] Caps: a center vertex and a ring of 32 with the cap's own flat normal, so the edge stays sharp. The texture is laid flat across each cap, mirrored on the bottom so it reads the right way round from below.
		/// [JP] フタ。中心の頂点と 32 頂点の輪で、法線はフタ自身の平らな向きにするので、縁は角が立ったままになる。テクスチャは各フタに平らに張り、下のフタでは下から見て正しい向きになるよう裏返す。
		for (Uint32 capIndex = 0;capIndex < 2;++capIndex)
		{
			Float y = capIndex == 0 ? 1.0f : -1.0f;

			Uint32 centerIndex = static_cast<Uint32>(vertices.size());
			PrimitiveVertex center{};
			center.position_ = Vector3(0.0f, y, 0.0f);
			center.normal_ = Vector3(0.0f, y, 0.0f);
			center.texcoord_ = Vector2(0.5f, 0.5f);
			center.cap_ = 0.0f;
			vertices.push_back(center);

			for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
			{
				Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

				PrimitiveVertex vertex{};
				vertex.position_ = Vector3(Cos(phi), y, Sin(phi));
				vertex.normal_ = Vector3(0.0f, y, 0.0f);
				vertex.texcoord_ = Vector2(Cos(phi) * 0.5f + 0.5f, 0.5f - Sin(phi) * 0.5f * y);
				vertex.cap_ = 0.0f;
				vertices.push_back(vertex);
			}

			/// [EN] The top cap faces +Y and the bottom cap -Y, so their fans turn in opposite orders.
			/// [JP] 上のフタは +Y、下のフタは -Y を向くので、扇の並び順を逆にする。
			for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
			{
				Uint32 current = centerIndex + 1 + segmentIndex;
				Uint32 next = centerIndex + 1 + (segmentIndex + 1) % 32;
				if (capIndex == 0)
				{
					triangles.insert(triangles.end(), { centerIndex,current,next });
				}
				else
				{
					triangles.insert(triangles.end(), { centerIndex,next,current });
				}
			}
		}
	}

	/**
	* [EN]
	* Builds the unit cone: its base ring of radius 1 at y = -1, its apex
	* at y = 1, with 32 segments around Y and a flat cap on the base.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単位の円錐を作る。半径 1 の底面の円が y = -1、頂点が y = 1。
	* Y 軸まわりに 32 分割し、底面に平らなフタを付ける。
	*/
	void PrimitiveMesh::CreateConeShape(DynamicArray<PrimitiveVertex>& vertices, DynamicArray<Uint32>& triangles)
	{
		/// [EN] One apex vertex per segment, its normal taken at the segment's middle angle: all 32 sit on the same point, but each face gets the normal of its own direction instead of one averaged to straight up. The slope rises 2 over a run of 1, so the side normal at angle phi is (2 cos phi, 1, 2 sin phi) normalized, perpendicular to the line from the base up to the apex.
		/// [JP] 頂点は分割ごとに 1 つずつ置き、法線はその分割の真ん中の角度で取る。32 個とも同じ位置だが、各面は真上に平均された法線ではなく、自分の向きの法線を持てる。斜面は横 1 に対して高さ 2 なので、角度 phi での側面の法線は (2 cos phi, 1, 2 sin phi) を正規化したもの。底面から頂点へ向かう線に垂直になる。
		Uint32 apexBase = static_cast<Uint32>(vertices.size());
		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			Float phi = (static_cast<Float>(segmentIndex) + 0.5f) / 32.0f * Pi<Float>::Two;

			PrimitiveVertex vertex{};
			vertex.position_ = Vector3(0.0f, 1.0f, 0.0f);
			vertex.normal_ = Vector3(2.0f * Cos(phi), 1.0f, 2.0f * Sin(phi));
			vertex.normal_.Normalize();
			vertex.texcoord_ = Vector2((static_cast<Float>(segmentIndex) + 0.5f) / 32.0f, 0.0f);
			vertex.cap_ = 0.0f;
			vertices.push_back(vertex);
		}

		/// [EN] The base ring of the side, 33 vertices with the seam doubled as on the sphere.
		/// [JP] 側面の底の輪。球と同じく継ぎ目を二重にして 33 頂点。
		Uint32 ringBase = static_cast<Uint32>(vertices.size());
		for (Uint32 segmentIndex = 0;segmentIndex <= 32;++segmentIndex)
		{
			Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

			PrimitiveVertex vertex{};
			vertex.position_ = Vector3(Cos(phi), -1.0f, Sin(phi));
			vertex.normal_ = Vector3(2.0f * Cos(phi), 1.0f, 2.0f * Sin(phi));
			vertex.normal_.Normalize();
			vertex.texcoord_ = Vector2(static_cast<Float>(segmentIndex) / 32.0f, 1.0f);
			vertex.cap_ = 0.0f;
			vertices.push_back(vertex);
		}

		/// [EN] One triangle per segment, from the apex down to the base ring, in the same winding as the cylinder's side.
		/// [JP] 分割ごとに、頂点から底の輪へ下りる三角形を 1 つ。巻き順は円柱の側面と同じ。
		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			triangles.insert(triangles.end(), { apexBase + segmentIndex,ringBase + segmentIndex,ringBase + segmentIndex + 1 });
		}

		/// [EN] The base cap faces -Y, built the same way as the cylinder's bottom cap.
		/// [JP] 底面のフタは -Y を向く。円柱の下のフタと同じ作り方。
		Uint32 centerIndex = static_cast<Uint32>(vertices.size());
		PrimitiveVertex center{};
		center.position_ = Vector3(0.0f, -1.0f, 0.0f);
		center.normal_ = Vector3(0.0f, -1.0f, 0.0f);
		center.texcoord_ = Vector2(0.5f, 0.5f);
		center.cap_ = 0.0f;
		vertices.push_back(center);

		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

			PrimitiveVertex vertex{};
			vertex.position_ = Vector3(Cos(phi), -1.0f, Sin(phi));
			vertex.normal_ = Vector3(0.0f, -1.0f, 0.0f);
			vertex.texcoord_ = Vector2(Cos(phi) * 0.5f + 0.5f, 0.5f + Sin(phi) * 0.5f);
			vertex.cap_ = 0.0f;
			vertices.push_back(vertex);
		}

		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			Uint32 current = centerIndex + 1 + segmentIndex;
			Uint32 next = centerIndex + 1 + (segmentIndex + 1) % 32;
			triangles.insert(triangles.end(), { centerIndex,next,current });
		}
	}
}