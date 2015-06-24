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
		OFBool extractResponsesToFile,
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

	/// if true, C-FIND-RSP datasets will be stored as DICOM files
	OFBool extractResponsesToFile_;

	/// if non-negative, a C-FIND-CANCEL will be issued after the given number of incoming C-FIND-RSP messages
	int cancelAfterNResponses_;
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

	OFCondition moveSCU(T_ASC_Association * assoc, const char *fname);

	OFCondition	cmove(T_ASC_Association * assoc, const char *fname);




	// callback
	static OFCondition acceptSubAssoc(T_ASC_Network * aNet, T_ASC_Association ** assoc);

	static OFCondition echoSCP(T_ASC_Association * assoc, T_DIMSE_Message * msg, T_ASC_PresentationContextID presID);

	static OFCondition storeSCP(T_ASC_Association *assoc,T_DIMSE_Message *msg,T_ASC_PresentationContextID presID);

	static void		storeSCPCallback(
			/* in */
			void *callbackData,
			T_DIMSE_StoreProgress *progress,    /* progress state */
			T_DIMSE_C_StoreRQ *req,             /* original store request */
			char *imageFileName, DcmDataset **imageDataSet, /* being received into */
															/* out */
			T_DIMSE_C_StoreRSP *rsp,            /* final store response */
			DcmDataset **statusDetail);


	static OFCondition
		subOpSCP(T_ASC_Association **subAssoc);

	static void
		subOpCallback(void * /*subOpCallbackData*/,
			T_ASC_Network *aNet, T_ASC_Association **subAssoc);

	static void
		moveCallback(void *callbackData, T_DIMSE_C_MoveRQ *request,
			int responseCount, T_DIMSE_C_MoveRSP *response);


public:
	/** initialize the network structure. This should be done only once.
	*  @param acse_timeout timeout for ACSE operations, in seconds
	*  @return EC_Normal if successful, an error code otherwise
	*/
	OFCondition initializeNetwork(int acse_timeout);

	/** enable user-defined transport layer. This method is needed when
	*  the network association should use a non-default transport layer
	*  (e.g. a TLS connection). In this case a fully initialized transport
	*  layer object must be passed with this call after a call to 
	*  initializeNetwork, but prior to any call to performQuery.
	*  The transport layer object will not be deleted by this class and
	*  must remain alive until this object is deleted or a new transport
	*  layer is set.
	*  @param tLayer pointer to transport layer object
	*  @return EC_Normal if successful, an error code otherwise
	*/
	OFCondition setTransportLayer(DcmTransportLayer *tLayer);

	/** destroy network struct. This should be done only once.
	*  @return EC_Normal if successful, an error code otherwise
	*/
	OFCondition dropNetwork();




private:




private:


};

