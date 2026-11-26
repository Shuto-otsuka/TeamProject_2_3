#pragma once

namespace SeedCore
{
	class SEEDCORE_API Aes256
	{
	public:
		static DynamicArray<Byte> Encrypt(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& plaintext);

		static DynamicArray<Byte> Decrypt(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& ciphertext);

		static DynamicArray<Byte> DecryptUnpadded(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& ciphertext);

	private:
		SC_CONSTEX Size hardwareThreshold_ = 1024 * 1024;

		static DynamicArray<Byte> EncryptSoftware(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& paddedPlaintext);

		static DynamicArray<Byte> EncryptHardware(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& paddedPlaintext);

		static DynamicArray<Byte> DecryptUnpaddedSoftware(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& ciphertext);

		static DynamicArray<Byte> DecryptUnpaddedHardware(const DynamicArray<Byte>& key, const DynamicArray<Byte>& iv, const DynamicArray<Byte>& ciphertext);

		static void ExpandKey(const Byte* key, Uint8 roundKeys[15][16]);

		static void EncryptBlock(const Byte* input, Byte* output, const Uint8 roundKeys[15][16]);

		static void DecryptBlock(const Byte* input, Byte* output, const Uint8 roundKeys[15][16]);
	};
}
