#include "stdafx.h"
#include "unique.h"

namespace Konnekt { namespace Unique {

	tDomains domains;

	// --------------------------------------   Range

/*	Range_impl::Range_impl():Range(rangeDefault, 0, typeBoth, 0, 0) {
	}*/
	Range_impl::Range_impl(const Range & r):Range(r) {
	}
	tId Range_impl::createUniqueId() {
		return this->_nextUnique++;
	}
	bool Range_impl::canRegisterId(tId id) {
		if (this->_type == typeUnique)
			return false;
		if (!this->containsId(id))
			return false;
		if (this->_type == typeBoth && this->_nextUnique <= id)
			return false;
		return true;
	}
	bool Range_impl::canRegisterName() {
		return (this->_type != typeStatic);
	}
	bool Range_impl::containsId(tId id) {
		return id <= this->_maximum && id >= this->_minimum;
	}

	// --------------------------------------   Domain
	CStdString Domain::getDomainName() const {
		return Unique::getName(domainDomain, this->_domainId);
	}

	tId Domain::getId(const CStdString & name) const {
		tIdMap::const_iterator it = this->findId(name);
		return it != this->_idMap.end() ? it->second : idNotFound;
	}
	CStdString Domain::getName(tId id) const {
		tIdMap::const_iterator it = this->findId(id);
		return it != this->_idMap.end() ? it->first : "";
	}
	RangePtr Domain::inRange(tId id, Range::enType check) const {
		RangePtr found;
		for (tRanges::const_iterator it = _ranges.begin(); it != _ranges.end(); it++) {
			if ((it->second->getType() & check) && (!found || found->getPriority() < it->second->getPriority())
				&& it->second->containsId(id)) 
			{
				found = it->second;
			}
		}
		return found;
	}
	bool Domain::idExists(tId id) const {
		return this->findId(id) != this->_idMap.end();
	}
	bool Domain::nameExists(const CStdString & name) const {
		return this->findId(name) != this->_idMap.end();
	}
	Domain::tIdMap::iterator Domain::findId(tId id) {
		tIdMap::iterator it;
		for (it = _idMap.begin(); it != _idMap.end(); it++) {
			if (it->second == id) break;
		}
		return it;
	}
	Domain::tIdMap::iterator Domain::findId(const char * name) {
		return this->_idMap.find(name);
	}
	Domain::tIdMap::const_iterator Domain::findId(tId id) const {
		tIdMap::const_iterator it;
		for (it = _idMap.begin(); it != _idMap.end(); it++) {
			if (it->second == id) break;
		}
		return it;
	}
	Domain::tIdMap::const_iterator Domain::findId(const char * name) const {
		return this->_idMap.find(name);
	}

	bool Domain::registerId(tId id, const CStdString & name) {
		if (id == idNotFound || this->idExists(id) || this->nameExists(name))
			return false;
		this->_idMap[name] = id;
		return true;
	}
	tId Domain::registerName(const CStdString & name, tRangeId range) {
		return registerName(name, this->getRange(range));		
	}
	tId Domain::registerName(const CStdString & name, RangePtr range) {
		tId id = 0;
		if ((id = this->getId(name)) != idNotFound) return id;
		if (!range || !range->canRegisterName()) return idNotFound;
		while ((id=range->createUniqueId()) != idNotFound) {
			if (!this->idExists(id)) {
				this->_idMap[name] = id;
				break;
			}
		}
		return id;
	}
	bool Domain::unregister(tId id) {
		return unregister(getName(id));
	}
	bool Domain::unregister(const CStdString & name) {
		if (!this->nameExists(name)) return false;
		this->_idMap.erase(name);
		return true;
	}

	bool Domain::addRange(RangePtr range, bool setAsDefault) {
		if (range->getRangeId()==rangeNotFound || this->rangeExists(range)) return false;
		this->_ranges[range->getRangeId()] = range;
		if (setAsDefault && range->getType() != Range::typeStatic)
			this->_defaultRange = range;
		return true;
	}
	bool Domain::rangeExists(tRangeId rangeId) {
		return this->_ranges.find(rangeId) != this->_ranges.end();
	}
	RangePtr Domain::getRange(tRangeId rangeId) {
		if (rangeId == rangeDefault)
			return this->_defaultRange;
		tRanges::iterator it = this->_ranges.find(rangeId);
		return it == this->_ranges.end() ? RangePtr((Range_impl*)0) : it->second;
	}
	bool Domain::removeRange(RangePtr range) {
		if (!this->rangeExists(range)) 
			return false;
		this->_ranges.erase(range->getRangeId());
		return true;
	}


	// --------------------------------------   Reszta

	DomainPtr getDomain(tDomainId domainId) {
		tDomains::iterator it = domains.find(domainId);
		if (it == domains.end()) return DomainPtr((Domain*)0);
		return it->second;
	}
	DomainPtr getDomain(const char * domainName) {
		return getDomain(getDomainId(domainName));
	}
	bool domainExists(tDomainId domainId) {
		return domains.find(domainId) != domains.end();
	}
	bool removeDomain(tDomainId domainId) {
		tDomains::iterator it = domains.find(domainId);
		if (it == domains.end()) return false;
		domains.erase(it);
		return true;
	}

