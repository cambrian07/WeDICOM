#pragma once
#include "MoveExecuteSCU.h"


#include "dcmtkinclude.h"

class IWEMoveSCUEventHandler
{
public:

	// virtual int		OnRetrieveRecord(const char* strRecord)	 = 0;
	// virtual void	OnRetrieveEnd()						     = 0;


};





class CWEMoveSCUCallback: public CMoveExecuteSCUCallback
{
public:
	/** constructor
	*  @param extractResponsesToFile if true, C-FIND-RSP datasets will be stored as DICOM files
	*  @param cancelAfterNResponses if non-negative, a C-FIND-CANCEL will be issued after the
	*    given number of incoming C-FIND-RSP messages
	*/
	CWEMoveSCUCallback(int cancelAfterNResponses = -1);

	/// destructor
	virtual ~CWEMoveSCUCallback() {}

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

	IWEMoveSCUEventHandler * m_pEventHandler;

public:
	void SetEventHandler(IWEMoveSCUEventHandler * pEventHandler);
	void SetEndFlag();

};







class CWEMoveSCU
{
public:
	CWEMoveSCU(void);
	~CWEMoveSCU(void);


// private params
private:
	IWEMoveSCUEventHandler * m_pEventHandler;
	CWEMoveSCUCallback  m_Callback;
	OFLogger m_moveSCULogger;

// private function
private:
	OFCondition PerformRetrieve(const char * strIP,
	                    int nPort, 
		                const char * strCallingAE,
	                	const char * strCalledAE, 
	                	const char * strDestdAE, 
		                OFList<DcmDataset> & vecDataset,
	                	OFList<OFString> & vecTagValue,
	                	const char* strRetrieveLevel = "STUDY"
		                );

	bool InitLog4Cplus();

	bool ParseJsontoDcmDataset(DcmDataset& dcmDataset, const char* strJson);

// public function
public:
	virtual void	SetEventHandler(IWEMoveSCUEventHandler * pEventHandler);

	virtual bool	SendRetrieve(const char* pszIP,
		                         int nPort,
							     const char* pszCallingAE,
							     const char* pszCalledAE,
		                         const char* pszDestAE,
							     const char* pszSearchMask
							     );


protected:
	unsigned int opt_retrievePort;
	OFString opt_outputDirectory;


};

