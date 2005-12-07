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

		Interface_konnekt() {

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
		

		virtual Result handleFailedLoad(FileBase* file, DTException& e, int retry) {
			mainLogger->log(Stamina::logError, "Tables", "failedLoad", "%s (%s)", e.getReason().a_str(), file->getFilename().a_str());
			return __super::handleFailedLoad(file, e, retry);
		}
		virtual Result handleFailedSave(FileBase* file, DTException& e, int retry) {
			mainLogger->log(Stamina::logError, "Tables", "failedSave", "%s (%s)", e.getReason().a_str(), file->getFilename().a_str());
			return __super::handleFailedSave(file, e, retry);
		}
		virtual Result handleFailedAppend(FileBase* file, DTException& e, int retry) {
			mainLogger->log(Stamina::logError, "Tables", "failedAppend", "%s (%s)", e.getReason().a_str(), file->getFilename().a_str());
			return __super::handleFailedAppend(file, e, retry);
		}

		virtual Result handleRestoreBackup(FileBin* file, DTException& e, int retry) {

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
					return iInterface::failQuiet;
				}
			}
			mainLogger->log(Stamina::logError, "Tables", "restoreBackup", "%s -> %s", found.a_str(), file->getFilename().a_str());
			file->restoreBackup(found); 
			return iInterface::retry;
		}

		virtual Stamina::MD5Digest askForPassword(FileBase* file, int retry) {

			if (Plug.size() > 0) {

				String title = "Has³o pliku";
				String info = "Podaj has³o do pliku " + Stamina::getFileName( file->getFilename() );

				sDIALOG_access sd;
				sd.pass = "";
				sd.save = 0;
				sd.title = title.a_str();
				sd.info = info.a_str();
				if (!IMessage(IMI_DLGPASS , 0 , 0 , (int)&sd , 0)) return MD5Digest();

				return DataTable::createPasswordDigest( safeChar(sd.pass) );
			} else {
				return MD5Digest();
			}
		}

	};

	extern StaticObj<Interface_konnekt> iface;

} }