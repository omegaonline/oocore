#include "StdAfx.h"
#include "./OORegEdit.h"
#include "./MainFrame.h"
#include "./TreeItemData.h"
#include "./AboutDlg.h"
#include "./FindDlg.h"
#include "./AddFavDlg.h"
#include "./RemoveFavDlg.h"

MainFrame::MainFrame(void) : wxFrame(NULL, wxID_ANY, _("Omega Online Registry Editor")),
	m_fileHistory(8)
{
	wxIconBundle icon_bundle;
	wxImage* pImage = wxGetApp().LoadImage(wxT("frame_icon.bmp"));
	if (pImage)
	{
		pImage->SetMaskColour(255,0,255);
		wxIcon icon;
		icon.CopyFromBitmap(wxBitmap(*pImage));
		icon_bundle.AddIcon(icon);
		delete pImage;
		
	}
	pImage = wxGetApp().LoadImage(wxT("frame_icon_small.bmp"));
	if (pImage)
	{
		pImage->SetMaskColour(255,0,255);
		wxIcon icon;
		icon.CopyFromBitmap(wxBitmap(*pImage));
		icon_bundle.AddIcon(icon);
		delete pImage;
	}

	SetIcons(icon_bundle);

	m_bKeys = false;
	m_bValues = false;
	m_bData = false;
	m_bMatchAll = false;
	m_bIgnoreCase = false;
	
	CreateStatusBar(1);
	CreateChildWindows();
	CreateMenus();
}

MainFrame::~MainFrame(void)
{
}

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	// Main events
    EVT_CLOSE(MainFrame::OnClose)
	EVT_CONTEXT_MENU(MainFrame::OnContextMenu)
	
	// Menu commands
    EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
	EVT_MENU(wxID_DELETE, MainFrame::OnDelete)
	EVT_UPDATE_UI(wxID_DELETE, MainFrame::MustHaveTreeSelection)
	EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
	EVT_MENU(ID_RENAME, MainFrame::OnRename)
	EVT_UPDATE_UI(ID_RENAME, MainFrame::MustHaveTreeSelection)
	EVT_MENU(ID_REFRESH, MainFrame::OnRefresh)
	EVT_MENU(ID_FIND, MainFrame::OnFind)
	EVT_MENU(ID_FIND_NEXT, MainFrame::OnFindNext)
	EVT_MENU(ID_MODIFY, MainFrame::OnModify)
	EVT_MENU(ID_NEW_KEY, MainFrame::OnNewKey)
	EVT_UPDATE_UI(ID_NEW_KEY, MainFrame::MustHaveTreeSelection)
	EVT_MENU(ID_NEW_STRING_VALUE, MainFrame::OnNewString)
	EVT_UPDATE_UI(ID_NEW_STRING_VALUE, MainFrame::MustHaveTreeSelection)
	EVT_MENU(ID_NEW_UINT32_VALUE, MainFrame::OnNewUInt)
	EVT_UPDATE_UI(ID_NEW_UINT32_VALUE, MainFrame::MustHaveTreeSelection)
	EVT_MENU(ID_NEW_BINARY_VALUE, MainFrame::OnNewBinary)
	EVT_UPDATE_UI(ID_NEW_BINARY_VALUE, MainFrame::MustHaveTreeSelection)
	EVT_MENU(ID_VIEW_STATUSBAR, MainFrame::OnViewStatusBar)
	EVT_MENU(ID_COPY_NAME, MainFrame::OnCopyName)
	EVT_MENU(ID_FAVOURITES_ADD, MainFrame::OnAddFav)
	EVT_MENU(ID_FAVOURITES_REMOVE, MainFrame::OnRemoveFav)
	EVT_UPDATE_UI(ID_COPY_NAME, MainFrame::MustHaveTreeSelection)
	EVT_UPDATE_UI(ID_VIEW_STATUSBAR, MainFrame::OnUpdateToggleStatusbar)
	EVT_UPDATE_UI(ID_MODIFY, MainFrame::OnUpdateModify)
	EVT_UPDATE_UI(ID_FIND_NEXT, MainFrame::OnUpdateFindNext)
	EVT_UPDATE_UI(ID_FAVOURITES_ADD, MainFrame::OnUpdateFavouritesAdd)
	EVT_UPDATE_UI(ID_FAVOURITES_REMOVE, MainFrame::OnUpdateFavouritesRemove)
	EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFavourites)

	// Tree events
	EVT_TREE_ITEM_EXPANDING(ID_TREE, MainFrame::OnItemExpanding)
	EVT_TREE_SEL_CHANGED(ID_TREE, MainFrame::OnTreeSelChanged)
	EVT_TREE_END_LABEL_EDIT(ID_TREE,MainFrame::OnTreeEndLabel)

	// List events
	EVT_LIST_END_LABEL_EDIT(ID_LIST,MainFrame::OnListEndLabel)
	EVT_LIST_ITEM_ACTIVATED(ID_LIST,MainFrame::OnListDblClk)
