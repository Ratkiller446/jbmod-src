#include "daslang_vscript.h"
#include "cbase.h"
#include "tier0/memdbgon.h"

int main()
{
    printf("Testing Daslang integration with JBMod...\n");
    
    IScriptVM *pVM = CreateDaslangVM();
    if (!pVM)
    {
        printf("Failed to create Daslang VM\n");
        return 1;
    }
    
    printf("Successfully created Daslang VM\n");
    
    if (!pVM->Init())
    {
        printf("Failed to initialize Daslang VM\n");
        delete pVM;
        return 1;
    }
    
    printf("Successfully initialized Daslang VM\n");
    
    const char *testScript = 
        "def add(a: int, b: int): int {\n"
        "    return a + b;\n"
        "}\n"
        "\n"
        "var result = add(5, 3);\n"
        "print(\"5 + 3 = \" + string(result));\n";
    
    ScriptStatus_t status = pVM->Run(testScript);
    if (status != SCRIPT_OK)
    {
        printf("Failed to run test script (status: %d)\n", status);
        pVM->Shutdown();
        delete pVM;
        return 1;
    }
    
    printf("Successfully ran test script\n");
    
    pVM->Shutdown();
    delete pVM;
    
    printf("Daslang integration test completed successfully!\n");
    return 0;
}
