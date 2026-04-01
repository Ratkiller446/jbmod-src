//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef DASLANG_VSCRIPT_H
#define DASLANG_VSCRIPT_H
#ifdef _WIN32
#pragma once
#endif

#include "ivscript.h"
#include "utlvector.h"
#include "utlmap.h"
#include "utlstring.h"

#define DASLANG_INTERFACE_VERSION		"DaslangVM001"

#include <daScript/daScript.h>
#include <daScript/simulate/simulate.h>

namespace das
{
    class Context;
    class Program;
    class ModuleLibrary;
    class ModuleGroup;
    struct Function;
    struct Type;
    class Annotation;
}

struct DaslangFunctionBinding
{
    ScriptFunctionBinding_t binding;
    das::Function *pDaslangFunc;
};

struct DaslangClassBinding
{
    ScriptClassDesc_t *pClassDesc;
    CUtlVector<DaslangFunctionBinding> functions;
};

struct DaslangInstanceBinding
{
    ScriptClassDesc_t *pClassDesc;
    void *pInstance;
    CUtlString uniqueId;
};

class CDaslangVM : public IScriptVM
{
public:
    CDaslangVM();
    virtual ~CDaslangVM();

    virtual bool Init() override;
    virtual void Shutdown() override;
    
    virtual bool ConnectDebugger() override;
    virtual void DisconnectDebugger() override;
    
    virtual ScriptLanguage_t GetLanguage() override;
    virtual const char *GetLanguageName() override;
    
    virtual void AddSearchPath(const char *pszSearchPath) override;
    
    virtual bool Frame(float simTime) override;
    
    virtual ScriptStatus_t Run(const char *pszScript, bool bWait = true) override;
    inline ScriptStatus_t Run(const unsigned char *pszScript, bool bWait = true) { 
        return Run((char *)pszScript, bWait); 
    }
    
    virtual HSCRIPT CompileScript(const char *pszScript, const char *pszId = NULL) override;
    inline HSCRIPT CompileScript(const unsigned char *pszScript, const char *pszId = NULL) { 
        return CompileScript((char *)pszScript, pszId); 
    }
    virtual void ReleaseScript(HSCRIPT) override;
    
    virtual ScriptStatus_t Run(HSCRIPT hScript, HSCRIPT hScope = NULL, bool bWait = true) override;
    virtual ScriptStatus_t Run(HSCRIPT hScript, bool bWait) override;
    
    virtual HSCRIPT CreateScope(const char *pszScope, HSCRIPT hParent = NULL) override;
    virtual HSCRIPT ReferenceScope(HSCRIPT hScript) override;
    virtual void ReleaseScope(HSCRIPT hScript) override;
    
    virtual HSCRIPT LookupFunction(const char *pszFunction, HSCRIPT hScope = NULL, bool bNoDelegation = false) override;
    virtual void ReleaseFunction(HSCRIPT hScript) override;
    
    virtual void RegisterFunction(ScriptFunctionBinding_t *pScriptFunction) override;
    
    virtual bool RegisterClass(ScriptClassDesc_t *pClassDesc) override;
    virtual void RegisterAllClasses();
    
    virtual HSCRIPT RegisterInstance(ScriptClassDesc_t *pDesc, void *pInstance) override;
    virtual void SetInstanceUniqeId(HSCRIPT hInstance, const char *pszId) override;
    template <typename T> HSCRIPT RegisterInstance(T *pInstance) { 
        return RegisterInstance(GetScriptDesc(pInstance), pInstance); 
    }
    template <typename T> HSCRIPT RegisterInstance(T *pInstance, const char *pszInstance, HSCRIPT hScope = NULL) {
        HSCRIPT hInstance = RegisterInstance(GetScriptDesc(pInstance), pInstance); 
        SetValue(hScope, pszInstance, hInstance); 
        return hInstance; 
    }
    virtual void RemoveInstance(HSCRIPT) override;
    virtual void RemoveInstance(HSCRIPT hInstance, const char *pszInstance, HSCRIPT hScope = NULL) override;
    virtual void *GetInstanceValue(HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType = NULL) override;
    
