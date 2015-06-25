#include "MoveSCUSubOpSCP.h"

static void		storeSCPCallback(
	/* in */
	void *callbackData,
	T_DIMSE_StoreProgress *progress,    /* progress state */
	T_DIMSE_C_StoreRQ *req,             /* original store request */
	char *imageFileName, DcmDataset **imageDataSet, /* being received into */
													/* out */
	T_DIMSE_C_StoreRSP *rsp,            /* final store response */
	DcmDataset **statusDetail)
{
	CMoveExecuteSCUSubOpStoreSCPCallback *callback = OFreinterpret_cast(CMoveExecuteSCUSubOpStoreSCPCallback *, callbackData);
	if (callback) callback->callback(progress, req, imageFileName, imageDataSet, rsp, statusDetail);
}



/* ---------------- class CMoveExecuteSCUSubOpStoreSCPCallback ---------------- */
CMoveExecuteSCUSubOpStoreSCPCallback::CMoveExecuteSCUSubOpStoreSCPCallback()
{
}

void CMoveExecuteSCUSubOpStoreSCPCallback::setAssociation(T_ASC_Association * pAssoc)
{
	assoc = pAssoc;
}

void CMoveExecuteSCUSubOpStoreSCPCallback::setDcmFileFormat(DcmFileFormat * pDcmff)
{
	dcmff = pDcmff;
}

void CMoveExecuteSCUSubOpStoreSCPCallback::setImageFileName(const char * strImageFileName)
{
	imageFileName = strImageFileName;
}


/* ---------------- class CMoveExecuteSCUSubOpStoreSCPDefaultCallback ---------------- */
CMoveExecuteSCUSubOpStoreSCPDefaultCallback::CMoveExecuteSCUSubOpStoreSCPDefaultCallback()
	:opt_sleepDuring(0),
	opt_useMetaheader(OFTrue),
	opt_writeTransferSyntax(EXS_Unknown),
	opt_groupLength(EGL_recalcGL),
	opt_sequenceType(EET_ExplicitLength),
	opt_paddingType(EPD_withoutPadding),
	opt_filepad(0),
	opt_itempad(0),
	opt_bitPreserving(OFFalse),
	opt_ignore(OFFalse),
	opt_abortDuringStore(OFFalse),
	opt_abortAfterStore(OFFalse),
	opt_correctUIDPadding(OFFalse),
	opt_outputDirectory(".")



{



}

void CMoveExecuteSCUSubOpStoreSCPDefaultCallback::callback(T_DIMSE_StoreProgress * progress, T_DIMSE_C_StoreRQ * req, char * imageFileName, DcmDataset ** imageDataSet, T_DIMSE_C_StoreRSP * rsp, DcmDataset ** statusDetail)
{
	DIC_UI sopClass;
	DIC_UI sopInstance;

	if ((opt_abortDuringStore && progress->state != DIMSE_StoreBegin) ||
		(opt_abortAfterStore && progress->state == DIMSE_StoreEnd)) {
		DCMNET_INFO("ABORT initiated (due to command line options)");
		ASC_abortAssociation(assoc);
		rsp->DimseStatus = STATUS_STORE_Refused_OutOfResources;
		return;
	}

	if (opt_sleepDuring > 0)
	{
		OFStandard::sleep(OFstatic_cast(unsigned int, opt_sleepDuring));
	}

	// dump some information if required (depending on the progress state)
	// We can't use oflog for the pdu output, but we use a special logger for
	// generating this output. If it is set to level "INFO" we generate the
	// output, if it's set to "DEBUG" then we'll assume that there is debug output
	// generated for each PDU elsewhere.

	//OFLogger progressLogger = OFLog::getLogger("dcmtk.apps." OFFIS_CONSOLE_APPLICATION ".progress");
	//if (progressLogger.getChainedLogLevel() == OFLogger::INFO_LOG_LEVEL)
	{
		switch (progress->state)
		{
		case DIMSE_StoreBegin:
			COUT << "RECV: ";
			break;
		case DIMSE_StoreEnd:
			COUT << OFendl;
			break;
		default:
			COUT << '.';
			break;
		}
		COUT.flush();
	}

	if (progress->state == DIMSE_StoreEnd)
	{
		*statusDetail = NULL;    /* no status detail */

								 /* could save the image somewhere else, put it in database, etc */
								 /*
								 * An appropriate status code is already set in the resp structure, it need not be success.
								 * For example, if the caller has already detected an out of resources problem then the
								 * status will reflect this.  The callback function is still called to allow cleanup.
								 */
								 // rsp->DimseStatus = STATUS_Success;

		if ((imageDataSet != NULL) && (*imageDataSet != NULL) && !opt_bitPreserving && !opt_ignore)
		{
			//StoreCallbackData *cbdata = OFstatic_cast(StoreCallbackData*, callbackData);
			/* create full path name for the output file */
			OFString ofname;
			OFStandard::combineDirAndFilename(ofname, opt_outputDirectory, imageFileName, OFTrue /* allowEmptyDirName */);

			E_TransferSyntax xfer = opt_writeTransferSyntax;
			if (xfer == EXS_Unknown) xfer = (*imageDataSet)->getOriginalXfer();

			OFCondition cond = dcmff->saveFile(ofname.c_str(), xfer, opt_sequenceType, opt_groupLength,
				opt_paddingType, OFstatic_cast(Uint32, opt_filepad), OFstatic_cast(Uint32, opt_itempad),
				(opt_useMetaheader) ? EWM_fileformat : EWM_dataset);
			if (cond.bad())
			{
				DCMNET_ERROR("cannot write DICOM file: " << ofname);
				rsp->DimseStatus = STATUS_STORE_Refused_OutOfResources;
			}

			/* should really check the image to make sure it is consistent,
			* that its sopClass and sopInstance correspond with those in
			* the request.
			*/
			if ((rsp->DimseStatus == STATUS_Success) && !opt_ignore)
			{
				/* which SOP class and SOP instance ? */
				if (!DU_findSOPClassAndInstanceInDataSet(*imageDataSet, sopClass, sopInstance, opt_correctUIDPadding))
				{
					DCMNET_FATAL("bad DICOM file: " << imageFileName);
					rsp->DimseStatus = STATUS_STORE_Error_CannotUnderstand;
				}
				else if (strcmp(sopClass, req->AffectedSOPClassUID) != 0)
				{
					rsp->DimseStatus = STATUS_STORE_Error_DataSetDoesNotMatchSOPClass;
				}
				else if (strcmp(sopInstance, req->AffectedSOPInstanceUID) != 0)
				{
					rsp->DimseStatus = STATUS_STORE_Error_DataSetDoesNotMatchSOPClass;
				}
			}
		}
	}
}



