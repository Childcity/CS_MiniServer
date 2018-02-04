#include "CDatabase.h"

boost::recursive_mutex driverConnect;

/*******************************************/
/* Macro to call ODBC functions and        */
/* report an error on failure.             */
/* Takes handle, handle type, and stmt     */
/*******************************************/

#define TRYODBC(h, ht, x)   {   RETCODE rc = x;\
                                if (rc != SQL_SUCCESS) \
                                { \
                                    HandleDiagnosticRecord (h, ht, rc); \
									return; \
                                } \
                            }


/*****************************************/
/* Some constants                        */
/*****************************************/


#define DISPLAY_MAX 50          // Arbitrary limit on column width to display
#define DISPLAY_FORMAT_EXTRA 3  // Per column extra display bytes (| <data> )
#define DISPLAY_FORMAT      L"%c %*.*s "
#define DISPLAY_FORMAT_C    L"%c %-*.*s "
#define NULL_SIZE           6   // <NULL>

#define PIPE                L'|'

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

SHORT   gHeight = 80;       // Users screen height

namespace ODBCDatabase
{

	CDatabase::CDatabase(WCHAR delim):
		delim_(delim)
	{
		connected_ = false;

		RETCODE retCode;
		hEnv_ = NULL;
		hDbc_ = NULL;
		hStmt_ = NULL;

		// Allocate an environment
		if( SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv_) == SQL_ERROR )
		{
			fwprintf(stderr, L"Unable to allocate an environment handle\n");
			return;
		}

