// meetphonemainDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonemainDlg.h"
#include "support.h"
#include "private.h"
#include <string>
#include <json/json.h>


typedef struct _ConfData{
	CString confUID;
	std::string did;
	int confId;
}ConfData;

typedef struct _MemberData{
	CString confUID;
	int memberId;
}MemberData;



// CmeetphonemainDlg �Ի���

IMPLEMENT_DYNAMIC(CmeetphonemainDlg, CDialog)

CmeetphonemainDlg::CmeetphonemainDlg(CWnd* pParent /*=NULL*/)
: CDialog(CmeetphonemainDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CmeetphonemainDlg::~CmeetphonemainDlg()
{
}

void CmeetphonemainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CONFERENCE, m_ConfList);
	DDX_Control(pDX, IDC_LIST_MEMBER, m_MemberList);
}


BEGIN_MESSAGE_MAP(CmeetphonemainDlg, CDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDOK, &CmeetphonemainDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CmeetphonemainDlg::OnBnClickedCancel)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CONFERENCE, &CmeetphonemainDlg::OnLvnItemchangedListConference)
	ON_NOTIFY(NM_CLICK, IDC_LIST_CONFERENCE, &CmeetphonemainDlg::OnNMClickListConference)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_CONFERENCE, &CmeetphonemainDlg::OnLvnDeleteitemListConference)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_MEMBER, &CmeetphonemainDlg::OnLvnDeleteitemListMember)
	ON_NOTIFY(NM_CLICK, IDC_LIST_MEMBER, &CmeetphonemainDlg::OnNMClickListMember)
	ON_MESSAGE(WM_RELOAD_MEMBER, OnReloadMemberMsg)
	ON_MESSAGE(WM_RELOAD_CONEFENCE,OnReloadConferenceListMsg)
END_MESSAGE_MAP()

BOOL CmeetphonemainDlg::InitMemberList()
{
	LinphoneCore *lc = theApp.GetCore();
	m_MemberList.AddHandOverColumn(4);
	m_MemberList.SetImageList(&m_hActionImage, LVSIL_SMALL);
	m_MemberList.SetExtendedStyle(LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT );
	m_MemberList.InsertColumn(0, L"����", LVCFMT_LEFT, 0);
	m_MemberList.InsertColumn(1, L"����", LVCFMT_LEFT, 80);
	m_MemberList.InsertColumn(2, L"IP",LVCFMT_LEFT, 108);
	m_MemberList.InsertColumn(3, L"״̬", LVCFMT_LEFT, 80);
	if(lc->is_admin)
		m_MemberList.InsertColumn(4, L"", LVCFMT_CENTER, 20);
	return TRUE;
}

BOOL CmeetphonemainDlg::ReloadMemberList(CString &confUID)
{
	if(!confUID.IsEmpty())
	{
		LinphoneCore *lc = theApp.GetCore();
		CString restMethod;
		restMethod.Format(_T("/mcuWeb/controller/getConferenceMember?confid=%s"), confUID);
		Json::Value response;
		if(http_get_request(restMethod, response)) 
		{
			m_MemberList.DeleteAllItems();
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
				m_MemberList.InsertItem(index, _T(""));
				m_MemberList.SetItemText(index, 1, cStrName);
				m_MemberList.SetItemText(index, 2, CString(ip));
				m_MemberList.SetItemText(index, 3, CString(state));
				if(lc->is_admin)
					m_MemberList.SetItem(index, 4, LVIF_IMAGE, NULL, 1,LVIS_SELECTED, LVIS_SELECTED, NULL );
				MemberData *memberData = new MemberData;
				memberData->memberId = memberId;
				memberData->confUID = confUID;
				m_MemberList.SetItemData(index, (DWORD_PTR)memberData);
			}
		}
	}
	return TRUE;
}

BOOL CmeetphonemainDlg::InitConferenceList()
{
	LinphoneCore *lc = theApp.GetCore();
	m_ConfList.AddHandOverColumn(4);
	m_ConfList.AddHandOverColumn(5);
	m_ConfList.SetImageList(&m_hActionImage, LVSIL_SMALL);
	m_ConfList.SetExtendedStyle(LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT );
	m_ConfList.InsertColumn(0, L"����", LVCFMT_LEFT, 0);
	m_ConfList.InsertColumn(1, L"����", LVCFMT_LEFT, 80);
	m_ConfList.InsertColumn(2, L"����ʱ��",LVCFMT_LEFT, 132);
	m_ConfList.InsertColumn(3, L"�������", LVCFMT_LEFT, 68);
	m_ConfList.InsertColumn(4, L"",  LVCFMT_CENTER, 26);
	if(lc->is_admin)
		m_ConfList.InsertColumn(5, L"",  LVCFMT_CENTER, 26);
	return TRUE;
}

