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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

വർഗ്ഗം StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    സ്ഥിതം StatColl s_allStats;  ///< All statistics

    സ്ഥിതം ശൂന്യം sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        വേണ്ടി (auto& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        വേണ്ടി (സ്ഥിരം auto& itr : byName) {
            V3Statistic* repp = itr.second;
            എങ്കിൽ (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } അല്ലെങ്കിൽ {
                lastp = repp;
            }
        }
    }

    ശൂന്യം stars() {
        // Find all stages
        size_t maxWidth = 0;
        std::multimap<std::string, സ്ഥിരം V3Statistic*> byName;
        // * is always first
        വേണ്ടി (സ്ഥിരം auto& itr : s_allStats) {
            സ്ഥിരം V3Statistic* repp = &itr;
            എങ്കിൽ (repp->stage() == "*" && repp->printit()) {
                എങ്കിൽ (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        വേണ്ടി (സ്ഥിരം auto& itr : byName) {
            സ്ഥിരം V3Statistic* repp = itr.second;
            എങ്കിൽ (repp->perf()) തുടരുക;
            os << "  " << std::ഇടത് << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        വേണ്ടി (സ്ഥിരം auto& itr : byName) {
            സ്ഥിരം V3Statistic* repp = itr.second;
            എങ്കിൽ (!repp->perf()) തുടരുക;
            os << "  " << std::ഇടത് << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    ശൂന്യം stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        പൂർണ്ണസംഖ്യ stage = 0;
        size_t maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, പൂർണ്ണസംഖ്യ> stageInt;
        std::multimap<std::string, സ്ഥിരം V3Statistic*> byName;
        // * is always first
        വേണ്ടി (auto it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            സ്ഥിരം V3Statistic* repp = &(*it);
            എങ്കിൽ (repp->stage() != "*" && repp->printit()) {
                എങ്കിൽ (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                സ്ഥിരം auto itFoundPair = stageInt.emplace(repp->stage(), stage);
                എങ്കിൽ (itFoundPair.second) {
                    ++stage;
                    stages.തള്ളുക_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::ഇടത് << std::setw(maxWidth - 5 - 2) << "";
        വേണ്ടി (സ്ഥിരം string& i : stages) os << " " << std::ഇടത് << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::ഇടത് << std::setw(maxWidth - 5 - 2) << "";
        വേണ്ടി (auto it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::ഇടത് << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        unsigned col = 0;
        വേണ്ടി (auto it = byName.cbegin(); it != byName.cend(); ++it) {
            സ്ഥിരം V3Statistic* repp = it->second;
            എങ്കിൽ (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    എങ്കിൽ (pos != string::npos) commaName.erase(pos);
                    എങ്കിൽ (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::ഇടത് << std::setw(maxWidth) << repp->name();
            }
            ആകുമ്പോൾ (col < stages.size() && stages.at(col) != repp->stage()) {
                os << std::setw(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

പൊതു:
    // METHODS
    സ്ഥിതം ശൂന്യം addStat(സ്ഥിരം V3Statistic& stat) { s_allStats.തള്ളുക_back(stat); }

    സ്ഥിതം ഇരട്ട getStatSum(സ്ഥിരം string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        വേണ്ടി (സ്ഥിരം V3Statistic& itr : s_allStats) {
            സ്ഥിരം V3Statistic* സ്ഥിരം repp = &itr;
            എങ്കിൽ (repp->name() == name) തിരികെ repp->മൂല്യം();
        }
        തിരികെ 0.0;
    }

    സ്ഥിതം ശൂന്യം calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = സ്ഥിരസ്ഥിതി;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

ശൂന്യം V3Statistic::dump(std::ofstream& os) സ്ഥിരം {
    os << " " << std::വലത് << std::fixed << std::setprecision(precision()) << std::setw(9)
       << മൂല്യം();
}

//######################################################################
// Top Stats class

ശൂന്യം V3Stats::addStat(സ്ഥിരം V3Statistic& stat) { StatsReport::addStat(stat); }

ഇരട്ട V3Stats::getStatSum(സ്ഥിരം string& name) { തിരികെ StatsReport::getStatSum(name); }

ശൂന്യം V3Stats::statsStage(സ്ഥിരം string& name) {
    സ്ഥിതം ഇരട്ട s_lastWallTime = -1;
    സ്ഥിതം പൂർണ്ണസംഖ്യ s_file_സംഖ്യ = 0;

    സ്ഥിരം string digitName = V3Global::digitsFilename(++s_file_സംഖ്യ) + "_" + name;

    സ്ഥിരം ഇരട്ട wallTime = V3Os::timeUsecs() / 1.0e6;
    എങ്കിൽ (s_lastWallTime < 0) s_lastWallTime = wallTime;
    സ്ഥിരം ഇരട്ട wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

ശൂന്യം V3Stats::infoHeader(std::ofstream& os, സ്ഥിരം string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

ശൂന്യം V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    സ്ഥിരം ഇരട്ട memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    സ്ഥിരം string filename
        = v3Global.opt.hier_മുകൾ_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    എങ്കിൽ (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(delete ofp, ofp);
}

ശൂന്യം V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    സ്ഥിരം ഇരട്ട sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    സ്ഥിരം ഇരട്ട cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    സ്ഥിരം uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    സ്ഥിരം uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    സ്ഥിരം ഇരട്ട modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    സ്ഥിരം ഇരട്ട walltime = V3Stats::getStatSum(STAT_WALLTIME);
    സ്ഥിരം ഇരട്ട walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    സ്ഥിരം ഇരട്ട walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    സ്ഥിരം ഇരട്ട walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    സ്ഥിരം ഇരട്ട cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    സ്ഥിരം ഇരട്ട memory = memPeak / 1024.0 / 1024.0;
    എങ്കിൽ (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
