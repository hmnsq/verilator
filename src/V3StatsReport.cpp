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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

ଶ୍ରେଣୀ StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ସ୍ଥିର StatColl s_allStats;  ///< All statistics

    ସ୍ଥିର ଶୂନ୍ୟ sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        ପାଇଁ (auto& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        ପାଇଁ (ଅଚଳ auto& itr : byName) {
            V3Statistic* repp = itr.second;
            ଯଦି (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } ଅନ୍ୟଥା {
                lastp = repp;
            }
        }
    }

    ଶୂନ୍ୟ stars() {
        // Find all stages
        size_t maxWidth = 0;
        std::multimap<std::string, ଅଚଳ V3Statistic*> byName;
        // * is always first
        ପାଇଁ (ଅଚଳ auto& itr : s_allStats) {
            ଅଚଳ V3Statistic* repp = &itr;
            ଯଦି (repp->stage() == "*" && repp->printit()) {
                ଯଦି (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        ପାଇଁ (ଅଚଳ auto& itr : byName) {
            ଅଚଳ V3Statistic* repp = itr.second;
            ଯଦି (repp->perf()) ଜାରି;
            os << "  " << std::ବାମ << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        ପାଇଁ (ଅଚଳ auto& itr : byName) {
            ଅଚଳ V3Statistic* repp = itr.second;
            ଯଦି (!repp->perf()) ଜାରି;
            os << "  " << std::ବାମ << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    ଶୂନ୍ୟ stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା stage = 0;
        size_t maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> stageInt;
        std::multimap<std::string, ଅଚଳ V3Statistic*> byName;
        // * is always first
        ପାଇଁ (auto it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            ଅଚଳ V3Statistic* repp = &(*it);
            ଯଦି (repp->stage() != "*" && repp->printit()) {
                ଯଦି (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                ଅଚଳ auto itFoundPair = stageInt.emplace(repp->stage(), stage);
                ଯଦି (itFoundPair.second) {
                    ++stage;
                    stages.ଠେଲ_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::ବାମ << std::setw(maxWidth - 5 - 2) << "";
        ପାଇଁ (ଅଚଳ string& i : stages) os << " " << std::ବାମ << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::ବାମ << std::setw(maxWidth - 5 - 2) << "";
        ପାଇଁ (auto it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::ବାମ << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        unsigned col = 0;
        ପାଇଁ (auto it = byName.cbegin(); it != byName.cend(); ++it) {
            ଅଚଳ V3Statistic* repp = it->second;
            ଯଦି (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    ଯଦି (pos != string::npos) commaName.erase(pos);
                    ଯଦି (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::ବାମ << std::setw(maxWidth) << repp->name();
            }
            ଯେତେବେଳେ (col < stages.size() && stages.at(col) != repp->stage()) {
                os << std::setw(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

ସର୍ବସାଧାରଣ:
    // METHODS
    ସ୍ଥିର ଶୂନ୍ୟ addStat(ଅଚଳ V3Statistic& stat) { s_allStats.ଠେଲ_back(stat); }

    ସ୍ଥିର ଦ୍ୱିଗୁଣ getStatSum(ଅଚଳ string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        ପାଇଁ (ଅଚଳ V3Statistic& itr : s_allStats) {
            ଅଚଳ V3Statistic* ଅଚଳ repp = &itr;
            ଯଦି (repp->name() == name) ଫେରନ୍ତୁ repp->ମୂଲ୍ୟ();
        }
        ଫେରନ୍ତୁ 0.0;
    }

    ସ୍ଥିର ଶୂନ୍ୟ calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

ଶୂନ୍ୟ V3Statistic::dump(std::ofstream& os) ଅଚଳ {
    os << " " << std::ଡାହାଣ << std::fixed << std::setprecision(precision()) << std::setw(9)
       << ମୂଲ୍ୟ();
}

//######################################################################
// Top Stats class

ଶୂନ୍ୟ V3Stats::addStat(ଅଚଳ V3Statistic& stat) { StatsReport::addStat(stat); }

ଦ୍ୱିଗୁଣ V3Stats::getStatSum(ଅଚଳ string& name) { ଫେରନ୍ତୁ StatsReport::getStatSum(name); }

ଶୂନ୍ୟ V3Stats::statsStage(ଅଚଳ string& name) {
    ସ୍ଥିର ଦ୍ୱିଗୁଣ s_lastWallTime = -1;
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_file_ସଂଖ୍ୟା = 0;

    ଅଚଳ string digitName = V3Global::digitsFilename(++s_file_ସଂଖ୍ୟା) + "_" + name;

    ଅଚଳ ଦ୍ୱିଗୁଣ wallTime = V3Os::timeUsecs() / 1.0e6;
    ଯଦି (s_lastWallTime < 0) s_lastWallTime = wallTime;
    ଅଚଳ ଦ୍ୱିଗୁଣ wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

ଶୂନ୍ୟ V3Stats::infoHeader(std::ofstream& os, ଅଚଳ string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

ଶୂନ୍ୟ V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ଅଚଳ ଦ୍ୱିଗୁଣ memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    ଅଚଳ string filename
        = v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    ଯଦି (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(delete ofp, ofp);
}

ଶୂନ୍ୟ V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    ଅଚଳ ଦ୍ୱିଗୁଣ sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    ଅଚଳ ଦ୍ୱିଗୁଣ cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ଅଚଳ uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    ଅଚଳ uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    ଅଚଳ ଦ୍ୱିଗୁଣ modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ଅଚଳ ଦ୍ୱିଗୁଣ walltime = V3Stats::getStatSum(STAT_WALLTIME);
    ଅଚଳ ଦ୍ୱିଗୁଣ walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    ଅଚଳ ଦ୍ୱିଗୁଣ walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    ଅଚଳ ଦ୍ୱିଗୁଣ walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    ଅଚଳ ଦ୍ୱିଗୁଣ cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ଅଚଳ ଦ୍ୱିଗୁଣ memory = memPeak / 1024.0 / 1024.0;
    ଯଦି (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
