#include "pch.h"
#include "XMLCommon.h"

namespace VSProjEditor
{
	static void PrintHelp()
	{
		std::cout << "parameters not provided, provide at least on of params:" << std::endl;
		std::cout << "param:\n";
		std::cout << "-source \"source directory\"\n";
		std::cout << "-a (warning codes to add to project exclude node) for C5967 and C5955 add \"5967,5955\" \"comma_separated_values,5967,2,3,4\"\n";
		std::cout << "-r same like above but for removing from node\n";
		std::cout << "-vw turn on specifci Warning version for all nodes under source dir (recursive)\n";
		std::cout << "-bt specify build types that we will work in example -bt \"Release,Debug\" \n";

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
	std::regex resource_patter(".*_res.*");
	_bstr_t strCondition{ "Condition" };

	//params
	std::vector<std::wstring> buildTypeToFind{};
	std::vector<std::wstring> allWarningsToAdd{};
	std::wstring sourceDir{};
	std::wstring strWarningVersion{};
	std::vector<std::wstring> allWarningsToRemove{};

	for (int i = 1; i < argc; ++i) {
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
			std::string temp{ argv[++i] };
			sourceDir = std::wstring{ temp.begin(),temp.end() };
		}
		else if ((arg == "-vw") && i + 1 < argc) {
			std::string temp{ argv[++i] };
			strWarningVersion = std::wstring{ temp.begin(),temp.end() };
		}
		else if ((arg == "-bt") && i + 1 < argc) {
			std::string separatedValues{ argv[i + 1] };
			std::wstring tempSepareted{ separatedValues.begin(), separatedValues.end() };
			buildTypeToFind = SplitStr(tempSepareted, ',');
		}
	}

	if (sourceDir.empty())
	{
		std::cout << "source directory not provided! cannot perform recursive search!." << std::endl;
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
		if (std::regex_search(filename, matches, resource_patter))
		{
			continue;
		}

		if (std::regex_search(filename, matches, vcxproj_pattern)) {
			allProjectfiles.push_back(file.path());
		}
	}

	for (auto& file : allProjectfiles)
	{
		bool bUpdated = false;
		CComPtr<IXMLDOMDocument> pXMLDoc;
		if (FAILED(pXMLDoc.CoCreateInstance(__uuidof(DOMDocument60))))
			continue;

		VARIANT_BOOL status{ VARIANT_FALSE };
		if (FAILED(pXMLDoc->load(_variant_t(file.c_str()), &status)) || !status)
		{
			std::cout << "failed to load xml file : " << file << "\n";
			continue;
		}
		std::vector<CComPtr<IXMLDOMNode>> allItemDefinitionGroups{};
		FindNodesByName((CComPtr<IXMLDOMNode>)pXMLDoc, L"ItemDefinitionGroup", allItemDefinitionGroups, true);
		for (auto& itemDefinitionGroup : allItemDefinitionGroups)
		{
			CComPtr<IXMLDOMNamedNodeMap> itemDefinitionGroupAttMap{};

			if (FAILED(itemDefinitionGroup->get_attributes(&itemDefinitionGroupAttMap)))
			{
				continue;
			}
			long nDefinitionAttMapLength{ 0 };
			if (FAILED(itemDefinitionGroupAttMap->get_length(&nDefinitionAttMapLength)))
			{
				continue;
			}
			bool bFoundBuildType = false;
			if (auto conditionNode = FindNodeAttribute(itemDefinitionGroup, L"Condition"))
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

			FindNodesByName(itemDefinitionGroup, L"ClCompile", allClCompileNodes, false);
			for (auto& nodeClCompile : allClCompileNodes)
			{
				std::vector<CComPtr<IXMLDOMNode>> allDisableSpecificWarningsNodes{};
				std::vector<CComPtr<IXMLDOMNode>> allWarningVersionNodes{};
				FindNodesByName(nodeClCompile, L"DisableSpecificWarnings", allDisableSpecificWarningsNodes, true);
				FindNodesByName(nodeClCompile, L"WarningVersion", allWarningVersionNodes, true);

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
					if (auto disableSpecificWarningsNode = CreateNode(pXMLDoc, L"DisableSpecificWarnings", nodeClCompile))
					{
						std::vector<std::wstring> emptyVec{};
						disableSpecificWarningsNode->put_text((_bstr_t)UpdateNodeWarnings(GetNodeText(disableSpecificWarningsNode), allWarningsToAdd, emptyVec).c_str());
						bUpdated = true;
					}

				}

				if (!allWarningVersionNodes.empty())
				{
					for (auto& warningVersion : allWarningVersionNodes)
					{

					}
				}
				else
				{

				}
			}

		}

		if (bUpdated)
		{
			_variant_t v{ file.c_str() };
			if (SUCCEEDED(pXMLDoc->save(v)))
			{
				updatedFiles.push_back(file);
			}
		}
	}

	std::cout << "numer of projects found : " << allProjectfiles.size() << std::endl;
	std::cout << "numer of updated projects : " << updatedFiles.size() << std::endl;
	std::cout << "List of updated projects :\n";
	for (const auto& file : updatedFiles)
	{
		std::cout << file << '\n';
	}
	return 0;
}