// Created by Microsoft (R) C/C++ Compiler Version 14.16.27026.1 (2a718dbc).
//
// Wrapper implementations for Win32 type library msscript.ocx
// compiler-generated file created 02/06/19 at 06:31:52 - DO NOT EDIT!

#include "msscript.h"


//
// interface IScriptProcedure wrapper method implementations
//

_bstr_t IScriptProcedure::GetName ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_Name(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

long IScriptProcedure::GetNumArgs ( ) {
    long _result = 0;
    HRESULT _hr = get_NumArgs(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

VARIANT_BOOL IScriptProcedure::GetHasReturnValue ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_HasReturnValue(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

//
// interface IScriptProcedureCollection wrapper method implementations
//

IUnknownPtr IScriptProcedureCollection::Get_NewEnum ( ) {
    IUnknown * _result = 0;
    HRESULT _hr = get__NewEnum(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IUnknownPtr(_result, false);
}

IScriptProcedurePtr IScriptProcedureCollection::GetItem ( const _variant_t & Index ) {
    struct IScriptProcedure * _result = 0;
    HRESULT _hr = get_Item(Index, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IScriptProcedurePtr(_result, false);
}

long IScriptProcedureCollection::GetCount ( ) {
    long _result = 0;
    HRESULT _hr = get_Count(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

//
// interface IScriptModule wrapper method implementations
//

_bstr_t IScriptModule::GetName ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_Name(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

IDispatchPtr IScriptModule::GetCodeObject ( ) {
    IDispatch * _result = 0;
    HRESULT _hr = get_CodeObject(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDispatchPtr(_result, false);
}

IScriptProcedureCollectionPtr IScriptModule::GetProcedures ( ) {
    struct IScriptProcedureCollection * _result = 0;
    HRESULT _hr = get_Procedures(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IScriptProcedureCollectionPtr(_result, false);
}

HRESULT IScriptModule::AddCode ( _bstr_t Code ) {
    HRESULT _hr = raw_AddCode(Code);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

_variant_t IScriptModule::Eval ( _bstr_t Expression ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = raw_Eval(Expression, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

HRESULT IScriptModule::ExecuteStatement ( _bstr_t Statement ) {
    HRESULT _hr = raw_ExecuteStatement(Statement);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

_variant_t IScriptModule::Run ( _bstr_t ProcedureName, SAFEARRAY * * Parameters ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = raw_Run(ProcedureName, Parameters, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

//
// interface IScriptModuleCollection wrapper method implementations
//

IUnknownPtr IScriptModuleCollection::Get_NewEnum ( ) {
    IUnknown * _result = 0;
    HRESULT _hr = get__NewEnum(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IUnknownPtr(_result, false);
}

IScriptModulePtr IScriptModuleCollection::GetItem ( const _variant_t & Index ) {
    struct IScriptModule * _result = 0;
    HRESULT _hr = get_Item(Index, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IScriptModulePtr(_result, false);
}

long IScriptModuleCollection::GetCount ( ) {
    long _result = 0;
    HRESULT _hr = get_Count(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

IScriptModulePtr IScriptModuleCollection::Add ( _bstr_t Name, VARIANT * Object ) {
    struct IScriptModule * _result = 0;
    HRESULT _hr = raw_Add(Name, Object, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IScriptModulePtr(_result, false);
}

//
// interface IScriptError wrapper method implementations
//

long IScriptError::GetNumber ( ) {
    long _result = 0;
    HRESULT _hr = get_Number(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

_bstr_t IScriptError::GetSource ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_Source(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

_bstr_t IScriptError::GetDescription ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_Description(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

_bstr_t IScriptError::GetHelpFile ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_HelpFile(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

long IScriptError::GetHelpContext ( ) {
    long _result = 0;
    HRESULT _hr = get_HelpContext(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

_bstr_t IScriptError::GetText ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_Text(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

long IScriptError::GetLine ( ) {
    long _result = 0;
    HRESULT _hr = get_Line(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

long IScriptError::GetColumn ( ) {
    long _result = 0;
    HRESULT _hr = get_Column(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

HRESULT IScriptError::Clear ( ) {
    HRESULT _hr = raw_Clear();
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

//
// interface IScriptControl wrapper method implementations
//

_bstr_t IScriptControl::GetLanguage ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_Language(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

void IScriptControl::PutLanguage ( _bstr_t pbstrLanguage ) {
    HRESULT _hr = put_Language(pbstrLanguage);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

enum ScriptControlStates IScriptControl::GetState ( ) {
    enum ScriptControlStates _result;
    HRESULT _hr = get_State(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

void IScriptControl::PutState ( enum ScriptControlStates pssState ) {
    HRESULT _hr = put_State(pssState);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

void IScriptControl::PutSitehWnd ( long phwnd ) {
    HRESULT _hr = put_SitehWnd(phwnd);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

long IScriptControl::GetSitehWnd ( ) {
    long _result = 0;
    HRESULT _hr = get_SitehWnd(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

long IScriptControl::GetTimeout ( ) {
    long _result = 0;
    HRESULT _hr = get_Timeout(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

void IScriptControl::PutTimeout ( long plMilleseconds ) {
    HRESULT _hr = put_Timeout(plMilleseconds);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

VARIANT_BOOL IScriptControl::GetAllowUI ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_AllowUI(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

void IScriptControl::PutAllowUI ( VARIANT_BOOL pfAllowUI ) {
    HRESULT _hr = put_AllowUI(pfAllowUI);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

VARIANT_BOOL IScriptControl::GetUseSafeSubset ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_UseSafeSubset(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

void IScriptControl::PutUseSafeSubset ( VARIANT_BOOL pfUseSafeSubset ) {
    HRESULT _hr = put_UseSafeSubset(pfUseSafeSubset);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

IScriptModuleCollectionPtr IScriptControl::GetModules ( ) {
    struct IScriptModuleCollection * _result = 0;
    HRESULT _hr = get_Modules(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IScriptModuleCollectionPtr(_result, false);
}

IScriptErrorPtr IScriptControl::GetError ( ) {
    struct IScriptError * _result = 0;
    HRESULT _hr = get_Error(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IScriptErrorPtr(_result, false);
}

IDispatchPtr IScriptControl::GetCodeObject ( ) {
    IDispatch * _result = 0;
    HRESULT _hr = get_CodeObject(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDispatchPtr(_result, false);
}

IScriptProcedureCollectionPtr IScriptControl::GetProcedures ( ) {
    struct IScriptProcedureCollection * _result = 0;
    HRESULT _hr = get_Procedures(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IScriptProcedureCollectionPtr(_result, false);
}

HRESULT IScriptControl::_AboutBox ( ) {
    HRESULT _hr = raw__AboutBox();
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

HRESULT IScriptControl::AddObject ( _bstr_t Name, IDispatch * Object, VARIANT_BOOL AddMembers ) {
    HRESULT _hr = raw_AddObject(Name, Object, AddMembers);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

HRESULT IScriptControl::Reset ( ) {
    HRESULT _hr = raw_Reset();
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

HRESULT IScriptControl::AddCode ( _bstr_t Code ) {
    HRESULT _hr = raw_AddCode(Code);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

_variant_t IScriptControl::Eval ( _bstr_t Expression ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = raw_Eval(Expression, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

HRESULT IScriptControl::ExecuteStatement ( _bstr_t Statement ) {
    HRESULT _hr = raw_ExecuteStatement(Statement);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

_variant_t IScriptControl::Run ( _bstr_t ProcedureName, SAFEARRAY * * Parameters ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = raw_Run(ProcedureName, Parameters, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

//
// dispinterface DScriptControlSource wrapper method implementations
//

HRESULT DScriptControlSource::Error ( ) {
    return _com_dispatch_method(this, 0xbb8, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

HRESULT DScriptControlSource::Timeout ( ) {
    return _com_dispatch_method(this, 0xbb9, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}
