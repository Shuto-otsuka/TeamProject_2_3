cbuffer Constants : register(b0)
{
	uint texture_width_;
	uint texture_height_;
	uint block_count_x_;
	uint block_count_y_;
};

StructuredBuffer<uint> source_pixels : register(t0);
RWStructuredBuffer<uint4> output_blocks : register(u0);

static const uint weights4[16] = { 0,4,9,13,17,21,26,30,34,38,43,47,51,55,60,64 };

uint4 UnpackPixel(uint texel_index)
{
	uint packed = source_pixels[texel_index];
	return uint4(packed & 0xFF, (packed >> 8) & 0xFF, (packed >> 16) & 0xFF, (packed >> 24) & 0xFF);
}

void SetBits(inout uint block_bits[4], uint bit_offset, uint bit_count, uint value)
{
	value &= (bit_count >= 32) ? 0xFFFFFFFFu : ((1u << bit_count) - 1u);
	uint lane = bit_offset / 32;
	uint bit_in_lane = bit_offset % 32;
	uint low_bits = min(bit_count, 32u - bit_in_lane);
	uint low_mask = (low_bits >= 32) ? 0xFFFFFFFFu : ((1u << low_bits) - 1u);
	block_bits[lane] |= (value & low_mask) << bit_in_lane;

	if (low_bits < bit_count)
	{
		uint high_bits = bit_count - low_bits;
		uint high_value = value >> low_bits;
		block_bits[lane + 1] |= high_value & ((1u << high_bits) - 1u);
	}
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
	uint block_x = dispatch_id.x;
	uint block_y = dispatch_id.y;
	if (block_x >= block_count_x_ || block_y >= block_count_y_)
	{
		return;
	}

	uint4 texels[16];
	for (uint ty = 0; ty < 4; ty++)
	{
		for (uint tx = 0; tx < 4; tx++)
		{
			uint px = min(block_x * 4 + tx, texture_width_ - 1);
			uint py = min(block_y * 4 + ty, texture_height_ - 1);
			texels[ty * 4 + tx] = UnpackPixel(py * texture_width_ + px);
		}
	}

	uint4 endpoint_min = texels[0];
	uint4 endpoint_max = texels[0];
	for (uint i = 1; i < 16; i++)
	{
		endpoint_min = min(endpoint_min, texels[i]);
		endpoint_max = max(endpoint_max, texels[i]);
	}

	uint4 raw_endpoints[2] = { endpoint_min, endpoint_max };
	uint4 endpoints7[2];
	uint pbits[2];
	uint4 endpoints_full[2];

	for (uint e = 0; e < 2; e++)
	{
		uint4 raw = raw_endpoints[e];
		uint best_error = 0xFFFFFFFFu;
		uint best_p = 0;
		uint4 best7 = uint4(0, 0, 0, 0);
		for (uint p = 0; p < 2; p++)
		{
			uint4 q7 = uint4(
				clamp((raw.x - p + 1) / 2, 0, 127),
				clamp((raw.y - p + 1) / 2, 0, 127),
				clamp((raw.z - p + 1) / 2, 0, 127),
				clamp((raw.w - p + 1) / 2, 0, 127));
			uint4 recon = (q7 << 1) | p;
			int4 diff = int4(recon) - int4(raw);
			uint err = dot(diff, diff);
			if (err < best_error)
			{
				best_error = err;
				best_p = p;
				best7 = q7;
			}
		}
		endpoints7[e] = best7;
		pbits[e] = best_p;
		endpoints_full[e] = (best7 << 1) | best_p;
	}

	uint indices[16];
	for (uint t = 0; t < 16; t++)
	{
		uint best_index = 0;
		uint best_err = 0xFFFFFFFFu;
		for (uint w = 0; w < 16; w++)
		{
			uint weight = weights4[w];
			uint4 interpolated = (endpoints_full[0] * (64 - weight) + endpoints_full[1] * weight + 32) >> 6;
			int4 diff = int4(texels[t]) - int4(interpolated);
			uint err = dot(diff, diff);
			if (err < best_err)
			{
				best_err = err;
				best_index = w;
			}
		}
		indices[t] = best_index;
	}

	if (indices[0] >= 8)
	{
		uint4 temp_endpoint7 = endpoints7[0];
		endpoints7[0] = endpoints7[1];
		endpoints7[1] = temp_endpoint7;
		uint temp_p = pbits[0];
		pbits[0] = pbits[1];
		pbits[1] = temp_p;
		for (uint i = 0; i < 16; i++)
		{
			indices[i] = 15 - indices[i];
		}
	}

	uint block_bits[4] = { 0, 0, 0, 0 };
	uint offset = 0;

	SetBits(block_bits, offset, 7, 0x40);
	offset += 7;

	SetBits(block_bits, offset, 7, endpoints7[0].x); offset += 7;
	SetBits(block_bits, offset, 7, endpoints7[1].x); offset += 7;
	SetBits(block_bits, offset, 7, endpoints7[0].y); offset += 7;
	SetBits(block_bits, offset, 7, endpoints7[1].y); offset += 7;
	SetBits(block_bits, offset, 7, endpoints7[0].z); offset += 7;
	SetBits(block_bits, offset, 7, endpoints7[1].z); offset += 7;
	SetBits(block_bits, offset, 7, endpoints7[0].w); offset += 7;
	SetBits(block_bits, offset, 7, endpoints7[1].w); offset += 7;

	SetBits(block_bits, offset, 1, pbits[0]); offset += 1;
	SetBits(block_bits, offset, 1, pbits[1]); offset += 1;

	SetBits(block_bits, offset, 3, indices[0] & 0x7); offset += 3;
	for (uint idx = 1; idx < 16; idx++)
	{
		SetBits(block_bits, offset, 4, indices[idx] & 0xF);
		offset += 4;
	}

	output_blocks[block_y * block_count_x_ + block_x] = uint4(block_bits[0], block_bits[1], block_bits[2], block_bits[3]);
}
