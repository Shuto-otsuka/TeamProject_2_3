#pragma once

namespace SeedCore
{
	enum class ShaderMount :Uint8
	{
		Engine,
		Project,
	};

	class ShaderPath
	{
	private:
		friend class ShaderCache;

	public:
		NODISCARD static consteval ShaderPath Engine(const Char* path)
		{
			return ShaderPath(ShaderMount::Engine, path);
		}

		NODISCARD static consteval ShaderPath Project(const Char* path)
		{
			return ShaderPath(ShaderMount::Project, path);
		}

	private:
		consteval ShaderPath(ShaderMount mount, const Char* path) :mount_(mount), path_(path)
		{
			if (path_.empty() || path_.front() == '/' || path_.find("..") != std::string_view::npos || path_.find('\\') != std::string_view::npos || !path_.ends_with(".hlsl"))
			{
				throw "ShaderPath はマウントからの相対パスで、'/' で始まらず、'..' と '\\' を含まず、.hlsl で終わること";
			}
		}

		NODISCARD constexpr ShaderMount Mount()const
		{
			return mount_;
		}

		NODISCARD constexpr std::string_view Path()const
		{
			return path_;
		}

		NODISCARD std::filesystem::path Absolute()const;

	private:
		ShaderMount mount_ = ShaderMount::Engine;

		std::string_view path_;
	};
}