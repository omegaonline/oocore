#include "StdAfx.h"
#include "./OORegEdit.h"
#include "./MainFrame.h"
#include "./TreeItemData.h"
#include "./AboutDlg.h"
#include "./FindDlg.h"
#include "./AddFavDlg.h"
#include "./RemoveFavDlg.h"
#include "./EditKeyDescDlg.h"
#include "./EditValueDescDlg.h"

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
	EVT_MENU(ID_COPY_NAME, MainFrame::OnCopyName)
	EVT_MENU(ID_FAVOURITES_ADD, MainFrame::OnAddFav)
	EVT_MENU(ID_FAVOURITES_REMOVE, MainFrame::OnRemoveFav)
	EVT_UPDATE_UI(ID_COPY_NAME, MainFrame::MustHaveTreeSelection)
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
	EVT_LIST_ITEM_SELECTED(ID_LIST,MainFrame::OnListSel)

	// HTML events
	EVT_HTML_LINK_CLICKED(ID_DESC,MainFrame::OnDescEdit)
END_EVENT_TABLE()

void MainFrame::CreateChildWindows(void)
{
	Omega::uint32_t split_width = 100;
	Omega::uint32_t split_width2 = 100;
	Omega::uint32_t col_width[3] = { 100, 100, 100 };
	Omega::string_t strSelection;
		
	try
	{
		// get some defaults...
		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey(L"\\Local User\\Applications\\OORegEdit\\Layout");
		
		wxPoint ptPos;
		ptPos.x = ptrKey->GetIntegerValue(L"Left");
		ptPos.y = ptrKey->GetIntegerValue(L"Top");
		SetPosition(ptPos);
	
		wxSize sz;
		sz.x = ptrKey->GetIntegerValue(L"Width");
		sz.y = ptrKey->GetIntegerValue(L"Height");
		SetSize(sz);

        split_width = ptrKey->GetIntegerValue(L"SplitWidth");
		split_width2 = ptrKey->GetIntegerValue(L"SplitWidth2");

		col_width[0] = ptrKey->GetIntegerValue(L"ColWidth0");
		col_width[1] = ptrKey->GetIntegerValue(L"ColWidth1");
		col_width[2] = ptrKey->GetIntegerValue(L"ColWidth2");

		strSelection = ptrKey->GetStringValue(L"Selection");

		m_bKeys = ptrKey->GetIntegerValue(L"FindKeys")!=0;
		m_bValues = ptrKey->GetIntegerValue(L"FindValues")!=0;
		m_bData = ptrKey->GetIntegerValue(L"FindData")!=0;
		m_bMatchAll = ptrKey->GetIntegerValue(L"MatchAll")!=0;
		m_bIgnoreCase = ptrKey->GetIntegerValue(L"IgnoreCase")!=0;

		for (int nFiles = (int)ptrKey->GetIntegerValue(L"Favourites")-1;nFiles>=0;--nFiles)
		{
			Omega::string_t val = ptrKey->GetStringValue(Omega::string_t::Format(L"Favourite%u",nFiles));

			size_t pos = val.ReverseFind(L'\\');
			if (pos != Omega::string_t::npos)
			{
				wxString strName(val.Mid(pos+1).c_str());
				m_fileHistory.AddFileToHistory(strName);

				m_mapMRU.insert(std::map<wxString,Omega::string_t>::value_type(strName,val.Left(pos)));
			}
		}
	}
	catch (Omega::IException* e)
	{
		e->Release();
	}

	// Create the splitter
	m_pSplitter2 = new wxSplitterWindow(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxNO_BORDER | wxSP_LIVE_UPDATE);
    m_pSplitter2->SetSashGravity(0.90);
	m_pSplitter2->SetMinimumPaneSize(50);
	
	// Create the second splitter
	m_pSplitter = new wxSplitterWindow(m_pSplitter2,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxNO_BORDER | wxSP_LIVE_UPDATE);
    m_pSplitter->SetSashGravity(0.25);
	m_pSplitter->SetMinimumPaneSize(150);

	// Create the list and tree
	m_pList = new wxListCtrl(m_pSplitter,ID_LIST,wxPoint(0,0),wxSize(0,0),wxLC_SORT_ASCENDING | wxLC_REPORT | wxLC_NO_SORT_HEADER | wxLC_EDIT_LABELS);
	m_pTree = new wxTreeCtrl(m_pSplitter,ID_TREE,wxPoint(0,0),wxSize(0,0),wxTR_DEFAULT_STYLE | wxTR_SINGLE | wxTR_EDIT_LABELS); 

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

	// Create the description text field
	m_pDescription = new wxHtmlWindow(m_pSplitter2,ID_DESC,wxPoint(0,0),wxSize(0,0),wxBORDER_SUNKEN | wxHW_SCROLLBAR_NEVER);
	wxFont ft = m_pTree->GetFont();
	m_pDescription->SetFonts(ft.GetFaceName(),wxT(""));
	m_pDescription->SetBorders(1);
		
	m_pSplitter->SplitVertically(m_pTree, m_pList, split_width);
	m_pSplitter2->SplitHorizontally(m_pSplitter, m_pDescription, split_width2);

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
		
	try
	{
		// Open the registry root
		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey(L"\\");

		// Init the tree
		TreeItemData* pItem = new TreeItemData(ptrKey,5);
		wxTreeItemId tree_id = m_pTree->AddRoot(_("Local Computer"),0,0,pItem);
		pItem->Fill(m_pTree,tree_id);
		m_pTree->Expand(tree_id);

		SelectItem(strSelection);
	}
	catch (Omega::IException* pE)
	{
		wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
		pE->Release();
	}
}

