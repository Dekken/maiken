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
#include "maiken.hpp"

void maiken::Application::showConfig(){
    if(AppVars::INSTANCE().show()) return;
    if(kul::LogMan::INSTANCE().inf()){
        std::string repo = (Settings::INSTANCE().root()[LOCAL] && Settings::INSTANCE().root()[LOCAL][REPO])
            ? Settings::INSTANCE().root()[LOCAL][REPO].Scalar() : kul::os::userAppDir(MAIKEN).join(REPO);
        using namespace kul::cli;
        KOUT(INF) << "+++++++++ BUILD INFO ++++++++";
        KOUT(INF) << "REPO    : " << repo;
        KOUT(INF) << "THREADS : " << AppVars::INSTANCE().threads() << "\n";
        KOUT(INF) << "BINARIES";
        std::string path = kul::env::GET("PATH");
        for(const YAML::Node& c : Settings::INSTANCE().root()[ENV]){
            if(c[NAME].Scalar() != "PATH") continue;
            EnvVarMode mode = EnvVarMode::APPE;
            if      (c[MODE].Scalar().compare(APPEND)   == 0) mode = EnvVarMode::APPE;
            else if (c[MODE].Scalar().compare(PREPEND)  == 0) mode = EnvVarMode::PREP;
            else if (c[MODE].Scalar().compare(REPLACE)  == 0) mode = EnvVarMode::REPL;
            else KEXCEPT(Exception, "Unhandled EnvVar mode: " + c[MODE].Scalar());
            path = EnvVar(c[NAME].Scalar(), c[VALUE].Scalar(), mode).toString();
            break;
        }
        for(const auto& c : Settings::INSTANCE().root()[FILE]){
            bool a = 0, g = 0, l = 0;
            KOUT(INF) << "TYPE    : " << c[TYPE].Scalar();
            for(const auto& d : kul::String::split(path, kul::env::SEP())){
                if(a && g && l) break;
                kul::Dir dir(d);
                if(!dir) continue;
                for(const auto& f : dir.files()){
                    std::string b = (f.name().size() > 3 && f.name().substr(f.name().size() - 4) == ".exe") ?
                        f.name().substr(0, f.name().size() - 4) : f.name();
                    if(!a && c[ARCHIVER] && b == kul::String::split(c[ARCHIVER].Scalar(), " ")[0]){
                        KOUT(INF) << "ARCHIVER: " << f.full(); a = 1; break;
                    }
                }
                for(const auto& f : dir.files()){
                    std::string b = (f.name().size() > 3 && f.name().substr(f.name().size() - 4) == ".exe") ?
                        f.name().substr(0, f.name().size() - 4) : f.name();
                    if(!g && c[COMPILER] && b == kul::code::Compilers::INSTANCE().key(c[COMPILER].Scalar())){
                        KOUT(INF) << "COMPILER: " << f.full(); g = 1; break;
                    }
                }
                for(const auto& f : dir.files()){
                    std::string b = (f.name().size() > 3 && f.name().substr(f.name().size() - 4) == ".exe") ?
                        f.name().substr(0, f.name().size() - 4) : f.name();
                    if(!l && c[LINKER] && b == kul::String::split(c[LINKER].Scalar(), " ")[0]){
                        KOUT(INF) << "LINKER  : " << f.full(); l = 1; break;
                    }
                }
            }
        }
        KOUT(INF) << "+++++++++++++++++++++++++++++";
    }
    AppVars::INSTANCE().show(1);
}

void maiken::Application::showHelp(){
    std::vector<std::string> ss;
    ss.push_back(MKN_DEFS_CMD);
    ss.push_back(MKN_DEFS_BUILD);
    ss.push_back(MKN_DEFS_CLEAN);
    ss.push_back(MKN_DEFS_COMP);
    ss.push_back(MKN_DEFS_INIT);
    ss.push_back(MKN_DEFS_LINK);
    ss.push_back(MKN_DEFS_PROFS);
    ss.push_back(MKN_DEFS_RUN);
    ss.push_back(MKN_DEFS_INC);
    ss.push_back(MKN_DEFS_SRC);
    ss.push_back(MKN_DEFS_TRIM);
    ss.push_back("");
    ss.push_back(MKN_DEFS_ARG);
    ss.push_back(MKN_DEFS_ARGS);
    ss.push_back(MKN_DEFS_DEPS);
    ss.push_back(MKN_DEFS_PROF);
    ss.push_back(MKN_DEFS_STAT);
    ss.push_back(MKN_DEFS_THREDS);
    ss.push_back(MKN_DEFS_UPDATE);
    ss.push_back(MKN_DEFS_VERSON);
    ss.push_back(MKN_DEFS_SETTNGS);
    ss.push_back("");
    ss.push_back(MKN_DEFS_EXMPL);
    ss.push_back(MKN_DEFS_EXMPL1);
    ss.push_back(MKN_DEFS_EXMPL2);
    ss.push_back(MKN_DEFS_EXMPL3);
    ss.push_back("");
    for(const auto& s : ss) KOUT(NON) << s;
}

void maiken::Application::showProfiles(){
    std::vector<std::string> ss;
    uint b = 0, o = 0;
    for(const auto& n : this->project().root()[PROFILE]){
        b = n[NAME].Scalar().size() > b ? n[NAME].Scalar().size() : b;
        o = n["os"] ? n["os"].Scalar().size() > o ? n["os"].Scalar().size() : o : o;
    }
    for(const auto& n : this->project().root()[PROFILE]){
        std::string s(n[NAME].Scalar());
        kul::String::pad(s, b);
        std::string os(n["os"] ? "("+n["os"].Scalar()+")" : "");
        if(!os.empty()) kul::String::pad(os, o);
        std::stringstream s1;
        s1 << "\t" << s << os;
        if(n[PARENT]) s1 << "\t" << MKN_PARENT << ": " << resolveFromProperties(n[PARENT].Scalar());
        ss.push_back(s1.str());
    }
    KOUT(NON) << MKN_PROFILE;
    for(const auto& s : ss) KOUT(NON) << s;
}
