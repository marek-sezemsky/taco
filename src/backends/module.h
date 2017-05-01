#ifndef TACO_MODULE_H
#define TACO_MODULE_H

#include <map>
#include <vector>
#include <string>
#include <utility>

#include "taco/target.h"
#include "ir/ir.h"
#include "codegen_c.h"

namespace taco {
namespace ir {

class Module {
public:
  /// Create a module for some target
  Module(Target target=getTargetFromEnvironment()) :  target(target) {
    setJITLibname();
    setJITTmpdir();
    
  }

  /// Compile the source into a library, returning
  /// its full path
  std::string compile();
  
  /// Compile the module into a source file located
  /// at the specified location path and prefix.  The generated
  /// source will be path/prefix.{.c|.bc, .h}
  void compileToSource(std::string path, std::string prefix);
  
  /// Compile the module into a static library located
  /// at the specified location path and prefix.  The generated
  /// library will be path/prefix.a
  void compileToStaticLibrary(std::string path, std::string prefix);
  
  /// Add a lowered function to this module */
  void addFunction(Stmt func, bool internal=false);
  
  /// Get the source of the module as a string */
  std::string getSource();
  
  /// Get a function pointer to a compiled function.
  /// This returns a void* pointer, which the caller is
  /// required to cast to the correct function type before
  /// calling.
  void *getFunc(std::string name);
  
  /// Call a function in this module and return the result
  int callFuncPacked(std::string name, void** args);
  
  /// Call a function in this module and return the result
  int callFuncPacked(std::string name, std::vector<void*> args) {
    return callFuncPacked(name, &(args[0]));
  }

  /// Set the source of the module
  void setSource(std::string source);
  
private:
  std::stringstream source;
  std::stringstream header;
  std::string libname;
  std::string tmpdir;
  void* lib_handle;
  std::vector<std::pair<Stmt, bool>> funcs;

  Target target;
  
  void setJITLibname();
  void setJITTmpdir();
};

} // namespace ir
} // namespace taco
#endif