	bool addRange(tDomainId domainId, RangePtr range, bool setAsDefault) {
		if (domainId == domainNotFound) return false;
		DomainPtr d = getDomain(domainId);
		if (!domainExists(domainId)) {
			d.reset(new Domain(domainId));
			domains[domainId] = d;
			setAsDefault = true;
		}
		return d->addRange(range, setAsDefault);
		
	}
	bool addRange(tDomainId domainId, const Range & range, bool setAsDefault) {
		return addRange(domainId, RangePtr(new Range_impl(range)), setAsDefault);
	}

	RangePtr getRange(tDomainId domainId, tRangeId rangeId) {
		DomainPtr d = getDomain(domainId);
		if (!d) return RangePtr((Range_impl*)0);
		return d->getRange(rangeId);
	}

	bool registerId(tDomainId domainId, tId id, const char * name) {
		DomainPtr d = getDomain(domainId);
		if (!d) return false;
		return d->registerId(id, name);
	}
	tId registerName(tDomainId domainId, const char * name, tRangeId rangeId) {
		DomainPtr d = getDomain(domainId);
		if (!d) return idNotFound;
		return d->registerName(name, rangeId);
	}
	bool unregister(tDomainId domainId, tId id) {
		DomainPtr d = getDomain(domainId);
		if (!d) return false;
		return d->unregister(id);
	}
	tId getId(tDomainId domainId, const char * name) {
		DomainPtr d = getDomain(domainId);
		if (!d) return idNotFound;
		return d->getId(name);
	}
	tDomainId getDomainId(const char * name) {
		return (tDomainId)getId(domainDomain, name);
	}
	tRangeId getRangeId(const char * name) {
		return (tRangeId)getId(domainRange, name);
	}

	CStdString getName(tDomainId domainId, tId id) {
		DomainPtr d = getDomain(domainId);
		if (!d) return "";
		return d->getName(id);
	}

	bool idInRange(tDomainId domainId, tRangeId rangeId, tId id) {
		DomainPtr d = getDomain(domainId);
		if (!d) return false;
		RangePtr r = d->getRange(rangeId);
		if (!r) return false;
		return r->containsId(id);
	}
	tRangeId idInRange(tDomainId domainId, tId id, Range::enType check) {
		DomainPtr d = getDomain(domainId);
		if (!d) return rangeNotFound;
		RangePtr r = d->inRange(id, check);
		if (!r) return rangeNotFound;
		return r->getRangeId();
	}


	void initializeDomains() {
		addRange(domainDomain, Range(rangeMain, Range::typeBoth, 0, 1, 0x7FFFFFFF, commonUniqueStart), true);

		addRange(domainRange, Range(rangeMain, Range::typeBoth, 0, 1, 0x7FFFFFFF, commonUniqueStart), true);

		addRange(domainTable, Range(rangeMain, Range::typeBoth, 0, 0, 0x7FFF, 0x80), true);

		addRange(domainIMessage, Range(rangeMain, Range::typeBoth, 0, 1, 0x00FFFFFF, commonUniqueStart), true);

		addRange(domainMessageType, Range(rangeMain, Range::typeBoth, 0, 1, 0xFFF, 0x800), true);

		addRange(domainAction, Range(rangeMain, Range::typeBoth, 0, 1, 0x00FFFFFF, commonUniqueStart), true);

		addRange(domainIcon, Range(rangeMain, Range::typeBoth, 0, 1, 0x00FFFFFF, commonUniqueStart), true);

		addRange(domainNet, Range(rangeMain, Range::typeBoth, 0, 0, 0x0000FFFF, 0x00007FFF), true);

		addRange(domainNet, Range(rangeUniqueLow, Range::typeBoth, 0x80, 0x80, 0xFE, 0x81), false);

		registerId(domainDomain, domainDomain, "Domain");
		registerId(domainDomain, domainRange, "Range");
		registerId(domainDomain, domainTable, "Table");
		registerId(domainDomain, domainIMessage, "IMessage");
		registerId(domainDomain, domainMessageType, "MessageType");
		registerId(domainDomain, domainAction, "Action");
		registerId(domainDomain, domainIcon, "Icon");
		registerId(domainDomain, domainNet, "Net");

	}

