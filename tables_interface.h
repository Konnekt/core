#pragma once

#include "KLogger.h"
#include "tables.h"
#include "plugins.h"

#include <Stamina/DataTable/InterfaceBasic.h>
#include <Stamina/MD5.h>

using namespace Stamina;

namespace Konnekt { namespace Tables {

	class Interface_konnekt: public Interface_passList {
	public:

		Interface_konnekt(bool silent = false) {
			_silent = silent;
		}

		Result confirmFileError(FileBase* file, const StringRef& message, const StringRef& title, DTException& e) {
			Result result = __super::confirmFileError(file, message, title, e);

			mainLogger->log(Stamina::logError, "Tables", "confirmFileError", "%s :: %s (%s) - %d", title.a_str(), message.a_str(), file->getFilename().a_str(), result);

			return result;
		}
		
		void showFileMessage(FileBase* file, const StringRef& _message, const StringRef& _title, bool error) {

			mainLogger->log(Stamina::logError, "Tables", "fileMessage", "%s :: %s (%s)", _title.a_str(), _message.a_str(), file->getFilename().a_str());

			__super::showFileMessage(file, _message, _title, error);
		}

		char* resultToString(Result res) {
			switch (res) {
				case iInterface::fail:
					return "fail";
				case iInterface::failQuiet:
					return "failQuiet";
				case iInterface::retry:
					return "retry";
			};
			return "unknown";
		}

		virtual Result handleFailedLoad(FileBase* file, DTException& e, int retry) {
			mainLogger->log(Stamina::logError, "Tables", "failedLoad", "%s (%s)", e.getReason().a_str(), file->getFilename().a_str());
			if (_silent) return fail;
			Result res = __super::handleFailedLoad(file, e, retry);
			mainLogger->log(Stamina::logError, "Tables", "failedLoad", "=%s %s (%s)", resultToString(res), e.getReason().a_str(), file->getFilename().a_str());
			return res;
		}
		virtual Result handleFailedSave(FileBase* file, DTException& e, int retry) {
			mainLogger->log(Stamina::logError, "Tables", "failedSave", "%s (%s)", e.getReason().a_str(), file->getFilename().a_str());
			if (_silent) return fail;

			Result res = __super::handleFailedSave(file, e, retry);
			mainLogger->log(Stamina::logError, "Tables", "failedSave", "=%s %s (%s)", resultToString(res), e.getReason().a_str(), file->getFilename().a_str());
			return res;
		}
		virtual Result handleFailedAppend(FileBase* file, DTException& e, int retry) {
			mainLogger->log(Stamina::logError, "Tables", "failedAppend", "%s (%s)", e.getReason().a_str(), file->getFilename().a_str());
			if (_silent) return fail;

			Result res = __super::handleFailedAppend(file, e, retry);
			mainLogger->log(Stamina::logError, "Tables", "failedAppend", "=%s %s (%s)", resultToString(res), e.getReason().a_str(), file->getFilename().a_str());
			return res;
		}

		virtual Result handleRestoreBackup(FileBin* file, DTException& e, int retry) {
			if (_silent) return fail;

			Date64 date;
			String found = file->findLastBackupFile("", &date);
			if (found.empty()) {
				return iInterface::fail;
			}
			// o backupy starsze ni¿ 24h pytamy...
			if (((__time64_t)Time64(true) - date.getTime64()) > (60 * 60 * 24)) {
				String title = "Wyst¹pi³ problem podczas wczytywania pliku (" + Stamina::getFileName( file->getFilename() ) + ")";
				String msg = "Znalaz³em kopiê zapasow¹ tego pliku z dnia " + date.strftime("%d-%m-%Y %H:%M:%S") + "\r\n\r\nCzy chcesz jej u¿yæ?";
				if (MessageBox(0, msg.c_str(), title.c_str(), MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL) {
					return iInterface::fail;
				}
			}
			mainLogger->log(Stamina::logError, "Tables", "restoreBackup", "%s -> %s", found.a_str(), file->getFilename().a_str());
			file->restoreBackup(found); 
			return iInterface::retry;
		}

		virtual Stamina::MD5Digest askForPassword(FileBase* file, int retry) {
			if (_silent) return MD5Digest();

			if (plugins.count() > 0) {

				String title = "Has³o pliku";
				String info = "Podaj has³o do pliku " + Stamina::getFileName( file->getFilename() );

				sDIALOG_access sd;
				sd.pass = "";
				sd.save = 0;
				sd.title = title.a_str();
				sd.info = info.a_str();
				if (!ICMessage(IMI_DLGPASS , (int)&sd , 0)) return MD5Digest();

				return DataTable::createPasswordDigest( safeChar(sd.pass) );
			} else {
				return MD5Digest();
			}
		}

	private:
		bool _silent;

	};

	class Interface_konnekt_silent: public Interface_konnekt {
	public:
		Interface_konnekt_silent():Interface_konnekt(true) {

		}
	};

	extern StaticObj<Interface_konnekt> iface;
	extern StaticObj<Interface_konnekt_silent> ifaceSilent;

} }