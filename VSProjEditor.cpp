#include "pch.h"
#include "XMLCommon.h"

namespace VSProjEditor
{
	constexpr auto WARNING_VERSION_NODE_NAME = L"WarningVersion";
	constexpr auto DISABLE_SPECIFIC_WARNINGS_NODE_NAME = L"DisableSpecificWarnings";
	constexpr auto ITEM_DEFINITION_GROUP_NODE_NAME = L"ItemDefinitionGroup";
	constexpr auto ITEM_DEFINITION_GROUP_CONDITION_ATTR = L"Condition";
	constexpr auto CL_COMPILE_NODE_NAME = L"ClCompile";

	std::mutex _m{};
	static void PrintHelp()
	{
		std::cout << "parameters not provided, provide at least on of params:" << std::endl;
		std::cout << "param:\n";
		std::cout << "-source \"source directory\"\n";
		std::cout << "-a (warning codes to add to project exclude node) for C5967 and C5955 add \"5967,5955\" \"comma_separated_values,5967,2,3,4\"\n";
		std::cout << "-r same like above but for removing from node\n";
		std::cout << "-vw turn on specifci Warning version for all nodes under source dir (recursive)\n";
		std::cout << "-bt specify build types that we will work in example -bt \"Release,Debug\" \n";
		std::cout << "-exclude_regex \".*\" you can exclude specific filenames from updating\n";
		std::cout << "-execution \"par\" or \"seq\"  by default parallel = \"par\"\n";

	}
	void PushIfDoNotExist(std::vector<std::wstring>& vec, const std::wstring& obj)
	{
		if (auto it = std::find_if(vec.begin(), vec.end(), [&obj](const std::wstring& txt) {
			return !txt.empty() && !txt.compare(obj);
			}); it == vec.end())
			vec.push_back(obj);
	}
	std::wstring UpdateNodeWarnings(const std::wstring& nodeText, const std::vector<std::wstring>& addWarnings, const std::vector<std::wstring>& removeWarnings)
	{
		std::wstring outputNewValue{};
		std::vector<std::wstring> updatedWarnings{};
		for (const auto& warning : SplitStr(nodeText, ';'))
		{
			if (warning.empty())
				continue;
			PushIfDoNotExist(updatedWarnings, warning);
		}

		for (const auto& warning : addWarnings)
		{
			PushIfDoNotExist(updatedWarnings, warning);
		}

		for (const auto& remwarning : removeWarnings)
		{
			auto it = std::find_if(updatedWarnings.begin(), updatedWarnings.end(), [&remwarning](const std::wstring& warning) {
				return !warning.compare(remwarning);
				});

			if (it != updatedWarnings.end())
			{
				updatedWarnings.erase(it);
			}
		}

		for (size_t warningIndex = 0; warningIndex < updatedWarnings.size(); warningIndex++)
		{
			outputNewValue += updatedWarnings[warningIndex];
			if (warningIndex != updatedWarnings.size() - 1)
			{
				outputNewValue += L";";
			}
		}
		return outputNewValue;
	}
}

