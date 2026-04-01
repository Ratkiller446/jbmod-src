//========= Copyright Valve Corporation, All rights reserved. ============//

#include "daslang_vscript.h"
#include "cbase.h"
#include "tier0/icommandline.h"
#include "tier0/memdbgon.h"
#include "vscript/ivscript.h"
#include "vscript/variant.h"

#include <daScript/daScript.h>
#include <daScript/module/built_in.h>
#include <daScript/module/math.h>
#include <daScript/module/strings.h>
#include <daScript/module/rtti.h>
#include <daScript/module/debugger.h>
#include <daScript/module/jit.h>
#include <daScript/module/fio.h>
#include <daScript/module/network.h>

extern void RegisterDaslangJBModModule(das::ModuleLibrary& lib);

CDaslangVM::CDaslangVM()
    : m_pContext(nullptr)
    , m_pModuleLibrary(nullptr)
    , m_pModuleGroup(nullptr)
    , m_pOutputCallback(nullptr)
    , m_pErrorCallback(nullptr)
    , m_nUniqueKeyCounter(0)
    , m_GlobalVariables(0, 0, CaselessStringLessThan)
{
}

CDaslangVM::~CDaslangVM()
{
    Shutdown();
}

bool CDaslangVM::Init()
{
    das::Module::Initialize();
    das::register_builtin_modules();
    
    m_pModuleGroup = new das::ModuleGroup();
    m_pContext = new das::Context(4096);
    
    if (!m_pContext)
    {
        Warning("Failed to create Daslang context\n");
        return false;
    }
    
    m_pModuleLibrary = new das::ModuleLibrary(m_pContext);
    RegisterDaslangJBModModule(*m_pModuleLibrary);
    RegisterBuiltinFunctions();
    
    return true;
}

void CDaslangVM::Shutdown()
{
    m_RegisteredFunctions.RemoveAll();
    m_RegisteredClasses.RemoveAll();
    m_RegisteredInstances.RemoveAll();
    m_GlobalVariables.RemoveAll();
    
    if (m_pModuleLibrary)
    {
        delete m_pModuleLibrary;
        m_pModuleLibrary = nullptr;
    }
    
    if (m_pContext)
    {
        delete m_pContext;
        m_pContext = nullptr;
    }
    
    if (m_pModuleGroup)
    {
        delete m_pModuleGroup;
        m_pModuleGroup = nullptr;
    }
    
    das::Module::Shutdown();
}

bool CDaslangVM::ConnectDebugger()
{
    return false;
}

void CDaslangVM::DisconnectDebugger()
{
}

ScriptLanguage_t CDaslangVM::GetLanguage()
{
    return SL_DASLANG;
}

const char *CDaslangVM::GetLanguageName()
{
    return "daslang";
}

void CDaslangVM::AddSearchPath(const char *pszSearchPath)
{
}

bool CDaslangVM::Frame(float simTime)
{
    return false;
}

ScriptStatus_t CDaslangVM::Run(const char *pszScript, bool bWait)
{
    if (!m_pContext || !pszScript)
        return SCRIPT_ERROR;
    
    das::CompileOptions options;
    auto program = das::compileDaScript(pszScript, options, *m_pModuleGroup);
    
    if (!program || !program->isOk())
    {
        Warning("Failed to compile Daslang script\n");
        if (program)
        {
            Warning("Compilation errors: %s\n", program->errors.c_str());
        }
        return SCRIPT_ERROR;
    }
    
    if (!program->simulate(*m_pContext, *m_pModuleLibrary))
    {
        Warning("Failed to simulate Daslang script\n");
        return SCRIPT_ERROR;
    }
    
    das::Function *mainFunc = m_pContext->findFunction("main");
    if (mainFunc)
    {
        m_pContext->restart();
        m_pContext->eval(mainFunc, nullptr);
        
        if (m_pContext->getException())
        {
            Warning("Daslang script exception: %s\n", m_pContext->getException()->what());
            return SCRIPT_ERROR;
        }
    }
    
    return SCRIPT_OK;
}

HSCRIPT CDaslangVM::CompileScript(const char *pszScript, const char *pszId)
{
    if (!pszScript)
        return nullptr;
    
    das::CompileOptions options;
    auto program = das::compileDaScript(pszScript, options, *m_pModuleGroup);
    
    if (!program || !program->isOk())
    {
        Warning("Failed to compile Daslang script\n");
        return nullptr;
    }
    
    das::Program *pProgram = program.release();
    return reinterpret_cast<HSCRIPT>(pProgram);
}

void CDaslangVM::ReleaseScript(HSCRIPT hScript)
{
    if (hScript)
    {
        das::Program *pProgram = reinterpret_cast<das::Program*>(hScript);
        delete pProgram;
    }
}