END_EVENT_TABLE()

void MainFrame::CreateChildWindows(void)
{
	Omega::uint32_t split_width = 100;
	Omega::uint32_t col_width[3] = { 100, 100, 100 };
	bool bShowBar = true;
	Omega::string_t strSelection;
		
	try
	{
		// get some defaults...
		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey("Current User\\Applications\\OORegEdit\\Layout");
		
		wxPoint ptPos;
		ptPos.x = ptrKey->GetUIntValue("Left");
		ptPos.y = ptrKey->GetUIntValue("Top");
		SetPosition(ptPos);
	
		wxSize sz;
		sz.x = ptrKey->GetUIntValue("Width");
		sz.y = ptrKey->GetUIntValue("Height");
		SetSize(sz);

        split_width = ptrKey->GetUIntValue("SplitWidth");

		col_width[0] = ptrKey->GetUIntValue("ColWidth0");
		col_width[1] = ptrKey->GetUIntValue("ColWidth1");
		col_width[2] = ptrKey->GetUIntValue("ColWidth2");

		bShowBar = ptrKey->GetUIntValue("Statusbar")!=0;

		strSelection = ptrKey->GetStringValue("Selection");

		m_bKeys = ptrKey->GetUIntValue("FindKeys")!=0;
		m_bValues = ptrKey->GetUIntValue("FindValues")!=0;
		m_bData = ptrKey->GetUIntValue("FindData")!=0;
		m_bMatchAll = ptrKey->GetUIntValue("MatchAll")!=0;
		m_bIgnoreCase = ptrKey->GetUIntValue("IgnoreCase")!=0;

		for (Omega::uint32_t nFiles = ptrKey->GetUIntValue("Favourites")-1;nFiles>=0;--nFiles)
		{
			Omega::string_t val = ptrKey->GetStringValue(Omega::string_t::Format(L"Favourite%u",nFiles));

			size_t pos = val.ReverseFind(L'\\');
			if (pos != Omega::string_t::npos)
			{
				wxString strName(val.Mid(pos+1));
				m_fileHistory.AddFileToHistory(strName);

				m_mapMRU.insert(std::map<wxString,Omega::string_t>::value_type(strName,val.Left(pos)));
			}
		}
	}
	catch (Omega::Registry::INotFoundException* e)
	{
		e->Release();
	}
	
	// Create the splitter
	m_pSplitter = new wxSplitterWindow(this);
    m_pSplitter->SetSashGravity(0.25);
	m_pSplitter->SetMinimumPaneSize(150);

	// Create the list and tree
	m_pList = new wxListCtrl(m_pSplitter,ID_LIST,wxPoint(0,0),wxSize(0,0),wxNO_BORDER | wxLC_SORT_ASCENDING | wxLC_REPORT | wxLC_NO_SORT_HEADER | wxLC_EDIT_LABELS);
	m_pTree = new wxTreeCtrl(m_pSplitter,ID_TREE,wxPoint(0,0),wxSize(0,0),wxNO_BORDER | wxTR_DEFAULT_STYLE | wxTR_SINGLE | wxTR_EDIT_LABELS); 

	// Load the imagelist
	wxImage* pImage = wxGetApp().LoadImage(wxT("imagelist.bmp"));
	if (pImage)
	{
		wxImageList* pImagelist = new wxImageList(16,16);
        pImagelist->Add(wxBitmap(*pImage),wxColor(255,0,255));

		m_pList->SetImageList(pImagelist,wxIMAGE_LIST_SMALL);
		m_pTree->AssignImageList(pImagelist);

		delete pImage;
	}

	m_pSplitter->SplitVertically(m_pTree, m_pList, split_width);

	// Init the list
	wxListItem itemCol; 
	itemCol.SetText(_("Name")); 
	m_pList->InsertColumn(0,itemCol); 
	m_pList->SetColumnWidth(0, col_width[0]); 
	itemCol.SetText(_("Type")); 
	m_pList->InsertColumn(1,itemCol); 
	m_pList->SetColumnWidth(1, col_width[1]); 
	itemCol.SetText(_("Data")); 
	m_pList->InsertColumn(2,itemCol);
	m_pList->SetColumnWidth(2, col_width[2]); 
		
	// Open the registry root
	OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey("\\");

	// Init the tree
	TreeItemData* pItem = new TreeItemData(ptrKey,5);
	wxTreeItemId tree_id = m_pTree->AddRoot(_("Local Computer"),0,0,pItem);
	pItem->Fill(m_pTree,tree_id);
	m_pTree->Expand(tree_id);

	GetStatusBar()->Show(bShowBar);

	SelectItem(strSelection);	
}

