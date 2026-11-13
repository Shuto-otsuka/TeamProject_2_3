#include <GraphicsEngine/Shape/Primitive/PrimitiveMesh.h>
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>
#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Log/DxFail.h>

namespace SeedCore
{
	PrimitiveMesh::PrimitiveMesh()
	{
		/// No Code
	}

	PrimitiveMesh::~PrimitiveMesh() = default;

	void PrimitiveMesh::Create(ID3D12Device* device, BindlessHeap& heap)
	{
		DynamicArray<Point> points;
		DynamicArray<Uint32> triangles;
		DynamicArray<Meshlet::TriangleRange> triangleRanges;
		StaticArray<Uint32, kindCount_> indexOffsets = {};
		StaticArray<Uint32, kindCount_> indexCounts = {};

		DynamicArray<Point> shapePoints;
		DynamicArray<Uint32> shapeTriangles;
		for (Uint32 slot = 0;slot < kindCount_;++slot)
		{
			shapePoints.clear();
			shapeTriangles.clear();
			if (slot == 0)
			{
				CreateBox(shapePoints, shapeTriangles);
			}
			else if (slot == 1)
			{
				CreateSphere(shapePoints, shapeTriangles);
			}
			else if (slot == 2)
			{
				CreateCylinder(shapePoints, shapeTriangles);
			}
			else
			{
				CreateCone(shapePoints, shapeTriangles);
			}

			Uint32 pointBase = static_cast<Uint32>(points.size());
			points.insert(points.end(), shapePoints.begin(), shapePoints.end());

			Meshlet::TriangleRange triangleRange;
			triangleRange.triangleOffset_ = static_cast<Uint32>(triangles.size() / 3);
			triangleRange.triangleCount_ = static_cast<Uint32>(shapeTriangles.size() / 3);
			triangleRanges.push_back(triangleRange);

			indexOffsets[slot] = static_cast<Uint32>(triangles.size());
			indexCounts[slot] = static_cast<Uint32>(shapeTriangles.size());
			for (Uint32 pointIndex : shapeTriangles)
			{
				triangles.push_back(pointBase + pointIndex);
			}
		}

		Meshlet meshlet(triangles, triangleRanges);
		for (Uint32 slot = 0;slot < kindCount_;++slot)
		{
			meshlets_[slot] = meshlet.MeshletRanges()[slot];
		}

		std::span<const MeshletDesc> meshletDescs = meshlet.Meshlets();
		std::span<const Uint32> vertexIndices = meshlet.VertexIndices();
		DynamicArray<MeshletBound> meshletBounds(meshletDescs.size());
		for (Size meshletIndex = 0;meshletIndex < meshletDescs.size();++meshletIndex)
		{
			const MeshletDesc& meshletDesc = meshletDescs[meshletIndex];
			MeshletBound& bound = meshletBounds[meshletIndex];

			Vector3 center = Vector3::Zero;
			Vector3 axis = Vector3::Zero;
			for (Uint32 vertexIndex = 0;vertexIndex < meshletDesc.vertexCount_;++vertexIndex)
			{
				const Point& point = points[vertexIndices[meshletDesc.vertexOffset_ + vertexIndex]];
				center += point.position_;
				axis += point.normal_;
			}
			center /= static_cast<Float>(meshletDesc.vertexCount_);

			Float radius = 0.0f;
			for (Uint32 vertexIndex = 0;vertexIndex < meshletDesc.vertexCount_;++vertexIndex)
			{
				radius = Max(radius, (points[vertexIndices[meshletDesc.vertexOffset_ + vertexIndex]].position_ - center).Length());
			}
			bound.center_ = center;
			bound.radius_ = radius;

			Float axisLength = axis.Length();
			if (axisLength > 1e-6f)
			{
				axis /= axisLength;
				Float minDot = 1.0f;
				for (Uint32 vertexIndex = 0;vertexIndex < meshletDesc.vertexCount_;++vertexIndex)
				{
					minDot = Min(minDot, points[vertexIndices[meshletDesc.vertexOffset_ + vertexIndex]].normal_.Dot(axis));
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

		CristerQuantizer quantizer;
		DynamicArray<CompressedVertex> vertices;
		DynamicArray<Vector3> positions;
		vertices.reserve(points.size());
		positions.reserve(points.size());
		for (const Point& point : points)
		{
			Vector3 reference = Abs(point.normal_.y) < 0.99f ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
			Vector3 tangent = reference.Cross(point.normal_);
			tangent.Normalize();

			Vertex vertex;
			vertex.position_ = point.position_;
			vertex.normal_ = point.normal_;
			vertex.tangent_ = Vector4(tangent.x, tangent.y, tangent.z, 1.0f);
			vertex.texcoord_ = point.texcoord_;
			vertices.push_back(quantizer.EncodeVertex(vertex, Vector3(-1.0f, -1.0f, -1.0f), Vector3(2.0f, 2.0f, 2.0f), Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f)));
			positions.push_back(point.position_);
		}

		DynamicArray<Uint8> primitiveIndices(meshlet.PrimitiveIndices().begin(), meshlet.PrimitiveIndices().end());
		primitiveIndices.resize((primitiveIndices.size() + 3) / 4 * 4, 0);

		vertexBufferIndex_ = CreateBuffer(device, heap, vertices.data(), vertices.size() * sizeof(CompressedVertex), sizeof(CompressedVertex), false, vertexBuffer_);
		meshletBufferIndex_ = CreateBuffer(device, heap, meshletDescs.data(), meshletDescs.size() * sizeof(MeshletDesc), sizeof(MeshletDesc), false, meshletBuffer_);
		meshletBoundBufferIndex_ = CreateBuffer(device, heap, meshletBounds.data(), meshletBounds.size() * sizeof(MeshletBound), sizeof(MeshletBound), false, meshletBoundBuffer_);
		vertexIndicesBufferIndex_ = CreateBuffer(device, heap, vertexIndices.data(), vertexIndices.size() * sizeof(Uint32), sizeof(Uint32), false, vertexIndicesBuffer_);
		primitiveIndicesBufferIndex_ = CreateBuffer(device, heap, primitiveIndices.data(), primitiveIndices.size(), 0, true, primitiveIndicesBuffer_);
		CreateBuffer(device, heap, positions.data(), positions.size() * sizeof(Vector3), 0, false, positionBuffer_);
		CreateBuffer(device, heap, triangles.data(), triangles.size() * sizeof(Uint32), 0, false, indexBuffer_);

		for (Uint32 slot = 0;slot < kindCount_;++slot)
		{
			indexBufferIndices_[slot] = heap.AllocateIndex();
			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Buffer.FirstElement = indexOffsets[slot];
			shaderResourceViewDesc.Buffer.NumElements = indexCounts[slot];
			shaderResourceViewDesc.Buffer.StructureByteStride = sizeof(Uint32);
			device->CreateShaderResourceView(indexBuffer_.Get(), &shaderResourceViewDesc, heap.CPUHandle(indexBufferIndices_[slot]));

			D3D12_RAYTRACING_GEOMETRY_DESC& geometry = geometries_[slot];
			geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
			geometry.Triangles.VertexBuffer.StartAddress = positionBuffer_->GetGPUVirtualAddress();
			geometry.Triangles.VertexBuffer.StrideInBytes = sizeof(Vector3);
			geometry.Triangles.VertexCount = static_cast<Uint32>(positions.size());
			geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			geometry.Triangles.IndexBuffer = indexBuffer_->GetGPUVirtualAddress() + static_cast<Uint64>(indexOffsets[slot]) * sizeof(Uint32);
			geometry.Triangles.IndexCount = indexCounts[slot];
			geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
			geometry.Triangles.Transform3x4 = 0;
		}
	}

	Uint32 PrimitiveMesh::Slot(ShapeKind kind)const
	{
		if (kind == ShapeKind::Box)
		{
			return 0;
		}
		if (kind == ShapeKind::Sphere)
		{
			return 1;
		}
		if (kind == ShapeKind::Cylinder)
		{
			return 2;
		}
		if (kind == ShapeKind::Cone)
		{
			return 3;
		}
		return SC_INVALID;
	}

	const Meshlet::MeshletRange& PrimitiveMesh::Meshlets(Uint32 slot)const
	{
		return meshlets_[slot];
	}

	Uint PrimitiveMesh::IndexBufferIndex(Uint32 slot)const
	{
		return indexBufferIndices_[slot];
	}

	const D3D12_RAYTRACING_GEOMETRY_DESC& PrimitiveMesh::Geometry(Uint32 slot)const
	{
		return geometries_[slot];
	}

	Uint PrimitiveMesh::VertexBufferIndex()const
	{
		return vertexBufferIndex_;
	}

	Uint PrimitiveMesh::MeshletBufferIndex()const
	{
		return meshletBufferIndex_;
	}

	Uint PrimitiveMesh::MeshletBoundBufferIndex()const
	{
		return meshletBoundBufferIndex_;
	}

	Uint PrimitiveMesh::VertexIndicesBufferIndex()const
	{
		return vertexIndicesBufferIndex_;
	}

	Uint PrimitiveMesh::PrimitiveIndicesBufferIndex()const
	{
		return primitiveIndicesBufferIndex_;
	}

	void PrimitiveMesh::CreateBox(DynamicArray<Point>& points, DynamicArray<Uint32>& triangles)
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
			Uint32 baseIndex = static_cast<Uint32>(points.size());
			for (const Vector2& corner : corners)
			{
				Point point;
				point.position_ = normals[faceIndex] + rights[faceIndex] * corner.x + ups[faceIndex] * corner.y;
				point.normal_ = normals[faceIndex];
				point.texcoord_ = Vector2(corner.x * 0.5f + 0.5f, 0.5f - corner.y * 0.5f);
				points.push_back(point);
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
	void PrimitiveMesh::CreateSphere(DynamicArray<Point>& points, DynamicArray<Uint32>& triangles)
	{
		/// [EN] A grid of 17 rows (both poles included) by 33 columns: the seam column is doubled so the texture wraps around exactly once, so each row is 33 vertices apart.
		/// [JP] 17 行（両極を含む）× 33 列の格子。テクスチャがちょうど一周で巻くよう継ぎ目の列は二重にするので、行は 33 頂点ずつ離れる。
		Uint32 baseIndex = static_cast<Uint32>(points.size());
		for (Uint32 ringIndex = 0;ringIndex <= 16;++ringIndex)
		{
			Float theta = static_cast<Float>(ringIndex) / 16.0f * Pi<Float>::Value;
			Float sinTheta = Sin(theta);
			Float cosTheta = Cos(theta);

			for (Uint32 segmentIndex = 0;segmentIndex <= 32;++segmentIndex)
			{
				Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

				Point point;
				point.position_ = Vector3(sinTheta * Cos(phi), cosTheta, sinTheta * Sin(phi));
				point.normal_ = point.position_;
				point.texcoord_ = Vector2(static_cast<Float>(segmentIndex) / 32.0f, static_cast<Float>(ringIndex) / 16.0f);
				points.push_back(point);

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
	void PrimitiveMesh::CreateCylinder(DynamicArray<Point>& points, DynamicArray<Uint32>& triangles)
	{
		/// [EN] Side: a top row and a bottom row of 33 vertices, the seam doubled as on the sphere. The normals point straight out from the axis, so the side shades smoothly.
		/// [JP] 側面。上の行と下の行に 33 頂点ずつ。継ぎ目は球と同じく二重にする。法線は軸から真横に向けるので、側面はなめらかに陰影が付く。
		Uint32 sideBase = static_cast<Uint32>(points.size());
		for (Uint32 rowIndex = 0;rowIndex < 2;++rowIndex)
		{
			Float y = rowIndex == 0 ? 1.0f : -1.0f;
			for (Uint32 segmentIndex = 0;segmentIndex <= 32;++segmentIndex)
			{
				Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

				Point point;
				point.position_ = Vector3(Cos(phi), y, Sin(phi));
				point.normal_ = Vector3(Cos(phi), 0.0f, Sin(phi));
				point.texcoord_ = Vector2(static_cast<Float>(segmentIndex) / 32.0f, static_cast<Float>(rowIndex));
				points.push_back(point);
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

			Uint32 centerIndex = static_cast<Uint32>(points.size());
			Point center;
			center.position_ = Vector3(0.0f, y, 0.0f);
			center.normal_ = Vector3(0.0f, y, 0.0f);
			center.texcoord_ = Vector2(0.5f, 0.5f);
			points.push_back(center);

			for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
			{
				Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

				Point point;
				point.position_ = Vector3(Cos(phi), y, Sin(phi));
				point.normal_ = Vector3(0.0f, y, 0.0f);
				point.texcoord_ = Vector2(Cos(phi) * 0.5f + 0.5f, 0.5f - Sin(phi) * 0.5f * y);
				points.push_back(point);
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
	void PrimitiveMesh::CreateCone(DynamicArray<Point>& points, DynamicArray<Uint32>& triangles)
	{
		/// [EN] One apex vertex per segment, its normal taken at the segment's middle angle: all 32 sit on the same point, but each face gets the normal of its own direction instead of one averaged to straight up. The slope rises 2 over a run of 1, so the side normal at angle phi is (2 cos phi, 1, 2 sin phi) normalized, perpendicular to the line from the base up to the apex.
		/// [JP] 頂点は分割ごとに 1 つずつ置き、法線はその分割の真ん中の角度で取る。32 個とも同じ位置だが、各面は真上に平均された法線ではなく、自分の向きの法線を持てる。斜面は横 1 に対して高さ 2 なので、角度 phi での側面の法線は (2 cos phi, 1, 2 sin phi) を正規化したもの。底面から頂点へ向かう線に垂直になる。
		Uint32 apexBase = static_cast<Uint32>(points.size());
		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			Float phi = (static_cast<Float>(segmentIndex) + 0.5f) / 32.0f * Pi<Float>::Two;

			Point point;
			point.position_ = Vector3(0.0f, 1.0f, 0.0f);
			point.normal_ = Vector3(2.0f * Cos(phi), 1.0f, 2.0f * Sin(phi));
			point.normal_.Normalize();
			point.texcoord_ = Vector2((static_cast<Float>(segmentIndex) + 0.5f) / 32.0f, 0.0f);
			points.push_back(point);
		}

		/// [EN] The base ring of the side, 33 vertices with the seam doubled as on the sphere.
		/// [JP] 側面の底の輪。球と同じく継ぎ目を二重にして 33 頂点。
		Uint32 ringBase = static_cast<Uint32>(points.size());
		for (Uint32 segmentIndex = 0;segmentIndex <= 32;++segmentIndex)
		{
			Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

			Point point;
			point.position_ = Vector3(Cos(phi), -1.0f, Sin(phi));
			point.normal_ = Vector3(2.0f * Cos(phi), 1.0f, 2.0f * Sin(phi));
			point.normal_.Normalize();
			point.texcoord_ = Vector2(static_cast<Float>(segmentIndex) / 32.0f, 1.0f);
			points.push_back(point);
		}

		/// [EN] One triangle per segment, from the apex down to the base ring, in the same winding as the cylinder's side.
		/// [JP] 分割ごとに、頂点から底の輪へ下りる三角形を 1 つ。巻き順は円柱の側面と同じ。
		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			triangles.insert(triangles.end(), { apexBase + segmentIndex,ringBase + segmentIndex,ringBase + segmentIndex + 1 });
		}

		/// [EN] The base cap faces -Y, built the same way as the cylinder's bottom cap.
		/// [JP] 底面のフタは -Y を向く。円柱の下のフタと同じ作り方。
		Uint32 centerIndex = static_cast<Uint32>(points.size());
		Point center;
		center.position_ = Vector3(0.0f, -1.0f, 0.0f);
		center.normal_ = Vector3(0.0f, -1.0f, 0.0f);
		center.texcoord_ = Vector2(0.5f, 0.5f);
		points.push_back(center);

		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			Float phi = static_cast<Float>(segmentIndex) / 32.0f * Pi<Float>::Two;

			Point point;
			point.position_ = Vector3(Cos(phi), -1.0f, Sin(phi));
			point.normal_ = Vector3(0.0f, -1.0f, 0.0f);
			point.texcoord_ = Vector2(Cos(phi) * 0.5f + 0.5f, 0.5f + Sin(phi) * 0.5f);
			points.push_back(point);
		}

		for (Uint32 segmentIndex = 0;segmentIndex < 32;++segmentIndex)
		{
			Uint32 current = centerIndex + 1 + segmentIndex;
			Uint32 next = centerIndex + 1 + (segmentIndex + 1) % 32;
			triangles.insert(triangles.end(), { centerIndex,next,current });
		}
	}

	Uint PrimitiveMesh::CreateBuffer(ID3D12Device* device, BindlessHeap& heap, const void* data, Uint64 size, Uint32 stride, Bool raw, Microsoft::WRL::ComPtr<ID3D12Resource>& resource)
	{
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = Max(size, 4ull);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
		SC_HR_CHECK(hr, "PrimitiveMesh: 形のメッシュのバッファの生成に失敗しました");

		void* mapped = nullptr;
		D3D12_RANGE readRange{};
		hr = resource->Map(0, &readRange, &mapped);
		SC_HR_CHECK(hr, "PrimitiveMesh: 形のメッシュのバッファの Map に失敗しました");
		std::memcpy(mapped, data, static_cast<Size>(size));
		resource->Unmap(0, nullptr);

		if (stride == 0 && !raw)
		{
			return SC_INVALID;
		}

		Uint index = heap.AllocateIndex();
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (raw)
		{
			shaderResourceViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			shaderResourceViewDesc.Buffer.NumElements = static_cast<Uint32>(size / 4);
			shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		}
		else
		{
			shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
			shaderResourceViewDesc.Buffer.NumElements = static_cast<Uint32>(size / stride);
			shaderResourceViewDesc.Buffer.StructureByteStride = stride;
		}
		device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, heap.CPUHandle(index));
		return index;
	}
}
