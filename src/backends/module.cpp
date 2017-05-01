#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <unistd.h>


#include "module.h"
#include "taco/error.h"
#include "taco/util/strings.h"
#include "taco/util/env.h"

using namespace std;

namespace taco {
namespace ir {

void Module::setJITTmpdir() {
  tmpdir = util::getTmpdir();
}

void Module::setJITLibname() {
  string chars = "abcdefghijkmnpqrstuvwxyz0123456789";
  libname.resize(12);
  for (int i=0; i<12; i++)
    libname[i] = chars[rand() % chars.length()];
}

void Module::addFunction(Stmt func, bool internal) {
  funcs.push_back({func, internal});
  
  //REMOVE
  stringstream tmp;
  CodeGen_C cg(tmp, CodeGen_C::C99Implementation);
  cg.compile(func, false, CodeGen_C::InterfaceKind::Normal);
  std::cout << tmp.str();
}

void Module::compileToSource(string path, string prefix) {
  // create a codegen instance and add all the funcs
  bool didGenRuntime = false;
  
  header.str("");
  source.str("");
  header.clear();
  source.clear();
  
  taco_tassert(target.arch == Target::C99)
  << "Only C99 codegen supported currently";
  CodeGen_C codegen(source, CodeGen_C::OutputKind::C99Implementation);
  CodeGen_C headergen(header, CodeGen_C::OutputKind::C99Header);
  
  
  for (auto func: funcs) {
    auto interfaceKind = func.second ?
      CodeGen_C::InterfaceKind::Internal :
      CodeGen_C::InterfaceKind::Normal;
    codegen.compile(func.first, !didGenRuntime, interfaceKind);
    headergen.compile(func.first, interfaceKind);
    didGenRuntime = true;
  }

  ofstream source_file;
  source_file.open(path+prefix+".c");
  source_file << source.str();
  source_file.close();
  
  ofstream header_file;
  header_file.open(path+prefix+".h");
  header_file << header.str();
  header_file.close();

}

void Module::compileToStaticLibrary(string path, string prefix) {
  taco_tassert(false) << "Compiling to a static library is not supported";
}

string Module::compile() {
  string prefix = tmpdir+libname;
  string fullpath = prefix + ".so";
  
  string cc = util::getFromEnv("TACO_CC", "cc");
  string cflags = util::getFromEnv("TACO_CFLAGS",
    "-O3 -ffast-math -std=c99") + " -shared -fPIC";
  
  string cmd = cc + " " + cflags + " " +
    prefix + ".c " +
    "-o " + prefix + ".so";

  // open the output file & write out the source
  compileToSource(tmpdir, libname);
  
  // now compile it
  int err = system(cmd.data());
  taco_uassert(err == 0) << "Compilation command failed:\n" << cmd
    << "\nreturned " << err;

  // use dlsym() to open the compiled library
  lib_handle = dlopen(fullpath.data(), RTLD_NOW | RTLD_LOCAL);

  return fullpath;
}

void Module::setSource(string source) {
  this->source << source;
}

string Module::getSource() {
  return source.str();
}

void* Module::getFunc(std::string name) {
  void* ret = dlsym(lib_handle, name.data());
  taco_uassert(ret != nullptr) <<
      "Function " << name << " not found in module " << tmpdir << libname;
  return ret;
}

int Module::callFuncPacked(std::string name, void** args) {
  typedef int (*fnptr_t)(void**);
  static_assert(sizeof(void*) == sizeof(fnptr_t),
    "Unable to cast dlsym() returned void pointer to function pointer");
  void* v_func_ptr = getFunc(name);
  fnptr_t func_ptr;
  *reinterpret_cast<void**>(&func_ptr) = v_func_ptr;
  return func_ptr(args);
}

} // namespace ir
} // namespace taco
