#include "WEMoveSCU.h"
#include "../logger/WEAppender.h"
#include "../DcmJson/WEJsonDataset.h"

#include "json/json.h"

#define OFFIS_CONSOLE_FINDSCU "findscu"

static OFLogger m_moveSCULogger = OFLog::getLogger("dcmtk.apps." OFFIS_CONSOLE_FINDSCU );

/* default application titles */
#define APPLICATIONTITLE        "FINDSCU"
#define PEERAPPLICATIONTITLE    "ANY-SCP"

#define SHORTCOL 4
#define LONGCOL 19


/* ---------------- class CWEFindSCUCallback ---------------- */

CWEMoveSCUCallback::CWEMoveSCUCallback(int cancelAfterNResponses)
	: CMoveExecuteSCUCallback()
	, cancelAfterNResponses_(cancelAfterNResponses)
	, m_pEventHandler(NULL)
{
}

void CWEMoveSCUCallback::SetEventHandler(IWEMoveSCUEventHandler * pEventHandler)
{
	m_pEventHandler = pEventHandler;
}

void CWEMoveSCUCallback::SetEndFlag()
{
	if (m_pEventHandler != NULL)
	{
		//m_pEventHandler->OnRetrieveEnd();
	}
}


void CWEMoveSCUCallback::callback(
	T_DIMSE_C_MoveRQ *request,
	int responseCount,
	T_DIMSE_C_MoveRSP *rsp)
{
	/* dump delimiter */
	DCMNET_WARN("---------------------------");

	/* dump response number */
	DCMNET_WARN("Find Response: " << responseCount << " (" << DU_cfindStatusString(rsp->DimseStatus) << ")");

	/* dump data set which was received */
	//DCMNET_WARN(DcmObject::PrintHelper(*responseIdentifiers));


	if (m_pEventHandler != NULL)
	{
		OFString strRecord;
		//GetXMLFromDataset((*responseIdentifiers), strXML);
		CWEJsonDataset jsonDataset;
		//jsonDataset.copyFrom(*responseIdentifiers);
		//jsonDataset.writeJson(strRecord);
		//m_pEventHandler->OnRetrieveRecord(strRecord.c_str());
	}


	// 我们可以在查询到合适的result之后想SCP发送取消命令
	// 不过目测并不太需要这个功能
	/* should we send a cancel back ?? */
	if (cancelAfterNResponses_ == responseCount)
	{
		DCMNET_INFO("Sending Cancel Request, MsgID: " << request->MessageID << ", PresID: " << presId_);
		OFCondition cond = DIMSE_sendCancelRequest(assoc_, presId_, request->MessageID);
		if (cond.bad())
		{
			OFString temp_str;
			DCMNET_ERROR("Cancel Request Failed: " << DimseCondition::dump(temp_str, cond));
		}
	}
}



/* ---------------- class CMoveExecuteSCU ---------------- */


CWEMoveSCU::CWEMoveSCU(void)
	:m_pEventHandler(NULL)
	,m_Callback(-1)
	,m_moveSCULogger( OFLog::getLogger("WePACS.apps." "moveSCU"))
	,opt_outputDirectory("D:\\dcm")
	,opt_retrievePort(104)
{
	//=================================================================================
	// 这个地方要对Appender进行操作
	/* Setup DICOM connection parameters */ 
	
	InitLog4Cplus();

}


CWEMoveSCU::~CWEMoveSCU(void)
{
}

