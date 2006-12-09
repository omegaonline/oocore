// -*- C++ -*- generated by wxGlade 0.4.1 on Thu May 18 07:29:30 2006
#include "stdafx.h"
#include "EditUIntDlg.h"


EditUIntDlg::EditUIntDlg(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
    // begin wxGlade: EditUIntDlg::EditUIntDlg
    m_lblName = new wxStaticText(this, -1, _("Value &name:"));
    m_txtName = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_lblValue = new wxStaticText(this, -1, _("&Value data:"));
    m_txtValue = new wxTextCtrl(this, -1, _("500"));
    const wxString m_rbxBase_choices[] = {
        _("&Hexadecimal"),
        _("&Decimal")
    };
    m_rbxBase = new wxRadioBox(this, ID_BASE, _("Base"), wxDefaultPosition, wxDefaultSize, 2, m_rbxBase_choices, 2, wxRA_SPECIFY_ROWS);
    m_buttons = new wxStdButtons(this, -1, wxOK|wxCANCEL);

    set_properties();
    do_layout();
    // end wxGlade

	m_txtValue->SetValidator(wxTextValidator(wxFILTER_NUMERIC, &m_strValue));
	m_txtName->SetValidator(wxGenericValidator(&m_strName));
	m_rbxBase->SetValidator(wxGenericValidator(&m_nBase));
}


BEGIN_EVENT_TABLE(EditUIntDlg, wxDialog)
    // begin wxGlade: EditUIntDlg::event_table
    EVT_RADIOBOX(ID_BASE, EditUIntDlg::OnBaseChanged)
    // end wxGlade
END_EVENT_TABLE();


void EditUIntDlg::OnBaseChanged(wxCommandEvent &event)
{
	if (m_rbxBase->GetSelection()==0)
	{
		unsigned long lVal;
		m_txtValue->GetValue().ToULong(&lVal,10);
		m_txtValue->SetValue(wxString::Format(wxT("%x"),lVal));
	}
	else
	{
		unsigned long lVal;
		m_txtValue->GetValue().ToULong(&lVal,16);
		m_txtValue->SetValue(wxString::Format(wxT("%lu"),lVal));
	}
}


// wxGlade: add EditUIntDlg event handlers


void EditUIntDlg::set_properties()
{
    // begin wxGlade: EditUIntDlg::set_properties
    SetTitle(_("Edit UInt32 Value"));
    m_rbxBase->SetSelection(0);
    // end wxGlade
}


void EditUIntDlg::do_layout()
{
    // begin wxGlade: EditUIntDlg::do_layout
    wxBoxSizer* m_sizer_1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* m_sizer_2 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* m_sizer_3 = new wxBoxSizer(wxVERTICAL);
    m_sizer_1->Add(m_lblName, 0, wxLEFT|wxRIGHT|wxTOP, 7);
    m_sizer_1->Add(m_txtName, 0, wxALL|wxEXPAND, 7);
    m_sizer_3->Add(m_lblValue, 0, wxLEFT|wxRIGHT|wxTOP, 7);
    m_sizer_3->Add(m_txtValue, 0, wxALL|wxEXPAND, 7);
    m_sizer_2->Add(m_sizer_3, 1, wxEXPAND, 0);
    m_sizer_2->Add(m_rbxBase, 1, wxALL, 7);
    m_sizer_1->Add(m_sizer_2, 1, wxEXPAND, 0);
    m_sizer_1->Add(m_buttons, 0, wxALL|wxEXPAND, 7);
    SetAutoLayout(true);
    SetSizer(m_sizer_1);
    m_sizer_1->Fit(this);
    m_sizer_1->SetSizeHints(this);
    Layout();
    // end wxGlade
}

