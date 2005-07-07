#pragma once

#include "main.h"
#include "konnekt_sdk.h"
#include "konnekt/core_unique.h"

namespace Konnekt { namespace Unique {
	class eIdNotFound {
	};

	class Range_impl:public Range {
	public:
        //Range_impl();
        Range_impl(const Range & r);
		virtual tId createUniqueId();
		virtual bool canRegisterId(tId id);
		virtual bool canRegisterName();
		virtual bool containsId(tId id);
	};


	typedef boost::shared_ptr<Range_impl> RangePtr;

	class Domain {
	public:
		typedef map<CStdString, tId> tIdMap;
		typedef map<tRangeId, RangePtr> tRanges;

		Domain(tDomainId domainId):_domainId(domainId) {}

		tDomainId getDomainId() const {return _domainId;}
		CStdString getDomainName() const;

		tId getId(const CStdString & name) const;
		CStdString getName(tId id) const;
		RangePtr inRange(tId id, Range::enType check = Range::typeBoth) const;
		bool idExists(tId id) const;
		bool nameExists(const CStdString & name) const;
		tIdMap::iterator findId(tId id);
		tIdMap::iterator findId(const char * name);
		tIdMap::const_iterator findId(tId id) const;
		tIdMap::const_iterator findId(const char * name) const;

		bool registerId(tId id, const CStdString & name);
		tId registerName(const CStdString & name, tRangeId range);
		tId registerName(const CStdString & name, RangePtr range);
		bool unregister(tId id);
		bool unregister(const CStdString & name);

		tRanges & getRanges() {return _ranges;}
		bool addRange(RangePtr range, bool setAsDefault = false);
		bool rangeExists(tRangeId rangeId);
		bool rangeExists(RangePtr range) {return rangeExists(range->getRangeId());}
		RangePtr getRange(tRangeId rangeId);
        bool removeRange(RangePtr range);

	private:
		tDomainId _domainId;
		//RangePtr _defaultStaticRange;
		RangePtr _defaultRange;
		tIdMap _idMap;
		tRanges _ranges;
	};

	typedef boost::shared_ptr<Domain> DomainPtr;
	typedef map<tDomainId, DomainPtr> tDomains;
	extern tDomains domains;

//	bool domainExists(tDomainId domainId);
	DomainPtr getDomain(tDomainId domainId);
	DomainPtr getDomain(const char * domainName);
//	bool removeDomain(tDomainId domainId);


	bool addRange(tDomainId domainId, RangePtr range, bool setAsDefault = false);
//	bool addRange(tDomainId domainId, const Range & range, bool setAsDefault = false);
	RangePtr getRange(tDomainId domainId, tRangeId rangeId);

//    bool registerId(tDomainId domainId, tId id, const char * name);
//	tId registerName(tDomainId domainId, const char * name, tRangeId rangeId = rangeDefault);
//	bool unregister(tDomainId domainId, tId id);
	tId getId(tDomainId domainId, const char * name);
	CStdString getName(tDomainId domainId, tId id);
	tDomainId getDomainId(const char * name);
	tRangeId getRangeId(const char * name);
	bool idInRange(tDomainId domainId, tRangeId rangeId, tId id);
	tRangeId idInRange(tDomainId domainId, tId id, Range::enType check = Range::typeBoth);
	
	void initializeDomains();
	
	void debugCommand(sIMessage_debugCommand * arg);
	void testFeature(sIMessage_debugCommand * arg);
};};