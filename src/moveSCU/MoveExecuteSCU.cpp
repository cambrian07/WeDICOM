#include "MoveExecuteSCU.h"
#include "MoveSCUSubOpSCP.h"

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#define INCLUDE_CSTDARG
#define INCLUDE_CERRNO

/* ---------------- static functions ---------------- */

#define OFFIS_CONSOLE_APPLICATION "movescu"

static void moveCallback(
	void *callbackData,
	T_DIMSE_C_MoveRQ *request,
	int responseCount,
	T_DIMSE_C_MoveRSP *rsp)
{
	CMoveExecuteSCUCallback *pCallback = OFreinterpret_cast(CMoveExecuteSCUCallback *, callbackData);
	if (pCallback) pCallback->callback(request, responseCount, rsp);
}


static void	subOpCallback(void *callbackData, T_ASC_Network *aNet, T_ASC_Association **subAssoc)
{
	CMoveExecuteSCUSubOpCallback *pCallback = OFreinterpret_cast(CMoveExecuteSCUSubOpCallback *, callbackData);
	if (pCallback) pCallback->callback(aNet, subAssoc);
}


/* ---------------- class CMoveExecuteSCUCallback ---------------- */

CMoveExecuteSCUCallback::CMoveExecuteSCUCallback()
	: assoc_(NULL)
	, presId_(0)
{
}

void CMoveExecuteSCUCallback::setAssociation(T_ASC_Association *assoc)
{
	assoc_ = assoc;
}

void CMoveExecuteSCUCallback::setPresentationContextID(T_ASC_PresentationContextID presId)
{
	presId_ = presId;
}

/* ---------------- class CMoveExecuteSCUDefaultCallback ---------------- */

CMoveExecuteSCUDefaultCallback::CMoveExecuteSCUDefaultCallback(
	int cancelAfterNResponses)
	: CMoveExecuteSCUCallback()
	, cancelAfterNResponses_(cancelAfterNResponses)
{
}

void CMoveExecuteSCUDefaultCallback::callback(
	T_DIMSE_C_MoveRQ *request,
	int responseCount,
	T_DIMSE_C_MoveRSP *rsp)
{
	OFCondition cond = EC_Normal;

	OFString temp_str;
	DCMNET_INFO("Move Response " << responseCount << ":" << OFendl << DIMSE_dumpMessage(temp_str, *rsp, DIMSE_INCOMING));

	/* should we send a cancel back ?? */
	if (cancelAfterNResponses_ == responseCount) {
		DCMNET_INFO("Sending Cancel Request: MsgID " << request->MessageID
			<< ", PresID " << presId_);
		cond = DIMSE_sendCancelRequest(assoc_,
			presId_, request->MessageID);
		if (cond != EC_Normal) {
			DCMNET_ERROR("Cancel Request Failed: " << DimseCondition::dump(temp_str, cond));
		}
	}
}




/* ---------------- class CMoveExecuteSCUSubOpCallback ---------------- */

CMoveExecuteSCUSubOpCallback::CMoveExecuteSCUSubOpCallback()
{
}



/* ---------------- class CMoveExecuteSCUSubOpDefaultCallback ---------------- */
CMoveExecuteSCUSubOpDefaultCallback::CMoveExecuteSCUSubOpDefaultCallback()
{
}

void CMoveExecuteSCUSubOpDefaultCallback::callback(T_ASC_Network * aNet, T_ASC_Association ** subAssoc)
{
	if (aNet == NULL) return;   /* help no net ! */


	if (*subAssoc == NULL) {
		/* negotiate association */
		subOpSCP.acceptSubAssoc(aNet, subAssoc);
	}
	else {
		/* be a service class provider */
		subOpSCP.subOpSCP(subAssoc);
	}
}




/* ---------------- class CMoveExecuteSCU ---------------- */

