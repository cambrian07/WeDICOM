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
	OFList<OFString>      overrideKeys;



	char tempstr[20];
	OFString temp_str;

	/*
	** Don't let dcmdata remove tailing blank padding or perform other
	** maipulations.  We want to see the real data.
	*/
	dcmEnableAutomaticInputDataCorrection.set(OFFalse);

	WSAData winSockData;
	/* we need at least version 1.1 */
	WORD winSockVersionNeeded = MAKEWORD( 1, 1 );
	WSAStartup(winSockVersionNeeded, &winSockData);

	/* evaluate command line */
	{
		/* check exclusive options first */

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
			opt_abstractSyntax = UID_MOVEStudyRootQueryRetrieveInformationModel;
		}
		if (strModel.compare("PATIENT") == 0)
		{
			opt_abstractSyntax = UID_MOVEPatientRootQueryRetrieveInformationModel;
		}
		else
		{
			opt_abstractSyntax = UID_MOVEStudyRootQueryRetrieveInformationModel;
		}


		//if (cmd.findOption("--enable-new-vr"))
		if (false)
		{
			dcmEnableUnknownVRGeneration.set(OFTrue);
			dcmEnableUnlimitedTextVRGeneration.set(OFTrue);
		}
		//if (cmd.findOption("--disable-new-vr"))
		if (false)
		{
			dcmEnableUnknownVRGeneration.set(OFFalse);
			dcmEnableUnlimitedTextVRGeneration.set(OFFalse);
		}

		//if (cmd.findOption("--timeout"))
		if (false)
		{
			OFCmdSignedInt opt_timeout = 0;
			dcmConnectionTimeout.set(OFstatic_cast(Sint32, opt_timeout));
		}

		//if (cmd.findOption("--acse-timeout"))
		if (false)
		{
			OFCmdSignedInt opt_timeout = 0;
			opt_acse_timeout = OFstatic_cast(int, opt_timeout);
		}

		//if (cmd.findOption("--dimse-timeout"))
		if (false)
		{
			OFCmdSignedInt opt_timeout = 0;
			opt_dimse_timeout = OFstatic_cast(int, opt_timeout);
			opt_blockMode = DIMSE_NONBLOCKING;
		}

		//if (cmd.findOption("--max-pdu"))
		//{
		//  app.checkValue(cmd.getValueAndCheckMinMax(opt_maxReceivePDULength, ASC_MINIMUMPDUSIZE, ASC_MAXIMUMPDUSIZE));
		//}
		//if (cmd.findOption("--repeat"))
		//{
		//	app.checkValue(cmd.getValueAndCheckMin(opt_repeatCount, 1));
		//}
		//if (cmd.findOption("--abort"))  
		//{
		//	opt_abortAssociation = OFTrue;
		//}
		//if (cmd.findOption("--cancel"))
		//{
		//	app.checkValue(cmd.getValueAndCheckMin(opt_cancelAfterNResponses, 0));
		//}
		//if (cmd.findOption("--extract")) 
		//{
		//	opt_extractResponsesToFile = OFTrue;
		//}

		/* finally parse filenames */
		//for (int i = 3; i <= paramCount; i++)
		//{
		//	cmd.getParam(i, currentFilename);
		//	if (access(currentFilename, R_OK) < 0)
		//	{
		//		errormsg = "cannot access file: ";
		//		errormsg += currentFilename;
		//		app.printError(errormsg.c_str());
		//	}
		//	fileNameList.push_back(currentFilename);
		//}

		//if (fileNameList.empty() && overrideKeys.empty())
		//{
		//	app.printError("either query file or override keys (or both) must be specified");
		//}

	}

	/* print resource identifier */
	//OFLOG_DEBUG(m_moveSCULogger, rcsid << OFendl);

	/* make sure data dictionary is loaded */
	if (!dcmDataDict.isDictionaryLoaded())
	{
		OFLOG_WARN(m_moveSCULogger, "no data dictionary loaded, check environment variable: "
			<< DCM_DICT_ENVIRONMENT_VARIABLE);
	}

	// declare findSCU handler and initialize network

	//CFndExecuteSCU findscu;
	//OFCondition cond = findscu.initializeNetwork(opt_acse_timeout);
	//if (cond.bad()) {
	//	OFLOG_ERROR(m_moveSCULogger, DimseCondition::dump(temp_str, cond));
	//	return 1;
	//}

	CMoveExecuteSCU moveSCU;
	OFCondition cond;
	//OFCondition cond = moveSCU.initializeNetwork(opt_acse_timeout);
	if (cond.bad()) {
		OFLOG_ERROR(m_moveSCULogger, DimseCondition::dump(temp_str, cond));
		return 1;
	}


	// test
	fileNameList.clear();
	//overrideKeys.clear();
	//fileNameList.push_back("F:\\DICOM\\DCMTK学习笔记\\DICOM--worklist\\bin\\SVPACS\\SVPACS\\wlistqry.wl");
	//fileNameList.push_back("g:\\namehello.dcm");

	//DcmFileFormat df;
	//df.loadFile("F:\\DICOM\\DCMTK学习笔记\\DICOM--worklist\\bin\\SVPACS\\SVPACS\\wlistqry.wl");
	//DcmDataset dataset(*df.getDataset());
	//DcmDataset dataset;
	//cond = dataset.putAndInsertString(DcmTag(0x0010,0x0010), "Hello");
	//if (cond.bad())
	//{
	//}
	//dataset.saveFile("g:\\namehello.dcm");

	// do the main work: negotiate network association, perform C-FIND transaction,
	// process results, and finally tear down the association.

	if (false)
	{
		//cond = moveSCU.performQuery(
		//	opt_peer,
		//	opt_port,
		//	opt_ourTitle,
		//	opt_peerTitle,
		//	opt_abstractSyntax,
		//	opt_networkTransferSyntax,
		//	opt_blockMode,
		//	opt_dimse_timeout,
		//	opt_maxReceivePDULength,
		//	opt_secureConnection,
		//	opt_abortAssociation,
		//	opt_repeatCount,
		//	opt_extractResponsesToFile,
		//	opt_cancelAfterNResponses,
		//	&overrideKeys,
		//	//NULL, /* we want to use the default callback */
		//	&m_Callback,
		//	&fileNameList);

	}
	else
	{
		//cond = moveSCU.performQuerybyDataset(
		//	opt_peer,
		//	opt_port,
		//	opt_ourTitle,
		//	opt_peerTitle,
		//	opt_abstractSyntax,
		//	opt_networkTransferSyntax,
		//	opt_blockMode,
		//	opt_dimse_timeout,
		//	opt_maxReceivePDULength,
		//	opt_secureConnection,
		//	opt_abortAssociation,
		//	opt_repeatCount,
		//	opt_extractResponsesToFile,
		//	opt_cancelAfterNResponses,
		//	&overrideKeys,
		//	//NULL, /* we want to use the default callback */
		//	&m_Callback,
		//	&vecDataset);

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

	// destroy network structure
	//cond = moveSCU.dropNetwork();
	if (cond.bad()) 
	{
		OFLOG_ERROR(m_moveSCULogger, DimseCondition::dump(temp_str, cond));
	}

	WSACleanup();

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



