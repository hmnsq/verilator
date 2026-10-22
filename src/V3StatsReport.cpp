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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <iomanip>
#অন্তর্ভাৱ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

শ্ৰেণী StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    স্থির StatColl s_allStats;  ///< All statistics

    স্থির শূন্য sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        বাবে (auto& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        বাবে (ধ্রুৱ auto& itr : byName) {
            V3Statistic* repp = itr.second;
            যদি (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } অন্যথা {
                lastp = repp;
            }
        }
    }

    শূন্য stars() {
        // Find all stages
        size_t maxWidth = 0;
        std::multimap<std::string, ধ্রুৱ V3Statistic*> byName;
        // * is always first
        বাবে (ধ্রুৱ auto& itr : s_allStats) {
            ধ্রুৱ V3Statistic* repp = &itr;
            যদি (repp->stage() == "*" && repp->printit()) {
                যদি (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        বাবে (ধ্রুৱ auto& itr : byName) {
            ধ্রুৱ V3Statistic* repp = itr.second;
            যদি (repp->perf()) অনুৱর্ত;
            os << "  " << std::বাম << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        বাবে (ধ্রুৱ auto& itr : byName) {
            ধ্রুৱ V3Statistic* repp = itr.second;
            যদি (!repp->perf()) অনুৱর্ত;
            os << "  " << std::বাম << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    শূন্য stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        পূর্ণাঙ্ক stage = 0;
        size_t maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, পূর্ণাঙ্ক> stageInt;
        std::multimap<std::string, ধ্রুৱ V3Statistic*> byName;
        // * is always first
        বাবে (auto it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            ধ্রুৱ V3Statistic* repp = &(*it);
            যদি (repp->stage() != "*" && repp->printit()) {
                যদি (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                ধ্রুৱ auto itFoundPair = stageInt.emplace(repp->stage(), stage);
                যদি (itFoundPair.second) {
                    ++stage;
                    stages.ঠেলো_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::বাম << std::setw(maxWidth - 5 - 2) << "";
        বাবে (ধ্রুৱ string& i : stages) os << " " << std::বাম << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::বাম << std::setw(maxWidth - 5 - 2) << "";
        বাবে (auto it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::বাম << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        unsigned col = 0;
        বাবে (auto it = byName.cbegin(); it != byName.cend(); ++it) {
            ধ্রুৱ V3Statistic* repp = it->second;
            যদি (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    যদি (pos != string::npos) commaName.erase(pos);
                    যদি (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::বাম << std::setw(maxWidth) << repp->name();
            }
            যেতিয়ালৈ (col < stages.size() && stages.at(col) != repp->stage()) {
                os << std::setw(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

সার্ৱজনিক:
    // METHODS
    স্থির শূন্য addStat(ধ্রুৱ V3Statistic& stat) { s_allStats.ঠেলো_back(stat); }

    স্থির দ্ৱিগুণ getStatSum(ধ্রুৱ string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        বাবে (ধ্রুৱ V3Statistic& itr : s_allStats) {
            ধ্রুৱ V3Statistic* ধ্রুৱ repp = &itr;
            যদি (repp->name() == name) ঘূৰাই repp->মান();
        }
        ঘূৰাই 0.0;
    }

    স্থির শূন্য calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = স্ৱভাৱ;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

শূন্য V3Statistic::dump(std::ofstream& os) ধ্রুৱ {
    os << " " << std::ডান << std::fixed << std::setprecision(precision()) << std::setw(9)
       << মান();
}

//######################################################################
// Top Stats class

শূন্য V3Stats::addStat(ধ্রুৱ V3Statistic& stat) { StatsReport::addStat(stat); }

দ্ৱিগুণ V3Stats::getStatSum(ধ্রুৱ string& name) { ঘূৰাই StatsReport::getStatSum(name); }

শূন্য V3Stats::statsStage(ধ্রুৱ string& name) {
    স্থির দ্ৱিগুণ s_lastWallTime = -1;
    স্থির পূর্ণাঙ্ক s_file_সংখ্যা = 0;

    ধ্রুৱ string digitName = V3Global::digitsFilename(++s_file_সংখ্যা) + "_" + name;

    ধ্রুৱ দ্ৱিগুণ wallTime = V3Os::timeUsecs() / 1.0e6;
    যদি (s_lastWallTime < 0) s_lastWallTime = wallTime;
    ধ্রুৱ দ্ৱিগুণ wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

শূন্য V3Stats::infoHeader(std::ofstream& os, ধ্রুৱ string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

শূন্য V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ধ্রুৱ দ্ৱিগুণ memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    ধ্রুৱ string filename
        = v3Global.opt.hier_শীৰ্ষ_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    যদি (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(delete ofp, ofp);
}

শূন্য V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    ধ্রুৱ দ্ৱিগুণ sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    ধ্রুৱ দ্ৱিগুণ cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ধ্রুৱ uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    ধ্রুৱ uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    ধ্রুৱ দ্ৱিগুণ modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ধ্রুৱ দ্ৱিগুণ walltime = V3Stats::getStatSum(STAT_WALLTIME);
    ধ্রুৱ দ্ৱিগুণ walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    ধ্রুৱ দ্ৱিগুণ walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    ধ্রুৱ দ্ৱিগুণ walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    ধ্রুৱ দ্ৱিগুণ cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ধ্রুৱ দ্ৱিগুণ memory = memPeak / 1024.0 / 1024.0;
    যদি (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
