#include <FoundationEngine/File/FilePath.h>

namespace SeedCore
{
	FilePath::FilePath(const std::filesystem::path& fullPath, const std::filesystem::path& rootPath) :
		fullPath_(fullPath),
		rootPath_(rootPath),
		relativePath_(fullPath_.lexically_relative(rootPath_)),
		parentPath_(fullPath_.parent_path()),
		filenamePath_(fullPath_.filename()),
		stemPath_(fullPath_.stem()),
		extensionPath_(fullPath_.extension()),
		fullText_(fullPath_.generic_string()),
		rootText_(rootPath_.generic_string()),
		relativeText_(relativePath_.generic_string()),
		parentText_(parentPath_.generic_string()),
		filenameText_(filenamePath_.generic_string()),
		stemText_(stemPath_.generic_string()),
		extensionText_(extensionPath_.generic_string())
	{
		/// No Code
	}

	const std::filesystem::path& FilePath::FullPath()const
	{
		return fullPath_;
	}

	const std::filesystem::path& FilePath::RootPath()const
	{
		return rootPath_;
	}

	const std::filesystem::path& FilePath::RelativePath()const
	{
		return relativePath_;
	}

	const std::filesystem::path& FilePath::ParentPath()const
	{
		return parentPath_;
	}

	const std::filesystem::path& FilePath::FilenamePath()const
	{
		return filenamePath_;
	}

	const std::filesystem::path& FilePath::StemPath()const
	{
		return stemPath_;
	}

	const std::filesystem::path& FilePath::ExtensionPath()const
	{
		return extensionPath_;
	}

	std::filesystem::path FilePath::ChildPath(const std::filesystem::path& childPath)const
	{
		return fullPath_ / childPath;
	}

	std::filesystem::path FilePath::SiblingPath(const std::filesystem::path& siblingPath)const
	{
		return parentPath_ / siblingPath;
	}

	std::filesystem::path FilePath::ReplacedPath(const std::filesystem::path& extension)const
	{
		std::filesystem::path replacedPath = fullPath_;
		replacedPath.replace_extension(extension);
		return replacedPath;
	}

	std::filesystem::path FilePath::AppendedSuffixPath(const std::filesystem::path& suffix)const
	{
		std::filesystem::path appendedPath = fullPath_;
		appendedPath += suffix;
		return appendedPath;
	}

	std::string& FilePath::FullText()
	{
		return fullText_;
	}

	const std::string& FilePath::FullText()const
	{
		return fullText_;
	}

	std::string& FilePath::RootText()
	{
		return rootText_;
	}

	const std::string& FilePath::RootText()const
	{
		return rootText_;
	}

	std::string& FilePath::RelativeText()
	{
		return relativeText_;
	}

	const std::string& FilePath::RelativeText()const
	{
		return relativeText_;
	}

	std::string& FilePath::ParentText()
	{
		return parentText_;
	}

	const std::string& FilePath::ParentText()const
	{
		return parentText_;
	}

	std::string& FilePath::FilenameText()
	{
		return filenameText_;
	}

	const std::string& FilePath::FilenameText()const
	{
		return filenameText_;
	}

	std::string& FilePath::StemText()
	{
		return stemText_;
	}

	const std::string& FilePath::StemText()const
	{
		return stemText_;
	}

	std::string& FilePath::ExtensionText()
	{
		return extensionText_;
	}

	const std::string& FilePath::ExtensionText()const
	{
		return extensionText_;
	}

	Bool FilePath::Empty()const
	{
		return fullPath_.empty();
	}

	Bool FilePath::Absolute()const
	{
		return fullPath_.is_absolute();
	}

	Bool FilePath::Relative()const
	{
		return fullPath_.is_relative();
	}

	Bool FilePath::Extension()const
	{
		return !extensionPath_.empty();
	}

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
	Bool FilePath::operator==(const FilePath& other)const
	{
		return fullPath_ == other.fullPath_;
	}
}
