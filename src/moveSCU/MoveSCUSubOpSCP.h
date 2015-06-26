#pragma once

#include "dcmtkinclude.h"

class CMoveExecuteSCUSubOpStoreSCPCallback
{
public:

	/// default constructor
	CMoveExecuteSCUSubOpStoreSCPCallback();

	/// destructor
	virtual ~CMoveExecuteSCUSubOpStoreSCPCallback() {}

public:
	/** callback method that is called once for each incoming C-FIND-RSP message.
	*  @param request DIMSE command of the original C-FIND request
	*  @param responseCount number of current response
	*  @param rsp DIMSE command of incoming C-FIND response
	*  @param responseIdentifiers dataset of incoming C-FIND response
	*/
	virtual void callback(	
		/* in */
		T_DIMSE_StoreProgress *progress,    /* progress state */
		T_DIMSE_C_StoreRQ *req,             /* original store request */
		char *imageFileName, DcmDataset **imageDataSet, /* being received into */
		/* out */
		T_DIMSE_C_StoreRSP *rsp,            /* final store response */
		DcmDataset **statusDetail) = 0;

	/** assigns a value to member variable assoc_. Used by FindSCU code
	*  (class CMoveExecuteSCU) to store a pointer to the current association
	*  before the callback object is used.
	*  @param assoc pointer to current association
	*/
	void setAssociation(T_ASC_Association * pAssoc);

	void setDcmFileFormat(DcmFileFormat * pDcmff);

	void setImageFileName(const char * strImageFileName);

	/** assigns a value to member variable presId_. Used by FindSCU code
	*  (class CMoveExecuteSCU) to store the current presentation context ID
	*  before the callback object is used.
	*  @param presId current presentation context ID
	*/
	//void setPresentationContextID(T_ASC_PresentationContextID presId);

protected: /* the two member variables are protected and can be accessed from derived classes */

		   /// pointer to current association. Will contain valid value when callback() is called.
		   //T_ASC_Association *assoc_;

		   /// current presentation context ID. Will contain valid value when callback() is called.
		   //T_ASC_PresentationContextID presId_;
	OFString imageFileName;
	DcmFileFormat* dcmff;
	T_ASC_Association* assoc;

};


class CMoveExecuteSCUSubOpStoreSCPDefaultCallback : public CMoveExecuteSCUSubOpStoreSCPCallback
{
public:

	/// default constructor
	CMoveExecuteSCUSubOpStoreSCPDefaultCallback();

	/// destructor
	virtual ~CMoveExecuteSCUSubOpStoreSCPDefaultCallback() {}

public:
	/** callback method that is called once for each incoming C-FIND-RSP message.
	*  @param request DIMSE command of the original C-FIND request
	*  @param responseCount number of current response
	*  @param rsp DIMSE command of incoming C-FIND response
	*  @param responseIdentifiers dataset of incoming C-FIND response
	*/
	virtual void callback(	
		/* in */
		T_DIMSE_StoreProgress *progress,    /* progress state */
		T_DIMSE_C_StoreRQ *req,             /* original store request */
		char *imageFileName, DcmDataset **imageDataSet, /* being received into */
		/* out */
		T_DIMSE_C_StoreRSP *rsp,            /* final store response */
		DcmDataset **statusDetail);

	/** assigns a value to member variable assoc_. Used by FindSCU code
	*  (class CMoveExecuteSCU) to store a pointer to the current association
	*  before the callback object is used.
	*  @param assoc pointer to current association
	*/
	//void setAssociation(T_ASC_Association *assoc);

	/** assigns a value to member variable presId_. Used by FindSCU code
	*  (class CMoveExecuteSCU) to store the current presentation context ID
	*  before the callback object is used.
	*  @param presId current presentation context ID
	*/
	//void setPresentationContextID(T_ASC_PresentationContextID presId);

protected: /* the two member variables are protected and can be accessed from derived classes */

		   /// pointer to current association. Will contain valid value when callback() is called.
		   //T_ASC_Association *assoc_;

		   /// current presentation context ID. Will contain valid value when callback() is called.
		   //T_ASC_PresentationContextID presId_;



	OFBool	opt_abortDuringStore;
	OFBool	opt_abortAfterStore;
	OFCmdUnsignedInt	opt_sleepDuring;
	OFBool	opt_bitPreserving;
	OFBool	opt_ignore;
	OFString	opt_outputDirectory;
	E_TransferSyntax	opt_writeTransferSyntax;
	E_EncodingType	opt_sequenceType;
	E_GrpLenEncoding	opt_groupLength;
	E_PaddingEncoding	opt_paddingType;
	OFBool	opt_useMetaheader;
	OFCmdUnsignedInt	opt_filepad;
	OFCmdUnsignedInt	opt_itempad;
	OFBool	opt_correctUIDPadding;





};



class CMoveSCUSubOpSCP
{
public:
	CMoveSCUSubOpSCP();
	~CMoveSCUSubOpSCP();


public:

	OFCondition acceptSubAssoc(T_ASC_Network * aNet, T_ASC_Association ** assoc);

	OFCondition	subOpSCP(T_ASC_Association **subAssoc);

	OFCondition echoSCP(T_ASC_Association * assoc, T_DIMSE_Message * msg, T_ASC_PresentationContextID presID);

	OFCondition storeSCP(T_ASC_Association *assoc,T_DIMSE_Message *msg,T_ASC_PresentationContextID presID);

private:

	T_DIMSE_BlockingMode	opt_blockMode;
	int	opt_dimse_timeout;
	OFBool	opt_acceptAllXfers;
	E_TransferSyntax	opt_in_networkTransferSyntax;
	OFCmdUnsignedInt	opt_maxPDU;
	OFBool	opt_ignore;
	OFCmdUnsignedInt	opt_sleepAfter;
	OFBool	opt_useMetaheader;
	OFBool	opt_bitPreserving;
	OFCmdUnsignedInt	opt_sleepDuring;


};