		// Register this as an application that expects 3.x behavior,
		// you must register something if you use AllocHandle
		retCode = SQLSetEnvAttr(hEnv_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
		if( retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO )
		{
			HandleDiagnosticRecord(hEnv_, SQL_HANDLE_ENV, retCode);
			Disconnect();
			return;
		}

		// Allocate a connection
		retCode = SQLAllocHandle(SQL_HANDLE_DBC, hEnv_, &hDbc_);
		if( retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO )
		{
			HandleDiagnosticRecord(hEnv_, SQL_HANDLE_ENV, retCode);
			Disconnect();
			return;
		}

		// Connect to the driver.  Use the connection string if supplied
		// on the input, otherwise let the driver manager prompt for input.
		
		{
			boost::recursive_mutex::scoped_lock lk(driverConnect);	
			retCode = SQLDriverConnectW(hDbc_, hWnd, (SQLWCHAR *)ConectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
		}
			if( retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO ){
				HandleDiagnosticRecord(hDbc_, SQL_HANDLE_DBC, retCode);
				Disconnect();
				return;
			}

		connected_ = true;
		fwprintf(stderr, L"ODBC: Connected!\n");
	}

	void CDatabase::Disconnect()
	{
		connected_ = false;

		fwprintf(stderr, L"ODBC: Disconnected!\n");

		for( auto &thisBinding : bindings_ )
		{
			if( thisBinding.wszBuffer )
			{
				fwprintf(stderr, L"ODBC: free bind!\n");
				free(thisBinding.wszBuffer);
			}
		}

		if( hStmt_ )
		{
			SQLFreeHandle(SQL_HANDLE_STMT, hStmt_);
		}

		if( hDbc_ )
		{
			SQLDisconnect(hDbc_);
			SQLFreeHandle(SQL_HANDLE_DBC, hDbc_);
		}

		if( hEnv_ )
		{
			SQLFreeHandle(SQL_HANDLE_ENV, hEnv_);
		}
	}

	void CDatabase::getAnswer(SQLSMALLINT & cCols)
	{
		RETCODE				retCode = SQL_SUCCESS;

		bool result;

		// Allocate memory for each column 

		result = allocateBindings(cCols);
		if( !result )
		{
			return;
		}

		// Set the display mode and write the titles

		result = getTitles();
		if( !result )
		{
			return;
		}

		// Fetch and display the data

		bool fNoData = false;

		do
		{
			// Fetch a row

			retCode = SQLFetch(hStmt_);
			if( retCode == SQL_ERROR || retCode == SQL_INVALID_HANDLE )
			{
				Disconnect();
				return;
			} else if( retCode == SQL_NO_DATA_FOUND )
			{
				fNoData = true;
			} else
			{

				// Display the data.   Ignore truncations

				for(auto & thisBinding: bindings_ )
				{
						/*WCHAR *pResultTmp = new WCHAR[thisBinding.cDisplaySize + 1];
						if( !pResultTmp )
						{
							Disconnect();
							return;
						}
						if( thisBinding.indPtr != SQL_NULL_DATA )
						{
							wsprintfW(pResultTmp, 
									L"%c %*.*s ",
									PIPE,
									thisBinding.cDisplaySize,
									thisBinding.cDisplaySize,
									thisBinding.wszBuffer);
						} else
						{
							wsprintfW(pResultTmp, 
									DISPLAY_FORMAT_C,
									PIPE,
									thisBinding.cDisplaySize,
									thisBinding.cDisplaySize,
									L"<NULL>");
						}
						answer_ += std::move(std::wstring(pResultTmp));
						delete[] pResultTmp;*/

					if( thisBinding.indPtr != SQL_NULL_DATA )
						{
							answer_ += std::move(std::wstring(thisBinding.wszBuffer));
							answer_.push_back(delim_);
						} else
						{
							answer_ += std::move(std::wstring(L"<NULL>" + delim_));
						}
					
				}
				answer_.push_back(L'\n');
			}
		} while( !fNoData );

		for( auto & thisBinding : bindings_ )
		{
			if( thisBinding.wszBuffer )
			{
				free(thisBinding.wszBuffer);
			}
		}

		bindings_.clear();
	}

	bool CDatabase::allocateBindings(SQLSMALLINT & cCols)
	{
		Binding         ThisBinding;
		SQLLEN          cchDisplay = 0, ssType;
		SQLSMALLINT     cchColumnNameLength;
		RETCODE			retCode;

		for( SQLSMALLINT iCol = 1; iCol <= cCols; iCol++ )
		{
		
			// Figure out the display length of the column (we will
			// bind to char since we are only displaying data, in general
			// you should bind to the appropriate C type if you are going
			// to manipulate data since it is much faster...)

			retCode = SQLColAttributeW(hStmt_, iCol, SQL_DESC_DISPLAY_SIZE, NULL, 0, NULL, &cchDisplay);
			if( retCode != SQL_SUCCESS )
			{
				HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
				if( retCode == SQL_ERROR || retCode == SQL_INVALID_HANDLE )
				{
					Disconnect();
					return false;
				}
			}

			// Figure out if this is a character or numeric column; this is
			// used to determine if we want to display the data left- or right-
			// aligned.

			// SQL_DESC_CONCISE_TYPE maps to the 1.x SQL_COLUMN_TYPE. 
			// This is what you must use if you want to work
			// against a 2.x driver.

			retCode = SQLColAttributeW(hStmt_, iCol, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &ssType);
			if( retCode != SQL_SUCCESS )
			{
				HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
				if( retCode == SQL_ERROR || retCode == SQL_INVALID_HANDLE )
				{
					Disconnect();
					return false;
				}
			}

			ThisBinding.fChar = (ssType == SQL_CHAR ||
								 ssType == SQL_VARCHAR ||
								 ssType == SQL_LONGVARCHAR);


			// Arbitrary limit on display size
			if( cchDisplay > DISPLAY_MAX )
				cchDisplay = DISPLAY_MAX;

			// Allocate a buffer big enough to hold the text representation
			// of the data.  Add one character for the null terminator

			ThisBinding.wszBuffer = (WCHAR *)malloc((cchDisplay + 1) * sizeof(WCHAR));

			if( !(ThisBinding.wszBuffer) )
			{
				fwprintf(stderr, L"Out of memory!\n");
				Disconnect();
				return false;
			}

			// Map this buffer to the driver's buffer.   At Fetch time,
			// the driver will fill in this data.  Note that the size is 
			// count of bytes (for Unicode).  All ODBC functions that take
			// SQLPOINTER use count of bytes; all functions that take only
			// strings use count of characters.

			retCode = SQLBindCol(hStmt_, iCol, SQL_C_WCHAR, (SQLPOINTER)ThisBinding.wszBuffer, (cchDisplay + 1) * sizeof(WCHAR), &ThisBinding.indPtr);
			if( retCode != SQL_SUCCESS )
			{
				HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
				if( retCode == SQL_ERROR || retCode == SQL_INVALID_HANDLE )
				{
					Disconnect();
					return false;
				}
			}

			// Now set the display size that we will use to display
			// the data.   Figure out the length of the column name

			retCode = SQLColAttributeW(hStmt_, iCol, SQL_DESC_NAME, NULL, 0, &cchColumnNameLength, NULL);
			if( retCode != SQL_SUCCESS )
			{
				HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
				if( retCode == SQL_ERROR || retCode == SQL_INVALID_HANDLE )
				{
					Disconnect();
					return false;
				}
			}

			ThisBinding.cDisplaySize = max((SQLSMALLINT)cchDisplay, cchColumnNameLength);

			if( ThisBinding.cDisplaySize < NULL_SIZE )
				ThisBinding.cDisplaySize = NULL_SIZE;

			bindings_.push_back(ThisBinding);
		}

		return true;
	}

	bool CDatabase::getTitles()
	{
		RETCODE retCode;
		WCHAR           wszTitle[DISPLAY_MAX];
		SQLSMALLINT     iCol = 1;

		for( auto & thisBinding : bindings_ )
		{
			ZeroMemory(wszTitle, DISPLAY_MAX);

			retCode = SQLColAttributeW(hStmt_, iCol++, SQL_DESC_NAME, wszTitle, sizeof(wszTitle), /*Note count of bytes!*/ NULL, NULL);
			if( retCode != SQL_SUCCESS )
			{
				HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
				if( retCode == SQL_ERROR || retCode == SQL_INVALID_HANDLE )
				{
					Disconnect();
					return false;
				}
			}
 
			size_t len = 0;
				for(auto &ch: wszTitle )
				{
					if( ch == 0 )
					{
						break;
					}

					len++;
				}

				answer_ += std::move(std::wstring(wszTitle, len));
				answer_.push_back(delim_);
				
		}	
		answer_ += std::move(std::wstring(L"\n\n"));

		return true;
	}

	void CDatabase::HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
	{
		SQLSMALLINT iRec = 0;
		SQLINTEGER  iError;
		WCHAR       wszMessage[1000];
		WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];

		if( RetCode == SQL_INVALID_HANDLE )
		{
			fwprintf(stderr, L"ODBC: Invalid handle!\n");
			return;
		}

		bool ind = true;

		while( SQLGetDiagRecW(hType,
			  hHandle,
			  ++iRec,
			  wszState,
			  &iError,
			  wszMessage,
			  (SQLSMALLINT)countof(wszMessage),
			  (SQLSMALLINT *)NULL) == SQL_SUCCESS )
		{
			// Hide data truncated..
			if( wcsncmp(wszState, L"01004", 5) )
			{
				fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
			}

			ind = false;
		}


		if( ind && RetCode == SQL_ERROR )
		{
			fwprintf(stderr, L"ODBC: NoName error!\n");
		}

	}

