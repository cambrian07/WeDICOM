#pragma once

#include "dcmtkinclude.h"

#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/ofcond.h"    /* for class OFCondition */
#include "dcmtk/dcmdata/dcxfer.h"  /* for E_TransferSyntax */
#include "dcmtk/dcmnet/dimse.h"    /* for T_DIMSE_BlockingMode */

// forward declarations of classes and structs
class DcmDataset;
class DcmTransportLayer;
class OFConsoleApplication;
struct T_ASC_Association;
struct T_ASC_Parameters;
struct T_DIMSE_C_MoveRQ;
struct T_DIMSE_C_MoveRSP;

enum QueryModel{
	QMPatientRoot = 0,
	QMStudyRoot = 1,
	QMPatientStudyOnly = 2
} ;

struct QuerySyntax{
	const char *findSyntax;
	const char *moveSyntax;
};

static QuerySyntax querySyntax[3] = {
	{ UID_FINDPatientRootQueryRetrieveInformationModel,
	UID_MOVEPatientRootQueryRetrieveInformationModel },
	{ UID_FINDStudyRootQueryRetrieveInformationModel,
	UID_MOVEStudyRootQueryRetrieveInformationModel },
	{ UID_RETIRED_FINDPatientStudyOnlyQueryRetrieveInformationModel,
	UID_RETIRED_MOVEPatientStudyOnlyQueryRetrieveInformationModel }
};


/** Abstract base class for Find SCU callbacks. During a C-FIND operation, the 
*  callback() method of a callback handler object derived from this class is 
*  called once for each incoming C-FIND-RSP message. The callback method has 
*  access to the original C-FIND-RQ message (but not the request dataset), the 
*  current C-FIND-RSP message including its dataset, the number of the current 
*  request, the association over which the request is received and the 
*  presentation context ID. The callback is needed to process the incoming
*  message (e.g., display on screen, add to some list, store to file). The
*  callback may also issue a C-FIND-CANCEL message if needed. Implementations
*  may provide their own callbacks, which must be derived from this base
*  class.
*/
class CMoveExecuteSCUCallback 
{ 
public: 

	/// default constructor
	CMoveExecuteSCUCallback();

	/// destructor
	virtual ~CMoveExecuteSCUCallback() {}

public:
	/** callback method that is called once for each incoming C-FIND-RSP message.
	*  @param request DIMSE command of the original C-FIND request
	*  @param responseCount number of current response
	*  @param rsp DIMSE command of incoming C-FIND response
	*  @param responseIdentifiers dataset of incoming C-FIND response
	*/
	virtual void callback(
		T_DIMSE_C_MoveRQ *request,
		int responseCount,
		T_DIMSE_C_MoveRSP *rsp) = 0;

	/** assigns a value to member variable assoc_. Used by FindSCU code 
	*  (class CMoveExecuteSCU) to store a pointer to the current association
	*  before the callback object is used.
	*  @param assoc pointer to current association
	*/
	void setAssociation(T_ASC_Association *assoc);

	/** assigns a value to member variable presId_. Used by FindSCU code 
	*  (class CMoveExecuteSCU) to store the current presentation context ID
	*  before the callback object is used.
	*  @param presId current presentation context ID
	*/
	void setPresentationContextID(T_ASC_PresentationContextID presId);

protected: /* the two member variables are protected and can be accessed from derived classes */

	/// pointer to current association. Will contain valid value when callback() is called.
	T_ASC_Association *assoc_;

	/// current presentation context ID. Will contain valid value when callback() is called.
	T_ASC_PresentationContextID presId_;
};


/** Default implementation of FindSCU callback class. This implementation is 
*  used when no explicit callback is passed by the user, e.g. in the findscu tool.
*/
class CMoveExecuteSCUDefaultCallback: public CMoveExecuteSCUCallback
{
public:
	/** constructor
	*  @param extractResponsesToFile if true, C-FIND-RSP datasets will be stored as DICOM files
	*  @param cancelAfterNResponses if non-negative, a C-FIND-CANCEL will be issued after the
	*    given number of incoming C-FIND-RSP messages
	*/
	CMoveExecuteSCUDefaultCallback(
		int cancelAfterNResponses);

	/// destructor
	virtual ~CMoveExecuteSCUDefaultCallback() {}

public:
	/** callback method that is called once for each incoming C-FIND-RSP message.
	*  @param request DIMSE command of the original C-FIND request
	*  @param responseCount number of current response
	*  @param rsp DIMSE command of incoming C-FIND response
	*  @param responseIdentifiers dataset of incoming C-FIND response
	*/
	virtual void callback(
		T_DIMSE_C_MoveRQ *request,
		int responseCount,
		T_DIMSE_C_MoveRSP *rsp);

private:


