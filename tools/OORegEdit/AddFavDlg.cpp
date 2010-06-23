// -*- C++ -*- generated by wxGlade 0.4.1 on Fri May 19 12:16:37 2006

#include "stdafx.h"
#include "AddFavDlg.h"


AddFavDlg::AddFavDlg(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long /*style*/):
    wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
    // begin wxGlade: AddFavDlg::AddFavDlg
    m_lblName = new wxStaticText(this, -1, _("Favourite name:"));
    m_txtName = new wxTextCtrl(this, ID_NAME, wxT(""));
    m_buttons = new wxStdButtons(this, -1, wxOK|wxCANCEL);

    set_properties();
    do_layout();
    // end wxGlade

	wxTextValidator tv(wxFILTER_EXCLUDE_CHAR_LIST,&m_strName);
	wxArrayString excludes;
	excludes.Add(wxT('/'));
	tv.SetExcludes(excludes);

	m_txtName->SetValidator(tv);
}


BEGIN_EVENT_TABLE(AddFavDlg, wxDialog)
    // begin wxGlade: AddFavDlg::event_table
    EVT_TEXT(ID_NAME, AddFavDlg::OnTextChanged)
    // end wxGlade
END_EVENT_TABLE();


void AddFavDlg::OnTextChanged(wxCommandEvent &/*event*/)
{
	wxWindow* pOK = FindWindow(wxID_OK);
	if (pOK)
		pOK->Enable(!m_txtName->GetValue().IsEmpty());		
}


// wxGlade: add AddFavDlg event handlers


void AddFavDlg::set_properties()
{
    // begin wxGlade: AddFavDlg::set_properties
    SetTitle(_("Add to Favourites"));
    m_txtName->SetMinSize(wxDLG_UNIT(m_txtName, wxSize(200, 14)));
    // end wxGlade
}


void AddFavDlg::do_layout()
{
    // begin wxGlade: AddFavDlg::do_layout
    wxBoxSizer* m_sizer_1 = new wxBoxSizer(wxVERTICAL);
    m_sizer_1->Add(m_lblName, 0, wxLEFT|wxRIGHT|wxTOP, 7);
    m_sizer_1->Add(m_txtName, 0, wxALL|wxEXPAND, 7);
    m_sizer_1->Add(m_buttons, 0, wxALL|wxEXPAND, 7);
    SetAutoLayout(true);
    SetSizer(m_sizer_1);
    m_sizer_1->Fit(this);
    m_sizer_1->SetSizeHints(this);
    Layout();
    // end wxGlade
}

