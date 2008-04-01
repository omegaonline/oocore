#include "stdafx.h"
#include "./TreeItemData.h"
#include "./EditUIntDlg.h"
#include "./EditStringDlg.h"

TreeItemData::TreeItemData(OTL::ObjectPtr<Omega::Registry::IRegistryKey>& ptrKey, size_t nDepth) :
	m_ptrKey(ptrKey), m_nDepth(nDepth)
{
}

TreeItemData::~TreeItemData(void)
{
}

void TreeItemData::Fill(wxTreeCtrl* pTree, const wxTreeItemId& id)
{
	OTL::ObjectPtr<Omega::IEnumString> ptrEnum = m_ptrKey.EnumSubKeys();
	for (;;)
	{
		size_t count = 1;
		Omega::string_t strName;
		ptrEnum->Next(count,&strName);
		if (count==0)
			break;

		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey = m_ptrKey.OpenSubKey(strName);

		TreeItemData* pNewItem = new TreeItemData(ptrKey,(m_nDepth>0 ? m_nDepth-1 : 0));
		wxTreeItemId itemId = pTree->AppendItem(id,wxString(strName.c_str()),2,3,pNewItem);

		if (m_nDepth==0)
		{
			OTL::ObjectPtr<Omega::IEnumString> ptrEnum2 = ptrKey.EnumSubKeys();
			ptrEnum2->Next(count,&strName);
			if (count==1)
				pTree->AppendItem(itemId,wxT("DUFF!"));
		}
		else
		{
			pNewItem->Fill(pTree,itemId);
		}

		pTree->SortChildren(id);
	};
}

void TreeItemData::InitList(wxListCtrl* pList)
{
	pList->DeleteAllItems();

	OTL::ObjectPtr<Omega::IEnumString> ptrEnum = m_ptrKey.EnumValues();
	int i;
	for (i=0;;++i)
	{
		size_t count = 1;
		Omega::string_t strName;
		ptrEnum->Next(count,&strName);
		if (count==0)
			break;

		Omega::Registry::ValueType_t type = m_ptrKey->GetValueType(strName);

		long item = pList->InsertItem(i,wxString(strName.c_str()),type==Omega::Registry::String ? 4 : 5);

		if (type==Omega::Registry::String)
		{
			pList->SetItem(item,1,_("String"));
			pList->SetItem(item,2,wxString(m_ptrKey->GetStringValue(strName).c_str()));
		}
		else if (type==Omega::Registry::Integer)
		{
			Omega::int64_t val = m_ptrKey->GetIntegerValue(strName);
			pList->SetItem(item,1,_("Integer"));
			pList->SetItem(item,2,wxString::Format(wxT("%lld"),val));
		}
		else
		{
			Omega::byte_t szBuf[128];
			Omega::uint32_t cbLen = sizeof(szBuf);
			m_ptrKey->GetBinaryValue(strName,cbLen,szBuf);

			wxString val;
			if (cbLen==0)
				val = _("(Zero length binary value)");

			for (size_t j=0;j<cbLen && j<=128;++j)
			{
				val += wxString::Format(wxT("%x "),(int)szBuf[j]);
			}

			if (cbLen>128)
				val += wxT("...");

			pList->SetItem(item,1,_("Binary"));
			pList->SetItem(item,2,val);
		}
	};

	if (i)
		pList->SetItemState(0,wxLIST_STATE_FOCUSED,wxLIST_STATE_FOCUSED);
}

void TreeItemData::Refresh(wxListCtrl* pList, wxTreeCtrl* pTree, const wxTreeItemId& id)
{
	pTree->DeleteChildren(id);
	Fill(pTree,id);
	InitList(pList);
}

void TreeItemData::DeleteKey(const Omega::string_t& strKey)
{
	m_ptrKey->DeleteKey(strKey);
}

void TreeItemData::DeleteValue(const Omega::string_t& strVal)
{
	m_ptrKey->DeleteValue(strVal);
}

