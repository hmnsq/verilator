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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

વર્ગ StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    સ્થિર StatColl s_allStats;  ///< All statistics

    સ્થિર શૂન્ય sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        માટે (auto& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        માટે (અચલ auto& itr : byName) {
            V3Statistic* repp = itr.second;
            જો (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } નહીંતર {
                lastp = repp;
            }
        }
    }

    શૂન્ય stars() {
        // Find all stages
        size_t maxWidth = 0;
        std::multimap<std::string, અચલ V3Statistic*> byName;
        // * is always first
        માટે (અચલ auto& itr : s_allStats) {
            અચલ V3Statistic* repp = &itr;
            જો (repp->stage() == "*" && repp->printit()) {
                જો (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        માટે (અચલ auto& itr : byName) {
            અચલ V3Statistic* repp = itr.second;
            જો (repp->perf()) ચાલુ;
            os << "  " << std::ડાબું << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        માટે (અચલ auto& itr : byName) {
            અચલ V3Statistic* repp = itr.second;
            જો (!repp->perf()) ચાલુ;
            os << "  " << std::ડાબું << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    શૂન્ય stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        પૂર્ણાંક stage = 0;
        size_t maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, પૂર્ણાંક> stageInt;
        std::multimap<std::string, અચલ V3Statistic*> byName;
        // * is always first
        માટે (auto it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            અચલ V3Statistic* repp = &(*it);
            જો (repp->stage() != "*" && repp->printit()) {
                જો (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                અચલ auto itFoundPair = stageInt.emplace(repp->stage(), stage);
                જો (itFoundPair.second) {
                    ++stage;
                    stages.ધકેલો_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::ડાબું << std::setw(maxWidth - 5 - 2) << "";
        માટે (અચલ string& i : stages) os << " " << std::ડાબું << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::ડાબું << std::setw(maxWidth - 5 - 2) << "";
        માટે (auto it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::ડાબું << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        unsigned col = 0;
        માટે (auto it = byName.cbegin(); it != byName.cend(); ++it) {
            અચલ V3Statistic* repp = it->second;
            જો (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    જો (pos != string::npos) commaName.erase(pos);
                    જો (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::ડાબું << std::setw(maxWidth) << repp->name();
            }
            જ્યારે (col < stages.size() && stages.at(col) != repp->stage()) {
                os << std::setw(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

જાહેર:
    // METHODS
    સ્થિર શૂન્ય addStat(અચલ V3Statistic& stat) { s_allStats.ધકેલો_back(stat); }

    સ્થિર દ્વિગુણ getStatSum(અચલ string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        માટે (અચલ V3Statistic& itr : s_allStats) {
            અચલ V3Statistic* અચલ repp = &itr;
            જો (repp->name() == name) પાછા repp->કિંમત();
        }
        પાછા 0.0;
    }

    સ્થિર શૂન્ય calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = મૂળભૂત;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

શૂન્ય V3Statistic::dump(std::ofstream& os) અચલ {
    os << " " << std::જમણું << std::fixed << std::setprecision(precision()) << std::setw(9)
       << કિંમત();
}

//######################################################################
// Top Stats class

શૂન્ય V3Stats::addStat(અચલ V3Statistic& stat) { StatsReport::addStat(stat); }

દ્વિગુણ V3Stats::getStatSum(અચલ string& name) { પાછા StatsReport::getStatSum(name); }

શૂન્ય V3Stats::statsStage(અચલ string& name) {
    સ્થિર દ્વિગુણ s_lastWallTime = -1;
    સ્થિર પૂર્ણાંક s_file_સંખ્યા = 0;

    અચલ string digitName = V3Global::digitsFilename(++s_file_સંખ્યા) + "_" + name;

    અચલ દ્વિગુણ wallTime = V3Os::timeUsecs() / 1.0e6;
    જો (s_lastWallTime < 0) s_lastWallTime = wallTime;
    અચલ દ્વિગુણ wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

શૂન્ય V3Stats::infoHeader(std::ofstream& os, અચલ string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

શૂન્ય V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    અચલ દ્વિગુણ memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    અચલ string filename
        = v3Global.opt.hier_ટોચ_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    જો (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(delete ofp, ofp);
}

શૂન્ય V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    અચલ દ્વિગુણ sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    અચલ દ્વિગુણ cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    અચલ uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    અચલ uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    અચલ દ્વિગુણ modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    અચલ દ્વિગુણ walltime = V3Stats::getStatSum(STAT_WALLTIME);
    અચલ દ્વિગુણ walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    અચલ દ્વિગુણ walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    અચલ દ્વિગુણ walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    અચલ દ્વિગુણ cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    અચલ દ્વિગુણ memory = memPeak / 1024.0 / 1024.0;
    જો (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
