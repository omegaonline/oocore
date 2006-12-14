// -*- C++ -*- generated by wxGlade 0.4.1 on Thu May 18 08:34:03 2006

#include "stdafx.h"
#include "EditStringDlg.h"


EditStringDlg::EditStringDlg(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
    // begin wxGlade: EditStringDlg::EditStringDlg
    m_lblName = new wxStaticText(this, -1, _("Value &name:"));
    m_txtName = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_lblValue = new wxStaticText(this, -1, _("&Value data:"));
    m_txtValue = new wxTextCtrl(this, -1, wxT(""));
    m_buttons = new wxStdButtons(this, -1, wxOK|wxCANCEL);

    set_properties();
    do_layout();
    // end wxGlade

	m_txtValue->SetValidator(wxGenericValidator(&m_strValue));
	m_txtName->SetValidator(wxGenericValidator(&m_strName));
}


void EditStringDlg::set_properties()
{
    // begin wxGlade: EditStringDlg::set_properties
    SetTitle(_("Edit String Value"));
    // end wxGlade
}


void EditStringDlg::do_layout()
{
    // begin wxGlade: EditStringDlg::do_layout
    wxBoxSizer* m_sizer_1 = new wxBoxSizer(wxVERTICAL);
    m_sizer_1->Add(m_lblName, 0, wxLEFT|wxRIGHT|wxTOP, 7);
    m_sizer_1->Add(m_txtName, 0, wxALL|wxEXPAND, 7);
    m_sizer_1->Add(350, 2, 0, wxADJUST_MINSIZE, 0);
    m_sizer_1->Add(m_lblValue, 0, wxLEFT|wxRIGHT|wxTOP, 7);
    m_sizer_1->Add(m_txtValue, 0, wxALL|wxEXPAND, 7);
    m_sizer_1->Add(m_buttons, 0, wxALL|wxEXPAND, 7);
    SetAutoLayout(true);
    SetSizer(m_sizer_1);
    m_sizer_1->Fit(this);
    m_sizer_1->SetSizeHints(this);
    Layout();
    // end wxGlade
}
