#pragma once

namespace SeedCore
{
	class MovieDecoder
	{
	public:
		MovieDecoder() = default;

		~MovieDecoder();

		Bool Initialize(const std::string& filePath);

		void Finalize();

		Bool ReadNextSample(Double& outTimestampSeconds);

		Bool Seek(Double timeSeconds);

		NODISCARD const Byte* GetPixelData()const;

		NODISCARD Int GetWidth()const;

		NODISCARD Int GetHeight()const;

		NODISCARD Longlong GetRowPitch()const;

		NODISCARD Double GetDuration()const;

		NODISCARD Bool EndOfStream()const;

	private:
		Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader_;

		Microsoft::WRL::ComPtr<IMFByteStream> byteStream_;

		DynamicArray<Byte> pixelBuffer_;

		Int width_ = 0;

		Int height_ = 0;

		Longlong rowPitch_ = 0;

		Double duration_ = 0.0;

		Bool endOfStream_ = false;

		Bool initialized_ = false;
	};
}
