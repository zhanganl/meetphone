// meetphonemeet.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonemeet.h"
#include "support.h"
#include "private.h"


// Cmeetphonemeet 对话框
typedef struct _MemberData{
	int memberId;
}MemberData;


IMPLEMENT_DYNAMIC(Cmeetphonemeet, CDialog)

Cmeetphonemeet::Cmeetphonemeet(CWnd* pParent /*=NULL*/)
: CDialog(Cmeetphonemeet::IDD, pParent)
{

}

Cmeetphonemeet::~Cmeetphonemeet()
{
	while(!m_ListMember.IsEmpty()) 
	{
		Cmeetphonemember *member = m_ListMember.RemoveHead();
		member->DestroyWindow();
		delete member;
	}

	if(m_hLocalView != NULL)
	{
		m_hLocalView->DestroyWindow();
		delete m_hLocalView;
		m_hLocalView = NULL;
	}
}

void Cmeetphonemeet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MEMBER, m_hListMember);
	DDX_Control(pDX, IDC_STATIC_LOCAL, m_hStaticLocal);
}


BEGIN_MESSAGE_MAP(Cmeetphonemeet, CDialog)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_MESSAGE(WM_MEMBER_MAXIMIZE,OnMemberMaximizeMsg)
	ON_MESSAGE(WM_MEMBER_RESTORE,OnMemberRestoreMsg)
	ON_MESSAGE(WM_MEMBER_ADD,OnMemberAdd)
	ON_MESSAGE(WM_MEMBER_PREVIEW_HWND, OnMemberPreviewHwnd)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_MEMBER, &Cmeetphonemeet::OnLvnDeleteitemListMember)
END_MESSAGE_MAP()


// Cmeetphonemeet 消息处理程序

void Cmeetphonemeet::OnDestroy()
{
	CDialog::OnDestroy();
	LinphoneCore *lc = theApp.GetCore();
	LinphoneCall *call = call=linphone_core_get_current_call(lc);
	if (call!=NULL)
	{
		linphone_core_terminate_call(lc,call);
	}
	m_pParentWnd->PostMessage(WM_DELETE_MEETDLG,(WPARAM)this);
}

void Cmeetphonemeet::InitMemberList()
{
	LinphoneCore *lc = theApp.GetCore();
	m_hListMember.AddHandOverColumn(3);
	m_hListMember.AddHandOverColumn(4);
	m_hListMember.AddHandOverColumn(5);
	m_hListMember.SetImageList(&m_hActionImage, LVSIL_SMALL);
	m_hListMember.SetExtendedStyle(LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT );
	m_hListMember.InsertColumn(0, L"名称", LVCFMT_LEFT, 0);
	m_hListMember.InsertColumn(1, L"名称", LVCFMT_LEFT, 80);
	m_hListMember.InsertColumn(2, L"IP",LVCFMT_LEFT, 96);
	m_hListMember.InsertColumn(3, L"", LVCFMT_CENTER, 20);
	if(lc->is_admin) {
		m_hListMember.InsertColumn(4, L"", LVCFMT_CENTER, 20);
		m_hListMember.InsertColumn(5, L"", LVCFMT_CENTER, 20);
	}
}

BOOL Cmeetphonemeet::ReloadMemberList()
{
	if(!m_sConfUID.IsEmpty())
	{
		LinphoneCore *lc = theApp.GetCore();
		CString restMethod;
		restMethod.Format(_T("/mcuWeb/controller/getConferenceMember?confid=%s"), m_sConfUID);
		Json::Value response;
		if(http_get_request(restMethod, response)) 
		{
			m_hListMember.DeleteAllItems();
			int real_index = 0;
			for ( unsigned int index = 0; index < response.size(); ++index ) 
			{
				const char* strName = response[index]["name"].asCString();
				const char *ip = response[index]["ip"].asCString();
				const char *state = response[index]["state"].asCString();
				int memberId = response[index]["id"].asInt();
				if(state != NULL && strcasecmp("CONNECTED", state) == 0) 
				{
					state = _(state);
				} else {
					continue;
				}
				CString cStrName;
				convert_utf8_to_unicode(strName, cStrName);
				m_hListMember.InsertItem(index, _T(""));
				m_hListMember.SetItemText(real_index, 1, cStrName);
				m_hListMember.SetItemText(real_index, 2, CString(ip));
				m_hListMember.SetItem(index, 3, LVIF_IMAGE, NULL, 0,LVIS_SELECTED, LVIS_SELECTED, NULL );
				if(lc->is_admin)
				{
					m_hListMember.SetItem(real_index, 4, LVIF_IMAGE, NULL, 1,LVIS_SELECTED, LVIS_SELECTED, NULL );
					m_hListMember.SetItem(real_index, 5, LVIF_IMAGE, NULL, 2,LVIS_SELECTED, LVIS_SELECTED, NULL );
				}
				MemberData *memberData = new MemberData;
				memberData->memberId = memberId;
				m_hListMember.SetItemData(real_index, (DWORD_PTR)memberData);
				real_index++;
			}
		}
	}
	return TRUE;
}

