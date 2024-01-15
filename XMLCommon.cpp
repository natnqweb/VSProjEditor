#include "pch.h"
#include "XMLCommon.h"

namespace VSProjEditor
{
	std::vector<std::wstring> SplitStr(const std::wstring& s, wchar_t delimiter)
	{
		std::vector<std::wstring> tokens;
		std::wistringstream wss(s);
		std::wstring token;

		while (std::getline(wss, token, delimiter))
		{
			if (token.empty() || token == L" ")
				continue;
			tokens.push_back(token);
		}

		return tokens;
	}
	bool IsNodeNameEqTo(CComPtr<IXMLDOMNode> node, const std::wstring& str)
	{
		if (node)
		{
			_bstr_t nodeName{};
			if (FAILED(node->get_nodeName(nodeName.GetAddress()))) return false;

			if (StrCmpW(nodeName, str.c_str()) == 0) return true;
		}
		return false;
	}

	std::vector<CComPtr<IXMLDOMNode>> GetAllChildNodes(CComPtr<IXMLDOMNode> node)
	{
		std::vector<CComPtr<IXMLDOMNode>> childNodes{};
		if (!node)
			return childNodes;

		CComPtr<IXMLDOMNodeList> childNodesList;
		if (FAILED(node->get_childNodes(&childNodesList)))
		{
			return childNodes;
		}

		long childNodesLength{ 0 };
		if (FAILED(childNodesList->get_length(&childNodesLength)))
			return childNodes;
		for (long i = 0; i < childNodesLength; i++) {
			CComPtr<IXMLDOMNode> child{ nullptr };
			if (FAILED(childNodesList->get_item(i, &child)) || !child)
				continue;
			childNodes.push_back(child);
		}
		return childNodes;
	}

	std::wstring GetNodeName(CComPtr<IXMLDOMNode> node) {
		std::wstring result{};
		if (node) {
			_bstr_t temp{};
			if (FAILED(node->get_nodeName(temp.GetAddress()))) return result;

			if (!temp || !temp.length()) return result;

			result = temp;
		}
		return result;

	}
	std::wstring GetNodeValue(CComPtr<IXMLDOMNode> node) {
		std::wstring result{};
		if (node) {
			_variant_t temp{};
			if (FAILED(node->get_nodeValue(temp.GetAddress()))) return result;

			if (temp.vt != VT_BSTR) return result;
			if (!temp.bstrVal) return result;
			_bstr_t str{ temp.bstrVal };
			if (!str.length()) return result;
			result = str;
		}
		return result;

	}

	std::wstring GetNodeText(CComPtr<IXMLDOMNode> node) {
		std::wstring result{};
		if (node) {
			_bstr_t temp{};
			if (FAILED(node->get_text(temp.GetAddress()))) return result;

			if (!temp || !temp.length()) return result;

			result = temp;
		}
		return result;
	}
	CComPtr<IXMLDOMNode> CreateNode(CComPtr<IXMLDOMDocument> pDoc, const std::wstring& nodeName, CComPtr<IXMLDOMNode> parent = nullptr) {
		if (!pDoc)
			return nullptr;
		CComPtr<IXMLDOMNode> newNode{ nullptr };
		_bstr_t namespaceWarningversionUri{};
		if (parent && (FAILED(parent->get_namespaceURI(namespaceWarningversionUri.GetAddress())) || !namespaceWarningversionUri.length()))
		{
			return nullptr;
		}
		else if (FAILED(pDoc->get_namespaceURI(namespaceWarningversionUri.GetAddress())) || !namespaceWarningversionUri.length())
		{
			return nullptr;
		}


		if (SUCCEEDED(pDoc->createNode((_variant_t)NODE_ELEMENT, (_bstr_t)nodeName.c_str(), namespaceWarningversionUri, &newNode))) {
			CComPtr<IXMLDOMNode> outNewNode{ nullptr };
			if (parent && SUCCEEDED(parent->appendChild(newNode, &outNewNode)) && outNewNode) {
				return outNewNode;
			}

		}

		return newNode;
	}

	void FindNodesByName(CComPtr<IXMLDOMNode> parent, const std::wstring& strNodeName, std::vector<CComPtr<IXMLDOMNode>>& outputNodes, bool recursive = false)
	{
		for (const auto& child : GetAllChildNodes(parent))
		{
			if (!strNodeName.empty() && !strNodeName.compare(strNodeName))
			{
				outputNodes.push_back(child);
			}

			if (recursive)
				FindNodesByName(child, strNodeName, outputNodes, recursive);
		}
	}

	CComPtr<IXMLDOMNode> FindNodeAttribute(CComPtr<IXMLDOMNode> node, const std::wstring& strAttributeName)
	{
		CComPtr<IXMLDOMNode> outAttribute{ nullptr };
		CComPtr<IXMLDOMNamedNodeMap> attrMap{ nullptr };
		long nAttributesLength = 0;
		if (node &&
			SUCCEEDED(node->get_attributes(&attrMap)) &&
			attrMap &&
			SUCCEEDED(attrMap->get_length(&nAttributesLength)) &&
			nAttributesLength
			)
		{
			for (long i = 0; i < nAttributesLength; i++) {
				CComPtr<IXMLDOMNode> attr{ nullptr };
				if (SUCCEEDED(attrMap->get_item(i, &attr)) && attr && IsNodeNameEqTo(attr, strAttributeName))
				{
					outAttribute = attr;
					break;
				}
			}
		}
		return outAttribute;
	}


}