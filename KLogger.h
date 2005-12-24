#pragma once

#include <Stamina\LoggerImpl.h>
#include <Konnekt\core_plugin.h>

namespace Konnekt {


	class KLogger: public Stamina::LoggerImpl {
	public:

		KLogger(Plugin& plugin, Stamina::LogLevel level):LoggerImpl(level),_plugin(plugin) {

		}

		void logMsg(Stamina::LogLevel level, const char* module, const char* where, const StringRef& msg);

	private:

		Plugin& _plugin;

	};


};