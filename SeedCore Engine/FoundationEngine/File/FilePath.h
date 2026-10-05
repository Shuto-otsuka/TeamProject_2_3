#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	class SEEDCORE_API FilePath
	{
	public:
		FilePath() = default;

		FilePath(const std::filesystem::path& fullPath, const std::filesystem::path& rootPath);
		~FilePath() = default;

	public:
		const std::filesystem::path& FullPath()const;

		const std::filesystem::path& RootPath()const;

		const std::filesystem::path& RelativePath()const;

		const std::filesystem::path& ParentPath()const;

		const std::filesystem::path& FilenamePath()const;

		const std::filesystem::path& StemPath()const;

		const std::filesystem::path& ExtensionPath()const;

	public:
		std::filesystem::path ChildPath(const std::filesystem::path& childPath)const;

		std::filesystem::path SiblingPath(const std::filesystem::path& siblingPath)const;

		std::filesystem::path ReplacedPath(const std::filesystem::path& extension)const;

		std::filesystem::path AppendedSuffixPath(const std::filesystem::path& suffix)const;

	public:
		std::string& FullText();

		const std::string& FullText()const;

		std::string& RootText();

		const std::string& RootText()const;

		std::string& RelativeText();

		const std::string& RelativeText()const;

		std::string& ParentText();

		const std::string& ParentText()const;

		std::string& FilenameText();

		const std::string& FilenameText()const;

		std::string& StemText();

		const std::string& StemText()const;

		std::string& ExtensionText();

		const std::string& ExtensionText()const;

	public:
		Bool Empty()const;

		Bool Absolute()const;

		Bool Relative()const;

		Bool Extension()const;

	public:
		/**
		* [EN]
		* Whether both refer to the same file: their full paths compare equal
		* element by element. The root each was made with does not take part.
		* != follows from this.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 両者が同じファイルを指すか。フルパス同士を要素ごとに比べて等しければ
		* 同じとみなす。それぞれを作ったときのルートは比較に含めない。
		* != はこれから導かれる。
		*/
		Bool operator==(const FilePath& other)const;

	private:
		std::filesystem::path fullPath_;

		std::filesystem::path rootPath_;

		std::filesystem::path relativePath_;

		std::filesystem::path parentPath_;

		std::filesystem::path filenamePath_;

		std::filesystem::path stemPath_;

		std::filesystem::path extensionPath_;

	private:
		std::string fullText_;

		std::string rootText_;

		std::string relativeText_;

		std::string parentText_;

		std::string filenameText_;

		std::string stemText_;

		std::string extensionText_;
	};
}