int main(int argc, char* argv[]) {
	using namespace VSProjEditor;

	ComInitializer init{};
	static_cast<void>(init);
	std::regex vcxproj_pattern(".*\\.vcxproj$");
	std::regex exclude_regex(".*_res.*");

	//params
	std::vector<std::wstring> buildTypeToFind{};
	std::vector<std::wstring> allWarningsToAdd{};
	std::wstring sourceDir{};
	std::wstring strWarningVersion{};
	std::vector<std::wstring> allWarningsToRemove{};
	bool bAsyncExecution = true;
	for (int i = 1; i < argc; i+=2) {
		std::string arg = argv[i];
		if ((arg == "-a") && i + 1 < argc) {
			std::string separatedValues{ argv[i + 1] };
			std::wstring tempSepareted{ separatedValues.begin(), separatedValues.end() };
			allWarningsToAdd = SplitStr(tempSepareted, ',');
		}
		else if ((arg == "-r") && i + 1 < argc) {
			std::string separatedValues{ argv[i + 1] };
			std::wstring tempSepareted{ separatedValues.begin(), separatedValues.end() };
			allWarningsToRemove = SplitStr(tempSepareted, ',');
		}
		else if ((arg == "-source") && i + 1 < argc) {
			std::string temp{ argv[i + 1] };
			sourceDir = std::wstring{ temp.begin(),temp.end() };
		}
		else if ((arg == "-vw") && i + 1 < argc) {
			std::string temp{ argv[i + 1] };
			strWarningVersion = std::wstring{ temp.begin(),temp.end() };
		}
		else if ((arg == "-bt") && i + 1 < argc) {
			std::string separatedValues{ argv[i+1] };
			std::wstring tempSepareted{ separatedValues.begin(), separatedValues.end() };
			buildTypeToFind = SplitStr(tempSepareted, ',');
		}
		else if ((arg == "-exclude_regex") && i + 1 < argc)
		{
			std::string temp{ argv[i + 1] };
			exclude_regex = std::regex(temp);
		}
		else if ((arg == "-execution") && i + 1 < argc)
		{
			std::string temp{ argv[i + 1] };
			if (temp == "seq")
				bAsyncExecution = false;

		}
	}


	if (sourceDir.empty() || !std::filesystem::exists(sourceDir))
	{
		std::cout << "source directory not provided! or do not exist cannot perform recursive search!." << std::endl;
		PrintHelp();
		return 1;
	}

	if (buildTypeToFind.empty())
	{
		std::cout << "no build types specified" << std::endl;
		PrintHelp();
		return 2;
	}

	if (allWarningsToAdd.empty() && strWarningVersion.empty() && allWarningsToRemove.empty())
	{
		PrintHelp();
		return 3;
	}
	std::vector<std::filesystem::path> allProjectfiles{};
	std::vector<std::filesystem::path> updatedFiles{};
	for (auto& file : std::filesystem::recursive_directory_iterator(sourceDir)) {
		std::smatch matches;
		std::string filename = file.path().filename().string();
		if (std::regex_search(filename, matches, vcxproj_pattern))
		{
			if (std::regex_search(filename, matches, exclude_regex))
			{
				continue;
			}
			allProjectfiles.push_back(file.path());
		}
	}
auto fRun = [&](const auto& file)
{
	bool bUpdated = false;
	CComPtr<IXMLDOMDocument> pXMLDoc{};
	if (FAILED(pXMLDoc.CoCreateInstance(__uuidof(DOMDocument60))))
		return;

	VARIANT_BOOL status{ VARIANT_FALSE };
	if (FAILED(pXMLDoc->load(_variant_t(file.c_str()), &status)) || !status)
	{
		std::cout << "failed to load xml file : " << file << "\n";
		return;
	}
	std::vector<CComPtr<IXMLDOMNode>> allItemDefinitionGroups{};
	FindNodesByName((CComPtr<IXMLDOMNode>)pXMLDoc, ITEM_DEFINITION_GROUP_NODE_NAME, allItemDefinitionGroups, true);
	for (auto& itemDefinitionGroup : allItemDefinitionGroups)
	{

		bool bFoundBuildType = false;
		if (auto conditionNode = FindNodeAttribute(itemDefinitionGroup, ITEM_DEFINITION_GROUP_CONDITION_ATTR))
		{
			auto conditionValue = GetNodeValue(conditionNode);
			if (conditionValue.empty())
				continue;

			for (const auto& buildType : buildTypeToFind)
			{
				if (conditionValue.find(buildType + L"|") != std::wstring::npos) {
					bFoundBuildType = true;
					break;
				}
			}
		}

		if (!bFoundBuildType)
			continue;
		std::vector<CComPtr<IXMLDOMNode>> allClCompileNodes{};

		FindNodesByName(itemDefinitionGroup, CL_COMPILE_NODE_NAME, allClCompileNodes, false);
		for (auto& nodeClCompile : allClCompileNodes)
		{
			std::vector<CComPtr<IXMLDOMNode>> allDisableSpecificWarningsNodes{};
			std::vector<CComPtr<IXMLDOMNode>> allWarningVersionNodes{};
			if (!allWarningsToAdd.empty() || !allWarningsToRemove.empty()) {
				FindNodesByName(nodeClCompile, DISABLE_SPECIFIC_WARNINGS_NODE_NAME, allDisableSpecificWarningsNodes, true);

				if (!allDisableSpecificWarningsNodes.empty())
				{
					for (auto& disableSpecificWarningsNode : allDisableSpecificWarningsNodes)
					{
						disableSpecificWarningsNode->put_text((_bstr_t)UpdateNodeWarnings(GetNodeText(disableSpecificWarningsNode), allWarningsToAdd, allWarningsToRemove).c_str());
						bUpdated = true;
					}
				}
				else
				{
					if (auto disableSpecificWarningsNode = CreateNode(pXMLDoc, DISABLE_SPECIFIC_WARNINGS_NODE_NAME, nodeClCompile))
					{
						std::vector<std::wstring> emptyVec{};
						disableSpecificWarningsNode->put_text((_bstr_t)UpdateNodeWarnings(GetNodeText(disableSpecificWarningsNode), allWarningsToAdd, emptyVec).c_str());
						bUpdated = true;
					}
				}
			}

			if (!strWarningVersion.empty()) {
				FindNodesByName(nodeClCompile, WARNING_VERSION_NODE_NAME, allWarningVersionNodes, true);

				if (!allWarningVersionNodes.empty())
				{
					for (auto& warningVersion : allWarningVersionNodes)
					{
						if (!strWarningVersion.compare(GetNodeText(warningVersion)))
							continue;
						warningVersion->put_text((_bstr_t)strWarningVersion.c_str());
						bUpdated = true;
					}
				}
				else
				{
					if (auto warningVersion = CreateNode(pXMLDoc, WARNING_VERSION_NODE_NAME, nodeClCompile))
					{
						warningVersion->put_text((_bstr_t)strWarningVersion.c_str());
						bUpdated = true;
					}
				}
			}
		}

	}

	if (bUpdated)
	{
		_variant_t v{ file.c_str() };
		if (SUCCEEDED(pXMLDoc->save(v)))
		{
			std::scoped_lock lck(_m);
			updatedFiles.push_back(file);
		}
	}
};

	if (bAsyncExecution)
		std::for_each(std::execution::par_unseq, allProjectfiles.begin(), allProjectfiles.end(), fRun);
	else
		std::for_each(std::execution::seq, allProjectfiles.begin(), allProjectfiles.end(), fRun);

	std::cout << "numer of projects found : " << allProjectfiles.size() << std::endl;
	std::cout << "numer of updated projects : " << updatedFiles.size() << std::endl;
	std::cout << "List of updated projects :\n";
	for (const auto& file : updatedFiles)
	{
		std::cout << file << '\n';
	}
	return 0;
}