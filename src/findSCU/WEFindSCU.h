#pragma once
#include "FndExecuteSCU.h"


#include "dcmtkinclude.h"

class IWEFindSCUEventHandler
{
public:

	virtual int		OnRetrieveRecord(const char* strRecord)	 = 0;
	virtual void	OnRetrieveEnd()						     = 0;


};





class CWEFindSCUCallback: public CFndExecuteSCUCallback
{
public:
	/** constructor
	*  @param extractResponsesToFile if true, C-FIND-RSP datasets will be stored as DICOM files
	*  @param cancelAfterNResponses if non-negative, a C-FIND-CANCEL will be issued after the
	*    given number of incoming C-FIND-RSP messages
	*/
	CWEFindSCUCallback(int cancelAfterNResponses = -1);

	/// destructor
	virtual ~CWEFindSCUCallback() {}

	/** callback method that is called once for each incoming C-FIND-RSP message.
	*  @param request DIMSE command of the original C-FIND request
	*  @param responseCount number of current response
	*  @param rsp DIMSE command of incoming C-FIND response
	*  @param responseIdentifiers dataset of incoming C-FIND response
	*/
	virtual void callback(
		T_DIMSE_C_FindRQ *request,
		int responseCount,
		T_DIMSE_C_FindRSP *rsp,
		DcmDataset *responseIdentifiers);

private:

	/// if non-negative, a C-FIND-CANCEL will be issued after the given number of incoming C-FIND-RSP messages
	int cancelAfterNResponses_;

	IWEFindSCUEventHandler * m_pEventHandler;

public:
	void SetEventHandler(IWEFindSCUEventHandler * pEventHandler);
	void SetEndFlag();

};







class CWEFindSCU
{
public:
	CWEFindSCU(void);
	~CWEFindSCU(void);


// private params
private:
	IWEFindSCUEventHandler * m_pEventHandler;
	CWEFindSCUCallback  m_Callback;
	OFLogger m_findSCULogger;

// private function
private:
	int PerformQuery(const char * strIP, int nPort, const char * strCallingAE, const char * strCalledAE, OFList<DcmDataset> & vecDataset, OFList<OFString> & vecTagValue, const char* strQueryLevel = "STUDY");

	bool InitLog4Cplus();

	bool ParseJsontoDcmDataset(DcmDataset& dcmDataset, const char* strJson);

// public function
public:
	virtual void	SetEventHandler(IWEFindSCUEventHandler * pEventHandler);	

	virtual bool	SendQuery(const char* pszIP,
		                      int nPort,
							  const char* pszCallingAE,
							  const char* pszCalledAE,
							  const char* pszSearchMask
							  );




};

