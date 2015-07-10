#include "../src/findSCU/WEFindSCU.h"
#include "../src/moveSCU/WEMoveSCU.h"
#include "json/json.h"



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


	// form json string
	/*
	{
		"AccessionNumber": "",
		"PatientAge" : "056Y",
		"PatientBirthDate" : "19580410",
		"PatientID" : "CT378897",
		"PatientName" : "Wang^Rongchun",
		"PatientSex" : "M",
		"PatientWeight" : "10.5",
		"QueryRetrieveLevel" : "STUDY",
		"ReferringPhysicianName" : "µÇ¼Ç",
		"RetrieveAETitle" : "SVPACS",
		"StudyDate" : "",
		"StudyDescription" : "Abdomen",
		"StudyID" : "50122",
		"StudyInstanceUID" : "1.2.840.100.200.300.114040007509",
		"StudyTime" : "083722"
	}
	*/
	Json::FastWriter writer;
	Json::Value dcmJson;

	dcmJson["PatientAge"] = "";
	dcmJson["PatientBirthDate"] = "";
	dcmJson["PatientID"] = "";
	dcmJson["PatientName"] = "";
	dcmJson["PatientSex"] = "";
	dcmJson["PatientWeight"] = "";
	dcmJson["QueryRetrieveLevel"] = "STUDY";
	dcmJson["ReferringPhysicianName"] = "";
	dcmJson["RetrieveAETitle"] = "";
	dcmJson["StudyDate"] = "";
	dcmJson["StudyDescription"] = "";
	dcmJson["StudyID"] = "";
	dcmJson["StudyInstanceUID"] = "";
	dcmJson["StudyTime"] = "";

#define DCMTKQRSCPTEST

#ifdef DCMTKQRSCPTEST


	// for DCMTK qrscp test;
	dcmJson["QueryRetrieveLevel"] = "PATIENT";
	dcmJson["PatientName"] = "Wang^Rongchun";

	std::string strDcmJson = writer.write(dcmJson);
	OutputDebugStringA(strDcmJson.c_str());

	auto eventHandler = new DCMMoverEventHandler();
	CWEFindSCU findSCU;
	findSCU.SetEventHandler(eventHandler);
	findSCU.SendQuery("127.0.0.1", 1004, "WEPACS", "DCMQRSCP", strDcmJson.c_str());

	CWEMoveSCU moveSCU;
	if (!moveSCU.SendRetrieve("127.0.0.1", 1004, "WEPACS", "DCMQRSCP", "WEPACSSTORESCP", strDcmJson.c_str()))
	{
		OutputDebugStringA("Retrieve failed!\n");
	}

	if (!moveSCU.SendRetrieve("127.0.0.1", 1004, "WEPACS", "DCMQRSCP", "WEPACSSTORESCP", strDcmJson.c_str()))
	{
		OutputDebugStringA("Retrieve failed!\n");
	}


	//moveSCU.SendRetrieve("127.0.0.1", 1004, "WEPACS", "DCMQRSCP",NULL, strDcmJson.c_str());


#else

	/*
    PatientName: ZHENG KAI
	PatientID : 6400035041
	PatientSex : M
	PatientBD : 19870710
	StudyID : BP01
	StudyInstanceUID : 1.2.826.0.1.3680043.6.32484.1064.20150709101851.392.32
	StudyDate : 20150709
	StudyTime : 112202
	StudyDescription : Upper Extremities ^ 01_WristHR(Adult)
	AccessionNumber : 9053911
	*/

	// for junzong GE test;
	//dcmJson["QueryRetrieveLevel"] = "PATIENT";
	dcmJson["PatientName"] = "ZHENG KAI";
	dcmJson["PatientID"] = "6400035041";
	dcmJson["StudyID"] = "BP01";

	std::string strDcmJson = writer.write(dcmJson);

	OutputDebugStringA(strDcmJson.c_str());

	auto eventHandler = new DCMMoverEventHandler();
	CWEFindSCU findSCU;
	findSCU.SetEventHandler(eventHandler);

	// for junquzongyuan
	//findSCU.SendQuery("10.0.55.249", 104, "AE_YDCF", "AE_PACSB", strDcmJson.c_str());

	CWEMoveSCU moveSCU;

	// for junquzongyuan
	moveSCU.SendRetrieve("10.0.55.249", 104, "AE_YDCF", "AE_PACSB", "AE_YDCF", strDcmJson.c_str());

	//moveSCU.SendRetrieve("10.0.55.249", 104, "AE_YDCF", "AE_PACSB", "AE_YDCF", strDcmJson.c_str());

#endif

	return;
}