bool TreeItemData::RenameValue(const Omega::string_t& strFrom, const Omega::string_t& strTo)
{
	Omega::Registry::ValueType_t type = m_ptrKey->GetValueType(strFrom);

	if (m_ptrKey->IsValue(strTo))
		return false;

	if (type==Omega::Registry::String)
	{
		Omega::string_t val = m_ptrKey->GetStringValue(strFrom);
		m_ptrKey->SetStringValue(strTo,val);
		m_ptrKey->DeleteValue(strFrom);
	}
	else if (type==Omega::Registry::Integer)
	{
		Omega::uint64_t val = m_ptrKey->GetIntegerValue(strFrom);
		m_ptrKey->SetIntegerValue(strTo,val);
		m_ptrKey->DeleteValue(strFrom);
	}
	else
	{
		Omega::uint32_t cbLen = 0;
		m_ptrKey->GetBinaryValue(strFrom,cbLen,NULL);

		Omega::byte_t* pBuffer = (Omega::byte_t*)malloc(cbLen);
		try
		{
			m_ptrKey->GetBinaryValue(strFrom,cbLen,pBuffer);
			m_ptrKey->SetBinaryValue(strTo,cbLen,pBuffer);
		}
		catch (...)
		{
			free(pBuffer);
			throw;
		}
		free(pBuffer);

		m_ptrKey->DeleteValue(strFrom);
	}

	return true;
}

void TreeItemData::RenameKey(const Omega::string_t& strFrom, const Omega::string_t& strTo, TreeItemData* pItem)
{
	OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrNewKey = m_ptrKey.OpenSubKey(strTo,Omega::Registry::IRegistryKey::Create | Omega::Registry::IRegistryKey::FailIfThere);

	CopyKey(pItem->m_ptrKey,ptrNewKey);

	m_ptrKey->DeleteKey(strFrom);

	pItem->m_ptrKey = ptrNewKey;
}

void TreeItemData::CopyKey(OTL::ObjectPtr<Omega::Registry::IRegistryKey>& ptrOldKey, OTL::ObjectPtr<Omega::Registry::IRegistryKey>& ptrNewKey)
{
	OTL::ObjectPtr<Omega::IEnumString> ptrEnum = ptrOldKey.EnumValues();
	for (;;)
	{
		size_t count = 1;
		Omega::string_t strName;
		ptrEnum->Next(count,&strName);
		if (count==0)
			break;

		Omega::Registry::ValueType_t type = ptrOldKey->GetValueType(strName);

		if (type==Omega::Registry::String)
		{
			Omega::string_t val = ptrOldKey->GetStringValue(strName);
			ptrNewKey->SetStringValue(strName,val);
		}
		else if (type==Omega::Registry::Integer)
		{
			Omega::uint64_t val = ptrOldKey->GetIntegerValue(strName);
			ptrNewKey->SetIntegerValue(strName,val);
		}
		else
		{
			Omega::uint32_t cbLen = 0;
			ptrNewKey->GetBinaryValue(strName,cbLen,NULL);

			Omega::byte_t* pBuffer = (Omega::byte_t*)malloc(cbLen);
			try
			{
				ptrOldKey->GetBinaryValue(strName,cbLen,pBuffer);
				ptrNewKey->SetBinaryValue(strName,cbLen,pBuffer);
			}
			catch (...)
			{
				free(pBuffer);
				throw;
			}
			free(pBuffer);
		}
	}

	ptrEnum = ptrOldKey.EnumSubKeys();
	for (;;)
	{
		size_t count = 1;
		Omega::string_t strName;
		ptrEnum->Next(count,&strName);
		if (count==0)
			break;

		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrOldSub = ptrOldKey.OpenSubKey(strName,Omega::Registry::IRegistryKey::OpenExisting);
		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrNewSub = ptrNewKey.OpenSubKey(strName,Omega::Registry::IRegistryKey::Create | Omega::Registry::IRegistryKey::FailIfThere);

		CopyKey(ptrOldSub,ptrNewSub);
	}
}