void MainFrame::SelectItem(Omega::string_t strSelection)
{
	SetCursor(*wxHOURGLASS_CURSOR);

	// Expand the tree to strSelection
	wxTreeItemId tree_id = m_pTree->GetRootItem();
	for (;;)
	{
		size_t pos = strSelection.Find(L'\\');
		Omega::string_t strSubKey;
		if (pos != -1)
		{
			strSubKey = strSelection.Left(pos);
			strSelection = strSelection.Mid(pos+1);
		}
		else
		{
			strSubKey = strSelection;
			strSelection.Clear();
		}

		m_pTree->Expand(tree_id);

		if (strSubKey == (const char*)m_pTree->GetItemText(tree_id))
			continue;

		if (m_pTree->ItemHasChildren(tree_id))
		{
			wxTreeItemIdValue cookie;
			wxTreeItemId id_child = m_pTree->GetFirstChild(tree_id,cookie);

			while (id_child && strSubKey != (const char*)m_pTree->GetItemText(id_child))
			{
				id_child = m_pTree->GetNextChild(tree_id,cookie);
			}

			if (!id_child)
			{
				break;
			}

			tree_id = id_child;
		}
		else
		{
			break;
		}
	}
	m_pTree->SelectItem(tree_id);

	SetCursor(*wxSTANDARD_CURSOR);
}

void MainFrame::CreateMenus(void)
{
	wxMenu* pRegistryMenu = new wxMenu;
	pRegistryMenu->Append(wxID_OPEN, _("&Import Registry File..."), _("Imports a text file into the registry."));
	pRegistryMenu->Enable(wxID_OPEN,false);
	pRegistryMenu->Append(ID_EXPORT, _("&Export Registry File..."), _("Exports all or part of the registry to a text file."));
	pRegistryMenu->Enable(ID_EXPORT,false);
	pRegistryMenu->AppendSeparator();
	pRegistryMenu->Append(ID_CONNECT, _("&Connect Remote Registry..."));
	pRegistryMenu->Enable(ID_CONNECT,false);
	pRegistryMenu->Append(ID_DISCONNECT, _("&Disconnect Remote Registry..."));
	pRegistryMenu->Enable(ID_DISCONNECT,false);
	pRegistryMenu->AppendSeparator();
	pRegistryMenu->Append(wxID_PRINT, _("&Print...\tCtrl+P"), _T("Prints all or part of the registry."));
	pRegistryMenu->Enable(wxID_PRINT,false);
	pRegistryMenu->AppendSeparator();
	pRegistryMenu->Append(wxID_EXIT, _("E&xit"), _("Quits the registry editor."));

	wxMenu* pEditMenu = new wxMenu;
	pEditMenu->Append(ID_MODIFY, _("&Modify"), _("Modifies the value's data"));
	pEditMenu->AppendSeparator();

	wxMenu* pNewMenu = new wxMenu;
	pNewMenu->Append(ID_NEW_KEY, _("&Key"), _("Adds a new key."));
	pNewMenu->AppendSeparator();
	pNewMenu->Append(ID_NEW_STRING_VALUE, _("&String Value"), _("Adds a new string value."));
	pNewMenu->Append(ID_NEW_BINARY_VALUE, _("&Binary Value"), _("Adds a new binary value."));
	pNewMenu->Append(ID_NEW_UINT32_VALUE, _("&UInt32 Value"), _("Adds a new UInt32 value."));

	pEditMenu->Append(ID_NEW, _("&New"), pNewMenu, _("Contains commands for creating new keys or values."));
	pEditMenu->AppendSeparator();
	pEditMenu->Append(wxID_DELETE, _("&Delete\tDel"), _("Deletes the selection."));
	pEditMenu->Append(ID_RENAME, _("&Rename"), _("Renames the selection."));
	pEditMenu->AppendSeparator();
	pEditMenu->Append(ID_COPY_NAME, _("&Copy Key Name"), _("Copies the name of the selected key to the Clipboard."));
	pEditMenu->AppendSeparator();
	pEditMenu->Append(ID_FIND, _("&Find...\tCtrl+F"), _("Finds a text string in a key, value, or data."));
	pEditMenu->Append(ID_FIND_NEXT, _("Find Ne&xt\tF3"), _("Finds the next occurrence of text specified in a previous search."));
	
	wxMenu* pViewMenu = new wxMenu;
	pViewMenu->AppendCheckItem(ID_VIEW_STATUSBAR, _("Status &Bar"), _("Shows or hides the status bar."));
	pViewMenu->AppendSeparator();
	pViewMenu->Append(ID_REFRESH, _("&Refresh\tF5"), _("Refreshes the window."));

	wxMenu* pFavMenu = new wxMenu;
	pFavMenu->Append(ID_FAVOURITES_ADD, _("&Add to Favourites"), _("Adds keys to the Favourites list."));
	pFavMenu->Append(ID_FAVOURITES_REMOVE, _("&Remove Favourite"), _("Removes keys from the Favourites list"));
	m_fileHistory.UseMenu(pFavMenu);
	m_fileHistory.AddFilesToMenu();

	wxMenu* pHelpMenu = new wxMenu;
	pHelpMenu->Append(wxID_HELP_CONTENTS, _("&Help Topics"), _("Opens Omega Online Registry Editor Help."));
	pHelpMenu->AppendSeparator();
	pHelpMenu->Append(wxID_ABOUT, _("&About Omega Online Registry Editor"), _("Displays program information, version and copyright."));

    wxMenuBar* pMainMenu = new wxMenuBar;
    pMainMenu->Append(pRegistryMenu, _("&Registry"));
	pMainMenu->Append(pEditMenu, _("&Edit"));
	pMainMenu->Append(pViewMenu, _("&View"));
	pMainMenu->Append(pFavMenu, _("&Favourites"));
	pMainMenu->Append(pHelpMenu, _("&Help"));

    SetMenuBar(pMainMenu);
}