CMoveExecuteSCU::CMoveExecuteSCU()
	:opt_repeatCount(1),
	opt_out_networkTransferSyntax(EXS_Unknown),
	opt_moveDestination(NULL),
	opt_cancelAfterNResponses(-1),
	opt_blockMode(DIMSE_BLOCKING),
	opt_dimse_timeout(0),
	opt_ignorePendingDatasets(OFTrue),
	net(NULL), /* the global DICOM network */
	overrideKeys(NULL),
	opt_maxPDU(ASC_DEFAULTMAXPDU),
	opt_retrievePort(0),
	opt_abortAssociation(OFFalse),
	opt_queryModel(QMPatientRoot),
	opt_acse_timeout(30),
	opt_outputDirectory("."),
	m_pCallback(NULL),
	m_pSubOpCallback(NULL)

{

	//opt_queryModel = QMPatientRoot;
	//opt_queryModel = QMStudyRoot;
	//opt_queryModel = QMPatientStudyOnly;

}

CMoveExecuteSCU::~CMoveExecuteSCU()
{
}

void CMoveExecuteSCU::addOverrideKey(OFConsoleApplication& app, const char* s)
{
	unsigned int g = 0xffff;
	unsigned int e = 0xffff;
	int n = 0;
	OFString dicName, valStr;
	OFString msg;
	char msg2[200];

	// try to parse group and element number
	n = sscanf(s, "%x,%x=", &g, &e);
	OFString toParse = s;
	size_t eqPos = toParse.find('=');
	if (n < 2)  // if at least no tag could be parsed
	{
		// if value is given, extract it (and extrect dictname)
		if (eqPos != OFString_npos)
		{
			dicName = toParse.substr(0, eqPos).c_str();
			valStr = toParse.substr(eqPos + 1, toParse.length());
		}
		else // no value given, just dictionary name
			dicName = s; // only dictionary name given (without value)
						 // try to lookup in dictionary
		DcmTagKey key(0xffff, 0xffff);
		const DcmDataDictionary& globalDataDict = dcmDataDict.rdlock();
		const DcmDictEntry *dicent = globalDataDict.findEntry(dicName.c_str());
		dcmDataDict.unlock();
		if (dicent != NULL) {
			// found dictionary name, copy group and element number
			key = dicent->getKey();
			g = key.getGroup();
			e = key.getElement();
		}
		else {
			// not found in dictionary
			msg = "bad key format or dictionary name not found in dictionary: ";
			msg += dicName;
			app.printError(msg.c_str());
		}
	} // tag could be parsed, copy value if it exists
	else
	{
		if (eqPos != OFString_npos)
			valStr = toParse.substr(eqPos + 1, toParse.length());
	}
	DcmTag tag(g, e);
	if (tag.error() != EC_Normal) {
		sprintf(msg2, "unknown tag: (%04x,%04x)", g, e);
		app.printError(msg2);
	}
	DcmElement *elem = newDicomElement(tag);
	if (elem == NULL) {
		sprintf(msg2, "cannot create element for tag: (%04x,%04x)", g, e);
		app.printError(msg2);
	}
	if (valStr.length() > 0) {
		if (elem->putString(valStr.c_str()).bad())
		{
			sprintf(msg2, "cannot put tag value: (%04x,%04x)=\"", g, e);
			msg = msg2;
			msg += valStr;
			msg += "\"";
			app.printError(msg.c_str());
		}
	}

	if (overrideKeys == NULL) overrideKeys = new DcmDataset;
	if (overrideKeys->insert(elem, OFTrue).bad()) {
		sprintf(msg2, "cannot insert tag: (%04x,%04x)", g, e);
		app.printError(msg2);
	}
}


