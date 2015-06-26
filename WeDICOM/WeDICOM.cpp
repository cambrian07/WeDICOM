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
			"ReferringPhysicianName" : "�Ǽ�",
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


	// for test;
	dcmJson["QueryRetrieveLevel"] = "PATIENT";
	dcmJson["PatientName"] = "Liu Qing Zhu";
	//dcmJson["PatientName"] = "Wang^Rongchun";


	std::string strDcmJson = writer.write(dcmJson);

	OutputDebugStringA(strDcmJson.c_str());

	auto eventHandler = new DCMMoverEventHandler();
	CWEFindSCU findSCU;
	findSCU.SetEventHandler(eventHandler);
	//findSCU.SendQuery("192.168.1.108", 1004, "FINDSCU", "SVPACS", "aaa");
	//findSCU.SendQuery("127.0.0.1", 1004, "WEPACS", "DCMQRSCP", strDcmJson.c_str());

	// for junquzongyuan
	findSCU.SendQuery("10.0.55.249", 104, "AE_YDCF", "AE_PACSB", strDcmJson.c_str());


	CWEMoveSCU moveSCU;
	//moveSCU.SendRetireve("127.0.0.1", 1004, "WEPACS", "DCMQRSCP","WEPACS", strDcmJson.c_str());
	//moveSCU.SendRetireve("127.0.0.1", 1004, "WEPACS", "DCMQRSCP",NULL, strDcmJson.c_str());


	// for junquzongyuan
	moveSCU.SendRetireve("10.0.55.249", 104, "AE_YDCF", "AE_PACSB", NULL, strDcmJson.c_str());

	return;
}

