#include <FoundationEngine/Serialization/Encryption/Sha256.h>

namespace SeedCore
{
	namespace
	{
		constexpr Uint32 roundConstants[64] =
		{
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
			0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
			0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
			0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
		};

	}

	void Sha256::ProcessBlock(Uint32 state[8], const Byte block[64])
	{
		Uint32 schedule[64]{};
		for (Uint32 index = 0; index < 16; ++index)
		{
			Uint32 byteOffset = index * 4;
			schedule[index] = (static_cast<Uint32>(static_cast<Uint8>(block[byteOffset])) << 24) | (static_cast<Uint32>(static_cast<Uint8>(block[byteOffset + 1])) << 16) | (static_cast<Uint32>(static_cast<Uint8>(block[byteOffset + 2])) << 8) | static_cast<Uint32>(static_cast<Uint8>(block[byteOffset + 3]));
		}

		for (Uint32 index = 16; index < 64; ++index)
		{
			Uint32 sigma0 = ((schedule[index - 15] >> 7) | (schedule[index - 15] << 25)) ^ ((schedule[index - 15] >> 18) | (schedule[index - 15] << 14)) ^ (schedule[index - 15] >> 3);
			Uint32 sigma1 = ((schedule[index - 2] >> 17) | (schedule[index - 2] << 15)) ^ ((schedule[index - 2] >> 19) | (schedule[index - 2] << 13)) ^ (schedule[index - 2] >> 10);
			schedule[index] = schedule[index - 16] + sigma0 + schedule[index - 7] + sigma1;
		}

		Uint32 a = state[0];
		Uint32 b = state[1];
		Uint32 c = state[2];
		Uint32 d = state[3];
		Uint32 e = state[4];
		Uint32 f = state[5];
		Uint32 g = state[6];
		Uint32 h = state[7];

		for (Uint32 index = 0; index < 64; ++index)
		{
			Uint32 sum1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
			Uint32 choice = (e & f) ^ ((~e) & g);
			Uint32 temp1 = h + sum1 + choice + roundConstants[index] + schedule[index];
			Uint32 sum0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
			Uint32 majority = (a & b) ^ (a & c) ^ (b & c);
			Uint32 temp2 = sum0 + majority;

			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;
		}

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
		state[4] += e;
		state[5] += f;
		state[6] += g;
		state[7] += h;
	}

	DynamicArray<Byte> Sha256::Hash(const Byte* data, Size size)
	{
		Uint32 state[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

		Size fullBlockCount = size / 64;
		for (Size blockIndex = 0; blockIndex < fullBlockCount; ++blockIndex)
		{
			ProcessBlock(state, data + blockIndex * 64);
		}

		Size remainder = size - fullBlockCount * 64;
		Byte tail[128]{};
		std::memcpy(tail, data + fullBlockCount * 64, remainder);
		tail[remainder] = static_cast<Byte>(0x80);

		Size paddedSize = (remainder < 56) ? 64 : 128;
		Uint64 bitLength = static_cast<Uint64>(size) * 8;
		for (Uint32 index = 0; index < 8; ++index)
		{
			tail[paddedSize - 1 - index] = static_cast<Byte>(bitLength >> (index * 8));
		}

		ProcessBlock(state, tail);
		if (paddedSize == 128)
		{
			ProcessBlock(state, tail + 64);
		}

		DynamicArray<Byte> digest(32);
		for (Uint32 index = 0; index < 8; ++index)
		{
			digest[index * 4] = static_cast<Byte>(state[index] >> 24);
			digest[index * 4 + 1] = static_cast<Byte>(state[index] >> 16);
			digest[index * 4 + 2] = static_cast<Byte>(state[index] >> 8);
			digest[index * 4 + 3] = static_cast<Byte>(state[index]);
		}
		return digest;
	}

	DynamicArray<Byte> Sha256::Hash(const DynamicArray<Byte>& data)
	{
		return Hash(data.data(), data.size());
	}

	DynamicArray<Byte> Sha256::Hash(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
		{
			return DynamicArray<Byte>();
		}

		Uint32 state[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

		DynamicArray<Byte> buffer(1024 * 1024);
		Byte carry[64]{};
		Size carrySize = 0;
		Uint64 total = 0;
		while (stream)
		{
			stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
			Size read = static_cast<Size>(stream.gcount());
			total += read;

			Size offset = 0;
			while (offset < read)
			{
				Size take = std::min<Size>(64 - carrySize, read - offset);
				std::memcpy(carry + carrySize, buffer.data() + offset, take);
				carrySize += take;
				offset += take;
				if (carrySize == 64)
				{
					ProcessBlock(state, carry);
					carrySize = 0;
				}
			}
		}

		Byte tail[128]{};
		std::memcpy(tail, carry, carrySize);
		tail[carrySize] = static_cast<Byte>(0x80);

		Size paddedSize = (carrySize < 56) ? 64 : 128;
		Uint64 bitLength = total * 8;
		for (Uint32 index = 0; index < 8; ++index)
		{
			tail[paddedSize - 1 - index] = static_cast<Byte>(bitLength >> (index * 8));
		}

		ProcessBlock(state, tail);
		if (paddedSize == 128)
		{
			ProcessBlock(state, tail + 64);
		}

		DynamicArray<Byte> digest(32);
		for (Uint32 index = 0; index < 8; ++index)
		{
			digest[index * 4] = static_cast<Byte>(state[index] >> 24);
			digest[index * 4 + 1] = static_cast<Byte>(state[index] >> 16);
			digest[index * 4 + 2] = static_cast<Byte>(state[index] >> 8);
			digest[index * 4 + 3] = static_cast<Byte>(state[index]);
		}
		return digest;
	}
}
