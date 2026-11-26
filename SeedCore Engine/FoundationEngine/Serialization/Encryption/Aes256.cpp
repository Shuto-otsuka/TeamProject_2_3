#include <FoundationEngine/Serialization/Encryption/Aes256.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

namespace SeedCore
{
	namespace
	{
		constexpr Uint8 sbox[256] =
		{
			0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
			0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
			0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
			0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
			0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
			0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
			0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
			0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
			0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
			0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
			0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
			0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
			0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
			0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
			0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
			0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
		};

		constexpr Uint8 invSbox[256] =
		{
			0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
			0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
			0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
			0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
			0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
			0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
			0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
			0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
			0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
			0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
			0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
			0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
			0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
			0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
			0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
			0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d,
		};

		constexpr Uint8 rcon[8] = { 0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40 };
	}

	void Aes256::ExpandKey(const Byte* key, Uint8 roundKeys[15][16])
	{
		Uint8 words[60][4];
		for (Uint32 wordIndex = 0; wordIndex < 8; ++wordIndex)
		{
			for (Uint32 byteIndex = 0; byteIndex < 4; ++byteIndex)
			{
				words[wordIndex][byteIndex] = static_cast<Uint8>(key[wordIndex * 4 + byteIndex]);
			}
		}

		Uint8 rconIndex = 1;
		for (Uint32 wordIndex = 8; wordIndex < 60; ++wordIndex)
		{
			Uint8 temp[4];
			std::memcpy(temp, words[wordIndex - 1], 4);

			if (wordIndex % 8 == 0)
			{
				Uint8 rotated0 = sbox[temp[1]];
				Uint8 rotated1 = sbox[temp[2]];
				Uint8 rotated2 = sbox[temp[3]];
				Uint8 rotated3 = sbox[temp[0]];
				temp[0] = static_cast<Uint8>(rotated0 ^ rcon[rconIndex]);
				temp[1] = rotated1;
				temp[2] = rotated2;
				temp[3] = rotated3;
				++rconIndex;
			}
			else if (wordIndex % 8 == 4)
			{
				temp[0] = sbox[temp[0]];
				temp[1] = sbox[temp[1]];
				temp[2] = sbox[temp[2]];
				temp[3] = sbox[temp[3]];
			}

			for (Uint32 byteIndex = 0; byteIndex < 4; ++byteIndex)
			{
				words[wordIndex][byteIndex] = static_cast<Uint8>(words[wordIndex - 8][byteIndex] ^ temp[byteIndex]);
			}
		}

		for (Uint32 roundIndex = 0; roundIndex < 15; ++roundIndex)
		{
			for (Uint32 columnIndex = 0; columnIndex < 4; ++columnIndex)
			{
				for (Uint32 rowIndex = 0; rowIndex < 4; ++rowIndex)
				{
					roundKeys[roundIndex][rowIndex + 4 * columnIndex] = words[roundIndex * 4 + columnIndex][rowIndex];
				}
			}
		}
	}

