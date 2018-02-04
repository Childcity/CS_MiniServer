#pragma once
/*#include <windows.h>
#include <string>
#include <sqlext.h>
#include <sql.h>
#include <list>*/

#include "main.h"

namespace ODBCDatabase
{

	//boost::recursive_mutex driverConnect;

	class CDatabase{
		private:

		/******************************************/
		/* Structure to store information about   */
		/* a column.
		/******************************************/

		struct BINDING{
			SQLSMALLINT         cDisplaySize;           /* size to display  */
			WCHAR               *wszBuffer;             /* display buffer   */
			SQLLEN              indPtr;                 /* size or null     */
			BOOL                fChar;                  /* character col?   */
			struct BINDING  *sNext;                 /* linked list      */
		};


		struct Binding{
			SQLSMALLINT         cDisplaySize;           /* size to display  */
			WCHAR               *wszBuffer;             /* display buffer   */
			SQLLEN              indPtr;                 /* size or null     */
			BOOL                fChar;                  /* character col?   */
			struct Binding  *sNext;                 /* linked list      */
		};

	public:

		CDatabase(WCHAR delim = L',');

		~CDatabase();

		bool operator<<(std::wstring && os);

		bool ConnectedOk() const;
		
		void operator>>(std::wstring & str) const;
		
	private:

		void Disconnect();

		void HandleDiagnosticRecord(SQLHANDLE      hHandle,
									SQLSMALLINT    hType,
									RETCODE        RetCode);

		void CDatabase::getAnswer(SQLSMALLINT & cCols);

		bool CDatabase::allocateBindings(SQLSMALLINT & cCols);

		bool CDatabase::getTitles();

						void DisplayResults(HSTMT       hStmt_,
											SQLSMALLINT cCols);

						void AllocateBindings(HSTMT         hStmt_,
											  SQLSMALLINT   cCols,
											  BINDING**     ppBinding,
											  SQLSMALLINT*  pDisplay);


						void DisplayTitles(HSTMT    hStmt_,
										   DWORD    cDisplaySize,
										   BINDING* pBinding);

						void SetConsole(DWORD   cDisplaySize,
										BOOL    fInvert);

		CDatabase(CDatabase const&) = delete;
		CDatabase operator=(CDatabase const&) = delete;

	private:
		
		std::list<Binding>  bindings_;
		
		WCHAR delim_;

		std::wstring answer_;
		
		bool connected_;


		SQLHENV     hEnv_;
		SQLHDBC     hDbc_;
		SQLHSTMT    hStmt_;
	};

}

