#pragma once
#include "dcmtkinclude.h"
#include "dcmtk/oflog/config.h"
#include "dcmtk/oflog/appender.h"
#include "dcmtk/oflog/helpers/property.h"
#include <sstream>

interface ISvDICOMEventHandler;

namespace log4cplus 
{

	class CWEAppender : public Appender
	{

	public:
		CWEAppender();
		CWEAppender(const log4cplus::helpers::Properties& properties, log4cplus::tstring& error);
		virtual ~CWEAppender();

		// Methods
		virtual void close();

	// protected function
	protected:
		virtual void append(const log4cplus::spi::InternalLoggingEvent& event);
		//����һ��ostringsteam����
		STD_NAMESPACE ostringstream outString ;

	// private function
	private:
		// Disallow copying of instances of this class
		CWEAppender(const CWEAppender&);
		CWEAppender& operator=(const CWEAppender&);

	// public function
	public:
		void SetEventHandler(ISvDICOMEventHandler* pEventHandler);


	// private parameters
	private:
		// ����Ҫ�и�ָ�룬ָ��LogInfo����
		
		ISvDICOMEventHandler *    m_pEventHandler;





	};
} // end namespace log4cplus