	bool CDatabase::operator<<(std::wstring && query)
	{	
		RETCODE     retCode;
		SQLSMALLINT sNumResults;

		bool result = false;
		
		// Execute the query

		if( query.empty() || (! connected_) )
		{
			return result;
		}

		retCode = SQLAllocHandle(SQL_HANDLE_STMT, hDbc_, &hStmt_);
		if( retCode != SQL_SUCCESS )
		{
			HandleDiagnosticRecord(hDbc_, SQL_HANDLE_DBC, retCode);
			Disconnect();
			return result;
		}

		//wcscpy_s(wszInput, L"SELECT  Events.colEventTime, A_55.colAccountNumber, Events.colCardNumber, Events.colHolderID, colHolderType, Holder.colSurname, Holder.colName, Holder.colMiddlename, colStateCode, colComment, colDepartmentID, Holder.colDepartment, Holder.colTabNumber,  G_55.colName AS colAccessGroupName, CP.colID AS colObjectID, CP.colType AS colObjectType, CP.colName AS colObjectName, Events.colDirection, ZoneStart.colName AS colStartAreaName, ZoneTarget.colName AS colTargetAreaName, colEventCode FROM [StopNet4].[dbo].[tblEvents_55] AS Events WITH (NOLOCK) LEFT JOIN [StopNet4].[dbo].fnGetControlPoints() AS CP ON Events.colControlPointID = CP.colID LEFT JOIN (			SELECT				colID,				colSurname,				colName,				colMiddlename,  '' AS colStateCode,     colDepartmentID,				colDepartment,				colTabNumber,  colComment,				'1' AS colHolderType			FROM				[StopNet4].[dbo].[vwEmployees] WITH (NOLOCK)								UNION ALL						SELECT				colID,				colSurname,				colName,				colMiddlename,  '' AS colStateCode,  NULL,				'',				'',  colComment, 				'2' AS colHolderType			FROM				[StopNet4].[dbo].[vwVisitors] WITH (NOLOCK)      UNION ALL      SELECT				colID,				'',				'',				'',  colStateCode,  NULL,				'',				'',  colComment,				'3' AS colHolderType			FROM				[StopNet4].[dbo].[vwVehicles] WITH (NOLOCK)		) AS Holder ON Events.colHolderID = Holder.colID		LEFT JOIN [StopNet4].[dbo].[vwAccounts_55] AS A_55 WITH (NOLOCK) ON Events.colAccountID = A_55.colAccountID LEFT JOIN (			SELECT				colID,				colName			FROM				[StopNet4].[dbo].[tblGroupRightsEmployees_55] WITH (NOLOCK)							UNION ALL						SELECT				colID,				colName			FROM				[StopNet4].[dbo].[tblGroupRightsVisitors_55] WITH (NOLOCK)	UNION ALL						SELECT				colID,				colName			FROM				[StopNet4].[dbo].[tblGroupRightsVehicleEmployees_55] WITH (NOLOCK)	     UNION ALL						SELECT				colID,				colName			FROM				[StopNet4].[dbo].[tblGroupRightsVehicleVisitors_55] WITH (NOLOCK) ) AS G_55 ON [A_55].colAccessGroupRightsID = G_55.colID LEFT JOIN [StopNet4].[dbo].[tblZones_55] AS ZoneStart WITH (NOLOCK) ON Events.colStartZoneID = ZoneStart.colID LEFT JOIN [StopNet4].[dbo].[tblZones_55] AS ZoneTarget WITH (NOLOCK) ON  Events.colTargetZoneID = ZoneTarget.colID ");
		retCode = SQLExecDirectW(hStmt_, (SQLWCHAR *)query.c_str(), SQL_NTS);

		switch( retCode )
		{
			case SQL_SUCCESS_WITH_INFO:
				{
					HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
					// fall through
				}
			case SQL_SUCCESS:
				{
					// If this is a row-returning query, display
					// results

					retCode = SQLNumResultCols(hStmt_, &sNumResults);

					if( retCode != SQL_SUCCESS ){
						HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
						if( retCode == SQL_ERROR || retCode == SQL_INVALID_HANDLE ){
							Disconnect();
							break; //We must free hStmt_ before return
						}
					}

					if( sNumResults > 0 ){
						DisplayResults(hStmt_, sNumResults);
						//getAnswer(sNumResults);
					} else
					{
						SQLLEN cRowCount;

						retCode = SQLRowCount(hStmt_, &cRowCount);
						if( retCode != SQL_SUCCESS ){
							HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
							if( retCode == SQL_ERROR || retCode == SQL_INVALID_HANDLE ){
								Disconnect();
								break; //We must free hStmt_ before return
							}
						}

						if( cRowCount >= 0 )
						{
							answer_ += std::move(std::wstring(cRowCount + cRowCount == 1 ? L"row" : L"rows"));
						}

					}

					result = true;
					break;
				}

			case SQL_ERROR:
				{
					HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
					break;
				}

			default:
				fwprintf(stderr, L"ODBC: Unexpected return code %hd!\n", retCode);

		}

		retCode = SQLFreeStmt(hStmt_, SQL_CLOSE);
		if( retCode != SQL_SUCCESS ){ 
			HandleDiagnosticRecord(hStmt_, SQL_HANDLE_STMT, retCode);
		} 

		return result;
	}

