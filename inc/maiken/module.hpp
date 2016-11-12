/**
Copyright (c) 2013, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _MAIKEN_MODULE_HPP_
#define _MAIKEN_MODULE_HPP_

#include "kul/os.hpp"
#include "kul/log.hpp"
#include "kul/sys.hpp"

#include "maiken/defs.hpp"

/*
extern "C" 
KUL_PUBLISH 
void maiken_module_construct(maiken::Plugin* p);

extern "C" 
KUL_PUBLISH  
void maiken_module_destruct(maiken::Plugin* p);
*/

namespace maiken{ 

class ModuleException : public kul::Exception{
    public:
        ModuleException(const char*f, const uint16_t& l, const std::string& s) : kul::Exception(f, l, s){}
};

enum MODULE_PHASE {
    COMPILE = 0, LINK, PACK
};

class ModuleLoader;
class ModuleEnviron;

class Module{
    friend class ModuleLoader;
    friend class ModuleEnviron;
    private:
        const Application * app = nullptr;
        void application(const Application * _app){
            app = _app;
        }
    public:
    	virtual ~Module(){}
    	Module() throw(ModuleException) {}

        virtual void compile(Application& app, const YAML::Node& arg) throw(std::exception) {} 
        virtual void link   (Application& app, const YAML::Node& arg) throw(std::exception) {}
        virtual void pack   (Application& app, const YAML::Node& arg) throw(std::exception) {}
        virtual void destroy(Application& app) throw(ModuleException) {}
};

// class ModuleEnviron{
//     private:
//         std::string oldEnv;
// #ifdef _WIN32
//         static constexpr const char* KEY = "PATH";
// #else
//         static constexpr const char* KEY = "LD_LIBRARY_PATH";
// #endif
//         const Application& ap;

//     public:
//         ModuleEnviron(const Application& _ap) : ap(_ap)   { set(); }
//         ModuleEnviron(const Module* _mod) : ap(*_mod->app){ set(); }

//         void set(){
//             oldEnv = kul::env::GET(KEY);
//             KLOG(DBG) << oldEnv;
//             std::stringstream ss;
//             for(const std::string& s : ap.libraryPaths()) ss << s << kul::env::SEP();
//             kul::cli::EnvVar var(KEY, ss.str(), kul::cli::EnvVarMode::PREP);
            
//             const auto newEnv(var.toString());
//             KLOG(DBG) << newEnv;
//             kul::env::SET(KEY, newEnv.c_str());
//         }
//         ~ModuleEnviron(){
//             kul::env::SET(KEY, oldEnv.c_str());
//         }
// };

class GlobalModules;
class KUL_PUBLISH ModuleLoader : public kul::sys::SharedClass<maiken::Module> {
    friend class GlobalModules;
    private:
        bool loaded = 0;
        Module* p = nullptr;

        static kul::File FIND(const Application& a) throw(kul::sys::Exception);
    public:
        ModuleLoader(const Application& ap, const kul::File& f) throw(kul::sys::Exception) 
            : kul::sys::SharedClass<maiken::Module>(f, "maiken_module_construct", "maiken_module_destruct") {
            construct(p);
            p->application(&ap);
            loaded = 1;
        }
        ~ModuleLoader(){
            if(loaded) KERR << "WARNING: ModuleLoader not unloaded, possible memory leak";
        }
        void unload(){
            if(loaded) destruct(p);
            loaded = 0;
        }
        Module* module(){
            return p;
        }
        const Application* app(){
            return p->app;
        }

        static std::shared_ptr<ModuleLoader> LOAD(const Application& ap) throw(kul::sys::Exception);
};

class GlobalModules{
    friend class ModuleLoader;
    private:
        kul::hash::map::S2T<std::shared_ptr<kul::sys::SharedLibrary>> libs;
        static GlobalModules& INSTANCE(){
            static GlobalModules i;
            return i;
        }
        ~GlobalModules(){
            libs.clear();
        }
        void load(const Application& ap) throw (kul::sys::Exception) {
            if(!libs.count(ap.buildDir().real())){
                libs.insert(
                    std::make_pair(
                        ap.buildDir().real(), 
                        std::make_shared<kul::sys::SharedLibrary>(
                            ModuleLoader::FIND(ap)
                        )
                    )
                );
            }
        }
};


}
#endif /* _MAIKEN_MODULE_HPP_ */