OFCondition CMoveExecuteSCU::addPresentationContext(T_ASC_Parameters *params,T_ASC_PresentationContextID pid,const char* abstractSyntax)
{
	/*
	** We prefer to use Explicitly encoded transfer syntaxes.
	** If we are running on a Little Endian machine we prefer
	** LittleEndianExplicitTransferSyntax to BigEndianTransferSyntax.
	** Some SCP implementations will just select the first transfer
	** syntax they support (this is not part of the standard) so
	** organise the proposed transfer syntaxes to take advantage
	** of such behaviour.
	**
	** The presentation contexts proposed here are only used for
	** C-FIND and C-MOVE, so there is no need to support compressed
	** transmission.
	*/

	const char* transferSyntaxes[] = { NULL, NULL, NULL };
	int numTransferSyntaxes = 0;

	switch (opt_out_networkTransferSyntax) {
	case EXS_LittleEndianImplicit:
		/* we only support Little Endian Implicit */
		transferSyntaxes[0] = UID_LittleEndianImplicitTransferSyntax;
		numTransferSyntaxes = 1;
		break;
	case EXS_LittleEndianExplicit:
		/* we prefer Little Endian Explicit */
		transferSyntaxes[0] = UID_LittleEndianExplicitTransferSyntax;
		transferSyntaxes[1] = UID_BigEndianExplicitTransferSyntax;
		transferSyntaxes[2] = UID_LittleEndianImplicitTransferSyntax;
		numTransferSyntaxes = 3;
		break;
	case EXS_BigEndianExplicit:
		/* we prefer Big Endian Explicit */
		transferSyntaxes[0] = UID_BigEndianExplicitTransferSyntax;
		transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
		transferSyntaxes[2] = UID_LittleEndianImplicitTransferSyntax;
		numTransferSyntaxes = 3;
		break;
	default:
		/* We prefer explicit transfer syntaxes.
		* If we are running on a Little Endian machine we prefer
		* LittleEndianExplicitTransferSyntax to BigEndianTransferSyntax.
		*/
		if (gLocalByteOrder == EBO_LittleEndian)  /* defined in dcxfer.h */
		{
			transferSyntaxes[0] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[1] = UID_BigEndianExplicitTransferSyntax;
		}
		else {
			transferSyntaxes[0] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
		}
		transferSyntaxes[2] = UID_LittleEndianImplicitTransferSyntax;
		numTransferSyntaxes = 3;
		break;
	}

	return ASC_addPresentationContext(
		params, pid, abstractSyntax,
		transferSyntaxes, numTransferSyntaxes);
}

void CMoveExecuteSCU::substituteOverrideKeys(DcmDataset *dset)
{
	if (overrideKeys == NULL) {
		return; /* nothing to do */
	}

	/* copy the override keys */
	DcmDataset keys(*overrideKeys);

	/* put the override keys into dset replacing existing tags */
	unsigned long elemCount = keys.card();
	for (unsigned long i = 0; i < elemCount; i++) {
		DcmElement *elem = keys.remove(OFstatic_cast(unsigned long, 0));

		dset->insert(elem, OFTrue);
	}
}



