// -*- C++ -*- generated by wxGlade 0.4.1 on Thu May 18 09:30:38 2006

#include "stdafx.h"
#include "./FindDlg.h"


FindDlg::FindDlg(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
    // begin wxGlade: FindDlg::FindDlg
    m_sizer_4_staticbox = new wxStaticBox(this, -1, _("Look at"));
    m_lblFindWhat = new wxStaticText(this, -1, _("Fi&nd what:"));
    m_txtFind = new wxTextCtrl(this, ID_FIND, wxT(""));
    m_chkKeys = new wxCheckBox(this, -1, _("&Keys"));
    m_chkValues = new wxCheckBox(this, -1, _("&Values"));
    m_chkData = new wxCheckBox(this, -1, _("&Data"));
    m_chkMatchAll = new wxCheckBox(this, -1, _("Match &whole string only"));
    m_chkIgnoreCase = new wxCheckBox(this, -1, _("&Ignore case"));
    m_btnFind = new wxButton(this, wxID_OK, _("&Find Next"));
    m_btnCancel = new wxButton(this, wxID_CANCEL, _("Cancel"));

    set_properties();
    do_layout();
    // end wxGlade

	wxTextValidator tv(wxFILTER_EXCLUDE_CHAR_LIST,&m_strFind);
	wxArrayString excludes;
	excludes.Add(wxT('\\'));
	tv.SetExcludes(excludes);

	m_txtFind->SetValidator(tv);
	m_chkKeys->SetValidator(wxGenericValidator(&m_bKeys));
	m_chkValues->SetValidator(wxGenericValidator(&m_bValues));
	m_chkData->SetValidator(wxGenericValidator(&m_bData));
	m_chkMatchAll->SetValidator(wxGenericValidator(&m_bMatchAll));
	m_chkIgnoreCase->SetValidator(wxGenericValidator(&m_bIgnoreCase));
}


void FindDlg::set_properties()
{
    // begin wxGlade: FindDlg::set_properties
    SetTitle(_("Find"));
    m_txtFind->SetMinSize(wxDLG_UNIT(m_txtFind, wxSize(160, 14)));
    m_btnFind->SetDefault();
    // end wxGlade
}


BEGIN_EVENT_TABLE(FindDlg, wxDialog)
    // begin wxGlade: FindDlg::event_table
    EVT_TEXT(ID_FIND, FindDlg::OnTextChanged)
    // end wxGlade
END_EVENT_TABLE();


void FindDlg::do_layout()
{
    // begin wxGlade: FindDlg::do_layout
    wxBoxSizer* m_sizer_1 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* m_sizer_5 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* m_sizer_2 = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* m_sizer_4 = new wxStaticBoxSizer(m_sizer_4_staticbox, wxVERTICAL);
    wxBoxSizer* m_sizer_3 = new wxBoxSizer(wxHORIZONTAL);
    m_sizer_3->Add(m_lblFindWhat, 0, wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 7);
    m_sizer_3->Add(m_txtFind, 1, wxALL, 7);
    m_sizer_2->Add(m_sizer_3, 0, wxEXPAND, 0);
    m_sizer_4->Add(m_chkKeys, 0, wxLEFT|wxRIGHT|wxTOP, 7);
    m_sizer_4->Add(m_chkValues, 0, wxLEFT|wxRIGHT|wxTOP, 7);
    m_sizer_4->Add(m_chkData, 0, wxALL, 7);
    m_sizer_2->Add(m_sizer_4, 0, wxRIGHT|wxEXPAND, 7);
    m_sizer_2->Add(m_chkMatchAll, 0, wxRIGHT|wxTOP|wxBOTTOM, 7);
    m_sizer_2->Add(m_chkIgnoreCase, 0, wxRIGHT|wxBOTTOM, 7);
    m_sizer_1->Add(m_sizer_2, 1, wxLEFT|wxEXPAND, 7);
    m_sizer_5->Add(m_btnFind, 0, wxALL, 7);
    m_sizer_5->Add(m_btnCancel, 0, wxLEFT|wxRIGHT, 7);
    m_sizer_5->Add(20, 20, 0, wxADJUST_MINSIZE, 0);
    m_sizer_1->Add(m_sizer_5, 0, wxEXPAND, 0);
    SetAutoLayout(true);
    SetSizer(m_sizer_1);
    m_sizer_1->Fit(this);
    m_sizer_1->SetSizeHints(this);
    Layout();
    // end wxGlade
}

void FindDlg::OnTextChanged(wxCommandEvent &event)
{
	m_btnFind->Enable(!m_txtFind->GetValue().IsEmpty());		
}

// wxGlade: add FindDlg event handlers
