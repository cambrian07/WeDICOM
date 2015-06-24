#include "../src/findSCU/WEFindSCU.h"



// for findSCU test
class DCMMoverEventHandler : public IWEFindSCUEventHandler
{
public:
	DCMMoverEventHandler();
	~DCMMoverEventHandler();



	virtual int		OnRetrieveRecord(const char* strRecord);
	virtual void	OnRetrieveEnd();

private:

};


int	DCMMoverEventHandler::OnRetrieveRecord(const char* strRecord)
{
	OutputDebugStringA(strRecord);
	printf(strRecord);
	return 0;
}

void DCMMoverEventHandler::OnRetrieveEnd()
{
	OutputDebugStringA("\n ====================End==================== \n");
	printf("\n ====================End==================== \n");
}

DCMMoverEventHandler::DCMMoverEventHandler()
{
}

DCMMoverEventHandler::~DCMMoverEventHandler()
{
}

void main()
{

	DCMMoverEventHandler eventHandler;
	CWEFindSCU findSCU;
	findSCU.SetEventHandler(&eventHandler);
	findSCU.SendQuery("192.168.1.108", 1004, "FINDSCU", "SVPACS", "aaa");



	return ;
}