OFCondition CMoveExecuteSCU::moveSCU(T_ASC_Association * assoc, DcmDataset* dataset)
{
	T_ASC_PresentationContextID presId;
	T_DIMSE_C_MoveRQ    req;
	T_DIMSE_C_MoveRSP   rsp;
	DIC_US              msgId = assoc->nextMsgID++;
	DcmDataset          *rspIds = NULL;
	const char          *sopClass;
	DcmDataset          *statusDetail = NULL;

	//DcmFileFormat dcmff;

	//if (fname != NULL) {
	//	if (dcmff.loadFile(fname).bad()) {
	//		DCMNET_ERROR( "bad DICOM file: " << fname << ": " << dcmff.error().text());
	//		return DIMSE_BADDATA;
	//	}
	//}

	/* replace specific keys by those in overrideKeys */
	//substituteOverrideKeys(dcmff.getDataset());
	substituteOverrideKeys(dataset);

	sopClass = querySyntax[opt_queryModel].moveSyntax;

	/* which presentation context should be used */
	presId = ASC_findAcceptedPresentationContextID(assoc, sopClass);
	if (presId == 0) return DIMSE_NOVALIDPRESENTATIONCONTEXTID;

	//if (movescuLogger.isEnabledFor(OFLogger::INFO_LOG_LEVEL))
	{
		DCMNET_INFO( "Sending Move Request: MsgID " << msgId);
		//DCMNET_INFO( "Request:" << OFendl << DcmObject::PrintHelper(*dcmff.getDataset()));
		DCMNET_INFO( "Request:" << OFendl << DcmObject::PrintHelper(*dataset));
	}

	//callbackData.assoc = assoc;
	//callbackData.presId = presId;

	// for test
	//CMoveExecuteSCUCallback* callback = NULL;

	/* prepare the callback data */
	CMoveExecuteSCUDefaultCallback defaultCallback(opt_cancelAfterNResponses);
	if (m_pCallback == NULL) m_pCallback = &defaultCallback;
	m_pCallback->setAssociation(assoc);
	m_pCallback->setPresentationContextID(presId);


	CMoveExecuteSCUSubOpDefaultCallback defaultSubOpCallback;
	if (m_pSubOpCallback == NULL)
	{
		m_pSubOpCallback = &defaultSubOpCallback;
	}

	req.MessageID = msgId;
	strcpy(req.AffectedSOPClassUID, sopClass);
	req.Priority = DIMSE_PRIORITY_MEDIUM;
	req.DataSetType = DIMSE_DATASET_PRESENT;
	if (opt_moveDestination == NULL) {
		/* set the destination to be me */
		ASC_getAPTitles(assoc->params, req.MoveDestination,
			NULL, NULL);
	}
	else {
		strcpy(req.MoveDestination, opt_moveDestination);
	}

	OFCondition cond;

	cond = DIMSE_moveUser(assoc, presId, &req, dataset,
		moveCallback, m_pCallback, opt_blockMode, opt_dimse_timeout, net, subOpCallback,
		m_pSubOpCallback, &rsp, &statusDetail, &rspIds, opt_ignorePendingDatasets);

	if (cond == EC_Normal) {
		OFString temp_str;
		DCMNET_INFO( DIMSE_dumpMessage(temp_str, rsp, DIMSE_INCOMING));
		if (rspIds != NULL) {
			DCMNET_INFO( "Response Identifiers:" << OFendl << DcmObject::PrintHelper(*rspIds));
		}
	}
	else {
		OFString temp_str;
		DCMNET_ERROR( "Move Request Failed: " << DimseCondition::dump(temp_str, cond));
	}
	if (statusDetail != NULL) {
		DCMNET_WARN( "Status Detail:" << OFendl << DcmObject::PrintHelper(*statusDetail));
		delete statusDetail;
	}

	if (rspIds != NULL) delete rspIds;

	return cond;
}


OFCondition CMoveExecuteSCU::cmove(T_ASC_Association * assoc, DcmDataset* dataset)
{
	OFCondition cond = EC_Normal;
	int n = OFstatic_cast(int, opt_repeatCount);
	while (cond.good() && n--)
		cond = moveSCU(assoc, dataset);
	return cond;
}

OFCondition CMoveExecuteSCU::cmove(T_ASC_Association * assoc, const char * fname)
{
	OFCondition cond = EC_Normal;
	DcmFileFormat dcmff;

	if (fname != NULL) {
		if (dcmff.loadFile(fname).bad()) {
			DCMNET_ERROR( "bad DICOM file: " << fname << ": " << dcmff.error().text());
			return DIMSE_BADDATA;
		}
	}

	int n = OFstatic_cast(int, opt_repeatCount);
	while (cond.good() && n--)
		cond = moveSCU(assoc, dcmff.getDataset());
	return cond;

}


