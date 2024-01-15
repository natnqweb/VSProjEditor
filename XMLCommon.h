#pragma once

namespace VSProjEditor
{
	std::vector<std::wstring> SplitStr(const std::wstring& s, wchar_t delimiter);

	class ComInitializer
	{
	public:
		ComInitializer()
		{
			::CoInitialize(nullptr);
		}
		~ComInitializer()
		{
			::CoUninitialize();
		}
	};

	bool IsNodeNameEqTo(CComPtr<IXMLDOMNode> node, const std::wstring& str);

	std::vector<CComPtr<IXMLDOMNode>> GetAllChildNodes(CComPtr<IXMLDOMNode> node);
	std::wstring GetNodeName(CComPtr<IXMLDOMNode> node);
	std::wstring GetNodeValue(CComPtr<IXMLDOMNode> node);
	std::wstring GetNodeText(CComPtr<IXMLDOMNode> node);
	CComPtr<IXMLDOMNode> CreateNode(CComPtr<IXMLDOMDocument> pDoc, const std::wstring& nodeName, CComPtr<IXMLDOMNode> parent);
	void FindNodesByName(CComPtr<IXMLDOMNode> parent, const std::wstring& strNodeName, std::vector<CComPtr<IXMLDOMNode>>& outputNodes, bool recursive);
	CComPtr<IXMLDOMNode> FindNodeAttribute(CComPtr<IXMLDOMNode> node, const std::wstring& strAttributeName);
}