    virtual bool GenerateUniqueKey(const char *pszRoot, char *pBuf, int nBufSize) override;
    
    virtual bool ValueExists(HSCRIPT hScope, const char *pszKey) override;
    bool ValueExists(const char *pszKey) override { return ValueExists(NULL, pszKey); }
    
    virtual bool SetValue(HSCRIPT hScope, const char *pszKey, const char *pszValue) override;
    virtual bool SetValue(HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value) override;
    bool SetValue(const char *pszKey, const ScriptVariant_t &value) override { return SetValue(NULL, pszKey, value); }
    
    virtual void CreateTable(ScriptVariant_t &Table) override;
    virtual int GetNumTableEntries(HSCRIPT hScope) override;
    virtual int GetKeyValue(HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue) override;
    
    virtual bool GetValue(HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue) override;
    bool GetValue(const char *pszKey, ScriptVariant_t *pValue) override { return GetValue(NULL, pszKey, pValue); }
    virtual void ReleaseValue(ScriptVariant_t &value) override;
    
    virtual bool ClearValue(HSCRIPT hScope, const char *pszKey) override;
    bool ClearValue(const char *pszKey) override { return ClearValue(NULL, pszKey); }
    
    template <typename T>
    T Get(HSCRIPT hScope, const char *pszKey)
    {
        ScriptVariant_t variant;
        GetValue(hScope, pszKey, &variant);
        return variant.Get<T>();
    }
    
    template <typename T>
    T Get(const char *pszKey)
    {
        return Get<T>(NULL, pszKey);
    }
    
    bool Has(HSCRIPT hScope, const char* pszKey)
    {
        return ValueExists(hScope, pszKey);
    }
    
    bool Has(const char* pszKey)
    {
        return Has(NULL, pszKey);
    }
    
    template <typename T>
    using IfHasFuncType = std::function<void(T)>;
    
    template <typename T>
    void IfHas(HSCRIPT hScope, const char *pszKey, IfHasFuncType<T> func)
    {
        if (Has(hScope, pszKey))
        {
            func(Get<T>(hScope, pszKey));
        }
    }
    
    virtual void WriteState(CUtlBuffer *pBuffer) override;
    virtual void ReadState(CUtlBuffer *pBuffer) override;
    virtual void RemoveOrphanInstances() override;
    virtual void DumpState() override;
    
    virtual void SetOutputCallback(ScriptOutputFunc_t pFunc) override;
    virtual void SetErrorCallback(ScriptErrorFunc_t pFunc) override;
    
    virtual bool RaiseException(const char *pszExceptionText) override;
    
    ScriptStatus_t ExecuteFunction(HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait);
    
protected:
    das::Context *m_pContext;
    das::ModuleLibrary *m_pModuleLibrary;
    das::ModuleGroup *m_pModuleGroup;
    
    CUtlVector<DaslangFunctionBinding> m_RegisteredFunctions;
    CUtlVector<DaslangClassBinding> m_RegisteredClasses;
    CUtlVector<DaslangInstanceBinding> m_RegisteredInstances;
    
    CUtlMap<CUtlString, ScriptVariant_t> m_GlobalVariables;
    
    ScriptOutputFunc_t m_pOutputCallback;
    ScriptErrorFunc_t m_pErrorCallback;
    
    int m_nUniqueKeyCounter;
    
    bool BindFunctionToDaslang(const ScriptFunctionBinding_t *pBinding);
    bool BindClassToDaslang(const ScriptClassDesc_t *pClassDesc);
    void RegisterBuiltinFunctions();
};

IScriptVM *CreateDaslangVM();

#endif // DASLANG_VSCRIPT_H