	CDatabase::~CDatabase()
	{
		Disconnect();
	}

	bool CDatabase::ConnectedOk() const
	{	
		return connected_;
	}

	void CDatabase::operator>>(std::wstring & str) const
	{
		str = std::move(answer_);
	}






	/************************************************************************
	/* DisplayResults: display results of a select query
	/*
	/* Parameters:
	/*      hStmt_      ODBC statement handle
	/*      cCols      Count of columns
	/************************************************************************/

	void CDatabase::DisplayResults(HSTMT hStmt_, SQLSMALLINT cCols)
	{
		BINDING         *pFirstBinding, *pThisBinding;
		SQLSMALLINT     cDisplaySize;
		RETCODE         RetCode = SQL_SUCCESS;
		int             iCount = 0;

		// Allocate memory for each column 

		AllocateBindings(hStmt_, cCols, &pFirstBinding, &cDisplaySize);

		// Set the display mode and write the titles

		DisplayTitles(hStmt_, cDisplaySize + 1, pFirstBinding);


		// Fetch and display the data

		bool fNoData = false;

		do
		{
			// Fetch a row

			TRYODBC(hStmt_, SQL_HANDLE_STMT, RetCode = SQLFetch(hStmt_));

			if( RetCode == SQL_NO_DATA_FOUND )
			{
				fNoData = true;
			} else
			{

				// Display the data.   Ignore truncations

				for( pThisBinding = pFirstBinding;
					pThisBinding;
					pThisBinding = pThisBinding->sNext )
				{
					if( pThisBinding->indPtr != SQL_NULL_DATA )
					{
						wprintf(pThisBinding->fChar ? DISPLAY_FORMAT_C : DISPLAY_FORMAT,
								PIPE,
								pThisBinding->cDisplaySize,
								pThisBinding->cDisplaySize,
								pThisBinding->wszBuffer);
					} else
					{
						wprintf(DISPLAY_FORMAT_C,
								PIPE,
								pThisBinding->cDisplaySize,
								pThisBinding->cDisplaySize,
								L"<NULL>");
					}
				}
				wprintf(L" %c\n", PIPE);
			}
		} while( !fNoData );

		SetConsole(cDisplaySize + 2, TRUE);
		wprintf(L"%*.*s", cDisplaySize + 2, cDisplaySize + 2, L" ");
		SetConsole(cDisplaySize + 2, FALSE);
		wprintf(L"\n");

	Exit:
		// Clean up the allocated buffers

		while( pFirstBinding )
		{
			pThisBinding = pFirstBinding->sNext;
			free(pFirstBinding->wszBuffer);
			free(pFirstBinding);
			pFirstBinding = pThisBinding;
		}
	}

