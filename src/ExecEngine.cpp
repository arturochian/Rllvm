#include "Rllvm.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>

#if LLVM_VERSION < 3
#include <llvm/Target/TargetSelect.h>
#else
#include <llvm/Support/TargetSelect.h>
#endif


extern "C"
void
R_InitializeNativeTarget()
{
    llvm::InitializeNativeTarget();
}

#include <llvm/Support/ManagedStatic.h>
extern "C"
void
R_LLVMShutdown()
{
    llvm::llvm_shutdown();
}


#if 0
/* why did we define this here when it creates a routine that does nothing. */
#define LLVM_TARGET(target) \
    void inline Initialize##target##Target() { \
    }

#include <llvm/Config/Targets.def>

#undef LLVM_TARGET
#endif


#if 1
extern "C"
void
R_InitializeNVPTXTarget()
{
    LLVMInitializeNVPTXTarget();
    LLVMInitializeNVPTXTargetInfo();
    LLVMInitializeNVPTXTargetMC();
    LLVMInitializeNVPTXAsmPrinter();
}

extern "C"
void
R_InitializeCppBackendTarget()
{
    LLVMInitializeCppBackendTarget();
    LLVMInitializeCppBackendTargetInfo();
    LLVMInitializeCppBackendTargetMC();
}
#endif


extern "C"
SEXP
R_create_ExecutionEngine(SEXP r_module, SEXP r_optLevel)
{

    /* Do we want to use some of the create() methods in the ExecutionEngine class. */
    std::string errStr;
    llvm::Module *module = GET_REF(r_module, Module);
    llvm::ExecutionEngine *EE = llvm::EngineBuilder(module).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).setOptLevel((enum llvm::CodeGenOpt::Level) INTEGER(r_optLevel)[0]).create();
    if(!EE) {
        PROBLEM "failed to create execution engine: %s", errStr.c_str()
            ERROR;
    }

    return(R_createRef(EE, "ExecutionEngine"));
}

extern "C"
SEXP 
R_callFunction(SEXP r_fun, SEXP r_args, SEXP r_execEngine)
{
    llvm::Function *fun;
    fun = GET_REF(r_fun, Function);

    int nargs = Rf_length(r_args);
    const llvm::FunctionType *fty = fun->getFunctionType();
    int numParams = fty->getNumParams();
    if(nargs != numParams
           && !(fty->isVarArg() && nargs >= numParams)) {
          /* LLVM doesn't currently support passing ... through runFunction */
        PROBLEM "incorrect number of arguments to routine"
            ERROR; 
    }

    std::vector<llvm::GenericValue> args(nargs);
    if(nargs > 0) {
        for(int i = 0; i < nargs; i++)
            convertRToGenericValue(&args[i], VECTOR_ELT(r_args, i), i < numParams ? fty->getParamType(i) : NULL);
    }

    llvm::ExecutionEngine *ee = GET_REF(r_execEngine, ExecutionEngine);
    llvm::GenericValue val = ee->runFunction(fun, args);
    // need to return any mutable parameters.
    return(convertGenericValueToR(&val, fun->getReturnType()));
}

extern "C"
SEXP
R_ExecutionEngine_addModule(SEXP r_execEngine, SEXP r_mods)
{
    llvm::ExecutionEngine *ee = GET_REF(r_execEngine, ExecutionEngine);
    llvm::Module *m;
    for(int i = 0 ; i < Rf_length(r_mods); i++) {
        m = GET_REF(VECTOR_ELT(r_mods, i), Module);
        ee->addModule(m);
    }
    return(R_NilValue);
}


extern "C"
SEXP
R_ExecutionEngine_getPointerToFunction(SEXP r_execEngine, SEXP r_func)
{
    llvm::ExecutionEngine *ee = GET_REF(r_execEngine, ExecutionEngine);
    llvm::Function *fun = GET_REF(r_func, Function);
    void *ans = ee->getPointerToFunction(fun);
    
    return(R_createRef(ans, "NativeFunctionPointer", "native symbol"));
}

extern "C"
SEXP
R_ExecutionEngine_getPointerToGlobal(SEXP r_execEngine, SEXP r_var)
{
    llvm::ExecutionEngine *ee = GET_REF(r_execEngine, ExecutionEngine);
    llvm::GlobalValue *var = GET_REF(r_var, GlobalValue);
    void *ans = ee->getPointerToGlobal(var);
    
    return(R_createRef(ans, "NativeGlobalVariable"));
}


extern "C"
SEXP
R_ExecutionEngine_FindFunctionNamed(SEXP r_execEngine, SEXP r_id)
{
    llvm::ExecutionEngine *ee = GET_REF(r_execEngine, ExecutionEngine);
    
    llvm::Function *ans = ee->FindFunctionNamed(CHAR(STRING_ELT(r_id, 0)));
    
    return(R_createRef(ans, "Function"));
}

#if 0
extern "C"
SEXP
R_ExecutionEngine_getGlobalValue(SEXP r_gv, SEXP r_convert)
{
    const llvm::GlobalValue *gval = GET_REF(r_gv, GlobalValue);
    return(convertGenericValueToR(gval->getUnderlyingObject(), gval->getType()));
}
#endif