int CWEMoveSCU::PerformRetrieve(const char * strIP, int nPort, const char * strCallingAE, const char * strCalledAE, const char * strDestdAE, OFList<DcmDataset>& vecDataset, OFList<OFString>& vecTagValue, const char * strRetrieveLevel)
{
	OFList<OFString>      fileNameList;
	OFBool                opt_abortAssociation = OFFalse;
	const char *          opt_abstractSyntax = UID_FINDModalityWorklistInformationModel;
	int                   opt_acse_timeout = 30;
	T_DIMSE_BlockingMode  opt_blockMode = DIMSE_BLOCKING;
	OFCmdSignedInt        opt_cancelAfterNResponses = -1;
	int                   opt_dimse_timeout = 0;
	OFBool                opt_extractResponsesToFile = OFFalse;
	OFCmdUnsignedInt      opt_maxReceivePDULength = ASC_DEFAULTMAXPDU;
	E_TransferSyntax      opt_networkTransferSyntax = EXS_Unknown;
	const char *          opt_ourTitle = APPLICATIONTITLE;
	const char *          opt_peer;
	const char *          opt_peerTitle = PEERAPPLICATIONTITLE;
	OFCmdUnsignedInt      opt_port = 104;
	OFCmdUnsignedInt      opt_repeatCount = 1;
	OFBool                opt_secureConnection = OFFalse; /* default: no secure connection */
	DcmDataset            overrideKeys;


	const char * opt_moveDestination;
	QueryModel opt_queryModel;



	char tempstr[20];
	OFString temp_str;

	/*
	** Don't let dcmdata remove tailing blank padding or perform other
	** maipulations.  We want to see the real data.
	*/
	dcmEnableAutomaticInputDataCorrection.set(OFFalse);


	/* command line parameters */

	opt_ourTitle = "FINDSCU";
	opt_peerTitle = "SVPACS";
	opt_peer = "192.168.1.103";
	opt_port = 1005;
	//overrideKeys.push_back("0x0010,0x0040=M");

	opt_peer = strIP;
	opt_port = nPort;
	opt_ourTitle = strCallingAE;
	opt_peerTitle = strCalledAE;

	opt_moveDestination = strDestdAE;

	//OFString strDestAE;
	//strDestAE = opt_moveDestination;
	//if ( strDestAE.length() <= 0)
	//{
	//	opt_retrievePort = 0;
	//}
	//else
	//{
	//	opt_retrievePort = 104;
	//}

	opt_retrievePort = 10005;

	//opt_networkTransferSyntax = EXS_Unknown;
	opt_networkTransferSyntax = EXS_LittleEndianExplicit;
	//opt_networkTransferSyntax = EXS_BigEndianExplicit;
	//opt_networkTransferSyntax = EXS_LittleEndianImplicit;


	//opt_abstractSyntax = UID_FINDModalityWorklistInformationModel;
	//opt_abstractSyntax = UID_FINDPatientRootQueryRetrieveInformationModel;
	//opt_abstractSyntax = UID_FINDStudyRootQueryRetrieveInformationModel;
	//opt_abstractSyntax = UID_RETIRED_FINDPatientStudyOnlyQueryRetrieveInformationModel;
	OFString strModel(strRetrieveLevel);
	if (strModel.compare("STUDY") == 0)
	{
		opt_queryModel = QMStudyRoot;
		opt_abstractSyntax = UID_MOVEStudyRootQueryRetrieveInformationModel;
	}
	if (strModel.compare("PATIENT") == 0)
	{
		opt_queryModel = QMPatientRoot;
		opt_abstractSyntax = UID_MOVEPatientRootQueryRetrieveInformationModel;
	}
	else
	{
		opt_queryModel = QMStudyRoot;
		opt_abstractSyntax = UID_MOVEStudyRootQueryRetrieveInformationModel;
	}



	CMoveExecuteSCU moveSCU;
	OFCondition cond;


	fileNameList.clear();

	if (false)
	{
		cond = moveSCU.performRetrieve(
			opt_peer,
			opt_port,
			opt_ourTitle,
			opt_peerTitle,
			opt_abstractSyntax,
			opt_retrievePort,
			opt_moveDestination,
			opt_outputDirectory.c_str(),
			opt_networkTransferSyntax,
			opt_blockMode,
			opt_dimse_timeout,
			opt_maxReceivePDULength,
			opt_queryModel,
			opt_abortAssociation,
			opt_repeatCount,
			opt_cancelAfterNResponses,
			//&overrideKeys,
			NULL, /* we do not want to override keys */
			NULL, /* we want to use the default callback */
				  //&m_Callback,
			&fileNameList);

	}
	else
	{
		cond = moveSCU.performRetrievebyDataset(
			opt_peer,
			opt_port,
			opt_ourTitle,
			opt_peerTitle,
			opt_abstractSyntax,
			opt_retrievePort,
			opt_moveDestination,
			opt_outputDirectory.c_str(),
			opt_networkTransferSyntax,
			opt_blockMode,
			opt_dimse_timeout,
			opt_maxReceivePDULength,
			opt_queryModel,
			opt_abortAssociation,
			opt_repeatCount,
			opt_cancelAfterNResponses,
			//&overrideKeys,
			NULL, /* we do not want to override keys */
			NULL, /* we want to use the default callback */
			//&m_Callback,
			&vecDataset);

	}

	// end of the find request
	if (m_pEventHandler != NULL)
	{
		//m_pEventHandler->OnRetrieveEnd();
	}

	if (cond.bad())
	{
		OFLOG_ERROR(m_moveSCULogger, DimseCondition::dump(temp_str, cond));
	}


	return 0;




}