BOOL Cmeetphonemeet::OnInitDialog()
{
	CDialog::OnInitDialog();
	RECT rect, locaViewRect;
	GetClientRect(&rect);
	locaViewRect.top = rect.bottom - rect.top - 197;
	locaViewRect.bottom = rect.bottom;
	locaViewRect.left = 2;
	locaViewRect.right = 243;
	m_hLocalView = new Cmeetphonemember(this); 
	m_hLocalView->m_sMemberName  = _T("本地视频");
	m_hLocalView->Create(IDD_MEETPHONE_MEMBER,this);
	m_hLocalView->MoveWindow(&locaViewRect);
	m_hLocalView->ShowWindow(SW_SHOW);

	m_hActionImage.Create(16, 16, ILC_COLOR32,  2, 4);
	load_png_to_imagelist(m_hActionImage,CString("res/webphone_16.png"));
	load_png_to_imagelist(m_hActionImage, CString("res/delete.png"));
	load_png_to_imagelist(m_hActionImage, CString("res/sound.png"));
	InitMemberList();
	ReloadMemberList();
	return TRUE;
}

void Cmeetphonemeet::OnClose()
{
	CDialog::OnClose();
	DestroyWindow();
}

HWND Cmeetphonemeet::AddMeetMember(CString &memberName)
{
	int memberCount = m_ListMember.GetCount();
	Cmeetphonemember *memberDlg = new Cmeetphonemember(this);
	RECT rect;
	GetClientRect(&rect);
	rect.left = rect.right - rect.left - 241;
	rect.top = memberCount * 197;
	rect.bottom = rect.top + 197;
	memberDlg->m_sMemberName = memberName;
	memberDlg->Create(IDD_MEETPHONE_MEMBER,this);
	memberDlg->MoveWindow(&rect);
	memberDlg->ShowWindow(SW_SHOW);
	m_ListMember.AddTail(memberDlg);
	return memberDlg->m_hWnd;
}

LONG Cmeetphonemeet::OnMemberMaximizeMsg(WPARAM wP, LPARAM lP)
{
	Cmeetphonemember* hwnd = (Cmeetphonemember*)wP;
	POSITION   pos   =   m_ListMember.GetHeadPosition();
	Cmeetphonemember *ptr;
	while(pos != NULL) 
	{
		ptr =  m_ListMember.GetNext(pos);
		if(hwnd != ptr) {
			ptr->ShowWindow(SW_HIDE);
		}
	}
	if(hwnd != m_hLocalView) 
	{
		m_hLocalView->ShowWindow(SW_HIDE);
	}
	return 0;
}


LONG Cmeetphonemeet::OnMemberRestoreMsg(WPARAM wP,LPARAM lP)
{
	Cmeetphonemember* hwnd = (Cmeetphonemember*)wP;
	POSITION   pos   =   m_ListMember.GetHeadPosition();
	Cmeetphonemember *ptr;
	while(pos != NULL) 
	{
		ptr =  m_ListMember.GetNext(pos);
		if(hwnd != ptr) {
			ptr->ShowWindow(SW_SHOW);
		}
	}
	if(hwnd != m_hLocalView) 
	{
		m_hLocalView->ShowWindow(SW_SHOW);
	}
	hwnd->ShowWindow(SW_RESTORE);
	return 0;
}

LRESULT Cmeetphonemeet::OnMemberAdd(WPARAM wP,LPARAM lP)
{
	CString *memberName = (CString *)wP;
	return (unsigned long)AddMeetMember(*memberName);
}

LRESULT Cmeetphonemeet::OnMemberPreviewHwnd(WPARAM wP,LPARAM lP)
{
	return (unsigned long)m_hLocalView->m_hWnd;
}

void Cmeetphonemeet::OnLvnDeleteitemListMember(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->iItem != -1)
	{
		MemberData *memberData = (MemberData *)m_hListMember.GetItemData(pNMLV->iItem);
		ms_message("Release MemberData %p", memberData);
		if(memberData != NULL)
			delete memberData;
	}
	*pResult = 0;
}
