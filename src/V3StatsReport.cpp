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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

வகுப்பு StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    நிலையான StatColl s_allStats;  ///< All statistics

    நிலையான வெறுமை sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        க்கு (auto& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        க்கு (மாறிலி auto& itr : byName) {
            V3Statistic* repp = itr.second;
            எனில் (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } இல்லையெனில் {
                lastp = repp;
            }
        }
    }

    வெறுமை stars() {
        // Find all stages
        size_t maxWidth = 0;
        std::multimap<std::string, மாறிலி V3Statistic*> byName;
        // * is always first
        க்கு (மாறிலி auto& itr : s_allStats) {
            மாறிலி V3Statistic* repp = &itr;
            எனில் (repp->stage() == "*" && repp->printit()) {
                எனில் (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        க்கு (மாறிலி auto& itr : byName) {
            மாறிலி V3Statistic* repp = itr.second;
            எனில் (repp->perf()) தொடர்;
            os << "  " << std::இடது << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        க்கு (மாறிலி auto& itr : byName) {
            மாறிலி V3Statistic* repp = itr.second;
            எனில் (!repp->perf()) தொடர்;
            os << "  " << std::இடது << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    வெறுமை stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        முழு stage = 0;
        size_t maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, முழு> stageInt;
        std::multimap<std::string, மாறிலி V3Statistic*> byName;
        // * is always first
        க்கு (auto it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            மாறிலி V3Statistic* repp = &(*it);
            எனில் (repp->stage() != "*" && repp->printit()) {
                எனில் (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                மாறிலி auto itFoundPair = stageInt.emplace(repp->stage(), stage);
                எனில் (itFoundPair.second) {
                    ++stage;
                    stages.தள்ளு_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::இடது << std::setw(maxWidth - 5 - 2) << "";
        க்கு (மாறிலி string& i : stages) os << " " << std::இடது << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::இடது << std::setw(maxWidth - 5 - 2) << "";
        க்கு (auto it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::இடது << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        unsigned col = 0;
        க்கு (auto it = byName.cbegin(); it != byName.cend(); ++it) {
            மாறிலி V3Statistic* repp = it->second;
            எனில் (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    எனில் (pos != string::npos) commaName.erase(pos);
                    எனில் (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::இடது << std::setw(maxWidth) << repp->name();
            }
            போது (col < stages.size() && stages.at(col) != repp->stage()) {
                os << std::setw(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

பொது:
    // METHODS
    நிலையான வெறுமை addStat(மாறிலி V3Statistic& stat) { s_allStats.தள்ளு_back(stat); }

    நிலையான இரட்டை getStatSum(மாறிலி string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        க்கு (மாறிலி V3Statistic& itr : s_allStats) {
            மாறிலி V3Statistic* மாறிலி repp = &itr;
            எனில் (repp->name() == name) திரும்பு repp->மதிப்பு();
        }
        திரும்பு 0.0;
    }

    நிலையான வெறுமை calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = இயல்பு;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

வெறுமை V3Statistic::dump(std::ofstream& os) மாறிலி {
    os << " " << std::வலது << std::fixed << std::setprecision(precision()) << std::setw(9)
       << மதிப்பு();
}

//######################################################################
// Top Stats class

வெறுமை V3Stats::addStat(மாறிலி V3Statistic& stat) { StatsReport::addStat(stat); }

இரட்டை V3Stats::getStatSum(மாறிலி string& name) { திரும்பு StatsReport::getStatSum(name); }

வெறுமை V3Stats::statsStage(மாறிலி string& name) {
    நிலையான இரட்டை s_lastWallTime = -1;
    நிலையான முழு s_file_எண் = 0;

    மாறிலி string digitName = V3Global::digitsFilename(++s_file_எண்) + "_" + name;

    மாறிலி இரட்டை wallTime = V3Os::timeUsecs() / 1.0e6;
    எனில் (s_lastWallTime < 0) s_lastWallTime = wallTime;
    மாறிலி இரட்டை wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

வெறுமை V3Stats::infoHeader(std::ofstream& os, மாறிலி string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

வெறுமை V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    மாறிலி இரட்டை memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    மாறிலி string filename
        = v3Global.opt.hier_உச்சி_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    எனில் (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(delete ofp, ofp);
}

வெறுமை V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    மாறிலி இரட்டை sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    மாறிலி இரட்டை cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    மாறிலி uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    மாறிலி uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    மாறிலி இரட்டை modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    மாறிலி இரட்டை walltime = V3Stats::getStatSum(STAT_WALLTIME);
    மாறிலி இரட்டை walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    மாறிலி இரட்டை walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    மாறிலி இரட்டை walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    மாறிலி இரட்டை cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    மாறிலி இரட்டை memory = memPeak / 1024.0 / 1024.0;
    எனில் (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
