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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <iomanip>
#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

वर्ग StatsReport final {
    // TYPES
    using StatColl = std::vector<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    स्थिर StatColl s_allStats;  ///< All statistics

    स्थिर शून्य sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        प्रति (auto& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        प्रति (ध्रुव auto& itr : byName) {
            V3Statistic* repp = itr.second;
            यदि (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } अन्यथा {
                lastp = repp;
            }
        }
    }

    शून्य stars() {
        // Find all stages
        size_t maxWidth = 0;
        std::multimap<std::string, ध्रुव V3Statistic*> byName;
        // * is always first
        प्रति (ध्रुव auto& itr : s_allStats) {
            ध्रुव V3Statistic* repp = &itr;
            यदि (repp->stage() == "*" && repp->printit()) {
                यदि (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        प्रति (ध्रुव auto& itr : byName) {
            ध्रुव V3Statistic* repp = itr.second;
            यदि (repp->perf()) अनुवर्त;
            os << "  " << std::डावा << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        प्रति (ध्रुव auto& itr : byName) {
            ध्रुव V3Statistic* repp = itr.second;
            यदि (!repp->perf()) अनुवर्त;
            os << "  " << std::डावा << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    शून्य stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        पूर्णाङ्क stage = 0;
        size_t maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, पूर्णाङ्क> stageInt;
        std::multimap<std::string, ध्रुव V3Statistic*> byName;
        // * is always first
        प्रति (auto it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            ध्रुव V3Statistic* repp = &(*it);
            यदि (repp->stage() != "*" && repp->printit()) {
                यदि (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                ध्रुव auto itFoundPair = stageInt.emplace(repp->stage(), stage);
                यदि (itFoundPair.second) {
                    ++stage;
                    stages.ढकला_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::डावा << std::setw(maxWidth - 5 - 2) << "";
        प्रति (ध्रुव string& i : stages) os << " " << std::डावा << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::डावा << std::setw(maxWidth - 5 - 2) << "";
        प्रति (auto it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::डावा << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        unsigned col = 0;
        प्रति (auto it = byName.cbegin(); it != byName.cend(); ++it) {
            ध्रुव V3Statistic* repp = it->second;
            यदि (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    यदि (pos != string::npos) commaName.erase(pos);
                    यदि (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::डावा << std::setw(maxWidth) << repp->name();
            }
            यावत् (col < stages.size() && stages.at(col) != repp->stage()) {
                os << std::setw(10) << "";
                col++;
            }
            repp->dump(os);
            col++;
        }
        os << '\n';
    }

सार्वजनिक:
    // METHODS
    स्थिर शून्य addStat(ध्रुव V3Statistic& stat) { s_allStats.ढकला_back(stat); }

    स्थिर द्विगुण getStatSum(ध्रुव string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        प्रति (ध्रुव V3Statistic& itr : s_allStats) {
            ध्रुव V3Statistic* ध्रुव repp = &itr;
            यदि (repp->name() == name) प्रत्यागम repp->मूल्य();
        }
        प्रत्यागम 0.0;
    }

    स्थिर शून्य calculate() { sumit(); }

    // CONSTRUCTORS
    explicit StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::infoHeader(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = स्वभाव;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

शून्य V3Statistic::dump(std::ofstream& os) ध्रुव {
    os << " " << std::उजवा << std::fixed << std::setprecision(precision()) << std::setw(9)
       << मूल्य();
}

//######################################################################
// Top Stats class

शून्य V3Stats::addStat(ध्रुव V3Statistic& stat) { StatsReport::addStat(stat); }

द्विगुण V3Stats::getStatSum(ध्रुव string& name) { प्रत्यागम StatsReport::getStatSum(name); }

शून्य V3Stats::statsStage(ध्रुव string& name) {
    स्थिर द्विगुण s_lastWallTime = -1;
    स्थिर पूर्णाङ्क s_file_संख्या = 0;

    ध्रुव string digitName = V3Global::digitsFilename(++s_file_संख्या) + "_" + name;

    ध्रुव द्विगुण wallTime = V3Os::timeUsecs() / 1.0e6;
    यदि (s_lastWallTime < 0) s_lastWallTime = wallTime;
    ध्रुव द्विगुण wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

शून्य V3Stats::infoHeader(std::ofstream& os, ध्रुव string& prefix) {
    os << prefix << "Information:\n";
    os << prefix << "  Version: " << V3Options::version() << '\n';
    os << prefix << "  Arguments: " << v3Global.opt.allArgsString() << '\n';
    os << prefix << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << prefix << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

शून्य V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ध्रुव द्विगुण memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    ध्रुव string filename
        = v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    यदि (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(delete ofp, ofp);
}

शून्य V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    ध्रुव द्विगुण sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    ध्रुव द्विगुण cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ध्रुव uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    ध्रुव uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    ध्रुव द्विगुण modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ध्रुव द्विगुण walltime = V3Stats::getStatSum(STAT_WALLTIME);
    ध्रुव द्विगुण walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    ध्रुव द्विगुण walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    ध्रुव द्विगुण walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    ध्रुव द्विगुण cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    ध्रुव द्विगुण memory = memPeak / 1024.0 / 1024.0;
    यदि (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
