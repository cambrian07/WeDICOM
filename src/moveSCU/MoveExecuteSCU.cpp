#include "MoveExecuteSCU.h"

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#define INCLUDE_CSTDARG
#define INCLUDE_CERRNO

/* ---------------- static functions ---------------- */

#define OFFIS_CONSOLE_APPLICATION "findscu"

static void progressCallback(
	void *callbackData,
	T_DIMSE_C_MoveRQ *request,
	int responseCount,
	T_DIMSE_C_MoveRSP *rsp)
{
	CMoveExecuteSCUCallback *callback = OFreinterpret_cast(CMoveExecuteSCUCallback *, callbackData);
	if (callback) callback->callback(request, responseCount, rsp);
}

/* ---------------- class CFndExecuteSCUCallback ---------------- */

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

/* ---------------- class CFndExecuteSCUCallback ---------------- */

CMoveExecuteSCUDefaultCallback::CMoveExecuteSCUDefaultCallback(
	OFBool extractResponsesToFile,
	int cancelAfterNResponses)
	: CMoveExecuteSCUCallback()
	, extractResponsesToFile_(extractResponsesToFile)
	, cancelAfterNResponses_(cancelAfterNResponses)
{
}

void CMoveExecuteSCUDefaultCallback::callback(
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

	/* in case extractResponsesToFile is set the responses shall be extracted to a certain file */
	if (extractResponsesToFile_) {
		char rspIdsFileName[1024];
		sprintf(rspIdsFileName, "rsp%04d.dcm", responseCount);
		//CMoveExecuteSCU::writeToFile(rspIdsFileName, responseIdentifiers);
	}

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


CMoveExecuteSCU::CMoveExecuteSCU()
{
}

CMoveExecuteSCU::~CMoveExecuteSCU()
{
}

