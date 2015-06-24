#include "WEJsonDataset.h"
#include "json/json.h"


CWEJsonDataset::CWEJsonDataset()
{
}


CWEJsonDataset::~CWEJsonDataset()
{
}

OFCondition CWEJsonDataset::writeJson(OFString & strJson)
{
	OFCondition cond;
	if (this->isEmpty())
	{
		strJson = "";
		cond = EC_TagNotFound;
		return cond;
	}


	Json::Value root;
	Json::FastWriter writer;

	//root["xfer"] = getOriginalXfer().getXferID();


	if (!elementList->empty())
	{
		/* write content of all children */
		DcmObject *dO;
		OFString strTagValue;
		elementList->seek(ELP_first);
		do {
			dO = elementList->get();
			//dO->writeXML(out, flags & ~DCMTypes::XF_useDcmtkNamespace);

			findAndGetOFString(dO->getTag(), strTagValue);
			root[DcmTag(dO->getTag()).getTagName()] = strTagValue.c_str();


		} while (elementList->seek(ELP_next));
	}

	strJson = (writer.write(root)).c_str();


	/* always report success */
	return EC_Normal;


	return false;
}

OFCondition CWEJsonDataset::LoadfromJson(const OFString & strJson)
{

	OFCondition cond;

	if (strJson.length() <= 0)
	{
		cond = EC_InvalidStream;
		return cond;
	}


	clear();
	//putAndInsertString(DCM_TransferSyntaxUID, UID_LittleEndianExplicitTransferSyntax);

	putAndInsertString(DCM_AccessionNumber, "");
	putAndInsertString(DCM_PatientAge, "");
	putAndInsertString(DCM_PatientBirthDate, "");
	putAndInsertString(DCM_PatientID, "");
	putAndInsertString(DCM_PatientName, "");
	putAndInsertString(DCM_PatientSex, "");
	putAndInsertString(DCM_PatientWeight, "");
	putAndInsertString(DCM_QueryRetrieveLevel, "STUDY");
	putAndInsertString(DCM_StudyDate, "");
	putAndInsertString(DCM_ReferringPhysicianName, "");
	putAndInsertString(DCM_RetrieveAETitle, "");
	putAndInsertString(DCM_StudyDescription, "");
	putAndInsertString(DCM_StudyID, "");
	putAndInsertString(DCM_StudyInstanceUID, "");
	putAndInsertString(DCM_StudyTime, "");



	OFString strDestJson;
	writeJson(strDestJson);

	OutputDebugStringA(strDestJson.c_str());

	return EC_Normal;

}
