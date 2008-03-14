#ifndef OOREGEDIT_MAINFRAME_H_INCLUDED_
#define OOREGEDIT_MAINFRAME_H_INCLUDED_

class MainFrame : public wxFrame
{
	enum
	{
		ID_LIST = 1,
		ID_TREE,
		ID_DESC,
		ID_EXPORT,
		ID_CONNECT,
		ID_DISCONNECT,
		ID_MODIFY,
		ID_NEW,
		ID_NEW_KEY,
		ID_NEW_STRING_VALUE,
		ID_NEW_BINARY_VALUE,
		ID_NEW_UINT32_VALUE,
		ID_RENAME,
		ID_COPY_NAME,
		ID_FIND,
		ID_FIND_NEXT,
		ID_VIEW_STATUSBAR,
		ID_REFRESH,
		ID_FAVOURITES_ADD,
		ID_FAVOURITES_REMOVE
	};

public:
	MainFrame(void);
	virtual ~MainFrame(void);

	DECLARE_EVENT_TABLE()
	DECLARE_NO_COPY_CLASS(MainFrame)

private:
	wxFileHistory		m_fileHistory;
	wxHtmlWindow*       m_pDescription;
	wxSplitterWindow*	m_pSplitter2;
	wxSplitterWindow*	m_pSplitter;
	wxTreeCtrl* 		m_pTree;
	wxListCtrl* 		m_pList;
	wxString			m_strFind;
	wxString			m_strSelection;
	bool				m_bKeys;
	bool				m_bValues;
	bool				m_bData;
	bool				m_bMatchAll;
	bool				m_bIgnoreCase;

	std::map<wxString,Omega::string_t>	m_mapMRU;

	void OnContextMenu(wxContextMenuEvent&);
	void OnQuit(wxCommandEvent&);
	void OnClose(wxCloseEvent&);
	void OnDelete(wxCommandEvent&);
	void OnRefresh(wxCommandEvent&);
	void OnAbout(wxCommandEvent&);
	void OnRename(wxCommandEvent&);
	void OnModify(wxCommandEvent&);
	void OnCopyName(wxCommandEvent&);
	void OnFind(wxCommandEvent&);
	void OnFindNext(wxCommandEvent&);
	void OnNewKey(wxCommandEvent&);
	void OnNewString(wxCommandEvent&);
	void OnNewUInt(wxCommandEvent&);
	void OnNewBinary(wxCommandEvent&);
	void OnTreeEndLabel(wxTreeEvent&);
	void OnListEndLabel(wxListEvent&);
	void OnUpdateToggleStatusbar(wxUpdateUIEvent&);
	void OnViewStatusBar(wxCommandEvent&);
	void OnItemExpanding(wxTreeEvent& evt);
	void OnTreeSelChanged(wxTreeEvent& evt);
	void OnListDblClk(wxListEvent& evt);
	void OnUpdateModify(wxUpdateUIEvent&);
	void OnUpdateFindNext(wxUpdateUIEvent&);
	void OnMRUFavourites(wxCommandEvent& event);
	void OnUpdateFavouritesAdd(wxUpdateUIEvent& evt);
	void OnUpdateFavouritesRemove(wxUpdateUIEvent& evt);
	void MustHaveTreeSelection(wxUpdateUIEvent& evt);
	void OnAddFav(wxCommandEvent& evt);
	void OnRemoveFav(wxCommandEvent& evt);
	void OnDescEdit(wxHtmlLinkEvent& evt);
	void OnListSel(wxListEvent& evt);

	void CreateMenus(void);
	void CreateChildWindows(void);
	void SelectItem(Omega::string_t strSelection);
	void SetKeyDescription(const wxTreeItemId& id);
	void SetValueDescription(const wxString& strSel, const wxString& strDesc);
};

#endif // OOREGEDIT_MAINFRAME_H_INCLUDED_