bool CWEMoveSCU::InitLog4Cplus()
{
	/* specify log pattern */
	OFauto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout("%D{%Y-%m-%d %H:%M:%S.%q} %5p: %m%n"));

	//建立Gui日志输出类
	log4cplus::SharedAppenderPtr guiAppender(new log4cplus::CWEAppender());
	guiAppender->setLayout(layout);

	//获取全局日志对象
	log4cplus::Logger log = log4cplus::Logger::getRoot();
	//去除所有日志输出类
	log.removeAllAppenders();
	//加入Gui输出类
	log.addAppender(guiAppender);
	//设置日志输出层
	//log.setLogLevel(OFLogger::INFO_LOG_LEVEL);

	log.setLogLevel(OFLogger::DEBUG_LOG_LEVEL);

	return true;

}

bool CWEMoveSCU::ParseJsontoDcmDataset(DcmDataset & dcmDataset, const char * strJson)
{

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(strJson, root, false))
	{
		return false;
	}

	dcmDataset.clear();

	dcmDataset.putAndInsertString(DCM_AccessionNumber, root["AccessionNumber"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_PatientAge, root["PatientAge"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_PatientBirthDate, root["PatientBirthDate"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_PatientID, root["PatientID"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_PatientName, root["PatientName"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_PatientSex, root["PatientSex"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_PatientWeight, root["PatientWeight"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_QueryRetrieveLevel, root["QueryRetrieveLevel"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_StudyDate, root["StudyDate"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_ReferringPhysicianName, root["ReferringPhysicianName"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_RetrieveAETitle, root["RetrieveAETitle"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_StudyDescription, root["StudyDescription"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_StudyID, root["StudyID"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_StudyInstanceUID, root["StudyInstanceUID"].asString().c_str());
	dcmDataset.putAndInsertString(DCM_StudyTime, root["StudyTime"].asString().c_str());

	return true;
}


void CWEMoveSCU::SetEventHandler(IWEMoveSCUEventHandler * pEventHandler)
{
	m_pEventHandler = pEventHandler;
	m_Callback.SetEventHandler(pEventHandler);
}


bool CWEMoveSCU::SendRetireve(const char* pszIP,
	int nPort,
	const char* pszCallingAE,
	const char* pszCalledAE,
	const char* pszDestAE,
	const char* pszSearchMask
	)
{
	OFList<DcmDataset> datasetList;
	OFList<OFString> tagList;

	if (strlen(pszSearchMask) <= 0)
	{
		return false;
	}
	DcmDataset dataset;
	OFString strQueryLevel;

	if (!ParseJsontoDcmDataset(dataset, pszSearchMask))
	{
		return false;
	}

	datasetList.push_back(dataset);

	dataset.findAndGetOFString(DCM_QueryRetrieveLevel, strQueryLevel);
	if (strQueryLevel.length() <= 0)
	{
		return false;
	}

	PerformRetrieve(pszIP, nPort, pszCallingAE, pszCalledAE, pszDestAE, datasetList, tagList, strQueryLevel.c_str());
	return true;
}



