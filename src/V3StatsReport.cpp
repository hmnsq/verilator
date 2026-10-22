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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

ವರ್ಗ StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ಸ್ಥಿರ StatColl s_allStats;  ///< All statistics

    ಸ್ಥಿರ ಶೂನ್ಯ sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        ಪ್ರತಿ (auto& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : byName) {
            V3Statistic* repp = itr.second;
            ಯದಿ (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } ಅನ್ಯಥಾ {
                lastp = repp;
            }
        }
    }

    ಶೂನ್ಯ stars() {
        // Find all stages
        size_t maxWidth = 0;
        std::multimap<std::string, ಧ್ರುವ V3Statistic*> byName;
        // * is always first
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : s_allStats) {
            ಧ್ರುವ V3Statistic* repp = &itr;
            ಯದಿ (repp->stage() == "*" && repp->printit()) {
                ಯದಿ (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : byName) {
            ಧ್ರುವ V3Statistic* repp = itr.second;
            ಯದಿ (repp->perf()) ಅನುವರ್ತ;
            os << "  " << std::ಎಡ << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : byName) {
            ಧ್ರುವ V3Statistic* repp = itr.second;
            ಯದಿ (!repp->perf()) ಅನುವರ್ತ;
            os << "  " << std::ಎಡ << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    ಶೂನ್ಯ stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        ಪೂರ್ಣಾಙ್ಕ stage = 0;
        size_t maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, ಪೂರ್ಣಾಙ್ಕ> stageInt;
        std::multimap<std::string, ಧ್ರುವ V3Statistic*> byName;
        // * is always first
        ಪ್ರತಿ (auto it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            ಧ್ರುವ V3Statistic* repp = &(*it);
            ಯದಿ (repp->stage() != "*" && repp->printit()) {
                ಯದಿ (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                ಧ್ರುವ auto itFoundPair = stageInt.emplace(repp->stage(), stage);
                ಯದಿ (itFoundPair.second) {
                    ++stage;
                    stages.ತಳ್ಳು_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::ಎಡ << std::setw(maxWidth - 5 - 2) << "";
        ಪ್ರತಿ (ಧ್ರುವ string& i : stages) os << " " << std::ಎಡ << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::ಎಡ << std::setw(maxWidth - 5 - 2) << "";
        ಪ್ರತಿ (auto it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::ಎಡ << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        unsigned col = 0;
        ಪ್ರತಿ (auto it = byName.cbegin(); it != byName.cend(); ++it) {
            ಧ್ರುವ V3Statistic* repp = it->second;
            ಯದಿ (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    ಯದಿ (pos != string::npos) commaName.erase(pos);
                    ಯದಿ (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::ಎಡ << std::setw(maxWidth) << repp->name();
            }
            ಯಾವತ್ (col < stages.size() && stages.at(col) != repp->stage()) {
                os << std::setw(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ addStat(ಧ್ರುವ V3Statistic& stat) { s_allStats.ತಳ್ಳು_back(stat); }

    ಸ್ಥಿರ ದ್ವಿಗುಣ getStatSum(ಧ್ರುವ string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        ಪ್ರತಿ (ಧ್ರುವ V3Statistic& itr : s_allStats) {
            ಧ್ರುವ V3Statistic* ಧ್ರುವ repp = &itr;
            ಯದಿ (repp->name() == name) ಪ್ರತ್ಯಾಗಮ repp->ಮೌಲ್ಯ();
        }
        ಪ್ರತ್ಯಾಗಮ 0.0;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = ಸ್ವಭಾವ;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

ಶೂನ್ಯ V3Statistic::dump(std::ofstream& os) ಧ್ರುವ {
    os << " " << std::ಬಲ << std::fixed << std::setprecision(precision()) << std::setw(9)
       << ಮೌಲ್ಯ();
}

//######################################################################
// Top Stats class

ಶೂನ್ಯ V3Stats::addStat(ಧ್ರುವ V3Statistic& stat) { StatsReport::addStat(stat); }

ದ್ವಿಗುಣ V3Stats::getStatSum(ಧ್ರುವ string& name) { ಪ್ರತ್ಯಾಗಮ StatsReport::getStatSum(name); }

ಶೂನ್ಯ V3Stats::statsStage(ಧ್ರುವ string& name) {
    ಸ್ಥಿರ ದ್ವಿಗುಣ s_lastWallTime = -1;
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_file_ಸಂಖ್ಯೆ = 0;

    ಧ್ರುವ string digitName = V3Global::digitsFilename(++s_file_ಸಂಖ್ಯೆ) + "_" + name;

    ಧ್ರುವ ದ್ವಿಗುಣ wallTime = V3Os::timeUsecs() / 1.0e6;
    ಯದಿ (s_lastWallTime < 0) s_lastWallTime = wallTime;
    ಧ್ರುವ ದ್ವಿಗುಣ wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

ಶೂನ್ಯ V3Stats::infoHeader(std::ofstream& os, ಧ್ರುವ string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

ಶೂನ್ಯ V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ಧ್ರುವ ದ್ವಿಗುಣ memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    ಧ್ರುವ string filename
        = v3Global.opt.hier_ತುದಿ_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    ಯದಿ (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(delete ofp, ofp);
}

ಶೂನ್ಯ V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    ಧ್ರುವ ದ್ವಿಗುಣ sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    ಧ್ರುವ ದ್ವಿಗುಣ cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ಧ್ರುವ uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    ಧ್ರುವ uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    ಧ್ರುವ ದ್ವಿಗುಣ modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ಧ್ರುವ ದ್ವಿಗುಣ walltime = V3Stats::getStatSum(STAT_WALLTIME);
    ಧ್ರುವ ದ್ವಿಗುಣ walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    ಧ್ರುವ ದ್ವಿಗುಣ walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    ಧ್ರುವ ದ್ವಿಗುಣ walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    ಧ್ರುವ ದ್ವಿಗುಣ cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ಧ್ರುವ ದ್ವಿಗುಣ memory = memPeak / 1024.0 / 1024.0;
    ಯದಿ (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