	/// if non-negative, a C-FIND-CANCEL will be issued after the given number of incoming C-FIND-RSP messages
	int cancelAfterNResponses_;
};




class CMoveExecuteSCUSubOpCallback
{
public:

	/// default constructor
	CMoveExecuteSCUSubOpCallback();

	/// destructor
	virtual ~CMoveExecuteSCUSubOpCallback() {}

public:
	/** callback method that is called once for each incoming C-FIND-RSP message.
	*  @param request DIMSE command of the original C-FIND request
	*  @param responseCount number of current response
	*  @param rsp DIMSE command of incoming C-FIND response
	*  @param responseIdentifiers dataset of incoming C-FIND response
	*/
	virtual void callback(T_ASC_Network *aNet, T_ASC_Association **subAssoc) = 0;

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
};


class CMoveExecuteSCUSubOpDefaultCallback : public CMoveExecuteSCUSubOpCallback
{
public:

	/// default constructor
	CMoveExecuteSCUSubOpDefaultCallback();

	/// destructor
	virtual ~CMoveExecuteSCUSubOpDefaultCallback() {}

public:
	/** callback method that is called once for each incoming C-FIND-RSP message.
	*  @param request DIMSE command of the original C-FIND request
	*  @param responseCount number of current response
	*  @param rsp DIMSE command of incoming C-FIND response
	*  @param responseIdentifiers dataset of incoming C-FIND response
	*/
	virtual void callback(T_ASC_Network *aNet, T_ASC_Association **subAssoc);

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
};




/** This class implements a complete DICOM C-FIND SCU, including association set-up, execution of the
*  C-FIND transaction including processing of results, and closing of the association. By default,
*  incoming C-FIND-RSP messages will be displayed on console and, optionally, also stored in files.
*  By providing a user-defined callback, other types of processing are possible.
*/
class CMoveExecuteSCU
{
public:

	/// constructor, does not execute any network-related code
	CMoveExecuteSCU();

	/// destructor. Destroys network structure if not done already.
	virtual ~CMoveExecuteSCU();


public:

    void addOverrideKey(OFConsoleApplication& app, const char* s);

	OFCondition addPresentationContext(T_ASC_Parameters *params, T_ASC_PresentationContextID pid, const char* abstractSyntax);

	void substituteOverrideKeys(DcmDataset *dset);

	OFCondition moveSCU(T_ASC_Association * assoc, DcmDataset* dataset);

	OFCondition	cmove(T_ASC_Association * assoc, DcmDataset* dataset);

	OFCondition	cmove(T_ASC_Association * assoc, const char* fname);


	/** main worker method that negotiates an association, executes one or more
	*  C-FIND-RQ transactions, processes the responses and closes the association
	*  once everything is finished (or an error has occured).
	*  @param peer hostname or IP address of peer SCP host
	*  @param port TCP port number of peer SCP host
	*  @param ourTitle calling AE title
	*  @param peerTitle called AE title
	*  @param abstractSyntax SOP Class UID or Meta SOP Class UID of service
	*  @param preferredTransferSyntax. May be Unknown, Implicit Little Endian, or any of the
	*    two uncompressed explicit VR transfer syntaxes. By default (unknown), local endian
	*    explicit VR is proposed first, followed by opposite endian explicit VR, followed by
	*    implicit VR. This behaviour can be modified by explicitly specifying the preferred
	*    explicit VR transfer syntax. With Little Endian Implicit, only Implicit VR is proposed.
	*  @param blockMode DIMSE blocking mode
	*  @param dimse_timeout timeout for DIMSE operations (in seconds)
	*  @param maxReceivePDULength limit the maximum PDU size for incoming PDUs to the given value.
	*    This value should be less than or equal to ASC_DEFAULTMAXPDU, and is usually identical
	*    to ASC_DEFAULTMAXPDU (other values are only useful for debugging purposes).
	*  @param secureConnection this flag, if true, requests a secure TLS connection to be used
	*    instead of a normal connection. This will only work if DCMTK has been compiled with
	*    OpenSSL support (WITH_OPENSSL) and if a transport layer object supporting secure
	*    connections has been set with setTransportLayer() prior to this call.
	*  @param abortAssociation abort association instead of releasing it (for debugging purposes)
	*  @param repeatCount number of times this query should be repeated
	*    (for debugging purposes, works only with default callback)
	*  @param extractResponsesToFile if true, extract incoming response messages to file
	*    (works only with default callback)
	*  @param cancelAfterNResponses issue C-FIND-CANCEL after given number of responses
	*    (works only with default callback)
	*  @param overrideKeys list of keys/paths that override those in the query files, if any.
	*    Either the list of query files or override keys or both should be non-empty, because the query
	*    dataset will be empty otherwise. For path syntax see DcmPath.
	*  @param callback user-provided non-default callback handler object.
	*    For default callback, pass NULL.
	*  @param fileNameList list of query files. Each file is expected to be a DICOM file
	*    containing a dataset that is used as a query, possibly modified by override keys, if any.
	*    This parameter, if non-NULL, points to a list of filenames (paths).
	*  @return EC_Normal if successful, an error code otherwise
	*/
	OFCondition performRetrieve(
		const char * peer,
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
		DcmDataset* pOverrideKeys = NULL,
		CMoveExecuteSCUCallback * callback  = NULL,
		OFList<OFString> *fileNameList = NULL);

