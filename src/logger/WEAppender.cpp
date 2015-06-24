#include "WEAppender.h"
#include <string>

log4cplus::CWEAppender::CWEAppender()
{

}

log4cplus::CWEAppender::CWEAppender(const log4cplus::helpers::Properties& properties, tstring&)
: Appender(properties)
{

}

log4cplus::CWEAppender::~CWEAppender()
{

}


void log4cplus::CWEAppender::close()
{

}

// This method does not need to be locked since it is called by
// doAppend() which performs the locking
void log4cplus::CWEAppender::append(const spi::InternalLoggingEvent& event)
{
	//格式化输入DCMTK日志
	layout->formatAndAppend(outString, event);
	//获取DCMTK日志字符串流
	STD_NAMESPACE string m = outString.str();
	m.replace(m.size() - 1,-1,"");

	//=============================================================
	// Show Information
	//::MessageBox(NULL,m.c_str(),"",MB_OK);
	OutputDebugStringA(m.c_str());
	OutputDebugStringA("\n");
	//=============================================================
	//清空
	outString.str("");
}

void log4cplus::CWEAppender::SetEventHandler(ISvDICOMEventHandler * pEventHandler)
{
	m_pEventHandler = pEventHandler;
}

