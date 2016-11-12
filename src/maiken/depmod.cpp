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

void maiken::Application::loadDepOrMod(const YAML::Node& node, const kul::Dir& depOrMod, bool module){
    KOUT(NON) << MKN_PROJECT_NOT_FOUND << depOrMod;
    kul::env::CWD(this->project().dir());
    const std::string& tscr(node[SCM] ? Properties::RESOLVE(*this, node[SCM].Scalar()) : node[NAME].Scalar());
    const std::string& v(node[VERSION] ? Properties::RESOLVE(*this, node[VERSION].Scalar()) : "");
    KOUT(NON) << SCMGetter::GET(depOrMod, tscr, module)->co(depOrMod.path(), SCMGetter::REPO(depOrMod, tscr, module), v);
    kul::env::CWD(depOrMod);
    if(_MKN_REMOTE_EXEC_){
#ifdef _WIN32
        if(kul::File("mkn.bat").is() 
                && kul::proc::Call("mkn.bat", AppVars::INSTANCE().envVars()).run()) 
            KEXCEPTION("ERROR in "+depOrMod.path()+"/mkn.bat");
#else
        if(kul::File("mkn."+std::string(KTOSTRING(__KUL_OS__))+".sh").is() 
            && kul::proc::Call("sh mkn."+std::string(KTOSTRING(__KUL_OS__))+".sh", AppVars::INSTANCE().envVars()).run())
            KEXCEPTION("ERROR in "+depOrMod.path()+"mkn."+std::string(KTOSTRING(__KUL_OS__))+".sh");
        else
        if(kul::File("mkn.sh").is() && kul::proc::Call("sh mkn.sh", AppVars::INSTANCE().envVars()).run()) 
            KEXCEPTION("ERROR in "+depOrMod.path()+"/mkn.sh");
#endif
    }
    kul::env::CWD(this->project().dir());
}

kul::Dir maiken::Application::resolveDepOrModDirectory(const YAML::Node& n, bool module){
    std::string d;
    if(n[LOCAL]) d = Properties::RESOLVE(*this, n[LOCAL].Scalar());
    else{
        d = (*AppVars::INSTANCE().properkeys().find(
            module ? "MKN_MOD_REPO" : "MKN_REPO")).second;
        try{
            std::string version(n[VERSION] ? Properties::RESOLVE(*this, n[VERSION].Scalar()) : "default");
            if(_MKN_REP_VERS_DOT_) kul::String::REPLACE_ALL(version, ".", kul::Dir::SEP());
            std::string name(Properties::RESOLVE(*this, n[NAME].Scalar()));
            if(_MKN_REP_NAME_DOT_) kul::String::REPLACE_ALL(name, ".", kul::Dir::SEP());
            d = kul::Dir::JOIN(d, kul::Dir::JOIN(name, version));
        }catch(const kul::Exception& e){ KERR << e.debug(); }
    }
    return kul::Dir(d);
}

void maiken::Application::popDepOrMod(
        const YAML::Node& n, 
        std::vector<Application>& vec, 
        const std::string& s, 
        bool module) throw(kul::Exception){

    auto setApp = [&](Application& app, const YAML::Node& node){
        if(node[SCM]) app.scr = Properties::RESOLVE(*this, node[SCM].Scalar());
        if(node[VERSION]) app.scv = Properties::RESOLVE(*this, node[VERSION].Scalar());
        if(module && node[ARG]){
            if(node[ARG][COMPILE]) app.modCompile(node[ARG][COMPILE]);
            if(node[ARG][LINK])    app.modLink   (node[ARG][LINK]);
            if(node[ARG][PACK])    app.modPack   (node[ARG][PACK]);
            app.isMod = 1;
        }
    };

    std::vector<std::pair<std::string, std::string>> apps;
    for(const auto& depOrMod : n[s]){
        const kul::Dir& projectDir = resolveDepOrModDirectory(depOrMod, module);
        bool f = false;
        for(const Application& a : vec)
            if(projectDir == a.project().dir() && p == a.p){
                f = true; break;
            }
        if(f) continue;
        const maiken::Project c(maiken::Project::CREATE(projectDir));

        if(depOrMod[PROFILE]){
            for(auto s : kul::String::SPLIT(Properties::RESOLVE(*this, depOrMod[PROFILE].Scalar()), ' ')){
                if(s.empty()) continue;
                f = 0;
                if(s == "@") s = "";
                else
                    for(const auto& node : c.root()[PROFILE])
                        if(node[NAME].Scalar() == s){
                            f = 1;
                            break;
                        }
                    
                if(!f && !s.empty()) 
                    KEXCEPTION("profile does not exist\n"+s+"\n"+project().dir().path());
                Application app(c, s);
                app.par = this;
                setApp(app, depOrMod);
                vec.push_back(app);
                apps.push_back(std::make_pair(app.project().dir().path(), app.p));
            }
        }else{
            Application app(c, "");
            app.par = this;
            setApp(app, depOrMod);
            vec.push_back(app);
            apps.push_back(std::make_pair(app.project().dir().path(), app.p));
        }
    }
    if(n[SELF])
        for(const auto& s : kul::String::SPLIT(Properties::RESOLVE(*this, n[SELF].Scalar()), ' ')){
            Application app(project(), s);
            app.par = this;
            app.scr = scr;
            vec.push_back(app);
            apps.push_back(std::make_pair(app.project().dir().path(), app.p));
        }
    cyclicCheck(apps);
    for(auto& app : vec){
        if(app.buildDir().path().size()) continue;
        kul::env::CWD(app.project().dir());
        app.setSuper(this);
        app.setup();
        if(app.project().root()[SCM]) app.scr = Properties::RESOLVE(app, app.project().root()[SCM].Scalar());
        if(!app.sources().empty()){
            app.buildDir().mk();
            app.paths.push_back(app.inst ? app.inst.escr() : app.buildDir().escr());
        }
        kul::env::CWD(this->project().dir());
    }
}