	// dataset
	/** main worker method that negotiates an association, executes one or more
	*  C-FIND-RQ transactions, processes the responses and closes the association
	*  once everything is finished (or an error has occured).
	*  @param peer hostname or IP address of peer SCP host
	*  @param port TCP port number of peer SCP host
	*  @param ourTitle calling AE title
	*  @param peerTitle called AE title
	*  @param abstractSyntax SOP Class UID or Meta SOP Class UID of service
	*  @param preferredTransferSyntax. May be Unknown, Implicit Little Endian, or any of the
	*    two uncompressed explicit VR transfer syntaxes. By default (unknown), local endian
	*    explicit VR is proposed first, followed by opposite endian explicit VR, followed by
	*    implicit VR. This behaviour can be modified by explicitly specifying the preferred
	*    explicit VR transfer syntax. With Little Endian Implicit, only Implicit VR is proposed.
	*  @param blockMode DIMSE blocking mode
	*  @param dimse_timeout timeout for DIMSE operations (in seconds)
	*  @param maxReceivePDULength limit the maximum PDU size for incoming PDUs to the given value.
	*    This value should be less than or equal to ASC_DEFAULTMAXPDU, and is usually identical
	*    to ASC_DEFAULTMAXPDU (other values are only useful for debugging purposes).
	*  @param secureConnection this flag, if true, requests a secure TLS connection to be used
	*    instead of a normal connection. This will only work if DCMTK has been compiled with
	*    OpenSSL support (WITH_OPENSSL) and if a transport layer object supporting secure
	*    connections has been set with setTransportLayer() prior to this call.
	*  @param abortAssociation abort association instead of releasing it (for debugging purposes)
	*  @param repeatCount number of times this query should be repeated
	*    (for debugging purposes, works only with default callback)
	*  @param extractResponsesToFile if true, extract incoming response messages to file
	*    (works only with default callback)
	*  @param cancelAfterNResponses issue C-FIND-CANCEL after given number of responses
	*    (works only with default callback)
	*  @param overrideKeys list of keys/paths that override those in the query files, if any.
	*    Either the list of query files or override keys or both should be non-empty, because the query
	*    dataset will be empty otherwise. For path syntax see DcmPath.
	*  @param callback user-provided non-default callback handler object.
	*    For default callback, pass NULL.
	*  @param datasetList list of query datasets. Each file is expected to be a DICOM file
	*    containing a dataset that is used as a query, possibly modified by override keys, if any.
	*    This parameter, if non-NULL, points to a list of filenames (paths).
	*  @return EC_Normal if successful, an error code otherwise
	*/
	OFCondition CMoveExecuteSCU::performRetrieve(
		const char * peer,
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
		DcmDataset* pOverrideKeys = NULL,
		CMoveExecuteSCUCallback * callback  = NULL,
		OFList<DcmDataset>* datasetList = NULL);



private:

	OFCmdUnsignedInt	opt_retrievePort;
	OFString	opt_outputDirectory;
	int	opt_acse_timeout;
	OFCmdUnsignedInt	opt_maxPDU;

	OFBool	opt_abortAssociation;
	QueryModel	opt_queryModel;

	DcmDataset * overrideKeys;
	E_TransferSyntax	opt_out_networkTransferSyntax;
	OFCmdSignedInt	opt_cancelAfterNResponses;
	const char * 	opt_moveDestination;
	T_DIMSE_BlockingMode	opt_blockMode;
	int	opt_dimse_timeout;
	OFCmdUnsignedInt	opt_repeatCount;
	OFBool	opt_ignorePendingDatasets;
	T_ASC_Network *	net;



private:

	CMoveExecuteSCUCallback* m_pCallback;


};

