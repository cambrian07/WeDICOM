#include "WEFindSCU.h"
#include "../logger/WEAppender.h"
#include "../DcmJson/WEJsonDataset.h"


#define OFFIS_CONSOLE_FINDSCU "findscu"

static OFLogger findscuLogger = OFLog::getLogger("dcmtk.apps." OFFIS_CONSOLE_FINDSCU );

/* default application titles */
#define APPLICATIONTITLE        "FINDSCU"
#define PEERAPPLICATIONTITLE    "ANY-SCP"

#define SHORTCOL 4
#define LONGCOL 19


/* ---------------- class CWEFindSCUCallback ---------------- */

CWEFindSCUCallback::CWEFindSCUCallback(int cancelAfterNResponses)
	: CFndExecuteSCUCallback()
	, cancelAfterNResponses_(cancelAfterNResponses)
	, m_pEventHandler(NULL)
{
}

void CWEFindSCUCallback::SetEventHandler(IWEFindSCUEventHandler * pEventHandler)
{
	m_pEventHandler = pEventHandler;
}

void CWEFindSCUCallback::SetEndFlag()
{
	if (m_pEventHandler != NULL)
	{
		m_pEventHandler->OnRetrieveEnd();
	}
}


void CWEFindSCUCallback::callback(
	T_DIMSE_C_FindRQ *request,
	int responseCount,
	T_DIMSE_C_FindRSP *rsp,
	DcmDataset *responseIdentifiers)
{
	/* dump delimiter */
	DCMNET_WARN("---------------------------");

	/* dump response number */
	DCMNET_WARN("Find Response: " << responseCount << " (" << DU_cfindStatusString(rsp->DimseStatus) << ")");

	/* dump data set which was received */
	DCMNET_WARN(DcmObject::PrintHelper(*responseIdentifiers));


	if (m_pEventHandler != NULL)
	{
		OFString strRecord;
		//GetXMLFromDataset((*responseIdentifiers), strXML);
		CWEJsonDataset jsonDataset;
		jsonDataset.copyFrom(*responseIdentifiers);
		jsonDataset.writeJson(strRecord);
		m_pEventHandler->OnRetrieveRecord(strRecord.c_str());
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





CWEFindSCU::CWEFindSCU(void)
	:m_pEventHandler(NULL)
	,m_Callback(-1)
	,m_findSCULogger( OFLog::getLogger("dcmtk.apps." "findSCU"))
{
	//=================================================================================
	// 这个地方要对Appender进行操作
	/* Setup DICOM connection parameters */ 
	
	InitLog4Cplus();

}


CWEFindSCU::~CWEFindSCU(void)
{
}


int CWEFindSCU::PerformQuery(const char * strIP, int nPort, const char * strCallingAE, const char * strCalledAE, OFList<DcmDataset> & vecDataset, OFList<OFString> & vecTagValue)
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
		opt_abstractSyntax = UID_FINDStudyRootQueryRetrieveInformationModel;
		//opt_abstractSyntax = UID_RETIRED_FINDPatientStudyOnlyQueryRetrieveInformationModel;


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
	//OFLOG_DEBUG(findscuLogger, rcsid << OFendl);

	/* make sure data dictionary is loaded */
	if (!dcmDataDict.isDictionaryLoaded())
	{
		OFLOG_WARN(findscuLogger, "no data dictionary loaded, check environment variable: "
			<< DCM_DICT_ENVIRONMENT_VARIABLE);
	}

	// declare findSCU handler and initialize network
	CFndExecuteSCU findscu;
	OFCondition cond = findscu.initializeNetwork(opt_acse_timeout);
	if (cond.bad()) {
		OFLOG_ERROR(findscuLogger, DimseCondition::dump(temp_str, cond));
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
		cond = findscu.performQuery(
			opt_peer,
			opt_port,
			opt_ourTitle,
			opt_peerTitle,
			opt_abstractSyntax,
			opt_networkTransferSyntax,
			opt_blockMode,
			opt_dimse_timeout,
			opt_maxReceivePDULength,
			opt_secureConnection,
			opt_abortAssociation,
			opt_repeatCount,
			opt_extractResponsesToFile,
			opt_cancelAfterNResponses,
			&overrideKeys,
			//NULL, /* we want to use the default callback */
			&m_Callback,
			&fileNameList);

	}
	else
	{
		cond = findscu.performQuerybyDataset(
			opt_peer,
			opt_port,
			opt_ourTitle,
			opt_peerTitle,
			opt_abstractSyntax,
			opt_networkTransferSyntax,
			opt_blockMode,
			opt_dimse_timeout,
			opt_maxReceivePDULength,
			opt_secureConnection,
			opt_abortAssociation,
			opt_repeatCount,
			opt_extractResponsesToFile,
			opt_cancelAfterNResponses,
			&overrideKeys,
			//NULL, /* we want to use the default callback */
			&m_Callback,
			&vecDataset);

	}

	// end of the find request
	if (m_pEventHandler != NULL)
	{
		m_pEventHandler->OnRetrieveEnd();
	}

	if (cond.bad())
	{
		OFLOG_ERROR(findscuLogger, DimseCondition::dump(temp_str, cond));
	}

	// destroy network structure
	cond = findscu.dropNetwork();
	if (cond.bad()) 
	{
		OFLOG_ERROR(findscuLogger, DimseCondition::dump(temp_str, cond));
	}

	WSACleanup();

	return 0;
}

bool CWEFindSCU::InitLog4Cplus()
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


void CWEFindSCU::SetEventHandler(IWEFindSCUEventHandler * pEventHandler)	
{
	m_pEventHandler = pEventHandler;
	m_Callback.SetEventHandler(pEventHandler);
}



bool CWEFindSCU::SendQuery(const char* pszIP,
                  int nPort,
				  const char* pszCallingAE,
				  const char* pszCalledAE,
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

	CWEJsonDataset jsonDataset;
	if ((jsonDataset.LoadfromJson(pszSearchMask)).bad())
	{
		return false;
	}
	dataset.copyFrom(jsonDataset);

	datasetList.push_back(dataset);

	PerformQuery(pszIP, nPort, pszCallingAE, pszCalledAE, datasetList, tagList);
	return true;
}