ScriptStatus_t CDaslangVM::Run(HSCRIPT hScript, HSCRIPT hScope, bool bWait)
{
    if (!m_pContext || !hScript)
        return SCRIPT_ERROR;
    
    das::Program *pProgram = reinterpret_cast<das::Program*>(hScript);
    
    if (!pProgram->simulate(*m_pContext, *m_pModuleLibrary))
    {
        Warning("Failed to simulate Daslang script\n");
        return SCRIPT_ERROR;
    }
    
    das::Function *mainFunc = m_pContext->findFunction("main");
    if (mainFunc)
    {
        m_pContext->restart();
        m_pContext->eval(mainFunc, nullptr);
        
        if (m_pContext->getException())
        {
            Warning("Daslang script exception: %s\n", m_pContext->getException()->what());
            return SCRIPT_ERROR;
        }
    }
    
    return SCRIPT_OK;
}

ScriptStatus_t CDaslangVM::Run(HSCRIPT hScript, bool bWait)
{
    return Run(hScript, NULL, bWait);
}

HSCRIPT CDaslangVM::CreateScope(const char *pszScope, HSCRIPT hParent)
{
    return nullptr;
}

HSCRIPT CDaslangVM::ReferenceScope(HSCRIPT hScript)
{
    return nullptr;
}

void CDaslangVM::ReleaseScope(HSCRIPT hScript)
{
}

HSCRIPT CDaslangVM::LookupFunction(const char *pszFunction, HSCRIPT hScope, bool bNoDelegation)
{
    if (!m_pContext || !pszFunction)
        return nullptr;
    
    das::Function *func = m_pContext->findFunction(pszFunction);
    return reinterpret_cast<HSCRIPT>(func);
}

void CDaslangVM::ReleaseFunction(HSCRIPT hScript)
{
}

void CDaslangVM::RegisterFunction(ScriptFunctionBinding_t *pScriptFunction)
{
    if (!pScriptFunction)
        return;
    
    DaslangFunctionBinding binding;
    binding.binding = *pScriptFunction;
    binding.pDaslangFunc = nullptr;
    
    m_RegisteredFunctions.AddToTail(binding);
    BindFunctionToDaslang(pScriptFunction);
}

bool CDaslangVM::RegisterClass(ScriptClassDesc_t *pClassDesc)
{
    if (!pClassDesc)
        return false;
    
    DaslangClassBinding classBinding;
    classBinding.pClassDesc = pClassDesc;
    
    m_RegisteredClasses.AddToTail(classBinding);
    return BindClassToDaslang(pClassDesc);
}

void CDaslangVM::RegisterAllClasses()
{
    for (int i = 0; i < m_RegisteredClasses.Count(); i++)
    {
        BindClassToDaslang(m_RegisteredClasses[i].pClassDesc);
    }
}

HSCRIPT CDaslangVM::RegisterInstance(ScriptClassDesc_t *pDesc, void *pInstance)
{
    if (!pDesc || !pInstance)
        return nullptr;
    
    DaslangInstanceBinding instanceBinding;
    instanceBinding.pClassDesc = pDesc;
    instanceBinding.pInstance = pInstance;
    
    m_RegisteredInstances.AddToTail(instanceBinding);
    return reinterpret_cast<HSCRIPT>(&m_RegisteredInstances[m_RegisteredInstances.Count() - 1]);
}

void CDaslangVM::SetInstanceUniqeId(HSCRIPT hInstance, const char *pszId)
{
    if (!hInstance || !pszId)
        return;
    
    DaslangInstanceBinding *pBinding = reinterpret_cast<DaslangInstanceBinding*>(hInstance);
    pBinding->uniqueId = pszId;
}

void CDaslangVM::RemoveInstance(HSCRIPT hInstance)
{
    if (!hInstance)
        return;
    
    for (int i = 0; i < m_RegisteredInstances.Count(); i++)
    {
        if (&m_RegisteredInstances[i] == reinterpret_cast<DaslangInstanceBinding*>(hInstance))
        {
            m_RegisteredInstances.Remove(i);
            break;
        }
    }
}

void CDaslangVM::RemoveInstance(HSCRIPT hInstance, const char *pszInstance, HSCRIPT hScope)
{
    RemoveInstance(hInstance);
}

void *CDaslangVM::GetInstanceValue(HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType)
{
    if (!hInstance)
        return nullptr;
    
    DaslangInstanceBinding *pBinding = reinterpret_cast<DaslangInstanceBinding*>(hInstance);
    
    if (pExpectedType && pBinding->pClassDesc != pExpectedType)
        return nullptr;
    
    return pBinding->pInstance;
}

