#ifndef OOREGEDIT_TREEITEMDATA_H_INCLUDED_
#define OOREGEDIT_TREEITEMDATA_H_INCLUDED_

class TreeItemData : public wxTreeItemData
{
public:
	TreeItemData(OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey, size_t nDepth);
	virtual ~TreeItemData(void);

	void Fill(wxTreeCtrl* pTree, const wxTreeItemId& id);
	void InitList(wxListCtrl* pList);
	void Refresh(wxListCtrl* pList, wxTreeCtrl* pTree, const wxTreeItemId& id);
	void DeleteKey(const Omega::string_t& strKey);
	void DeleteValue(const Omega::string_t& strVal);
	bool RenameValue(const Omega::string_t& strFrom, const Omega::string_t& strTo);
	void RenameKey(const Omega::string_t& strFrom, const Omega::string_t& strTo, TreeItemData* pItem);
	void NewKey(wxTreeCtrl* pTree, const wxTreeItemId& id);
	void NewString(wxListCtrl* pList);
	void NewUInt(wxListCtrl* pList);
	void NewBinary(wxListCtrl* pList);
	void Modify(wxListCtrl* pList, long item_id);
	void Find(wxTreeCtrl* pTree, wxTreeItemId tree_id, wxListCtrl* pList, long list_id, const Omega::string_t& strFind, bool bKeys, bool bValues, bool bData, bool bMatchAll, bool bIgnoreCase);
	wxString GetDesc();
	wxString GetValueDesc(const wxString& strVal);

private:
	OTL::ObjectPtr<Omega::Registry::IKey> m_ptrKey;
	size_t m_nDepth;

	void CopyKey(OTL::ObjectPtr<Omega::Registry::IKey>& ptrOldKey, OTL::ObjectPtr<Omega::Registry::IKey>& ptrNewKey);
	static bool MatchValue(const Omega::string_t& strFind, OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey, const Omega::string_t& strName, bool bValues, bool bData, bool bMatchAll, bool bIgnoreCase);
	static void Find2(wxTreeCtrl* pTree, wxTreeItemId tree_id, wxListCtrl* pList, const Omega::string_t& strFind, bool bKeys, bool bValues, bool bData, bool bMatchAll, bool bIgnoreCase);
	static Omega::string_t Find3(OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey, const Omega::string_t& strFind, bool bKeys, bool bValues, bool bData, bool bMatchAll, bool bIgnoreCase, bool& bKey);
};

#endif // OOREGEDIT_TREEITEMDATA_H_INCLUDED_
