#pragma once

#include <Stamina\LoggerImpl.h>

namespace Konnekt {


	class KLogger: public Stamina::LoggerImpl {
	public:

		KLogger(Stamina::LogLevel level):LoggerImpl(level) {
		}

		void logMsg(Stamina::LogLevel level, const char* module, const char* where, const char* msg) {
			if ((!module || !*module) && (!where || !*where)) 
				IMessage(IMC_LOG , 0,0,(LPARAM)msg,level);
			else {
				std::string str;
				if (module && *module) {
					str += module;
				}
				if (where && *where) {
					if (!str.empty())
						str += "::";
					str += where;
				}
				if (!str.empty())
					str += " : ";
				str += msg;
				IMessage(IMC_LOG , 0,0,(LPARAM)str.c_str(),level);
			}
		}


	};


};