void MainFrame::OnQuit(wxCommandEvent& WXUNUSED(evt))
{
    Close(true);
}

void MainFrame::OnClose(wxCloseEvent& WXUNUSED(evt))
{
	// Set some defaults...
	OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey("Current User\\Applications\\OORegEdit\\Layout",Omega::Registry::IRegistryKey::Create);

	wxPoint pt = GetPosition();
	ptrKey->SetUIntValue("Top",pt.y);
	ptrKey->SetUIntValue("Left",pt.x);

	wxSize sz = GetSize();
	ptrKey->SetUIntValue("Height",sz.y);
	ptrKey->SetUIntValue("Width",sz.x);

	ptrKey->SetUIntValue("SplitWidth",m_pSplitter->GetSashPosition());

	ptrKey->SetUIntValue("ColWidth0",m_pList->GetColumnWidth(0));
	ptrKey->SetUIntValue("ColWidth1",m_pList->GetColumnWidth(1));
	ptrKey->SetUIntValue("ColWidth2",m_pList->GetColumnWidth(2));

	ptrKey->SetUIntValue("Statusbar",GetStatusBar()->IsShown() ? 1 : 0);

    ptrKey->SetUIntValue("FindKeys",m_bKeys ? 1 : 0);
	ptrKey->SetUIntValue("FindValues",m_bValues ? 1 : 0);
	ptrKey->SetUIntValue("FindData",m_bData ? 1 : 0);
	ptrKey->SetUIntValue("MatchAll",m_bMatchAll ? 1 : 0);
	ptrKey->SetUIntValue("IgnoreCase",m_bIgnoreCase? 1 : 0);

	ptrKey->SetStringValue("Selection",Omega::string_t(GetStatusBar()->GetStatusText()));

	Omega::uint32_t nFiles = static_cast<Omega::uint32_t>(m_fileHistory.GetCount());
	ptrKey->SetUIntValue("Favourites",nFiles);

	for (nFiles;nFiles>0;--nFiles)
	{
		wxString strName = m_fileHistory.GetHistoryFile(nFiles-1);

		Omega::string_t strVal = m_mapMRU[strName] + "\\" + Omega::string_t(strName);

		ptrKey->SetStringValue(Omega::string_t::Format(L"Favourite%u",nFiles-1),strVal);
	}
	
	Destroy();
}

