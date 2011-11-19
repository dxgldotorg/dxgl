// DXGL
// Copyright (C) 2011 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "stdafx.h"
#include "dxgltest.h"
#include "tests.h"

class DXGLTestApp : public wxApp
{
	virtual bool OnInit();
};

class DXGLTestDialog : public wxDialog
{
public:
	DXGLTestDialog(const wxString& title, const wxPoint& pos, const wxSize& size);
	
	void OnClose(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnTest2D(wxCommandEvent& event);
	void OnTestSelect2D(wxCommandEvent& event);
	void OnResolutionSelect2D(wxCommandEvent& event);

	wxListBox *lstTests2D;
	wxListBox *lstResolution2D;
	wxRadioBox *windowmode2d;
	wxSpinCtrl *backbuffers2d;
	wxCheckBox *resizable2d;
	wxButton *runtest2d;
	BOOL isDXGLDDraw;
	int selected_test;
	int selected_resolution;
	DECLARE_EVENT_TABLE()
};

enum
{
	ID_Close = 1,
	ID_Notebook = 2,
	ID_Test2DList = 3,
	ID_Resolution2DList = 4,
	ID_WindowMode2D = 5,
	ID_BackBuffers2D = 6,
	ID_Resizable2D = 7,
	ID_RunTest2D = 8,
};

BEGIN_EVENT_TABLE(DXGLTestDialog, wxDialog)
	EVT_BUTTON(ID_Close,DXGLTestDialog::OnClose)
	EVT_BUTTON(ID_RunTest2D,DXGLTestDialog::OnTest2D)
	EVT_LISTBOX(ID_Test2DList,DXGLTestDialog::OnTestSelect2D)
	EVT_LISTBOX(ID_Resolution2DList,DXGLTestDialog::OnResolutionSelect2D)
	EVT_CLOSE(DXGLTestDialog::OnClose)
END_EVENT_TABLE()

IMPLEMENT_APP(DXGLTestApp)

bool DXGLTestApp::OnInit()
{
    DXGLTestDialog *dialog = new DXGLTestDialog( _("DXGL Test App"), wxDefaultPosition, wxSize(640, 480) );
    dialog->Show(true);
    SetTopWindow(dialog);
    return true;
}

void GetFileVersion(wxString &version, LPCTSTR filename)
{
	UINT outlen;
	DWORD verinfosize = GetFileVersionInfoSize(filename,NULL);
	void *verinfo = malloc(verinfosize);
	VS_FIXEDFILEINFO *rootblock;
	GetFileVersionInfo(filename,0,verinfosize,verinfo);
	VerQueryValue(verinfo,_T("\\"),(VOID **)&rootblock,&outlen);
	TCHAR number[16];
	_itot(HIWORD(rootblock->dwFileVersionMS),number,10);
	version.assign(number);
	version.append(_("."));
	_itot(LOWORD(rootblock->dwFileVersionMS),number,10);
	version.append(number);
	version.append(_("."));
	_itot(HIWORD(rootblock->dwFileVersionLS),number,10);
	version.append(number);
	version.append(_("."));
	_itot(LOWORD(rootblock->dwFileVersionLS),number,10);
	version.append(number);
	free(verinfo);
}

HRESULT WINAPI EnumModesCallback(LPDDSURFACEDESC ddsd, wxListBox *list)
{
	wxString resolution;
	TCHAR number[16];
	_itot(ddsd->dwWidth,number,10);
	resolution.Append(number);
	resolution.Append(_("x"));
	_itot(ddsd->dwHeight,number,10);
	resolution.Append(number);
	resolution.Append(_("x"));
	_itot(ddsd->ddpfPixelFormat.dwRGBBitCount,number,10);
	resolution.Append(number);
	resolution.Append(_(","));
	_itot(ddsd->dwRefreshRate,number,10);
	resolution.Append(number);
	resolution.Append(_("Hz"));
	int listnum = list->Append(resolution);
	if(ddsd->dwWidth == 640 && ddsd->dwHeight == 480 && ddsd->dwRefreshRate == 60 && ddsd->ddpfPixelFormat.dwRGBBitCount == 8)
	{
		list->Select(listnum);
	}
	return DDENUMRET_OK;
}

DXGLTestDialog::DXGLTestDialog(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxDialog(NULL, -1, title, pos, size)
{
	selected_test = 0;
	// Create main dialog
	wxBoxSizer *sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);
	wxNotebook *notebook = new wxNotebook(this,-1,wxDefaultPosition,wxDefaultSize,0,_("Notebook"));
	sizer1->Add(notebook,1,wxLEFT|wxRIGHT|wxTOP|wxEXPAND,8);
	wxBoxSizer *sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2,0,wxALIGN_RIGHT);
	sizer2->Add(new wxButton(this,ID_Close,_("Close")),0,wxALL,8);
	// Create system page
	wxNotebookPage *pageSystem = new wxNotebookPage(notebook,-1);
	notebook->AddPage(pageSystem,_("System"));
	wxFlexGridSizer *sizerSystem = new wxFlexGridSizer(2,wxSize(4,4));
	pageSystem->SetSizer(sizerSystem);
	sizerSystem->Add(new wxStaticText(pageSystem,-1,_("ddraw.dll type:  ")));
	HMODULE mod_ddraw = LoadLibrary(_T("ddraw.dll"));
	BOOL (WINAPI *IsDXGLDDraw)() = GetProcAddress(mod_ddraw,"IsDXGLDDraw");
	if(IsDXGLDDraw)
		isDXGLDDraw = IsDXGLDDraw();
	else isDXGLDDraw = FALSE;
	FreeLibrary(mod_ddraw);
	if(isDXGLDDraw) sizerSystem->Add(new wxStaticText(pageSystem,-1,_("DXGL")));
	else sizerSystem->Add(new wxStaticText(pageSystem,-1,_("System")));
	sizerSystem->Add(new wxStaticText(pageSystem,-1,_("ddraw.dll version:  ")));
	wxString strver;
	GetFileVersion(strver,_T("ddraw.dll"));
	sizerSystem->Add(new wxStaticText(pageSystem,-1,strver));