	/************************************************************************
	/* AllocateBindings:  Get column information and allocate bindings_
	/* for each column.
	/*
	/* Parameters:
	/*      hStmt_      Statement handle
	/*      cCols       Number of columns in the result set
	/*      *lppBinding Binding pointer (returned)
	/*      lpDisplay   Display size of one line
	/************************************************************************/

	void CDatabase::AllocateBindings(HSTMT         hStmt_,
									 SQLSMALLINT   cCols,
									 BINDING       **ppBinding,
									 SQLSMALLINT   *pDisplay)
	{
		SQLSMALLINT     iCol;
		BINDING         *pThisBinding, *pLastBinding = NULL;
		SQLLEN          cchDisplay = 0, ssType;
		SQLSMALLINT     cchColumnNameLength;

		*pDisplay = 0;

		for( iCol = 1; iCol <= cCols; iCol++ )
		{
			pThisBinding = (BINDING *)(malloc(sizeof(BINDING)));
			if( !(pThisBinding) )
			{
				fwprintf(stderr, L"Out of memory!\n");
				exit(-100);
			}

			if( iCol == 1 )
			{
				*ppBinding = pThisBinding;
			} else
			{
				pLastBinding->sNext = pThisBinding;
			}
			pLastBinding = pThisBinding;

			pThisBinding->sNext = NULL;


			// Figure out the display length of the column (we will
			// bind to char since we are only displaying data, in general
			// you should bind to the appropriate C type if you are going
			// to manipulate data since it is much faster...)

			TRYODBC(hStmt_,
					SQL_HANDLE_STMT,
					SQLColAttributeW(hStmt_,
					iCol,
					SQL_DESC_DISPLAY_SIZE,
					NULL,
					0,
					NULL,
					&cchDisplay));


			// Figure out if this is a character or numeric column; this is
			// used to determine if we want to display the data left- or right-
			// aligned.

			// SQL_DESC_CONCISE_TYPE maps to the 1.x SQL_COLUMN_TYPE. 
			// This is what you must use if you want to work
			// against a 2.x driver.

			TRYODBC(hStmt_,
					SQL_HANDLE_STMT,
					SQLColAttributeW(hStmt_,
					iCol,
					SQL_DESC_CONCISE_TYPE,
					NULL,
					0,
					NULL,
					&ssType));


			pThisBinding->fChar = (ssType == SQL_CHAR ||
								   ssType == SQL_VARCHAR ||
								   ssType == SQL_LONGVARCHAR);


			// Arbitrary limit on display size
			if( cchDisplay > DISPLAY_MAX )
				cchDisplay = DISPLAY_MAX;

			// Allocate a buffer big enough to hold the text representation
			// of the data.  Add one character for the null terminator

			pThisBinding->wszBuffer = (WCHAR *)malloc((cchDisplay + 1) * sizeof(WCHAR));

			if( !(pThisBinding->wszBuffer) )
			{
				fwprintf(stderr, L"Out of memory!\n");
				exit(-100);
			}

			// Map this buffer to the driver's buffer.   At Fetch time,
			// the driver will fill in this data.  Note that the size is 
			// count of bytes (for Unicode).  All ODBC functions that take
			// SQLPOINTER use count of bytes; all functions that take only
			// strings use count of characters.

			TRYODBC(hStmt_,
					SQL_HANDLE_STMT,
					SQLBindCol(hStmt_,
					iCol,
					SQL_C_WCHAR,
					(SQLPOINTER)pThisBinding->wszBuffer,
					(cchDisplay + 1) * sizeof(WCHAR),
					&pThisBinding->indPtr));


			// Now set the display size that we will use to display
			// the data.   Figure out the length of the column name

			TRYODBC(hStmt_,
					SQL_HANDLE_STMT,
					SQLColAttributeW(hStmt_,
					iCol,
					SQL_DESC_NAME,
					NULL,
					0,
					&cchColumnNameLength,
					NULL));

			pThisBinding->cDisplaySize = max((SQLSMALLINT)cchDisplay, cchColumnNameLength);
			if( pThisBinding->cDisplaySize < NULL_SIZE )
				pThisBinding->cDisplaySize = NULL_SIZE;

			*pDisplay += pThisBinding->cDisplaySize + DISPLAY_FORMAT_EXTRA;

		}
	}