	// ------------------------------------------------------------
	void debugCommand(sIMessage_debugCommand * arg) {
		CStdString cmd = arg->argv[0];
#ifdef __TEST
		if (cmd == "test") {
			if (arg->argc < 2) {
				IMDEBUG(DBG_COMMAND, " unique");
			} else if (!stricmp(arg->argv[1], "unique")) {
				testFeature(arg);
			}
		}
#endif
	}
	void testFeature(sIMessage_debugCommand * arg) {
#ifdef __TEST
		IMDEBUG(DBG_TEST_TITLE, "- Konnekt::Unique::test {{{");
		IMDEBUG(DBG_TEST_TITLE, "registerName(domainDomain, \"TestDomain\")");
		tDomainId domain = (tDomainId)registerName(domainDomain, "TestDomain");
		testResult(0, domain, true);
		IMDEBUG(DBG_TEST_TITLE, "registerRange(domainRange, \"TestRange\")");
		tRangeId range = (tRangeId)registerName(domainRange, "TestRange");
		testResult(0, range, true);
		IMDEBUG(DBG_TEST_PASSED, "OK=%x", domain);
		IMDEBUG(DBG_TEST_TITLE, "addRanges:");
		IMDEBUG(DBG_TEST, "  rangeMain, Range::typeBoth, 0, 1, 0xFFFF, 0x80");
		addRange(domain, Range(rangeMain, Range::typeBoth, 0, 1, 0xFFFF, 0x80));
		IMDEBUG(DBG_TEST, "  range, Range::typeBoth, 0x8, 0x81, 0x90 - def");
		addRange(domain, Range(range, Range::typeBoth, 0x8, 0x81, 0x90), true);
		IMDEBUG(DBG_TEST, "  rangeStatic, Range::typeStatic, 0xF, 0x60, 0x0x90");
		addRange(domain, Range(rangeStatic, Range::typeStatic, 0xF, 0x60, 0x90));
		IMDEBUG(DBG_TEST, "  rangeUnique, Range::typeUnique, 4, 0x80, 0x100");
		addRange(domain, Range(rangeUnique, Range::typeUnique, 4, 0x80, 0x100));
		IMDEBUG(DBG_TEST_PASSED, "OK");
		IMDEBUG(DBG_TEST_TITLE, "addRange to non existent domain:");
		testResult(0, addRange(domainNotFound, Range(rangeMain, Range::typeUnique, 0)));
		IMDEBUG(DBG_TEST_TITLE, "addRange with id == -1:");
		testResult(0, addRange(domain, Range(rangeNotFound, Range::typeUnique, 0)));

		IMDEBUG(DBG_TEST_TITLE, "registerId 0x60:");
		testResult(true, registerId(domain, 0x60, "0x60"));
		IMDEBUG(DBG_TEST_TITLE, "registerId 0x85:");
		testResult(true, registerId(domain, 0x82, "0x82"));
		IMDEBUG(DBG_TEST_TITLE, "registerName rangeMain:");
		testResult(0x80, registerName(domain, "rangeMain", rangeMain));
		IMDEBUG(DBG_TEST_TITLE, "registerName range:");
		testResult(0x81, registerName(domain, "range", range));
		IMDEBUG(DBG_TEST_TITLE, "registerName rangeStatic:");
		testResult(idNotFound, registerName(domain, "rangeStatic", rangeStatic));
		IMDEBUG(DBG_TEST_TITLE, "registerName rangeUnique:");
		testResult(0x83, registerName(domain, "rangeUnique", rangeUnique));
		IMDEBUG(DBG_TEST_TITLE, "registerName rangeMain (existing):");
		testResult(0x80, registerName(domain, "rangeMain", rangeMain));

		IMDEBUG(DBG_TEST_TITLE, "getId rangeMain:");
		testResult(0x80, getId(domain, "rangeMain"));
		IMDEBUG(DBG_TEST_TITLE, "unregister rangeMain:");
		testResult(1, unregister(domain, getId(domain, "rangeMain")));
		IMDEBUG(DBG_TEST_TITLE, "registerName rangeMain:");
		testResult(0x84, registerName(domain, "rangeMain", rangeMain));

		IMDEBUG(DBG_TEST_TITLE, "idInRange range in range:");
		testResult(1, idInRange(domain, range, getId(domain, "range")));

		IMDEBUG(DBG_TEST_TITLE, "idInRange range checkBoth:");
		testResult(rangeStatic, idInRange(domain, getId(domain, "range"), Range::typeBoth));
		IMDEBUG(DBG_TEST_TITLE, "idInRange range checkUnique:");
		testResult(range, idInRange(domain, getId(domain, "range"), Range::typeUnique));

		IMDEBUG(DBG_TEST_TITLE, "IM::_addRange:");
		Unique::IM::_addRange imAr (domain, Range(rangeUniqueLow, Range::typeBoth, 0, 0xFFF));
		testResult(1, Ctrl->IMessage(&imAr));

		IMDEBUG(DBG_TEST_TITLE, "IM::_rangeIM:");
		Unique::IM::_rangeIM imRi (Unique::IM::registerId);
		imRi.domainId = domain;
		imRi.identifier = imAr.range->getMinimum();
		imRi.name = "rangeIM";
		testResult(1, Ctrl->IMessage(&imRi));
		imRi.id = Unique::IM::registerName;
		testResult(imAr.range->getMinimum(), Ctrl->IMessage(&imRi));
		



		IMDEBUG(DBG_TEST_TITLE, "cleanup:");
		testResult(true, removeDomain(domain));
		testResult(true, unregister(domainDomain, domain));
		testResult(true, unregister(domainRange, range));


		IMDEBUG(DBG_TEST, "- Konnekt::Unique::test }}}");
#endif
	}


};};