void MainFrame::OnContextMenu(wxContextMenuEvent& evt)
{
	if (evt.GetId()==ID_LIST)
	{
		wxPoint pt = m_pList->ScreenToClient(evt.GetPosition());

		int flags = 0;
		long list_id = m_pList->HitTest(pt,flags);

		pt = evt.GetPosition();

		if (list_id == wxNOT_FOUND && pt.x==-1 && pt.y==-1 && m_pList->GetSelectedItemCount()>0)
		{
			list_id = m_pList->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);

			m_pList->GetItemPosition(list_id,pt);
			wxRect rc;
			m_pList->GetItemRect(list_id,rc,wxLIST_RECT_LABEL);

			pt.x += rc.GetWidth()/2;
			pt.y += rc.GetHeight()/2;

			pt = m_pList->ClientToScreen(pt);
		}
		else if (pt.x==-1 && pt.y==-1)
		{
			pt = m_pList->ClientToScreen(pt);
		}

		pt = ScreenToClient(pt);

		wxMenu menu;

		if (list_id != wxNOT_FOUND)
		{
			menu.Append(ID_MODIFY, _("&Modify"), _("Modifies the value's data"));
			menu.AppendSeparator();
			menu.Append(wxID_DELETE, _("&Delete"), _("Deletes the selection."));
			menu.Append(ID_RENAME, _("&Rename"), _("Renames the selection."));
		}
		else
		{
			wxMenu* pNewMenu = new wxMenu;
			pNewMenu->Append(ID_NEW_KEY, _("&Key"), _("Adds a new key."));
			pNewMenu->AppendSeparator();
			pNewMenu->Append(ID_NEW_STRING_VALUE, _("&String Value"), _("Adds a new string value."));
			pNewMenu->Append(ID_NEW_BINARY_VALUE, _("&Binary Value"), _("Adds a new binary value."));
			pNewMenu->Append(ID_NEW_UINT32_VALUE, _("&UInt32 Value"), _("Adds a new UInt32 value."));
			menu.Append(ID_NEW, _("&New"), pNewMenu, _("Contains commands for creating new keys or values."));
		}

		PopupMenu(&menu,pt);
	}
	else if (evt.GetId()==ID_TREE)
	{
		wxPoint pt = m_pTree->ScreenToClient(evt.GetPosition());

		int flags = 0;
		wxTreeItemId tree_id = m_pTree->HitTest(pt,flags);

		pt = evt.GetPosition();

		if (!tree_id)
		{
			tree_id = m_pTree->GetSelection();
			if (!tree_id)
				return;

			wxRect rc;
			m_pTree->GetBoundingRect(tree_id,rc,true);

			pt.x  = rc.GetLeft() + rc.GetWidth()/2;
			pt.y  = rc.GetTop() + rc.GetHeight()/2;

			pt = m_pTree->ClientToScreen(pt);
		}

		m_pTree->SetFocus();

		pt = ScreenToClient(pt);

		wxMenu* pNewMenu = new wxMenu;
		pNewMenu->Append(ID_NEW_KEY, _("&Key"), _("Adds a new key."));
		pNewMenu->AppendSeparator();
		pNewMenu->Append(ID_NEW_STRING_VALUE, _("&String Value"), _("Adds a new string value."));
		pNewMenu->Append(ID_NEW_BINARY_VALUE, _("&Binary Value"), _("Adds a new binary value."));
		pNewMenu->Append(ID_NEW_UINT32_VALUE, _("&UInt32 Value"), _("Adds a new UInt32 value."));

		wxMenu menu;
		menu.Append(ID_NEW, _("&New"), pNewMenu, _("Contains commands for creating new keys or values."));
		menu.Append(ID_FIND, _("&Find..."), _("Finds a text string in a key, value, or data."));
		menu.AppendSeparator();
		menu.Append(wxID_DELETE, _("&Delete"), _("Deletes the selection."));
		menu.Append(ID_RENAME, _("&Rename"), _("Renames the selection."));
		menu.AppendSeparator();
		menu.Append(ID_COPY_NAME, _("&Copy Key Name"), _("Copies the name of the selected key to the Clipboard."));

		PopupMenu(&menu,pt);
	}
}

void MainFrame::OnAbout(wxCommandEvent& WXUNUSED(evt))
{
	AboutDlg dialog(this,-1,wxT(""));
	dialog.ShowModal();
}

void MainFrame::OnRename(wxCommandEvent& WXUNUSED(evt))
{
	wxWindow* pFocus = FindFocus();

	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return;

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return;

	if (pFocus==m_pTree)
	{
		if (tree_id!=m_pTree->GetRootItem())
		{
			m_pTree->EditLabel(tree_id);
		}
	}
	else if (pFocus==m_pList)
	{
		if (m_pList->GetSelectedItemCount()>0)
		{
			long item = m_pList->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);
			m_pList->EditLabel(item);
		}
	}
}