void MainFrame::SelectItem(Omega::string_t strSelection)
{
	SetCursor(*wxHOURGLASS_CURSOR);

	if (strSelection.Left(1) == L"\\")
		strSelection = strSelection.Mid(1);

	// Expand the tree to strSelection
	wxTreeItemId tree_id = m_pTree->GetRootItem();
	for (;;)
	{
		size_t pos = strSelection.Find(L'\\');
		wxString strSubKey;
		if (pos != -1)
		{
			strSubKey = strSelection.Left(pos).c_str();
			strSelection = strSelection.Mid(pos+1);
		}
		else
		{
			strSubKey = strSelection.c_str();
			strSelection.Clear();
		}

		m_pTree->Expand(tree_id);

		if (strSubKey == m_pTree->GetItemText(tree_id))
			continue;

		if (m_pTree->ItemHasChildren(tree_id))
		{
			wxTreeItemIdValue cookie;
			wxTreeItemId id_child = m_pTree->GetFirstChild(tree_id,cookie);

			while (id_child && strSubKey != m_pTree->GetItemText(id_child))
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
	pNewMenu->Append(ID_NEW_UINT32_VALUE, _("&Integer Value"), _("Adds a new integer value."));

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
	try
	{
		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey(L"\\Local User\\Applications\\OORegEdit\\Layout",Omega::Registry::IRegistryKey::Create);

		wxPoint pt = GetPosition();
		ptrKey->SetIntegerValue(L"Top",pt.y);
		ptrKey->SetIntegerValue(L"Left",pt.x);

		wxSize sz = GetSize();
		ptrKey->SetIntegerValue(L"Height",sz.y);
		ptrKey->SetIntegerValue(L"Width",sz.x);

		ptrKey->SetIntegerValue(L"SplitWidth",m_pSplitter->GetSashPosition());
		ptrKey->SetIntegerValue(L"SplitWidth2",m_pSplitter2->GetSashPosition());

		ptrKey->SetIntegerValue(L"ColWidth0",m_pList->GetColumnWidth(0));
		ptrKey->SetIntegerValue(L"ColWidth1",m_pList->GetColumnWidth(1));
		ptrKey->SetIntegerValue(L"ColWidth2",m_pList->GetColumnWidth(2));

		ptrKey->SetIntegerValue(L"FindKeys",m_bKeys ? 1 : 0);
		ptrKey->SetIntegerValue(L"FindValues",m_bValues ? 1 : 0);
		ptrKey->SetIntegerValue(L"FindData",m_bData ? 1 : 0);
		ptrKey->SetIntegerValue(L"MatchAll",m_bMatchAll ? 1 : 0);
		ptrKey->SetIntegerValue(L"IgnoreCase",m_bIgnoreCase? 1 : 0);

		ptrKey->SetStringValue(L"Selection",Omega::string_t(m_strSelection));

		Omega::uint32_t nFiles = static_cast<Omega::uint32_t>(m_fileHistory.GetCount());
		ptrKey->SetIntegerValue(L"Favourites",nFiles);

		for (nFiles;nFiles>0;--nFiles)
		{
			wxString strName = m_fileHistory.GetHistoryFile(nFiles-1);

			Omega::string_t strVal = m_mapMRU[strName] + "\\" + Omega::string_t(strName);

			ptrKey->SetStringValue(Omega::string_t::Format(L"Favourite%u",nFiles-1),strVal);
		}
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
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
			pNewMenu->Append(ID_NEW_UINT32_VALUE, _("&Integer Value"), _("Adds a new integer value."));
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
		pNewMenu->Append(ID_NEW_UINT32_VALUE, _("&Integer Value"), _("Adds a new integer value."));

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
	catch (Omega::IException* pE)
	{
		wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
		pE->Release();
		SetCursor(*wxSTANDARD_CURSOR);
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
	catch (Omega::IException* pE)
	{
		wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
		pE->Release();
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
				catch (Omega::IException* pE)
				{
                    wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
					pE->Release();
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
				catch (Omega::IException* pE)
				{
					wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
					pE->Release();
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
		wxTheClipboard->SetData(new wxTextDataObject(m_strSelection));
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
	catch (Omega::IException* pE)
	{
		wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
		pE->Release();
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
	catch (Omega::IException* pE)
	{
		wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
		pE->Release();
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
	catch (Omega::IException* pE)
	{
		wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
		pE->Release();
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
	catch (Omega::IException* pE)
	{
		wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
		pE->Release();
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
			catch (Omega::IException* pE)
			{
				wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
				pE->Release();
			}
		}
	}
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
			strText = wxT("\\") + m_pTree->GetItemText(item_id) + strText;
	
		item_id = m_pTree->GetItemParent(item_id);
	}
	if (strText.IsEmpty())
		strText = wxT("\\");

	m_strSelection = strText;

	SetKeyDescription(evt.GetItem());

	SetCursor(*wxSTANDARD_CURSOR);
}

void MainFrame::SetKeyDescription(const wxTreeItemId& id)
{
	wxString strHTML = wxT("<html><body><table><tr><td align=\"right\"><b>Key:</b></td><td>");
	strHTML += m_strSelection;
	strHTML += wxT("</td></tr><tr><td align=\"right\"><b>Description:</b></td><td>");
		
	if (id.IsOk())
	{
		TreeItemData* pItem = static_cast<TreeItemData*>(m_pTree->GetItemData(id));
		pItem->InitList(m_pList);
		wxString strDesc = pItem->GetDesc();
		if (!strDesc.IsEmpty())
			strHTML += strDesc + wxT(" <i><a href=\"edit_key\">edit...</a></i>");
		else if (m_strSelection != wxT("\\"))
			strHTML += wxT("<i><a href=\"edit_key\">Add...</a></i>");
	}

	strHTML += wxT("</td></tr></table></body></html>");
	m_pDescription->SetPage(strHTML);
}

void MainFrame::OnListSel(wxListEvent& evt)
{
	wxTreeItemId tree_id = m_pTree->GetSelection();
	if (!tree_id)
		return;

	TreeItemData* pItem = (TreeItemData*)m_pTree->GetItemData(tree_id);
	if (!pItem)
		return;

	wxString strSel = evt.GetText();
	SetValueDescription(strSel,pItem->GetValueDesc(strSel));
}

void MainFrame::SetValueDescription(const wxString& strSel, const wxString& strDesc)
{
	wxString strHTML = wxT("<html><body><table><tr><td align=\"right\"><b>Key:</b></td><td>");
	strHTML += m_strSelection;
	strHTML += wxT("</td><td align=\"right\"><b>Value:</b></td><td>");
	strHTML += strSel;
	strHTML += wxT("</td></tr><tr><td align=\"right\"><b>Description:</b></td><td colspan=\"3\">");
	
	if (!strDesc.IsEmpty())
	{
		strHTML += strDesc + wxT(" <i><a href=\"edit_value\" target=\"");
		strHTML += strSel;
		strHTML += wxT("\">edit...</a></i>");
	}
	else
	{
		strHTML += wxT("<i><a href=\"edit_value\" target=\"");
		strHTML += strSel;
		strHTML += wxT("\">Add...</a></i>");
	}
	
	strHTML += wxT("</td></tr></table></body></html>");
	m_pDescription->SetPage(strHTML);
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
	catch (Omega::IException* pE)
	{
		wxMessageBox(pE->Description().c_str(),_("System Error"),wxOK|wxICON_ERROR,this);
		pE->Release();
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
		m_mapMRU.insert(std::map<wxString,Omega::string_t>::value_type(dialog.m_strName,Omega::string_t(m_strSelection)));
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

void MainFrame::OnDescEdit(wxHtmlLinkEvent& evt)
{
	if (evt.GetLinkInfo().GetHref() == wxT("edit_key"))
	{
		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey(m_strSelection.c_str());

		EditKeyDescDlg dialog(this,-1,wxT(""));
		dialog.m_strName = m_strSelection;
		dialog.m_strDesc = ptrKey->GetDescription().c_str();

		if (dialog.ShowModal() == wxID_OK)
		{
			ptrKey->SetDescription(dialog.m_strDesc.c_str());
		
			SetKeyDescription(m_pTree->GetSelection());
		}
	}
	else if (evt.GetLinkInfo().GetHref() == wxT("edit_value"))
	{
		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey(m_strSelection.c_str());

		EditValueDescDlg dialog(this,-1,wxT(""));
		dialog.m_strName = m_strSelection;
		dialog.m_strValue = evt.GetLinkInfo().GetTarget();
		dialog.m_strDesc = ptrKey->GetValueDescription(dialog.m_strValue.c_str()).c_str();

		if (dialog.ShowModal() == wxID_OK)
		{
			ptrKey->SetValueDescription(dialog.m_strValue.c_str(),dialog.m_strDesc.c_str());
		
			SetValueDescription(dialog.m_strValue,dialog.m_strDesc);
		}
	}
}
