/************************************************************
* FILENAME : ScriptObject.h
*
* class CScriptObject
*		This class implements MSScript control
*		It can interface script from resource file or text file
*
* INITIAL CODING : Ernest Laurentin (EL)
*************************************************************/

#ifndef _SCRIPT_OBJECT_
#define _SCRIPT_OBJECT_
#include <xstring>
#include <list>

// Add this to your stdafx.h
//#pragma warning( disable: 4786)

// Adjust the following path if required
#import "msscript.ocx" no_namespace

#define LANGUAGE_DEFAULT	_T("VBScript")
#define RT_SCRIPT			_T("SCRIPT")
#define LANGUAGE_NAME_LEN	40
#define ERROR_DESC_LEN		256

#ifndef ASSERT
#define ASSERT PFC_ASSERT
#endif

#ifndef TRACE
#define TRACE __noop
#endif


using namespace std;

#ifndef _UNICODE
typedef list<string> stl_string_list;
#else
typedef list<wstring> stl_string_list;
#endif

class CScriptObject
{

// Construction
public:
	CScriptObject();
	~CScriptObject();

// Function
public:
	LPCTSTR GetLanguage();
	void SetLanguage(LPCTSTR szLanguage);
	int  GetMethodsCount() const;
	LPCTSTR GetNameAt(int index);
	void Reset();
	bool LoadScript(LPCTSTR szFilename);
	bool LoadScriptResource(LPCTSTR lpName, LPCTSTR lpType, HINSTANCE hInstance);
	bool AddScript(LPCTSTR szCode);
	LPCTSTR GetErrorString();
	bool ExecuteStatement(LPCTSTR szStatement);
	bool RunProcedure(LPCTSTR szProcName, SAFEARRAY** saParameters, VARIANT* varRet);
protected:
	void CommonConstruct();
	bool GetMethodsName();
	LPCTSTR GetScriptFunction(LPCTSTR name);

	IScriptControlPtr m_pScript;				// The one and only script control
	stl_string_list m_FunctionList;				// Function list
	TCHAR	m_szLanguage[LANGUAGE_NAME_LEN+1];	// Current language
	TCHAR	m_szError[ERROR_DESC_LEN+1];		// Description error
private:

};

#endif // _SCRIPT_OBJECT_