void MainFrame::OnTreeEndLabel(wxTreeEvent& evt)
{
	if (evt.IsEditCancelled())
		return;
	
	wxTreeItemId tree_id = m_pTree->GetItemParent(evt.GetItem());
	if (!tree_id)
		return evt.Veto();

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return evt.Veto();

	wxString strOld = m_pTree->GetItemText(evt.GetItem());

	SetCursor(*wxHOURGLASS_CURSOR);
	try
	{
		pItem->RenameKey(Omega::string_t(strOld),Omega::string_t(evt.GetLabel()),(TreeItemData*)m_pTree->GetItemData(evt.GetItem()));
	}
	catch (Omega::Registry::IAlreadyExistsException* pE)
	{
		pE->Release();

		wxMessageBox(wxString::Format(_("Cannot rename %s: The specified value name already exists. Type another name and try again."),strOld),_("Error Renaming Value"),wxOK|wxICON_ERROR,this);
		evt.Veto();
		m_pTree->EditLabel(evt.GetItem());
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		pE->Release();

		wxMessageBox(wxString::Format(_("Cannot rename %s: The specified value name is invalid. Type another name and try again."),strOld),_("Error Renaming Value"),wxOK|wxICON_ERROR,this);
		evt.Veto();
		m_pTree->EditLabel(evt.GetItem());
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		pE->Release();

		wxMessageBox(wxString::Format(_("Cannot rename %s: You do not have permission to edit this part of the registry."),strOld),_("Error Renaming Value"),wxOK|wxICON_ERROR,this);
		evt.Veto();
		m_pTree->EditLabel(evt.GetItem());
	}
	catch(...)
	{
		SetCursor(*wxSTANDARD_CURSOR);
		evt.Veto();
		throw;
	}
	SetCursor(*wxSTANDARD_CURSOR);

	pItem = (TreeItemData*)m_pTree->GetItemData(evt.GetItem());
	pItem->Refresh(m_pList,m_pTree,evt.GetItem());
}

void MainFrame::OnListEndLabel(wxListEvent& evt)
{
	if (evt.IsEditCancelled())
		return;
	
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return evt.Veto();

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return evt.Veto();

	wxString strOld = m_pList->GetItemText(evt.GetIndex());

	try
	{
		if (!pItem->RenameValue(Omega::string_t(strOld),Omega::string_t(evt.GetLabel())))
		{
			wxMessageBox(wxString::Format(_("Cannot rename %s: The specified value name already exists. Type another name and try again."),strOld),_("Error Renaming Value"),wxOK|wxICON_ERROR,this);
			evt.Veto();
		}
	}
	catch (Omega::Registry::IAlreadyExistsException* pE)
	{
		pE->Release();

		wxMessageBox(wxString::Format(_("Cannot rename %s: The specified value name already exists. Type another name and try again."),strOld),_("Error Renaming Value"),wxOK|wxICON_ERROR,this);
		evt.Veto();
	}
	catch (Omega::Registry::IBadNameException* pE)
	{
		pE->Release();

		wxMessageBox(wxString::Format(_("Cannot rename %s: The specified value name is invalid. Type another name and try again."),strOld),_("Error Renaming Value"),wxOK|wxICON_ERROR,this);
		evt.Veto();
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		pE->Release();

		wxMessageBox(wxString::Format(_("Cannot rename %s: You do not have permission to edit this part of the registry."),strOld),_("Error Renaming Value"),wxOK|wxICON_ERROR,this);
		evt.Veto();
	}
	catch(...)
	{
		evt.Veto();
		throw;
	}
}

void MainFrame::OnDelete(wxCommandEvent& WXUNUSED(evt))
{
	wxWindow* pFocus = FindFocus();

	if (pFocus==m_pTree)
	{
		wxTreeItemId sel_id = m_pTree->GetSelection();
		if (sel_id && sel_id!=m_pTree->GetRootItem())
		{
			wxTreeItemId tree_id = m_pTree->GetItemParent(sel_id);
			if (!tree_id)
				return;

			TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
			if (!pItem)
				return;

			if (wxMessageBox(_("Are you sure you want to delete this key?"),_("Confirm Key Delete"),wxYES_DEFAULT|wxYES_NO|wxICON_WARNING,this) == wxYES)
			{		
				try
				{
					pItem->DeleteKey(Omega::string_t(m_pTree->GetItemText(sel_id)));
					m_pTree->Delete(sel_id);
				}
				catch (Omega::Registry::IAccessDeniedException* pE)
				{
					pE->Release();

					wxMessageBox(_("You do not have permission to delete this key."),_("Access Denied"),wxOK|wxICON_ERROR,this);
				}
			}
		}
	}
	else if (pFocus==m_pList)
	{
        if (m_pList->GetSelectedItemCount()>0)
		{
			wxTreeItemId tree_id = m_pTree->GetSelection();
			if (!tree_id)
				return;

			TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
			if (!pItem)
				return;

			int val;
			if (m_pList->GetSelectedItemCount()>1)
				val = wxMessageBox(_("Are you sure you want to delete these values?"),_("Confirm Value Delete"),wxYES_DEFAULT|wxYES_NO|wxICON_WARNING,this);
			else
				val = wxMessageBox(_("Are you sure you want to delete this value?"),_("Confirm Value Delete"),wxYES_DEFAULT|wxYES_NO|wxICON_WARNING,this);

			if (val == wxYES)
			{
				try
				{
					long item;
					while ((item=m_pList->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) != -1)
					{
						pItem->DeleteValue(Omega::string_t(m_pList->GetItemText(item)));
						m_pList->DeleteItem(item);
					}
				}
				catch (Omega::Registry::IAccessDeniedException* pE)
				{
					pE->Release();

					wxMessageBox(_("You do not have permission to edit this part of the registry."),_("Access Denied"),wxOK|wxICON_ERROR,this);
				}
			}
		}
	}
}