void TreeItemData::NewKey(wxTreeCtrl* pTree, const wxTreeItemId& id)
{
	OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey;
	Omega::string_t strName;

	size_t c;
	for (c = 1;c<100;++c)
	{
		strName = wxString::Format(_("New Key #%lu"),c);
		if (!m_ptrKey->IsSubKey(strName))
		{
			ptrKey = m_ptrKey.OpenSubKey(strName,Omega::Registry::IRegistryKey::Create | Omega::Registry::IRegistryKey::FailIfThere);
			break;
		}
	}
	if (c >= 100)
	{
		wxMessageBox(_("Too many new keys!"),_("New Key"),wxOK|wxICON_INFORMATION,NULL);
		return;
	}

	TreeItemData* pNewItem = new TreeItemData(ptrKey,(m_nDepth>0 ? m_nDepth-1 : 0));
	wxTreeItemId itemId = pTree->AppendItem(id,wxString(strName.c_str()),2,3,pNewItem);

	if (m_nDepth==0)
	{
		size_t count = 1;
		OTL::ObjectPtr<Omega::IEnumString> ptrEnum2 = ptrKey.EnumSubKeys();
		ptrEnum2->Next(count,&strName);
		if (count==1)
			pTree->AppendItem(itemId,wxT("DUFF!"));
	}
	else
	{
		pNewItem->Fill(pTree,itemId);
	}

	pTree->EnsureVisible(itemId);
	pTree->SetFocus();
	pTree->EditLabel(itemId);
}

void TreeItemData::NewString(wxListCtrl* pList)
{
	OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey;
	Omega::string_t strName;

	for (size_t c = 1;;++c)
	{
		strName = wxString::Format(_("New Value #%lu"),c);
		if (!m_ptrKey->IsValue(strName))
			break;
	}

	m_ptrKey->SetStringValue(strName,L"");

	long item = pList->InsertItem(-1,wxString(strName.c_str()),4);
	pList->SetItem(item,1,_("String"));

	pList->EnsureVisible(item);
	pList->SetFocus();
	pList->EditLabel(item);
}

void TreeItemData::NewUInt(wxListCtrl* pList)
{
	OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey;
	Omega::string_t strName;

	for (size_t c = 1;;++c)
	{
		strName = wxString::Format(_("New Value #%lu"),c);
		if (!m_ptrKey->IsValue(strName))
			break;
	}

	m_ptrKey->SetIntegerValue(strName,0);

	long item = pList->InsertItem(-1,wxString(strName.c_str()),5);
	pList->SetItem(item,1,_("Integer"));
	pList->SetItem(item,2,wxT("0x0000000000000000 (0)"));

	pList->EnsureVisible(item);
	pList->SetFocus();
	pList->EditLabel(item);
}

void TreeItemData::NewBinary(wxListCtrl* pList)
{
	OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrKey;
	Omega::string_t strName;

	for (size_t c = 1;;++c)
	{
		strName = wxString::Format(_("New Value #%lu"),c);
		if (!m_ptrKey->IsValue(strName))
			break;
	}

	m_ptrKey->SetBinaryValue(strName,0,0);

	long item = pList->InsertItem(-1,wxString(strName.c_str()),5);
	pList->SetItem(item,1,_("Binary"));
	pList->SetItem(item,2,_("(Zero length binary value)"));

	pList->EnsureVisible(item);
	pList->SetFocus();
	pList->EditLabel(item);
}