	void Aes256::EncryptBlock(const Byte* input, Byte* output, const Uint8 roundKeys[15][16])
	{
		Uint8 state[16];
		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			state[byteIndex] = static_cast<Uint8>(input[byteIndex]);
		}

		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			state[byteIndex] = static_cast<Uint8>(state[byteIndex] ^ roundKeys[0][byteIndex]);
		}

		for (Uint32 roundIndex = 1; roundIndex < 14; ++roundIndex)
		{
			for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
			{
				state[byteIndex] = sbox[state[byteIndex]];
			}

			Uint8 shifted[16];
			std::memcpy(shifted, state, 16);
			for (Uint32 rowIndex = 0; rowIndex < 4; ++rowIndex)
			{
				for (Uint32 columnIndex = 0; columnIndex < 4; ++columnIndex)
				{
					state[rowIndex + 4 * columnIndex] = shifted[rowIndex + 4 * ((columnIndex + rowIndex) % 4)];
				}
			}

			for (Uint32 columnIndex = 0; columnIndex < 4; ++columnIndex)
			{
				Uint8 a0 = state[4 * columnIndex + 0];
				Uint8 a1 = state[4 * columnIndex + 1];
				Uint8 a2 = state[4 * columnIndex + 2];
				Uint8 a3 = state[4 * columnIndex + 3];
				Uint8 d0 = static_cast<Uint8>((a0 << 1) ^ ((a0 & 0x80) ? 0x1B : 0));
				Uint8 d1 = static_cast<Uint8>((a1 << 1) ^ ((a1 & 0x80) ? 0x1B : 0));
				Uint8 d2 = static_cast<Uint8>((a2 << 1) ^ ((a2 & 0x80) ? 0x1B : 0));
				Uint8 d3 = static_cast<Uint8>((a3 << 1) ^ ((a3 & 0x80) ? 0x1B : 0));
				state[4 * columnIndex + 0] = static_cast<Uint8>(d0 ^ d1 ^ a1 ^ a2 ^ a3);
				state[4 * columnIndex + 1] = static_cast<Uint8>(a0 ^ d1 ^ d2 ^ a2 ^ a3);
				state[4 * columnIndex + 2] = static_cast<Uint8>(a0 ^ a1 ^ d2 ^ d3 ^ a3);
				state[4 * columnIndex + 3] = static_cast<Uint8>(d0 ^ a0 ^ a1 ^ a2 ^ d3);
			}

			for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
			{
				state[byteIndex] = static_cast<Uint8>(state[byteIndex] ^ roundKeys[roundIndex][byteIndex]);
			}
		}

		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			state[byteIndex] = sbox[state[byteIndex]];
		}

		Uint8 shifted[16];
		std::memcpy(shifted, state, 16);
		for (Uint32 rowIndex = 0; rowIndex < 4; ++rowIndex)
		{
			for (Uint32 columnIndex = 0; columnIndex < 4; ++columnIndex)
			{
				state[rowIndex + 4 * columnIndex] = shifted[rowIndex + 4 * ((columnIndex + rowIndex) % 4)];
			}
		}

		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			state[byteIndex] = static_cast<Uint8>(state[byteIndex] ^ roundKeys[14][byteIndex]);
		}

		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			output[byteIndex] = static_cast<Byte>(state[byteIndex]);
		}
	}

	void Aes256::DecryptBlock(const Byte* input, Byte* output, const Uint8 roundKeys[15][16])
	{
		Uint8 state[16];
		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			state[byteIndex] = static_cast<Uint8>(input[byteIndex]);
		}

		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			state[byteIndex] = static_cast<Uint8>(state[byteIndex] ^ roundKeys[14][byteIndex]);
		}

		for (Uint32 roundIndex = 13; roundIndex >= 1; --roundIndex)
		{
			Uint8 shifted[16];
			std::memcpy(shifted, state, 16);
			for (Uint32 rowIndex = 0; rowIndex < 4; ++rowIndex)
			{
				for (Uint32 columnIndex = 0; columnIndex < 4; ++columnIndex)
				{
					state[rowIndex + 4 * columnIndex] = shifted[rowIndex + 4 * ((columnIndex - rowIndex + 4) % 4)];
				}
			}

			for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
			{
				state[byteIndex] = invSbox[state[byteIndex]];
			}

			for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
			{
				state[byteIndex] = static_cast<Uint8>(state[byteIndex] ^ roundKeys[roundIndex][byteIndex]);
			}

			for (Uint32 columnIndex = 0; columnIndex < 4; ++columnIndex)
			{
				Uint8 a0 = state[4 * columnIndex + 0];
				Uint8 a1 = state[4 * columnIndex + 1];
				Uint8 a2 = state[4 * columnIndex + 2];
				Uint8 a3 = state[4 * columnIndex + 3];

				Uint8 a0d1 = static_cast<Uint8>((a0 << 1) ^ ((a0 & 0x80) ? 0x1B : 0));
				Uint8 a0d2 = static_cast<Uint8>((a0d1 << 1) ^ ((a0d1 & 0x80) ? 0x1B : 0));
				Uint8 a0d3 = static_cast<Uint8>((a0d2 << 1) ^ ((a0d2 & 0x80) ? 0x1B : 0));

				Uint8 a1d1 = static_cast<Uint8>((a1 << 1) ^ ((a1 & 0x80) ? 0x1B : 0));
				Uint8 a1d2 = static_cast<Uint8>((a1d1 << 1) ^ ((a1d1 & 0x80) ? 0x1B : 0));
				Uint8 a1d3 = static_cast<Uint8>((a1d2 << 1) ^ ((a1d2 & 0x80) ? 0x1B : 0));

				Uint8 a2d1 = static_cast<Uint8>((a2 << 1) ^ ((a2 & 0x80) ? 0x1B : 0));
				Uint8 a2d2 = static_cast<Uint8>((a2d1 << 1) ^ ((a2d1 & 0x80) ? 0x1B : 0));
				Uint8 a2d3 = static_cast<Uint8>((a2d2 << 1) ^ ((a2d2 & 0x80) ? 0x1B : 0));

				Uint8 a3d1 = static_cast<Uint8>((a3 << 1) ^ ((a3 & 0x80) ? 0x1B : 0));
				Uint8 a3d2 = static_cast<Uint8>((a3d1 << 1) ^ ((a3d1 & 0x80) ? 0x1B : 0));
				Uint8 a3d3 = static_cast<Uint8>((a3d2 << 1) ^ ((a3d2 & 0x80) ? 0x1B : 0));

				state[4 * columnIndex + 0] = static_cast<Uint8>((a0d3 ^ a0d2 ^ a0d1) ^ (a1d3 ^ a1d1 ^ a1) ^ (a2d3 ^ a2d2 ^ a2) ^ (a3d3 ^ a3));
				state[4 * columnIndex + 1] = static_cast<Uint8>((a0d3 ^ a0) ^ (a1d3 ^ a1d2 ^ a1d1) ^ (a2d3 ^ a2d1 ^ a2) ^ (a3d3 ^ a3d2 ^ a3));
				state[4 * columnIndex + 2] = static_cast<Uint8>((a0d3 ^ a0d2 ^ a0) ^ (a1d3 ^ a1) ^ (a2d3 ^ a2d2 ^ a2d1) ^ (a3d3 ^ a3d1 ^ a3));
				state[4 * columnIndex + 3] = static_cast<Uint8>((a0d3 ^ a0d1 ^ a0) ^ (a1d3 ^ a1d2 ^ a1) ^ (a2d3 ^ a2) ^ (a3d3 ^ a3d2 ^ a3d1));
			}
		}

		Uint8 shifted[16];
		std::memcpy(shifted, state, 16);
		for (Uint32 rowIndex = 0; rowIndex < 4; ++rowIndex)
		{
			for (Uint32 columnIndex = 0; columnIndex < 4; ++columnIndex)
			{
				state[rowIndex + 4 * columnIndex] = shifted[rowIndex + 4 * ((columnIndex - rowIndex + 4) % 4)];
			}
		}

		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			state[byteIndex] = invSbox[state[byteIndex]];
		}

		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			state[byteIndex] = static_cast<Uint8>(state[byteIndex] ^ roundKeys[0][byteIndex]);
		}

		for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
		{
			output[byteIndex] = static_cast<Byte>(state[byteIndex]);
		}
	}

	DynamicArray<Byte> Aes256::Encrypt(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& plaintext)
	{
		Size paddedSize = (plaintext.size() / 16 + 1) * 16;
		Uint8 padValue = static_cast<Uint8>(paddedSize - plaintext.size());

		DynamicArray<Byte> padded(paddedSize);
		std::memcpy(padded.data(), plaintext.data(), plaintext.size());
		for (Size byteIndex = plaintext.size(); byteIndex < paddedSize; ++byteIndex)
		{
			padded[byteIndex] = static_cast<Byte>(padValue);
		}

		if (paddedSize >= hardwareThreshold_)
		{
			return EncryptHardware(key, iv, padded);
		}
		return EncryptSoftware(key, iv, padded);
	}

	DynamicArray<Byte> Aes256::EncryptSoftware(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& paddedPlaintext)
	{
		Uint8 roundKeys[15][16];
		ExpandKey(key.data(), roundKeys);

		DynamicArray<Byte> ciphertext(paddedPlaintext.size());
		Byte previousBlock[16];
		std::memcpy(previousBlock, iv.data(), 16);

		for (Size blockIndex = 0; blockIndex < paddedPlaintext.size() / 16; ++blockIndex)
		{
			Byte xored[16];
			for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
			{
				xored[byteIndex] = static_cast<Byte>(paddedPlaintext[blockIndex * 16 + byteIndex] ^ previousBlock[byteIndex]);
			}

			Byte encrypted[16];
			EncryptBlock(xored, encrypted, roundKeys);

			std::memcpy(&ciphertext[blockIndex * 16], encrypted, 16);
			std::memcpy(previousBlock, encrypted, 16);
		}

		return ciphertext;
	}

	DynamicArray<Byte> Aes256::EncryptHardware(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& paddedPlaintext)
	{
		BCRYPT_ALG_HANDLE algorithm = nullptr;
		if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0)))
		{
			return EncryptSoftware(key, iv, paddedPlaintext);
		}

		BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_CBC)), sizeof(BCRYPT_CHAIN_MODE_CBC), 0);

		BCRYPT_KEY_HANDLE keyHandle = nullptr;
		NTSTATUS keyStatus = BCryptGenerateSymmetricKey(algorithm, &keyHandle, nullptr, 0, reinterpret_cast<PUCHAR>(const_cast<Byte*>(key.data())), static_cast<ULONG>(key.size()), 0);

		DynamicArray<Byte> ciphertext;
		if (BCRYPT_SUCCESS(keyStatus))
		{
			Byte ivBuffer[16];
			std::memcpy(ivBuffer, iv.data(), 16);

			ULONG resultLength = 0;
			BCryptEncrypt(keyHandle, reinterpret_cast<PUCHAR>(const_cast<Byte*>(paddedPlaintext.data())), static_cast<ULONG>(paddedPlaintext.size()), nullptr, reinterpret_cast<PUCHAR>(ivBuffer), 16, nullptr, 0, &resultLength, 0);

			ciphertext.resize(resultLength);
			std::memcpy(ivBuffer, iv.data(), 16);

			ULONG written = 0;
			NTSTATUS encryptStatus = BCryptEncrypt(keyHandle, reinterpret_cast<PUCHAR>(const_cast<Byte*>(paddedPlaintext.data())), static_cast<ULONG>(paddedPlaintext.size()), nullptr, reinterpret_cast<PUCHAR>(ivBuffer), 16, reinterpret_cast<PUCHAR>(ciphertext.data()), resultLength, &written, 0);
			if (!BCRYPT_SUCCESS(encryptStatus))
			{
				ciphertext.clear();
			}

			BCryptDestroyKey(keyHandle);
		}

		BCryptCloseAlgorithmProvider(algorithm, 0);

		if (ciphertext.empty())
		{
			return EncryptSoftware(key, iv, paddedPlaintext);
		}
		return ciphertext;
	}

	DynamicArray<Byte> Aes256::Decrypt(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& ciphertext)
	{
		DynamicArray<Byte> plaintext = DecryptUnpadded(key, iv, ciphertext);
		if (plaintext.empty())
		{
			return {};
		}

		Uint8 padValue = static_cast<Uint8>(plaintext.back());
		if (padValue == 0 || padValue > 16 || static_cast<Size>(padValue) > plaintext.size())
		{
			return {};
		}
		plaintext.resize(plaintext.size() - padValue);
		return plaintext;
	}

	DynamicArray<Byte> Aes256::DecryptUnpadded(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& ciphertext)
	{
		if (ciphertext.empty() || ciphertext.size() % 16 != 0)
		{
			return {};
		}

		if (ciphertext.size() >= hardwareThreshold_)
		{
			return DecryptUnpaddedHardware(key, iv, ciphertext);
		}
		return DecryptUnpaddedSoftware(key, iv, ciphertext);
	}

	DynamicArray<Byte> Aes256::DecryptUnpaddedSoftware(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& ciphertext)
	{
		Uint8 roundKeys[15][16];
		ExpandKey(key.data(), roundKeys);

		DynamicArray<Byte> plaintext(ciphertext.size());
		Byte previousBlock[16];
		std::memcpy(previousBlock, iv.data(), 16);

		for (Size blockIndex = 0; blockIndex < ciphertext.size() / 16; ++blockIndex)
		{
			Byte decrypted[16];
			DecryptBlock(&ciphertext[blockIndex * 16], decrypted, roundKeys);

			for (Uint32 byteIndex = 0; byteIndex < 16; ++byteIndex)
			{
				plaintext[blockIndex * 16 + byteIndex] = static_cast<Byte>(decrypted[byteIndex] ^ previousBlock[byteIndex]);
			}

			std::memcpy(previousBlock, &ciphertext[blockIndex * 16], 16);
		}

		return plaintext;
	}

	DynamicArray<Byte> Aes256::DecryptUnpaddedHardware(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& ciphertext)
	{
		BCRYPT_ALG_HANDLE algorithm = nullptr;
		if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0)))
		{
			return DecryptUnpaddedSoftware(key, iv, ciphertext);
		}

		BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_CBC)), sizeof(BCRYPT_CHAIN_MODE_CBC), 0);

		BCRYPT_KEY_HANDLE keyHandle = nullptr;
		NTSTATUS keyStatus = BCryptGenerateSymmetricKey(algorithm, &keyHandle, nullptr, 0, reinterpret_cast<PUCHAR>(const_cast<Byte*>(key.data())), static_cast<ULONG>(key.size()), 0);

		DynamicArray<Byte> plaintext;
		if (BCRYPT_SUCCESS(keyStatus))
		{
			Byte ivBuffer[16];
			std::memcpy(ivBuffer, iv.data(), 16);

			ULONG resultLength = 0;
			BCryptDecrypt(keyHandle, reinterpret_cast<PUCHAR>(const_cast<Byte*>(ciphertext.data())), static_cast<ULONG>(ciphertext.size()), nullptr, reinterpret_cast<PUCHAR>(ivBuffer), 16, nullptr, 0, &resultLength, 0);

			plaintext.resize(resultLength);
			std::memcpy(ivBuffer, iv.data(), 16);

			ULONG written = 0;
			NTSTATUS decryptStatus = BCryptDecrypt(keyHandle, reinterpret_cast<PUCHAR>(const_cast<Byte*>(ciphertext.data())), static_cast<ULONG>(ciphertext.size()), nullptr, reinterpret_cast<PUCHAR>(ivBuffer), 16, reinterpret_cast<PUCHAR>(plaintext.data()), resultLength, &written, 0);
			if (!BCRYPT_SUCCESS(decryptStatus))
			{
				plaintext.clear();
			}

			BCryptDestroyKey(keyHandle);
		}

		BCryptCloseAlgorithmProvider(algorithm, 0);

		if (plaintext.empty())
		{
			return DecryptUnpaddedSoftware(key, iv, ciphertext);
		}
		return plaintext;
	}
}