	/************************************************************************
	/* DisplayTitles: print the titles of all the columns and set the
	/*                shell window's width
	/*
	/* Parameters:
	/*      hStmt_          Statement handle
	/*      cDisplaySize   Total display size
	/*      pBinding        list of binding information
	/************************************************************************/

	void CDatabase::DisplayTitles(HSTMT     hStmt_,
								  DWORD     cDisplaySize,
								  BINDING   *pBinding)
	{
		WCHAR           wszTitle[DISPLAY_MAX];
		SQLSMALLINT     iCol = 1;

		SetConsole(cDisplaySize + 2, TRUE);

		for( ; pBinding; pBinding = pBinding->sNext )
		{
			TRYODBC(hStmt_,
					SQL_HANDLE_STMT,
					SQLColAttributeW(hStmt_,
					iCol++,
					SQL_DESC_NAME,
					wszTitle,
					sizeof(wszTitle), // Note count of bytes!
					NULL,
					NULL));

			wprintf(DISPLAY_FORMAT_C,
					PIPE,
					pBinding->cDisplaySize,
					pBinding->cDisplaySize,
					wszTitle);
		}

	Exit:

		wprintf(L" %c", PIPE);
		SetConsole(cDisplaySize + 2, FALSE);
		wprintf(L"\n");

	}



	/************************************************************************
	/* SetConsole: sets console display size and video mode
	/*
	/*  Parameters
	/*      siDisplaySize   Console display size
	/*      fInvert         Invert video?
	/************************************************************************/

	void CDatabase::SetConsole(DWORD dwDisplaySize,
							   BOOL  fInvert)
	{
		HANDLE                          hConsole;
		CONSOLE_SCREEN_BUFFER_INFO      csbInfo;

		// Reset the console screen buffer size if necessary

		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		if( hConsole != INVALID_HANDLE_VALUE )
		{
			if( GetConsoleScreenBufferInfo(hConsole, &csbInfo) )
			{
				if( csbInfo.dwSize.X <  (SHORT)dwDisplaySize )
				{
					csbInfo.dwSize.X = (SHORT)dwDisplaySize;
					SetConsoleScreenBufferSize(hConsole, csbInfo.dwSize);
				}

				gHeight = csbInfo.dwSize.Y;
			}

			if( fInvert )
			{
				SetConsoleTextAttribute(hConsole, (WORD)(csbInfo.wAttributes | BACKGROUND_BLUE));
			} else
			{
				SetConsoleTextAttribute(hConsole, (WORD)(csbInfo.wAttributes & ~(BACKGROUND_BLUE)));
			}
		}
	}

}