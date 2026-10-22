// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Collect and print statistics
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#समावेश "V3PchAstMT.h"

#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3Os.h"
#समावेश "V3Stats.h"

#समावेश <iomanip>
#समावेश <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

श्रेणी StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::बख_प्रवाह& os;  ///< Output stream
    अटल StatColl s_allStats;  ///< All statistics

    अटल व्योम sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        क्रम (स्वतः& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        क्रम (स्थिर स्वतः& itr : byName) {
            V3Statistic* repp = itr.second;
            अगर (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } अन्यथा {
                lastp = repp;
            }
        }
    }

    व्योम stars() {
        // Find all stages
        माप_प्रकार maxWidth = 0;
        std::multimap<std::string, स्थिर V3Statistic*> byName;
        // * is always first
        क्रम (स्थिर स्वतः& itr : s_allStats) {
            स्थिर V3Statistic* repp = &itr;
            अगर (repp->stage() == "*" && repp->printit()) {
                अगर (maxWidth < repp->name().लंबाई()) maxWidth = repp->name().लंबाई();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        क्रम (स्थिर स्वतः& itr : byName) {
            स्थिर V3Statistic* repp = itr.second;
            अगर (repp->perf()) जारी;
            os << "  " << std::बाँए << std::रखो_चौड़ाई(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        क्रम (स्थिर स्वतः& itr : byName) {
            स्थिर V3Statistic* repp = itr.second;
            अगर (!repp->perf()) जारी;
            os << "  " << std::बाँए << std::रखो_चौड़ाई(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    व्योम stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        पूर्णांक stage = 0;
        माप_प्रकार maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, पूर्णांक> stageInt;
        std::multimap<std::string, स्थिर V3Statistic*> byName;
        // * is always first
        क्रम (स्वतः it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            स्थिर V3Statistic* repp = &(*it);
            अगर (repp->stage() != "*" && repp->printit()) {
                अगर (maxWidth < repp->name().लंबाई()) maxWidth = repp->name().लंबाई();
                स्थिर स्वतः itFoundPair = stageInt.emplace(repp->stage(), stage);
                अगर (itFoundPair.second) {
                    ++stage;
                    stages.धकेलो_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::बाँए << std::रखो_चौड़ाई(maxWidth - 5 - 2) << "";
        क्रम (स्थिर string& i : stages) os << " " << std::बाँए << std::रखो_चौड़ाई(9) << i;
        os << '\n';
        os << "  -------- " << std::बाँए << std::रखो_चौड़ाई(maxWidth - 5 - 2) << "";
        क्रम (स्वतः it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::बाँए << std::रखो_चौड़ाई(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        अचिन्हित col = 0;
        क्रम (स्वतः it = byName.cbegin(); it != byName.cend(); ++it) {
            स्थिर V3Statistic* repp = it->second;
            अगर (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::आकार_type pos = commaName.find(',');
                    अगर (pos != string::npos) commaName.erase(pos);
                    अगर (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::बाँए << std::रखो_चौड़ाई(maxWidth) << repp->name();
            }
            जबतक (col < stages.आकार() && stages.at(col) != repp->stage()) {
                os << std::रखो_चौड़ाई(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

खुला:
    // METHODS
    अटल व्योम addStat(स्थिर V3Statistic& stat) { s_allStats.धकेलो_back(stat); }

    अटल द्विगुन get_Stat_योग(स्थिर string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        क्रम (स्थिर V3Statistic& itr : s_allStats) {
            स्थिर V3Statistic* स्थिर repp = &itr;
            अगर (repp->name() == name) वापस repp->मूल्य();
        }
        वापस 0.0;
    }

    अटल व्योम calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::बख_प्रवाह* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = शेष;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

व्योम V3Statistic::dump(std::बख_प्रवाह& os) स्थिर {
    os << " " << std::दाँए << std::जड़ << std::रखो_सटीक(precision()) << std::रखो_चौड़ाई(9)
       << मूल्य();
}

//######################################################################
// Top Stats class

व्योम V3Stats::addStat(स्थिर V3Statistic& stat) { StatsReport::addStat(stat); }

द्विगुन V3Stats::get_Stat_योग(स्थिर string& name) { वापस StatsReport::get_Stat_योग(name); }

व्योम V3Stats::statsStage(स्थिर string& name) {
    अटल द्विगुन s_lastWallTime = -1;
    अटल पूर्णांक s_file_संख्या = 0;

    स्थिर string digitName = V3Global::digitsFilename(++s_file_संख्या) + "_" + name;

    स्थिर द्विगुन wallTime = V3Os::timeUsecs() / 1.0e6;
    अगर (s_lastWallTime < 0) s_lastWallTime = wallTime;
    स्थिर द्विगुन wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

व्योम V3Stats::infoHeader(std::बख_प्रवाह& os, स्थिर string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

व्योम V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    स्थिर द्विगुन memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    स्थिर string filename
        = v3Global.opt.hier_शीर्ष_तथ्य_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::बख_प्रवाह* ofp{V3File::new_ofstream(filename)};
    अगर (ofp->हार()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->बंद();
    VL_DO_DANGLING(मिटाओ ofp, ofp);
}

व्योम V3Stats::summaryReport() {
    StatsReport::calculate();
    std::अ_बाहर << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::अ_बाहर << std::रखो_सटीक(3) << std::जड़;
    स्थिर द्विगुन sourceCharsMB = V3Stats::get_Stat_योग_Q(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    स्थिर द्विगुन cppCharsMB = V3Stats::get_Stat_योग_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    स्थिर uint64_t sourceModules = V3Stats::get_Stat_योग_Q(STAT_SOURCE_MODULES);
    स्थिर uint64_t cppFiles = V3Stats::get_Stat_योग_Q(STAT_CPP_FILES);
    स्थिर द्विगुन modelMB = V3Stats::get_Stat_योग(STAT_MODEL_आकार) / 1024.0 / 1024.0;
    std::अ_बाहर << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    स्थिर द्विगुन walltime = V3Stats::get_Stat_योग(STAT_WALLTIME);
    स्थिर द्विगुन walltimeElab = V3Stats::get_Stat_योग(STAT_WALLTIME_ELAB);
    स्थिर द्विगुन walltimeCvt = V3Stats::get_Stat_योग(STAT_WALLTIME_CVT);
    स्थिर द्विगुन walltimeBuild = V3Stats::get_Stat_योग(STAT_WALLTIME_BUILD);
    स्थिर द्विगुन cputime = V3Stats::get_Stat_योग(STAT_CPUTIME);
    std::अ_बाहर << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    स्थिर द्विगुन memory = memPeak / 1024.0 / 1024.0;
    अगर (VL_UNCOVERABLE(memory != 0.0)) std::अ_बाहर << "; allocated " << memory << " MB";
    std::अ_बाहर << "\n";
}