/* ---------------- class CMoveSCUSubOpSCP ---------------- */
CMoveSCUSubOpSCP::CMoveSCUSubOpSCP()
	: opt_sleepAfter(0),
	opt_sleepDuring(0),
	opt_maxPDU(ASC_DEFAULTMAXPDU),
	opt_useMetaheader(OFTrue),
	opt_acceptAllXfers(OFFalse),
	opt_bitPreserving(OFFalse),
	opt_ignore(OFFalse),
	opt_in_networkTransferSyntax(EXS_Unknown),
	opt_blockMode(DIMSE_BLOCKING),
	opt_dimse_timeout(0)


{

}

CMoveSCUSubOpSCP::~CMoveSCUSubOpSCP()
{
}

OFCondition CMoveSCUSubOpSCP::acceptSubAssoc(T_ASC_Network * aNet, T_ASC_Association ** assoc)
{
	const char* knownAbstractSyntaxes[] = {
		UID_VerificationSOPClass
	};
	const char* transferSyntaxes[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	int numTransferSyntaxes;

	OFCondition cond = ASC_receiveAssociation(aNet, assoc, opt_maxPDU);
	if (cond.good())
	{
		switch (opt_in_networkTransferSyntax)
		{
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
		case EXS_JPEGProcess14SV1TransferSyntax:
			/* we prefer JPEGLossless:Hierarchical-1stOrderPrediction (default lossless) */
			transferSyntaxes[0] = UID_JPEGProcess14SV1TransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_JPEGProcess1TransferSyntax:
			/* we prefer JPEGBaseline (default lossy for 8 bit images) */
			transferSyntaxes[0] = UID_JPEGProcess1TransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_JPEGProcess2_4TransferSyntax:
			/* we prefer JPEGExtended (default lossy for 12 bit images) */
			transferSyntaxes[0] = UID_JPEGProcess2_4TransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_JPEG2000LosslessOnly:
			/* we prefer JPEG2000 Lossless */
			transferSyntaxes[0] = UID_JPEG2000LosslessOnlyTransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_JPEG2000:
			/* we prefer JPEG2000 Lossy */
			transferSyntaxes[0] = UID_JPEG2000TransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_JPEGLSLossless:
			/* we prefer JPEG-LS Lossless */
			transferSyntaxes[0] = UID_JPEGLSLosslessTransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_JPEGLSLossy:
			/* we prefer JPEG-LS Lossy */
			transferSyntaxes[0] = UID_JPEGLSLossyTransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_MPEG2MainProfileAtMainLevel:
			/* we prefer MPEG2 MP@ML */
			transferSyntaxes[0] = UID_MPEG2MainProfileAtMainLevelTransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_MPEG2MainProfileAtHighLevel:
			/* we prefer MPEG2 MP@HL */
			transferSyntaxes[0] = UID_MPEG2MainProfileAtHighLevelTransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
		case EXS_RLELossless:
			/* we prefer RLE Lossless */
			transferSyntaxes[0] = UID_RLELosslessTransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
#ifdef WITH_ZLIB
		case EXS_DeflatedLittleEndianExplicit:
			/* we prefer Deflated Explicit VR Little Endian */
			transferSyntaxes[0] = UID_DeflatedExplicitVRLittleEndianTransferSyntax;
			transferSyntaxes[1] = UID_LittleEndianExplicitTransferSyntax;
			transferSyntaxes[2] = UID_BigEndianExplicitTransferSyntax;
			transferSyntaxes[3] = UID_LittleEndianImplicitTransferSyntax;
			numTransferSyntaxes = 4;
			break;
#endif
		default:
			if (opt_acceptAllXfers)
			{
				/* we accept all supported transfer syntaxes
				* (similar to "AnyTransferSyntax" in "storescp.cfg")
				*/
				transferSyntaxes[0] = UID_JPEG2000TransferSyntax;
				transferSyntaxes[1] = UID_JPEG2000LosslessOnlyTransferSyntax;
				transferSyntaxes[2] = UID_JPEGProcess2_4TransferSyntax;
				transferSyntaxes[3] = UID_JPEGProcess1TransferSyntax;
				transferSyntaxes[4] = UID_JPEGProcess14SV1TransferSyntax;
				transferSyntaxes[5] = UID_JPEGLSLossyTransferSyntax;
				transferSyntaxes[6] = UID_JPEGLSLosslessTransferSyntax;
				transferSyntaxes[7] = UID_RLELosslessTransferSyntax;
				transferSyntaxes[8] = UID_MPEG2MainProfileAtMainLevelTransferSyntax;
				transferSyntaxes[9] = UID_MPEG2MainProfileAtHighLevelTransferSyntax;
				transferSyntaxes[10] = UID_DeflatedExplicitVRLittleEndianTransferSyntax;
				if (gLocalByteOrder == EBO_LittleEndian)
				{
					transferSyntaxes[11] = UID_LittleEndianExplicitTransferSyntax;
					transferSyntaxes[12] = UID_BigEndianExplicitTransferSyntax;
				}
				else {
					transferSyntaxes[11] = UID_BigEndianExplicitTransferSyntax;
					transferSyntaxes[12] = UID_LittleEndianExplicitTransferSyntax;
				}
				transferSyntaxes[13] = UID_LittleEndianImplicitTransferSyntax;
				numTransferSyntaxes = 14;
			}
			else {
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
			}
			break;

		}

		/* accept the Verification SOP Class if presented */
		cond = ASC_acceptContextsWithPreferredTransferSyntaxes(
			(*assoc)->params,
			knownAbstractSyntaxes, DIM_OF(knownAbstractSyntaxes),
			transferSyntaxes, numTransferSyntaxes);

		if (cond.good())
		{
			/* the array of Storage SOP Class UIDs comes from dcuid.h */
			cond = ASC_acceptContextsWithPreferredTransferSyntaxes(
				(*assoc)->params,
				dcmAllStorageSOPClassUIDs, numberOfAllDcmStorageSOPClassUIDs,
				transferSyntaxes, numTransferSyntaxes);
		}
	}
	if (cond.good()) cond = ASC_acknowledgeAssociation(*assoc);
	if (cond.bad()) {
		ASC_dropAssociation(*assoc);
		ASC_destroyAssociation(assoc);
	}
	return cond;
}



OFCondition CMoveSCUSubOpSCP::subOpSCP(T_ASC_Association **subAssoc)
{
	T_DIMSE_Message     msg;
	T_ASC_PresentationContextID presID;

	if (!ASC_dataWaiting(*subAssoc, 0)) /* just in case */
		return DIMSE_NODATAAVAILABLE;

	OFCondition cond = DIMSE_receiveCommand(*subAssoc, opt_blockMode, opt_dimse_timeout, &presID,
		&msg, NULL);

	if (cond == EC_Normal) {
		switch (msg.CommandField)
		{
		case DIMSE_C_STORE_RQ:
			cond = storeSCP(*subAssoc, &msg, presID);
			break;
		case DIMSE_C_ECHO_RQ:
			cond = echoSCP(*subAssoc, &msg, presID);
			break;
		default:
			cond = DIMSE_BADCOMMANDTYPE;
			DCMNET_ERROR("cannot handle command: 0x" << STD_NAMESPACE hex << OFstatic_cast(unsigned, msg.CommandField));
			break;
		}
	}
	/* clean up on association termination */
	if (cond == DUL_PEERREQUESTEDRELEASE)
	{
		cond = ASC_acknowledgeRelease(*subAssoc);
		ASC_dropSCPAssociation(*subAssoc);
		ASC_destroyAssociation(subAssoc);
		return cond;
	}
	else if (cond == DUL_PEERABORTEDASSOCIATION)
	{
	}
	else if (cond != EC_Normal)
	{
		OFString temp_str;
		DCMNET_ERROR("DIMSE failure (aborting sub-association): " << DimseCondition::dump(temp_str, cond));
		/* some kind of error so abort the association */
		cond = ASC_abortAssociation(*subAssoc);
	}

	if (cond != EC_Normal)
	{
		ASC_dropAssociation(*subAssoc);
		ASC_destroyAssociation(subAssoc);
	}
	return cond;
}


OFCondition CMoveSCUSubOpSCP::echoSCP(
	T_ASC_Association * assoc,
	T_DIMSE_Message * msg,
	T_ASC_PresentationContextID presID)
{
	OFString temp_str;
	DCMNET_INFO("Received Echo Request");
	DCMNET_DEBUG(DIMSE_dumpMessage(temp_str, msg->msg.CEchoRQ, DIMSE_INCOMING));

	/* the echo succeeded !! */
	OFCondition cond = DIMSE_sendEchoResponse(assoc, presID, &msg->msg.CEchoRQ, STATUS_Success, NULL);
	if (cond.bad())
	{
		DCMNET_ERROR("Echo SCP Failed: " << DimseCondition::dump(temp_str, cond));
	}
	return cond;
}


OFCondition CMoveSCUSubOpSCP::storeSCP(
	T_ASC_Association *assoc,
	T_DIMSE_Message *msg,
	T_ASC_PresentationContextID presID)
{
	OFCondition cond = EC_Normal;
	T_DIMSE_C_StoreRQ *req;
	char imageFileName[2048];

	req = &msg->msg.CStoreRQ;

	if (opt_ignore)
	{
#ifdef _WIN32
		tmpnam(imageFileName);
#else
		strcpy(imageFileName, NULL_DEVICE_NAME);
#endif
	}
	else {
		sprintf(imageFileName, "%s.%s",
			dcmSOPClassUIDToModality(req->AffectedSOPClassUID),
			req->AffectedSOPInstanceUID);
	}

	OFString temp_str;
	DCMNET_INFO( "Received Store Request: MsgID " << req->MessageID << ", ("
		<< dcmSOPClassUIDToModality(req->AffectedSOPClassUID, "OT") << ")");
	DCMNET_DEBUG( DIMSE_dumpMessage(temp_str, *req, DIMSE_INCOMING, NULL, presID));


	//StoreCallbackData callbackData;
	//callbackData.assoc = assoc;
	//callbackData.imageFileName = imageFileName;
	//DcmFileFormat dcmff;
	//callbackData.dcmff = &dcmff;


	// for test
	CMoveExecuteSCUSubOpStoreSCPCallback * callbackData = NULL;

	/* prepare the callback data */
	DcmFileFormat dcmff;
	CMoveExecuteSCUSubOpStoreSCPDefaultCallback defaultCallback;
	if (callbackData == NULL) callbackData = &defaultCallback;
	callbackData->setAssociation(assoc);
	callbackData->setDcmFileFormat(&dcmff);
	callbackData->setImageFileName(imageFileName);

	// store SourceApplicationEntityTitle in metaheader
	if (assoc && assoc->params)
	{
		const char *aet = assoc->params->DULparams.callingAPTitle;
		if (aet) dcmff.getMetaInfo()->putAndInsertString(DCM_SourceApplicationEntityTitle, aet);
	}

	DcmDataset *dset = dcmff.getDataset();

	if (opt_bitPreserving)
	{
		cond = DIMSE_storeProvider(assoc, presID, req, imageFileName, opt_useMetaheader,
			NULL, storeSCPCallback, OFreinterpret_cast(void*, &callbackData), opt_blockMode, opt_dimse_timeout);
	}
	else {
		cond = DIMSE_storeProvider(assoc, presID, req, NULL, opt_useMetaheader,
			&dset, storeSCPCallback, OFreinterpret_cast(void*, &callbackData), opt_blockMode, opt_dimse_timeout);
	}

	if (cond.bad())
	{
		DCMNET_ERROR( "Store SCP Failed: " << DimseCondition::dump(temp_str, cond));
		/* remove file */
		if (!opt_ignore)
		{
			if (strcmp(imageFileName, NULL_DEVICE_NAME) != 0) unlink(imageFileName);
		}
#ifdef _WIN32
	}
	else if (opt_ignore) {
		if (strcmp(imageFileName, NULL_DEVICE_NAME) != 0) unlink(imageFileName); // delete the temporary file
#endif
	}

	if (opt_sleepAfter > 0)
	{
		OFStandard::sleep(OFstatic_cast(unsigned int, opt_sleepDuring));
	}
	return cond;
}