	// Create 2D graphics page
	wxNotebookPage *page2D = new wxNotebookPage(notebook,-1);
	notebook->AddPage(page2D,_("2D Graphics"));
	wxSizer *sizer2d[7];
	sizer2d[0] = new wxBoxSizer(wxVERTICAL);
	page2D->SetSizer(sizer2d[0]);
	sizer2d[0]->Add(new wxStaticText(page2D,-1,_("Test 2D graphics functionality in DXGL or DirectDraw.")),0,wxLEFT|wxRIGHT|wxTOP,8);
	sizer2d[1] = new wxBoxSizer(wxHORIZONTAL);
	sizer2d[0]->Add(sizer2d[1],1,wxALL|wxEXPAND,8);
	sizer2d[2] = new wxStaticBoxSizer(new wxStaticBox(page2D,-1,_("Select test")),wxVERTICAL);
	sizer2d[1]->Add(sizer2d[2],1,wxEXPAND,0);
	lstTests2D = new wxListBox(page2D,ID_Test2DList);
	sizer2d[2]->Add(lstTests2D,1,wxALL|wxEXPAND,4);
	sizer2d[3] = new wxStaticBoxSizer(new wxStaticBox(page2D,-1,_("Resolution:")),wxVERTICAL);
	sizer2d[1]->Add(sizer2d[3],1,wxLEFT|wxEXPAND,8);
	lstResolution2D = new wxListBox(page2D,ID_Resolution2DList);
	sizer2d[3]->Add(lstResolution2D,1,wxALL|wxEXPAND,4);
	sizer2d[4] = new wxBoxSizer(wxHORIZONTAL);
	sizer2d[0]->Add(sizer2d[4],0,wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND,8);
	wxString windowmodechoices[] = {_("Windowed"),_("Fullscreen")};
	windowmode2d = new wxRadioBox(page2D,ID_WindowMode2D,_("Display mode:"),wxDefaultPosition,wxDefaultSize,2,
		windowmodechoices,2,wxRA_SPECIFY_ROWS);
	windowmode2d->SetSelection(1);
	sizer2d[4]->Add(windowmode2d);
	sizer2d[5] = new wxBoxSizer(wxVERTICAL);
	sizer2d[4]->Add(sizer2d[5],1,wxLEFT,8);
	sizer2d[6] = new wxBoxSizer(wxHORIZONTAL);
	sizer2d[5]->Add(sizer2d[6],0,wxALIGN_RIGHT);
	sizer2d[6]->Add(new wxStaticText(page2D,-1,("Back buffers:")),0,wxRIGHT,8);
	backbuffers2d = new wxSpinCtrl(page2D,ID_BackBuffers2D,_("1"),wxDefaultPosition,wxDefaultSize,wxSP_ARROW_KEYS|wxVERTICAL,0,10,1);
	sizer2d[6]->Add(backbuffers2d);
	resizable2d = new wxCheckBox(page2D,ID_Resizable2D,_("Use a resizable window"));
	sizer2d[5]->Add(resizable2d,0,wxTOP|wxALIGN_RIGHT,8);
	runtest2d = new wxButton(page2D,ID_RunTest2D,_("Run test"));
	sizer2d[5]->Add(runtest2d,0,wxTOP|wxALIGN_RIGHT,8);
	//backbuffers2d->Disable();
	runtest2d->Disable();

