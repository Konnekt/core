#include "stdafx.h"
#include "unique.h"

using namespace Stamina;

namespace Konnekt { namespace Unique {

	void initializeUnique() {

		oDomainList list = Unique::instance();

		oDomain tables = new Domain(domainTable, "table");
		oDomain imessages = new Domain(domainIMessage, "imessage");
		oDomain messageTypes = new Domain(domainMessageType, "messageType");
		oDomain actions = new Domain(domainAction, "action");
		oDomain icons = new Domain(domainIcon, "icon");
		oDomain nets = new Domain(domainNet, "net");

		list->registerDomain(tables);
		list->registerDomain(imessages);
		list->registerDomain(messageTypes);
		list->registerDomain(actions);
		list->registerDomain(icons);
		list->registerDomain(nets);

		tables->addRange(new Range(rangeMain, Range::typeBoth, 0, 0, 0x7FFF, 0x80), true);

		imessages->addRange(new Range(rangeMain, Range::typeBoth, 0, 1, 0x00FFFFFF, commonUniqueStart), true);

		messageTypes->addRange(new Range(rangeMain, Range::typeBoth, 0, 1, 0xFFF, 0x800), true);

		actions->addRange(new Range(rangeMain, Range::typeBoth, 0, 1, 0x00FFFFFF, commonUniqueStart), true);

		icons->addRange(new Range(rangeMain, Range::typeBoth, 0, 1, 0x00FFFFFF, commonUniqueStart), true);

		nets->addRange(new Range(rangeMain, Range::typeBoth, 0, 0, 0x0000FFFF, 0x00007FFF), true);

		nets->addRange(new Range(rangeUniqueLow, Range::typeBoth, 0x80, 0x80, 0xFE, 0x81), false);

	}

	// ------------------------------------------------------------
	void debugCommand(sIMessage_debugCommand * arg) {
		CStdString cmd = arg->argv[0];
	}



};};