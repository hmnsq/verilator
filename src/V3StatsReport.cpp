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

#شامل "V3PchAstMT.h"

#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3Os.h"
#شامل "V3Stats.h"

#شامل <iomanip>
#شامل <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

رده StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ایستا StatColl s_allStats;  ///< All statistics

    ایستا تهی sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        برای (auto& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        برای (ثابت auto& itr : byName) {
            V3Statistic* repp = itr.second;
            اگر (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } وگرنه {
                lastp = repp;
            }
        }
    }

    تهی stars() {
        // Find all stages
        size_t maxWidth = 0;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        برای (ثابت auto& itr : s_allStats) {
            ثابت V3Statistic* repp = &itr;
            اگر (repp->stage() == "*" && repp->printit()) {
                اگر (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        برای (ثابت auto& itr : byName) {
            ثابت V3Statistic* repp = itr.second;
            اگر (repp->perf()) ادامه;
            os << "  " << std::چپ << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        برای (ثابت auto& itr : byName) {
            ثابت V3Statistic* repp = itr.second;
            اگر (!repp->perf()) ادامه;
            os << "  " << std::چپ << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    تهی stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        صحیح stage = 0;
        size_t maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, صحیح> stageInt;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        برای (auto it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            ثابت V3Statistic* repp = &(*it);
            اگر (repp->stage() != "*" && repp->printit()) {
                اگر (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                ثابت auto itFoundPair = stageInt.emplace(repp->stage(), stage);
                اگر (itFoundPair.second) {
                    ++stage;
                    stages.افزودن_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::چپ << std::setw(maxWidth - 5 - 2) << "";
        برای (ثابت string& i : stages) os << " " << std::چپ << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::چپ << std::setw(maxWidth - 5 - 2) << "";
        برای (auto it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::چپ << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        unsigned col = 0;
        برای (auto it = byName.cbegin(); it != byName.cend(); ++it) {
            ثابت V3Statistic* repp = it->second;
            اگر (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    اگر (pos != string::npos) commaName.erase(pos);
                    اگر (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::چپ << std::setw(maxWidth) << repp->name();
            }
            وقتی (col < stages.size() && stages.at(col) != repp->stage()) {
                os << std::setw(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

عمومی:
    // METHODS
    ایستا تهی addStat(ثابت V3Statistic& stat) { s_allStats.افزودن_back(stat); }

    ایستا دوگانه getStatSum(ثابت string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        برای (ثابت V3Statistic& itr : s_allStats) {
            ثابت V3Statistic* ثابت repp = &itr;
            اگر (repp->name() == name) بازگشت repp->مقدار();
        }
        بازگشت 0.0;
    }

    ایستا تهی calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = پیشفرض;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

تهی V3Statistic::dump(std::ofstream& os) ثابت {
    os << " " << std::راست << std::fixed << std::setprecision(precision()) << std::setw(9)
       << مقدار();
}

//######################################################################
// Top Stats class

تهی V3Stats::addStat(ثابت V3Statistic& stat) { StatsReport::addStat(stat); }

دوگانه V3Stats::getStatSum(ثابت string& name) { بازگشت StatsReport::getStatSum(name); }

تهی V3Stats::statsStage(ثابت string& name) {
    ایستا دوگانه s_lastWallTime = -1;
    ایستا صحیح s_file_عدد = 0;

    ثابت string digitName = V3Global::digitsFilename(++s_file_عدد) + "_" + name;

    ثابت دوگانه wallTime = V3Os::timeUsecs() / 1.0e6;
    اگر (s_lastWallTime < 0) s_lastWallTime = wallTime;
    ثابت دوگانه wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

تهی V3Stats::infoHeader(std::ofstream& os, ثابت string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

تهی V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت دوگانه memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    ثابت string filename
        = v3Global.opt.hier_بالاترین_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    اگر (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(delete ofp, ofp);
}

تهی V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    ثابت دوگانه sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    ثابت دوگانه cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ثابت uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    ثابت uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    ثابت دوگانه modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ثابت دوگانه walltime = V3Stats::getStatSum(STAT_WALLTIME);
    ثابت دوگانه walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    ثابت دوگانه walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    ثابت دوگانه walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    ثابت دوگانه cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت دوگانه memory = memPeak / 1024.0 / 1024.0;
    اگر (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
