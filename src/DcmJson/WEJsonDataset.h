#pragma once
#include "dcmtkinclude.h"

class CWEJsonDataset : public DcmDataset
{
public:
	CWEJsonDataset();
	~CWEJsonDataset();


public:
	OFCondition writeJson(OFString& strJson);
	OFCondition LoadfromJson(const OFString& strJson);

};