	// Add tests to list
	lstTests2D->Append(_("Page Flipping and Direct Access"));

	// Get video resolutions list
	lstResolution2D->Clear();
	LPDIRECTDRAW ddinterface;
	HRESULT herr = DirectDrawCreate(NULL,&ddinterface,NULL);
	herr = ddinterface->EnumDisplayModes(DDEDM_REFRESHRATES,NULL,lstResolution2D,(LPDDENUMMODESCALLBACK)EnumModesCallback);
	selected_resolution = lstResolution2D->GetSelection();
	ddinterface->Release();
	ddinterface = NULL;
}

void DXGLTestDialog::OnClose(wxCommandEvent& WXUNUSED(event))
{
	Close(true);
}

void DXGLTestDialog::OnClose(wxCloseEvent& event)
{
	Destroy();
}

void TranslateResolutionString(wxString str, int &width, int &height, int &bpp, int &refresh)
{
	wxString tmp = str;
	wxString tmp2 = tmp.substr(0,tmp.Find(_("x")));
	width = _ttoi(tmp2);
	tmp = tmp.Mid(tmp2.Len()+1);
	tmp2 = tmp.substr(0,tmp.Find(_("x")));
	height = _ttoi(tmp2);
	tmp = tmp.Mid(tmp2.Len()+1);
	tmp2 = tmp.substr(0,tmp.Find(_(",")));
	bpp = _ttoi(tmp2);
	tmp = tmp.Mid(tmp2.Len()+1);
	tmp2 = tmp.substr(0,tmp.Find(_("H")));
	refresh = _ttoi(tmp2);
}

void DXGLTestDialog::OnTest2D(wxCommandEvent& event)
{
	wxString strResolution = lstResolution2D->GetString(lstResolution2D->GetSelection());
	int width,height,bpp,refresh;
	TranslateResolutionString(strResolution,width,height,bpp,refresh);
	int backbuffers = backbuffers2d->GetValue();
	bool fullscreen;
	if(windowmode2d->GetSelection() == 1) fullscreen = true;
	else fullscreen = false;
	bool resizable = resizable2d->GetValue();
	#define TEST_ARGS width,height,bpp,refresh,backbuffers,fullscreen,resizable,this
	switch(selected_test)
	{
	case 0:
		RunDDFlipTest(TEST_ARGS);
		break;
	}
}

void DXGLTestDialog::OnTestSelect2D(wxCommandEvent& event)
{
	if(event.IsSelection())
	{
		selected_test = event.GetSelection();
		runtest2d->Enable();
	}	
}

void DXGLTestDialog::OnResolutionSelect2D(wxCommandEvent& event)
{
	if(event.IsSelection()) selected_resolution = event.GetSelection();
}