void TreeItemData::Modify(wxListCtrl* pList, long item_id)
{
	Omega::string_t strName(pList->GetItemText(item_id));
	Omega::Registry::ValueType_t type = m_ptrKey->GetValueType(strName);

	if (type == Omega::Registry::String)
	{
		EditStringDlg dialog(NULL,-1,wxT(""));

		dialog.m_strName = wxString(strName.c_str());
		dialog.m_strValue = wxString(m_ptrKey->GetStringValue(strName).c_str());

		if (dialog.ShowModal() == wxID_OK)
		{
			m_ptrKey->SetStringValue(strName,Omega::string_t(dialog.m_strValue));

			pList->SetItem(item_id,2,dialog.m_strValue);
		}
	}
	else if (type == Omega::Registry::Integer)
	{
		EditUIntDlg dialog(NULL,-1,wxT(""));

		dialog.m_nBase = 0;
		dialog.m_strName = wxString(strName.c_str());
		dialog.m_strValue = wxString::Format(wxT("%lld"),m_ptrKey->GetIntegerValue(strName));

		if (dialog.ShowModal() == wxID_OK)
		{
			wxLongLong_t lVal;
			dialog.m_strValue.ToLongLong(&lVal,dialog.m_nBase==0 ? 16 : 10);
			m_ptrKey->SetIntegerValue(strName,lVal);

			pList->SetItem(item_id,2,wxString::Format(wxT("%lld"),lVal));
		}
	}
}

