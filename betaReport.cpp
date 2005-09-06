#include "stdafx.h"
#include <fstream>
#include <Stamina\Time64.h>
#include "main.h"
#include "beta.h"
#include "tables.h"
#include "profiles.h"

using namespace Stamina;
using namespace Tables;

namespace Konnekt { namespace Beta {

	void printValue(std::ofstream& f, const CStdString& name, const CStdString& value) {
		f << "<tr>" << endl;
		f << "<td class=\"name\">" << name << "</td>" << endl;
		f << "<td class=\"value\">" << value << "</td>" << endl;
		f << "</tr>" << endl;
	}
	void printHeader(std::ofstream& f, const CStdString& name, const CStdString& css = "") {
		f << "<tr>" << endl;
		f << "<td colspan=\"2\" class=\"header " << css << "\">" << name << "</td>" << endl;
		f << "</tr>" << endl ;
	}

	void makeReport(CStdString filename, bool open, bool usedate, bool silent) {
		std::ofstream f;
		if (filename.empty()) {
			if (usedate) {
				filename = Stamina::Time64(true).strftime(("betaReport " + Stamina::RegEx::doReplace("/[^a-zA-Z0-9_-]/", "", Konnekt::profile) + " (%d-%m-%Y %H.%M).html").c_str());
			} else {
				filename = "betaReport.html";
			}
		}
		oTableImpl beta(tableBeta);

		Stamina::Time64 startDate(false);

		if (usedate) {
			Stamina::Date64 date(beta->getInt64(0, BETA_LAST_STATICREPORT));
			date.msec = 0; // zerujemy koñcówkê...
			date.sec = 0;
			startDate = date;
			Stamina::Date64 now(true);
			now.sec = 0;
			if (now == date) {
				if (!silent) 
					ShellExecute(0, "OPEN", filename, 0, 0, SW_SHOW);
				return;
			}
			beta->setInt64(0, BETA_LAST_STATICREPORT, _time64(0));
			beta->save();
		}

		oTableImpl reports(tableReports);
		oTableImpl stats(tableStats);

		if (silent) {
			int count = 0;
			for (unsigned int i=0; i<reports->getRowCount(); i++) {
				if (reports->getInt64(i, REP_DATE) >= startDate) {
					count ++;
				}
			}
			if (count == 0)
				return;
		}

		f.open(filename);

		f << "<html>" << endl;

		f << "<style> @import url('betaReport.css'); </style>" << endl;

		f << "<body>" << endl;


		f << "<table id=\"beta\">" << endl;

		printHeader(f, "Beta");
		printValue(f, "Wygenerowany", Stamina::Time64(true).strftime("%d-%m-%Y %H:%M:%S"));
		if (!startDate.empty()) {
			printValue(f, "Raporty od", startDate.strftime("%d-%m-%Y %H:%M"));
		}
		printValue(f, "Login", beta->getStr(0, BETA_LOGIN));
		printValue(f, "Urid", inttostr( beta->getInt(0, BETA_URID), 16 ));
		printValue(f, "Serial", info_serial());
		printValue(f, "LastReport", Stamina::Time64(beta->getInt64(0, BETA_LAST_REPORT)).strftime("%d-%m-%Y %H:%M:%S"));

		printHeader(f, "Raporty");

		for (unsigned int i=0; i<reports->getRowCount(); i++) {
			if (reports->getInt64(i, REP_DATE) < startDate) {
				continue;
			}
			printHeader(f, "Raport", "report");
			
			f << "<tr><td colspan=2><table class=\"report\"><tr>" << endl;

			f << "<td valign=\"top\"><table class=\"rep_fields\">" << endl;
			// dane
			printValue(f, "LastReport", Stamina::Time64(reports->getInt64(i, REP_DATE)).strftime("%d-%m-%Y %H:%M:%S"));
			printValue(f, "Tytu³", reports->getStr(i, REP_TITLE));
			printValue(f, "Typ", inttostr(reports->getInt(i, REP_TYPE)));
			printValue(f, "Wersja", inttostr( reports->getInt(i, REP_VERSION), 16));
			CStdString plugs = reports->getStr(i, REP_PLUGS);
			Stamina::tStringVector plugList;
			Stamina::split(plugs, ",", plugList);
			plugs = "";
			for (Stamina::tStringVector::iterator plug = plugList.begin(); plug != plugList.end(); ++plug) {
				size_t sep = plug->find(' ');
				if (sep == -1) continue;
				if (!plugs.empty()) plugs += ", ";
				int v = chtoint(plug->substr(sep + 1).c_str(), 16);
				plugs += stringf("<abbr title=\"%s\">%s</abbr>", stringf("%d.%d.%d.%d", VERSION_MAJ(v), VERSION_MIN(v), VERSION_RLS(v), VERSION_BLD(v)).c_str(), plug->substr(0, sep).c_str());
			}

			printValue(f, "Wtyczki", plugs);

			f << "</table></td>" << endl;
			f << "<td class=\"rep_add\" valign=\"top\">" << endl;
			// zrzut i log
			f << "<h2>Message</h2>" << endl;
			f << "<div class=\"rep_message\"><pre>"<< reports->getStr(i, REP_MSG) <<"</pre></div>" << endl;
			f << "<h2>Info</h2>" << endl;
			f << "<div class=\"rep_info\"><pre>"<< reports->getStr(i, REP_INFO) <<"</pre></div>" << endl;
			f << "<h2>Log</h2>" << endl;
			f << "<div class=\"rep_log\"><pre>"<< reports->getStr(i, REP_LOG) <<"</pre></div>" << endl;
			f << "</td>" << endl;

			f << "</tr></table></td></tr>" << endl;

		}

		f << "</table>" << endl;

		f << "</body></html>" << endl;

		f.close();

		if (!silent)
			ShellExecute(0, "OPEN", filename, 0, 0, SW_SHOW);
	}

}; };