bool CDaslangVM::GenerateUniqueKey(const char *pszRoot, char *pBuf, int nBufSize)
{
    if (!pszRoot || !pBuf || nBufSize <= 0)
        return false;
    
    V_snprintf(pBuf, nBufSize, "%s_%d", pszRoot, m_nUniqueKeyCounter++);
    return true;
}

bool CDaslangVM::ValueExists(HSCRIPT hScope, const char *pszKey)
{
    if (!pszKey)
        return false;
    
    return m_GlobalVariables.Find(pszKey) != m_GlobalVariables.InvalidIndex();
}

bool CDaslangVM::SetValue(HSCRIPT hScope, const char *pszKey, const char *pszValue)
{
    if (!pszKey)
        return false;
    
    ScriptVariant_t variant(pszValue);
    m_GlobalVariables.InsertOrReplace(pszKey, variant);
    return true;
}

bool CDaslangVM::SetValue(HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value)
{
    if (!pszKey)
        return false;
    
    m_GlobalVariables.InsertOrReplace(pszKey, value);
    return true;
}

void CDaslangVM::CreateTable(ScriptVariant_t &Table)
{
    Table = ScriptVariant_t();
}

int CDaslangVM::GetNumTableEntries(HSCRIPT hScope)
{
    return 0;
}

int CDaslangVM::GetKeyValue(HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue)
{
    return 0;
}

bool CDaslangVM::GetValue(HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue)
{
    if (!pszKey || !pValue)
        return false;
    
    int index = m_GlobalVariables.Find(pszKey);
    if (index == m_GlobalVariables.InvalidIndex())
        return false;
    
    *pValue = m_GlobalVariables[index];
    return true;
}

void CDaslangVM::ReleaseValue(ScriptVariant_t &value)
{
    value = ScriptVariant_t();
}

bool CDaslangVM::ClearValue(HSCRIPT hScope, const char *pszKey)
{
    if (!pszKey)
        return false;
    
    int index = m_GlobalVariables.Find(pszKey);
    if (index == m_GlobalVariables.InvalidIndex())
        return false;
    
    m_GlobalVariables.RemoveAt(index);
    return true;
}

void CDaslangVM::WriteState(CUtlBuffer *pBuffer)
{
}

void CDaslangVM::ReadState(CUtlBuffer *pBuffer)
{
}

void CDaslangVM::RemoveOrphanInstances()
{
    for (int i = m_RegisteredInstances.Count() - 1; i >= 0; i--)
    {
        if (!m_RegisteredInstances[i].pInstance)
        {
            m_RegisteredInstances.Remove(i);
        }
    }
}

void CDaslangVM::DumpState()
{
    Msg("Daslang VM State:\n");
    Msg("  Registered Functions: %d\n", m_RegisteredFunctions.Count());
    Msg("  Registered Classes: %d\n", m_RegisteredClasses.Count());
    Msg("  Registered Instances: %d\n", m_RegisteredInstances.Count());
    Msg("  Global Variables: %d\n", m_GlobalVariables.Count());
}

void CDaslangVM::SetOutputCallback(ScriptOutputFunc_t pFunc)
{
    m_pOutputCallback = pFunc;
}

void CDaslangVM::SetErrorCallback(ScriptErrorFunc_t pFunc)
{
    m_pErrorCallback = pFunc;
}

bool CDaslangVM::RaiseException(const char *pszExceptionText)
{
    if (m_pErrorCallback && pszExceptionText)
    {
        m_pErrorCallback(pszExceptionText);
    }
    return true;
}

ScriptStatus_t CDaslangVM::ExecuteFunction(HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait)
{
    if (!m_pContext || !hFunction)
        return SCRIPT_ERROR;
    
    das::Function *func = reinterpret_cast<das::Function*>(hFunction);
    
    m_pContext->restart();
    m_pContext->eval(func, nullptr);
    
    if (m_pContext->getException())
    {
        Warning("Daslang function exception: %s\n", m_pContext->getException()->what());
        return SCRIPT_ERROR;
    }
    
    return SCRIPT_OK;
}

bool CDaslangVM::BindFunctionToDaslang(const ScriptFunctionBinding_t *pBinding)
{
    if (!pBinding || !m_pModuleLibrary)
        return false;
    
    return true;
}

bool CDaslangVM::BindClassToDaslang(const ScriptClassDesc_t *pClassDesc)
{
    if (!pClassDesc || !m_pModuleLibrary)
        return false;
    
    return true;
}

void CDaslangVM::RegisterBuiltinFunctions()
{
}

IScriptVM *CreateDaslangVM()
{
    return new CDaslangVM();
}