void TreeItemData::Find(wxTreeCtrl* pTree, wxTreeItemId tree_id, wxListCtrl* pList, long list_id, const Omega::string_t& strFind, bool bKeys, bool bValues, bool bData, bool bMatchAll, bool bIgnoreCase)
{
	// Manually check the current list contents
	if (bValues || bData)
	{
		while ((list_id = pList->GetNextItem(list_id))!=-1)
		{
			Omega::string_t strName(pList->GetItemText(list_id));

			if (MatchValue(strFind,m_ptrKey,strName,bValues,bData,bMatchAll,bIgnoreCase))
			{
				long item;
				while ((item=pList->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) != -1)
				{
					pList->SetItemState(item,0,wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
				}

				pList->SetItemState(list_id,wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
				pList->SetFocus();
				return;
			}
		}
	}

	Find2(pTree,tree_id,pList,strFind,bKeys,bValues,bData,bMatchAll,bIgnoreCase);
}

void TreeItemData::Find2(wxTreeCtrl* pTree, wxTreeItemId tree_id, wxListCtrl* pList, const Omega::string_t& strFind, bool bKeys, bool bValues, bool bData, bool bMatchAll, bool bIgnoreCase)
{
	TreeItemData* pItem = (TreeItemData*)pTree->GetItemData(tree_id);

	bool bKey = false;
	Omega::string_t strFoundPos = Find3(pItem->m_ptrKey,strFind,bKeys,bValues,bData,bMatchAll,bIgnoreCase,bKey);
	if (!strFoundPos.IsEmpty())
	{
		// Expand and select the item
		wxString strSubKey;
		for (;;)
		{
			size_t pos = strFoundPos.Find('\\');
			if (pos != Omega::string_t::npos)
			{
				strSubKey = strFoundPos.Left(pos).c_str();
				strFoundPos = strFoundPos.Mid(pos+1);
			}
			else
			{
				strSubKey = strFoundPos.c_str();
				strFoundPos.Clear();

				if (!bKey)
					break;
			}

			//pTree->SelectItem(tree_id);
			pTree->Expand(tree_id);

			if (strSubKey == pTree->GetItemText(tree_id))
				continue;

			if (pTree->ItemHasChildren(tree_id))
			{
				wxTreeItemIdValue cookie;
				wxTreeItemId id_child = pTree->GetFirstChild(tree_id,cookie);

				while (id_child && strSubKey != pTree->GetItemText(id_child))
				{
					id_child = pTree->GetNextChild(tree_id,cookie);
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

		pTree->SelectItem(tree_id);

		if (!bKey)
		{
			long item;
			while ((item=pList->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) != -1)
			{
				pList->SetItemState(item,0,wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
			}

			item = pList->FindItem(-1,wxString(strSubKey));
			pList->SetItemState(item,wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
			pList->SetFocus();
		}
		else
		{
			pTree->SetFocus();
		}

		return;
	}

	// Try the next sibling
	wxTreeItemId next_id = pTree->GetNextSibling(tree_id);
	while (!next_id)
	{
		if (tree_id == pTree->GetRootItem())
		{
			wxMessageBox(_("Reached the end of the registry."),_("Find"),wxOK|wxICON_INFORMATION,NULL);
			return;
		}

		tree_id = pTree->GetItemParent(tree_id);
		next_id = pTree->GetNextSibling(tree_id);
	}

	Find2(pTree,next_id,pList,strFind,bKeys,bValues,bData,bMatchAll,bIgnoreCase);
}

Omega::string_t TreeItemData::Find3(OTL::ObjectPtr<Omega::Registry::IRegistryKey>& ptrKey, const Omega::string_t& strFind, bool bKeys, bool bValues, bool bData, bool bMatchAll, bool bIgnoreCase, bool& bKey)
{
	// Check the current sub-keys
	OTL::ObjectPtr<Omega::IEnumString> ptrEnum = ptrKey.EnumSubKeys();
	for (;;)
	{
		size_t count = 1;
		Omega::string_t strName;
		ptrEnum->Next(count,&strName);
		if (count==0)
			break;

		// Check key name
		if (bKeys)
		{
			if (bMatchAll)
			{
				if (!bIgnoreCase && strFind == strName)
				{
					// Found it!
					bKey = true;
					return strName;
				}
				else if (strName.CompareNoCase(strName)==0)
				{
					// Found it!
					bKey = true;
					return strName;
				}
			}
			else if (strName.Find(strFind,0,bIgnoreCase) != Omega::string_t::npos)
			{
				// Found it!
				bKey = true;
				return strName;
			}
		}

		// Check key values
		OTL::ObjectPtr<Omega::Registry::IRegistryKey> ptrSubKey = ptrKey.OpenSubKey(strName);
		OTL::ObjectPtr<Omega::IEnumString> ptrEnum = ptrSubKey.EnumValues();
		for (;;)
		{
			count = 1;
			Omega::string_t strValue;
			ptrEnum->Next(count,&strValue);
			if (count==0)
				break;

			if (MatchValue(strFind,ptrSubKey,strValue,bValues,bData,bMatchAll,bIgnoreCase))
			{
				// Found it!
				bKey = false;
				return strName + L"\\" + strValue;
			}
		}

		// Recurse down...
		Omega::string_t strNext = Find3(ptrSubKey,strFind,bKeys,bValues,bData,bMatchAll,bIgnoreCase,bKey);
		if (!strNext.IsEmpty())
		{
			return strName + L"\\" + strNext;
		}
	}

	return Omega::string_t();
}

bool TreeItemData::MatchValue(const Omega::string_t& strFind, OTL::ObjectPtr<Omega::Registry::IRegistryKey>& ptrKey, const Omega::string_t& strName, bool bValues, bool bData, bool bMatchAll, bool bIgnoreCase)
{
	if (bValues)
	{
		if (bMatchAll)
		{
			if (!bIgnoreCase && strFind == strName)
				return true;
			else if (strName.CompareNoCase(strFind)==0)
				return true;
		}
		else if (strName.Find(strFind,0,bIgnoreCase) != Omega::string_t::npos)
			return true;
	}

	if (bData && ptrKey->GetValueType(strName)==Omega::Registry::String)
	{
		Omega::string_t strValue = ptrKey->GetStringValue(strName);
		if (bMatchAll)
		{
			if (!bIgnoreCase && strFind == strValue)
				return true;
			else if (strValue.CompareNoCase(strFind)==0)
				return true;
		}
		else if (strValue.Find(strFind,0,bIgnoreCase) != Omega::string_t::npos)
			return true;
	}

	return false;
}

wxString TreeItemData::GetDesc()
{
	return m_ptrKey->GetDescription().c_str();
}

wxString TreeItemData::GetValueDesc(const wxString& strVal)
{
	return m_ptrKey->GetValueDescription(strVal.c_str()).c_str();
}