OFCondition CMoveExecuteSCU::performRetrieve(const char * peer, 
	                                         unsigned int port,
	                                         const char * ourTitle,
	                                         const char * peerTitle,
	                                         const char * abstractSyntax,
	                                         unsigned int retrievePort,
	                                         const char * moveDestination,
	                                         const char * outputDirectory,
	                                         E_TransferSyntax preferredTransferSyntax,
	                                         T_DIMSE_BlockingMode blockMode, 
	                                         int dimse_timeout, 
	                                         Uint32 maxReceivePDULength,
	                                         QueryModel queryModel,
	                                         OFBool abortAssociation,
	                                         unsigned int repeatCount, 
	                                         int cancelAfterNResponses,
	                                         DcmDataset* pOverrideKeys/* = NULL*/,
	                                         CMoveExecuteSCUCallback * callback /* = NULL*/,
	                                         OFList<OFString>* fileNameList/* = NULL*/)
{

	OFString temp_str;

	T_ASC_Parameters *params = NULL;
	const char *opt_peer;
	OFCmdUnsignedInt opt_port = 104;
	DIC_NODENAME localHost;
	DIC_NODENAME peerHost;
	T_ASC_Association *assoc = NULL;
	const char *opt_peerTitle = "QRSCP";
	const char *opt_ourTitle = "WEPACS";

	opt_peer = peer;
	opt_port = port;
	opt_peerTitle = peerTitle;
	opt_ourTitle = ourTitle;


	opt_retrievePort = retrievePort;
	opt_moveDestination = moveDestination;
	opt_outputDirectory = outputDirectory;
	opt_maxPDU = maxReceivePDULength;

	opt_abortAssociation = abortAssociation;
	opt_queryModel = queryModel;

	overrideKeys = pOverrideKeys;

	opt_out_networkTransferSyntax = preferredTransferSyntax;
	opt_cancelAfterNResponses = cancelAfterNResponses;
	opt_blockMode = blockMode;
	opt_dimse_timeout = dimse_timeout;
	opt_repeatCount = repeatCount;

	m_pCallback = callback;


	/* print resource identifier */
	//DCMNET_DEBUG( rcsid << OFendl);

	/* make sure data dictionary is loaded */
	if (!dcmDataDict.isDictionaryLoaded())
	{
		DCMNET_WARN("no data dictionary loaded, check environment variable: "
			<< DCM_DICT_ENVIRONMENT_VARIABLE);
	}

	/* make sure output directory exists and is writeable */
	if (opt_retrievePort > 0)
	{
		if (!OFStandard::dirExists(opt_outputDirectory))
		{
			DCMNET_FATAL("specified output directory does not exist");

			return EC_CorruptedData;
		}
		else if (!OFStandard::isWriteable(opt_outputDirectory))
		{
			DCMNET_FATAL("specified output directory is not writeable");
			return EC_CorruptedData;
		}
	}



	/* network for move request and responses */
	T_ASC_NetworkRole role = (opt_retrievePort > 0) ? NET_ACCEPTORREQUESTOR : NET_REQUESTOR;
	OFCondition cond = ASC_initializeNetwork(role, OFstatic_cast(int, opt_retrievePort), opt_acse_timeout, &net);
	if (cond.bad())
	{
		DCMNET_FATAL("cannot create network: " << DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}



	/* set up main association */
	cond = ASC_createAssociationParameters(&params, opt_maxPDU);
	if (cond.bad()) {
		DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}
	ASC_setAPTitles(params, opt_ourTitle, opt_peerTitle, NULL);

	gethostname(localHost, sizeof(localHost) - 1);
	sprintf(peerHost, "%s:%d", opt_peer, OFstatic_cast(int, opt_port));
	ASC_setPresentationAddresses(params, localHost, peerHost);

	/*
	* We also add a presentation context for the corresponding
	* find sop class.
	*/
	cond = addPresentationContext(params, 1,
		querySyntax[opt_queryModel].findSyntax);

	cond = addPresentationContext(params, 3,
		querySyntax[opt_queryModel].moveSyntax);

	if (cond.bad()) {
		DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}

	DCMNET_DEBUG("Request Parameters:" << OFendl << ASC_dumpParameters(temp_str, params, ASC_ASSOC_RQ));

	/* create association */
	DCMNET_INFO("Requesting Association");
	cond = ASC_requestAssociation(net, params, &assoc);
	if (cond.bad()) {
		if (cond == DUL_ASSOCIATIONREJECTED) {
			T_ASC_RejectParameters rej;

			ASC_getRejectParameters(params, &rej);
			DCMNET_FATAL("Association Rejected:");
			DCMNET_FATAL(ASC_printRejectParameters(temp_str, &rej));
		}
		else {
			DCMNET_FATAL("Association Request Failed:");
			DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
			return EC_CorruptedData;
		}
	}
	/* what has been accepted/refused ? */
	DCMNET_DEBUG("Association Parameters Negotiated:" << OFendl << ASC_dumpParameters(temp_str, params, ASC_ASSOC_AC));

	if (ASC_countAcceptedPresentationContexts(params) == 0) {
		DCMNET_FATAL("No Acceptable Presentation Contexts");
		return EC_CorruptedData;
	}

	DCMNET_INFO("Association Accepted (Max Send PDV: " << assoc->sendPDVLength << ")");

	/* do the real work */
	cond = EC_Normal;
	if (fileNameList == NULL)
	{
		/* no files provided on command line */
		cond = cmove(assoc, "");
	}
	else {
		OFListIterator(OFString) iter = fileNameList->begin();
		OFListIterator(OFString) enditer = fileNameList->end();
		while ((iter != enditer) && cond.good())
		{
			cond = cmove(assoc, (*iter).c_str());
			++iter;
		}
	}

	/* tear down association */
	if (cond == EC_Normal)
	{
		if (opt_abortAssociation) {
			DCMNET_INFO("Aborting Association");
			cond = ASC_abortAssociation(assoc);
			if (cond.bad()) {
				DCMNET_FATAL("Association Abort Failed: " << DimseCondition::dump(temp_str, cond));
				return EC_CorruptedData;
			}
		}
		else {
			/* release association */
			DCMNET_INFO("Releasing Association");
			cond = ASC_releaseAssociation(assoc);
			if (cond.bad())
			{
				DCMNET_FATAL("Association Release Failed:");
				DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
				return EC_CorruptedData;
			}
		}
	}
	else if (cond == DUL_PEERREQUESTEDRELEASE)
	{
		DCMNET_ERROR("Protocol Error: Peer requested release (Aborting)");
		DCMNET_INFO("Aborting Association");
		cond = ASC_abortAssociation(assoc);
		if (cond.bad()) {
			DCMNET_FATAL("Association Abort Failed: " << DimseCondition::dump(temp_str, cond));
			return EC_CorruptedData;
		}
	}
	else if (cond == DUL_PEERABORTEDASSOCIATION)
	{
		DCMNET_INFO("Peer Aborted Association");
	}
	else
	{
		DCMNET_ERROR("Move SCU Failed: " << DimseCondition::dump(temp_str, cond));
		DCMNET_INFO("Aborting Association");
		cond = ASC_abortAssociation(assoc);
		if (cond.bad()) {
			DCMNET_FATAL("Association Abort Failed: " << DimseCondition::dump(temp_str, cond));
			return EC_CorruptedData;
		}
	}

	cond = ASC_destroyAssociation(&assoc);
	if (cond.bad()) {
		DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}
	cond = ASC_dropNetwork(&net);
	if (cond.bad()) {
		DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}

#ifdef HAVE_WINSOCK_H
	WSACleanup();
#endif

	return EC_Normal;


}


OFCondition CMoveExecuteSCU::performRetrievebyDataset(const char * peer,
	unsigned int port,
	const char * ourTitle,
	const char * peerTitle,
	const char * abstractSyntax,
	unsigned int retrievePort,
	const char * moveDestination,
	const char * outputDirectory,
	E_TransferSyntax preferredTransferSyntax,
	T_DIMSE_BlockingMode blockMode,
	int dimse_timeout,
	Uint32 maxReceivePDULength,
	QueryModel queryModel,
	OFBool abortAssociation,
	unsigned int repeatCount,
	int cancelAfterNResponses,
	DcmDataset* pOverrideKeys/* = NULL*/,
	CMoveExecuteSCUCallback * callback /* = NULL*/,
	OFList<DcmDataset>* datasetList/* = NULL*/)
{

	OFString temp_str;

	T_ASC_Parameters *params = NULL;
	const char *opt_peer;
	OFCmdUnsignedInt opt_port = 104;
	DIC_NODENAME localHost;
	DIC_NODENAME peerHost;
	T_ASC_Association *assoc = NULL;
	const char *opt_peerTitle = "QRSCP";
	const char *opt_ourTitle = "WEPACS";
	//OFList<OFString>* fileNameList = NULL;
	//OFList<DcmDataset>* datasetList;

	opt_peer = peer;
	opt_port = port;
	opt_peerTitle = peerTitle;
	opt_ourTitle = ourTitle;

	opt_retrievePort = retrievePort;
	opt_moveDestination = moveDestination;
	opt_outputDirectory = outputDirectory;
	opt_maxPDU = maxReceivePDULength;

	opt_abortAssociation = abortAssociation;
	opt_queryModel = queryModel;

	overrideKeys = pOverrideKeys;

	opt_out_networkTransferSyntax = preferredTransferSyntax;
	opt_cancelAfterNResponses = cancelAfterNResponses;
	opt_blockMode = blockMode;
	opt_dimse_timeout = dimse_timeout;
	opt_repeatCount = repeatCount;


	m_pCallback = callback;

	/* print resource identifier */
	//DCMNET_DEBUG( rcsid << OFendl);

	/* make sure data dictionary is loaded */
	if (!dcmDataDict.isDictionaryLoaded())
	{
		DCMNET_WARN("no data dictionary loaded, check environment variable: "
			<< DCM_DICT_ENVIRONMENT_VARIABLE);
	}

	/* make sure output directory exists and is writeable */
	if (opt_retrievePort > 0)
	{
		if (!OFStandard::dirExists(opt_outputDirectory))
		{
			DCMNET_FATAL("specified output directory does not exist");

			return EC_CorruptedData;
		}
		else if (!OFStandard::isWriteable(opt_outputDirectory))
		{
			DCMNET_FATAL("specified output directory is not writeable");
			return EC_CorruptedData;
		}
	}



	/* network for move request and responses */
	T_ASC_NetworkRole role = (opt_retrievePort > 0) ? NET_ACCEPTORREQUESTOR : NET_REQUESTOR;
	OFCondition cond = ASC_initializeNetwork(role, OFstatic_cast(int, opt_retrievePort), opt_acse_timeout, &net);
	if (cond.bad())
	{
		DCMNET_FATAL("cannot create network: " << DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}



	/* set up main association */
	cond = ASC_createAssociationParameters(&params, opt_maxPDU);
	if (cond.bad()) {
		DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}
	ASC_setAPTitles(params, opt_ourTitle, opt_peerTitle, NULL);

	gethostname(localHost, sizeof(localHost) - 1);
	sprintf(peerHost, "%s:%d", opt_peer, OFstatic_cast(int, opt_port));
	ASC_setPresentationAddresses(params, localHost, peerHost);

	/*
	* We also add a presentation context for the corresponding
	* find sop class.
	*/
	cond = addPresentationContext(params, 1,
		querySyntax[opt_queryModel].findSyntax);

	cond = addPresentationContext(params, 3,
		querySyntax[opt_queryModel].moveSyntax);

	if (cond.bad()) {
		DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}

	DCMNET_DEBUG("Request Parameters:" << OFendl << ASC_dumpParameters(temp_str, params, ASC_ASSOC_RQ));

	/* create association */
	DCMNET_INFO("Requesting Association");
	cond = ASC_requestAssociation(net, params, &assoc);
	if (cond.bad()) {
		if (cond == DUL_ASSOCIATIONREJECTED) {
			T_ASC_RejectParameters rej;

			ASC_getRejectParameters(params, &rej);
			DCMNET_FATAL("Association Rejected:");
			DCMNET_FATAL(ASC_printRejectParameters(temp_str, &rej));
		}
		else {
			DCMNET_FATAL("Association Request Failed:");
			DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
			return EC_CorruptedData;
		}
	}
	/* what has been accepted/refused ? */
	DCMNET_DEBUG("Association Parameters Negotiated:" << OFendl << ASC_dumpParameters(temp_str, params, ASC_ASSOC_AC));

	if (ASC_countAcceptedPresentationContexts(params) == 0) {
		DCMNET_FATAL("No Acceptable Presentation Contexts");
		return EC_CorruptedData;
	}

	DCMNET_INFO("Association Accepted (Max Send PDV: " << assoc->sendPDVLength << ")");

	/* do the real work */
	cond = EC_Normal;
	if (datasetList == NULL)
	{
		/* no files provided on command line */
		cond = cmove(assoc, "");
	}
	else {
		OFListIterator(DcmDataset) iter = datasetList->begin();
		OFListIterator(DcmDataset) enditer = datasetList->end();
		while ((iter != enditer) && cond.good())
		{
			cond = cmove(assoc, &(*iter));
			++iter;
		}
	}

	/* tear down association */
	if (cond == EC_Normal)
	{
		if (opt_abortAssociation) {
			DCMNET_INFO("Aborting Association");
			cond = ASC_abortAssociation(assoc);
			if (cond.bad()) {
				DCMNET_FATAL("Association Abort Failed: " << DimseCondition::dump(temp_str, cond));
				return EC_CorruptedData;
			}
		}
		else {
			/* release association */
			DCMNET_INFO("Releasing Association");
			cond = ASC_releaseAssociation(assoc);
			if (cond.bad())
			{
				DCMNET_FATAL("Association Release Failed:");
				DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
				return EC_CorruptedData;
			}
		}
	}
	else if (cond == DUL_PEERREQUESTEDRELEASE)
	{
		DCMNET_ERROR("Protocol Error: Peer requested release (Aborting)");
		DCMNET_INFO("Aborting Association");
		cond = ASC_abortAssociation(assoc);
		if (cond.bad()) {
			DCMNET_FATAL("Association Abort Failed: " << DimseCondition::dump(temp_str, cond));
			return EC_CorruptedData;
		}
	}
	else if (cond == DUL_PEERABORTEDASSOCIATION)
	{
		DCMNET_INFO("Peer Aborted Association");
	}
	else
	{
		DCMNET_ERROR("Move SCU Failed: " << DimseCondition::dump(temp_str, cond));
		DCMNET_INFO("Aborting Association");
		cond = ASC_abortAssociation(assoc);
		if (cond.bad()) {
			DCMNET_FATAL("Association Abort Failed: " << DimseCondition::dump(temp_str, cond));
			return EC_CorruptedData;
		}
	}

	cond = ASC_destroyAssociation(&assoc);
	if (cond.bad()) {
		DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}
	cond = ASC_dropNetwork(&net);
	if (cond.bad()) {
		DCMNET_FATAL(DimseCondition::dump(temp_str, cond));
		return EC_CorruptedData;
	}

#ifdef HAVE_WINSOCK_H
	WSACleanup();
#endif

	return EC_Normal;
}

