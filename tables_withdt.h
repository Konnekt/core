#pragma once
#ifndef __KONNEKT_TABLES__
#define __KONNEKT_TABLES__


#include <boost\shared_ptr.hpp>
#include <Konnekt\core_assert.h>
#include <Stamina\ObjectImpl.h>
#include <Stamina\Timer.h>
#include <Stamina\MD5.h>
#include <Stamina\CriticalSection.h>

namespace Konnekt { namespace Tables {


	using namespace Stamina;



/*#pragma warning( push )
#pragma warning(disable: 4250)*/

	class TableWithDT: public ::Stamina::SharedObject<iTable> {
	public:
		virtual oRow getRow(tRowId rowId) =0;
		virtual oColumn getColumn(tColId colId) =0;
		virtual oColumn getColumn(const Stamina::StringRef& colName) =0;

		virtual oColumn getColumnByPos(unsigned int colPos) =0;

		 virtual Stamina::DT::DataTable& getDT() =0;

	};




};};


#endif