void MainFrame::OnCopyName(wxCommandEvent& WXUNUSED(evt))
{
	if (wxTheClipboard->Open())
	{
		// Data objects are held by the clipboard,
		// so do not delete them in the app.
		wxTheClipboard->SetData(new wxTextDataObject(GetStatusBar()->GetStatusText()));
		wxTheClipboard->Close();
	}
}

void MainFrame::OnNewKey(wxCommandEvent& WXUNUSED(evt))
{
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return;

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return;

	try
	{
		pItem->NewKey(m_pTree,tree_id);
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		pE->Release();

		wxMessageBox(_("You do not have permission to edit this part of the registry."),_("Access Denied"),wxOK|wxICON_ERROR,this);
	}
}

void MainFrame::OnNewString(wxCommandEvent& WXUNUSED(evt))
{
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return;

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return;

	try
	{
		pItem->NewString(m_pList);
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		pE->Release();

		wxMessageBox(_("You do not have permission to edit this part of the registry."),_("Access Denied"),wxOK|wxICON_ERROR,this);
	}
}

void MainFrame::OnNewUInt(wxCommandEvent& WXUNUSED(evt))
{
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return;

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return;

	try
	{
		pItem->NewUInt(m_pList);
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		pE->Release();

		wxMessageBox(_("You do not have permission to edit this part of the registry."),_("Access Denied"),wxOK|wxICON_ERROR,this);
	}
}

void MainFrame::OnNewBinary(wxCommandEvent& WXUNUSED(evt))
{
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return;

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return;

	try
	{
		pItem->NewBinary(m_pList);
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		pE->Release();

		wxMessageBox(_("You do not have permission to edit this part of the registry."),_("Access Denied"),wxOK|wxICON_ERROR,this);
	}
}

void MainFrame::OnRefresh(wxCommandEvent& WXUNUSED(evt))
{
	SetCursor(*wxHOURGLASS_CURSOR);

	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (tree_id)
	{
		TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
		if (pItem)
			pItem->Refresh(m_pList,m_pTree,tree_id);
	}	

	SetCursor(*wxSTANDARD_CURSOR);
}

void MainFrame::OnUpdateModify(wxUpdateUIEvent& evt)
{
	bool bEnable = false;
	if (FindFocus()==m_pList)
	{
        if (m_pList->GetSelectedItemCount()>0)
		{
			wxTreeItemId tree_id = m_pTree->GetSelection();
			if (tree_id)
			{
				TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
				if (pItem)
				{
					bEnable = true;
				}
			}
		}
	}

	evt.Enable(bEnable);
}

void MainFrame::OnModify(wxCommandEvent& WXUNUSED(evt))
{
	if (FindFocus()==m_pList)
	{
        if (m_pList->GetSelectedItemCount()>0)
		{
			wxTreeItemId tree_id = m_pTree->GetSelection();
			if (!tree_id)
				return;

			TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
			if (!pItem)
				return;

			long item = m_pList->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);

			try
			{
				pItem->Modify(m_pList,item);		
			}
			catch (Omega::Registry::IAccessDeniedException* pE)
			{
				pE->Release();

				wxMessageBox(_("You do not have permission to edit this part of the registry."),_("Access Denied"),wxOK|wxICON_ERROR,this);
			}
		}
	}
}

void MainFrame::OnViewStatusBar(wxCommandEvent& WXUNUSED(evt))
{
	wxStatusBar* pBar = GetStatusBar();
	pBar->Show(!pBar->IsShown());

	SendSizeEvent();
}

void MainFrame::OnUpdateToggleStatusbar(wxUpdateUIEvent& evt)
{
	evt.Check(GetStatusBar()->IsShown());
}

void MainFrame::OnFind(wxCommandEvent& evt)
{
	FindDlg dialog(this,-1,wxT(""));

	dialog.m_strFind = m_strFind;
	dialog.m_bKeys = m_bKeys;
	dialog.m_bValues = m_bValues;
	dialog.m_bData = m_bData;
	dialog.m_bMatchAll = m_bMatchAll;
	dialog.m_bIgnoreCase = m_bIgnoreCase;

	if (dialog.ShowModal() == wxID_OK)
	{
		m_strFind = dialog.m_strFind;
		m_bKeys = dialog.m_bKeys;
		m_bValues = dialog.m_bValues;
		m_bData = dialog.m_bData;
		m_bMatchAll = dialog.m_bMatchAll;
		m_bIgnoreCase = dialog.m_bIgnoreCase;

		OnFindNext(evt);
	}
}