BOOL CmeetphonemainDlg::ReloadConferenceList()
{
	LinphoneCore *lc = theApp.GetCore();
	LinphoneProxyConfig *cfg;
	linphone_core_get_default_proxy(lc, &cfg);
	CString restMethod;
	restMethod.Format(_T("/mcuWeb/controller/getConference?userid=%s"), CString(sal_op_get_userid(cfg->op)));
	Json::Value response;
	if(http_get_request(restMethod, response)) 
	{
		m_ConfList.DeleteAllItems();
		for ( unsigned int index = 0; index < response.size(); ++index ) 
		{
			const char* strName = response[index]["name"].asCString();
			const char *server = response[index]["server"].asCString();
			const char *did = response[index]["did"].asCString();
			int confId = response[index]["id"].asInt();
			CString cStrName;
			CString cMemberCount;
			CString cConfUID;
			convert_utf8_to_unicode(strName, cStrName);
			cMemberCount.Format(_T("%d"), response[index]["member_count"].asInt());
			cConfUID.Format(_T("%d@%s"), confId, CString(server));
			m_ConfList.InsertItem(index, _T(""));
			m_ConfList.SetItemText(index, 1, cStrName);
			m_ConfList.SetItemText(index, 2, CString(response[index]["create_time"].asCString()));
			m_ConfList.SetItemText(index, 3, cMemberCount);
			m_ConfList.SetItem(index, 4, LVIF_IMAGE ,  NULL, 0,LVIS_SELECTED, LVIS_SELECTED, NULL );
			if(lc->is_admin)
				m_ConfList.SetItem(index, 5, LVIF_IMAGE, NULL, 1,LVIS_SELECTED, LVIS_SELECTED, NULL );
			ConfData *confData = new ConfData;
			confData->confId = confId;
			confData->confUID = cConfUID;
			confData->did = did;
			m_ConfList.SetItemData(index, (DWORD_PTR)confData);
		}
	}
	ReloadMemberList(CString(""));
	return TRUE;
}

// CmeetphonemainDlg ��Ϣ��������
BOOL CmeetphonemainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	CImage png;
	CBitmap bmp;
	HBITMAP hBmp;
	m_hActionImage.Create(16, 16, ILC_COLOR32,  2, 4);

	// Load image and add colunm to conference list
	png.Load(_T("res/webphone_16.png"));
	hBmp = png.Detach();
	bmp.Attach(hBmp);
	m_hActionImage.Add(&bmp, RGB(255, 255, 255));
	bmp.Detach();

	png.Load(_T("res/delete.png"));
	hBmp = png.Detach();
	bmp.Attach(hBmp);
	m_hActionImage.Add(&bmp, RGB(255, 255, 255));
	bmp.Detach();
	InitConferenceList();
	InitMemberList();
	ReloadConferenceList();
	// Enable logout menu
	GetMenu()->EnableMenuItem(ID_LOGOUT, MF_ENABLED);

	return TRUE;
}

void CmeetphonemainDlg::OnDestroy()
{
	CDialog::OnDestroy();
	m_pParentWnd->PostMessage(WM_DELETE_DLG,(WPARAM)this);
}

void CmeetphonemainDlg::OnBnClickedOk()
{
	OnOK();
	DestroyWindow();
}

void CmeetphonemainDlg::OnBnClickedCancel()
{
	OnCancel();
	DestroyWindow();
}


void CmeetphonemainDlg::OnLvnItemchangedListConference(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if(pNMLV->iItem!=-1 && pNMLV->uNewState & LVIS_SELECTED)
	{
		ConfData *confData = (ConfData *)m_ConfList.GetItemData(pNMLV->iItem);
		if(confData != NULL)
		{
			ReloadMemberList(confData->confUID);
		}
	}
	*pResult = 0;
}

void CmeetphonemainDlg::OnNMClickListConference(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if(pNMItemActivate->iItem!=-1 && (pNMItemActivate->iSubItem == 4 || pNMItemActivate->iSubItem == 5) )
	{
		if(pNMItemActivate->iSubItem == 4)
		{
			LinphoneCore *lc = theApp.GetCore();
			ConfData *confData = (ConfData *)m_ConfList.GetItemData(pNMItemActivate->iItem);
			linphone_core_invite(lc,confData->did.c_str());

		}
		CString strtemp;
		strtemp.Format(L"�������ǵ�%d�е�%d��\n",
			pNMItemActivate->iItem, pNMItemActivate->iSubItem);
		OutputDebugString(strtemp);
	}
	*pResult = 0;
}

void CmeetphonemainDlg::OnLvnDeleteitemListConference(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->iItem != -1)
	{
		ConfData *confData = (ConfData *)m_ConfList.GetItemData(pNMLV->iItem);
		ms_message("Release ConfData %p", confData);
		if(confData != NULL)
			delete confData;
	}
	*pResult = 0;
}

void CmeetphonemainDlg::OnLvnDeleteitemListMember(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->iItem != -1)
	{
		MemberData *memberData = (MemberData *)m_MemberList.GetItemData(pNMLV->iItem);
		ms_message("Release MemberData %p", memberData);
		if(memberData != NULL)
			delete memberData;
	}
	*pResult = 0;
}

void CmeetphonemainDlg::OnNMClickListMember(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if(pNMItemActivate->iItem!=-1 && pNMItemActivate->iSubItem == 4)
	{
		MemberData *memberData = (MemberData *)m_MemberList.GetItemData(pNMItemActivate->iItem);
		if(memberData != NULL)
		{
			CString restMethod("/mcuWeb/controller/removeParticipant");
			CString strFormData;
			strFormData.Format(_T("uid=%s&partId=%d&num=0\r\n"), memberData->confUID, memberData->memberId);
			Json::Value response;
			http_post_request(restMethod, strFormData, response);
			PostMessage(WM_RELOAD_MEMBER, (WPARAM)_wcsdup(memberData->confUID.GetString()));
			
		}
	}
	*pResult = 0;
}

LONG CmeetphonemainDlg::OnReloadMemberMsg(WPARAM wP,LPARAM lP)
{
	TCHAR *confUID = (TCHAR *)wP;
	ReloadMemberList(CString(confUID));
	delete[] confUID;
	return 0;
}


LONG CmeetphonemainDlg::OnReloadConferenceListMsg(WPARAM wP,LPARAM lP)
{
	ReloadConferenceList();
	return 0;
}