void MainFrame::OnFindNext(wxCommandEvent& WXUNUSED(evt))
{
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		tree_id = m_pTree->GetRootItem();

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return;

	SetCursor(*wxHOURGLASS_CURSOR);

	pItem->Find(m_pTree,tree_id,m_pList,m_pList->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED),Omega::string_t(m_strFind),m_bKeys,m_bValues,m_bData,m_bMatchAll,m_bIgnoreCase);

	SetCursor(*wxSTANDARD_CURSOR);
}

void MainFrame::OnUpdateFindNext(wxUpdateUIEvent& evt)
{
	evt.Enable(!m_strFind.IsEmpty());
}

void MainFrame::OnItemExpanding(wxTreeEvent& evt)
{
	SetCursor(*wxHOURGLASS_CURSOR);

	wxTreeItemId item_id = evt.GetItem();

	if (m_pTree->GetChildrenCount(item_id,false) == 1)
	{
		wxTreeItemIdValue cookie;
		wxTreeItemId child_id = m_pTree->GetFirstChild(item_id,cookie);

		if (m_pTree->GetItemData(child_id) == NULL)
		{
			m_pTree->Delete(child_id);

			static_cast<TreeItemData*>(m_pTree->GetItemData(item_id))->Fill(m_pTree,item_id);
		}
	}

	SetCursor(*wxSTANDARD_CURSOR);
}

void MainFrame::OnTreeSelChanged(wxTreeEvent& evt)
{
	SetCursor(*wxHOURGLASS_CURSOR);

	wxTreeItemId item_id = evt.GetItem();
	wxString strText;
	while (item_id)
	{
		if (item_id != m_pTree->GetRootItem())
			strText = m_pTree->GetItemText(item_id) + (strText.IsEmpty() ? wxT("") : wxT("\\")) + strText;
	
		item_id = m_pTree->GetItemParent(item_id);
	}
	GetStatusBar()->SetStatusText(strText);

	if (evt.GetItem())
		static_cast<TreeItemData*>(m_pTree->GetItemData(evt.GetItem()))->InitList(m_pList);	

	SetCursor(*wxSTANDARD_CURSOR);
}

void MainFrame::OnListDblClk(wxListEvent& evt)
{
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return;

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return;

	try
	{
		pItem->Modify(m_pList,evt.GetIndex());
	}
	catch (Omega::Registry::IAccessDeniedException* pE)
	{
		pE->Release();

		wxMessageBox(_("You do not have permission to edit this part of the registry."),_("Access Denied"),wxOK|wxICON_ERROR,this);
	}
}

void MainFrame::OnMRUFavourites(wxCommandEvent& event)
{
	wxString strFav(m_fileHistory.GetHistoryFile(event.GetId() - wxID_FILE1));
    if (!strFav.IsEmpty())
	{
		std::map<wxString,Omega::string_t>::const_iterator i=m_mapMRU.find(strFav);
		if (i!=m_mapMRU.end())
		{
			SelectItem(i->second);
			return;
		}
	}

	m_fileHistory.RemoveFileFromHistory(event.GetId() - wxID_FILE1);
}

void MainFrame::MustHaveTreeSelection(wxUpdateUIEvent& evt)
{
	evt.Enable(m_pTree->GetSelection());
}

void MainFrame::OnAddFav(wxCommandEvent& evt)
{
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return;

	AddFavDlg dialog(this,-1,wxT(""));

	dialog.m_strName = m_pTree->GetItemText(tree_id);

	if (dialog.ShowModal() == wxID_OK)
	{
		m_mapMRU.insert(std::map<wxString,Omega::string_t>::value_type(dialog.m_strName,Omega::string_t(GetStatusBar()->GetStatusText())));
		m_fileHistory.AddFileToHistory(dialog.m_strName);
	}
}

void MainFrame::OnRemoveFav(wxCommandEvent& evt)
{
	RemoveFavDlg dialog(this,&m_fileHistory);

	if (dialog.ShowModal() == wxID_OK)
	{
		for (size_t i=0;i<dialog.m_selections.GetCount();++i)
		{
			int index = dialog.m_selections.Item(i);

			m_mapMRU.erase(m_fileHistory.GetHistoryFile(index));
			m_fileHistory.RemoveFileFromHistory(index);
		}
	}
}

void MainFrame::OnUpdateFavouritesAdd(wxUpdateUIEvent& evt)
{
	evt.Enable(m_fileHistory.GetCount()<(size_t)m_fileHistory.GetMaxFiles() && m_pTree->GetSelection());
}

void MainFrame::OnUpdateFavouritesRemove(wxUpdateUIEvent& evt)
{
	evt.Enable(m_fileHistory.GetCount() > 0);
}
