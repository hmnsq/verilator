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
        साठी (स्वतः& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = nullptr;
        साठी (स्थिरांक स्वतः& itr : byName) {
            V3Statistic* repp = itr.second;
            जर (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } अन्यथा {
                lastp = repp;
            }
        }
    }

    शून्य stars() {
        // Find all stages
        माप_प्रकार maxWidth = 0;
        std::multimap<std::string, स्थिरांक V3Statistic*> byName;
        // * is always first
        साठी (स्थिरांक स्वतः& itr : s_allStats) {
            स्थिरांक V3Statistic* repp = &itr;
            जर (repp->stage() == "*" && repp->printit()) {
                जर (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        साठी (स्थिरांक स्वतः& itr : byName) {
            स्थिरांक V3Statistic* repp = itr.second;
            जर (repp->perf()) सुरूठेवा;
            os << "  " << std::डावा << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        साठी (स्थिरांक स्वतः& itr : byName) {
            स्थिरांक V3Statistic* repp = itr.second;
            जर (!repp->perf()) सुरूठेवा;
            os << "  " << std::डावा << std::setw(maxWidth) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    शून्य stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        पूर्णांक stage = 0;
        माप_प्रकार maxWidth = 0;
        std::vector<std::string> stages;
        std::unordered_map<string, पूर्णांक> stageInt;
        std::multimap<std::string, स्थिरांक V3Statistic*> byName;
        // * is always first
        साठी (स्वतः it = s_allStats.begin(); it != s_allStats.end(); ++it) {
            स्थिरांक V3Statistic* repp = &(*it);
            जर (repp->stage() != "*" && repp->printit()) {
                जर (maxWidth < repp->name().length()) maxWidth = repp->name().length();
                स्थिरांक स्वतः itFoundPair = stageInt.emplace(repp->stage(), stage);
                जर (itFoundPair.second) {
                    ++stage;
                    stages.ढकला_back(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        // Header
        os << "  Stat     " << std::डावा << std::setw(maxWidth - 5 - 2) << "";
        साठी (स्थिरांक string& i : stages) os << " " << std::डावा << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::डावा << std::setw(maxWidth - 5 - 2) << "";
        साठी (स्वतः it = stages.begin(); it != stages.end(); ++it) {
            os << " " << std::डावा << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string lastName = "__NONE__";
        string lastCommaName = "__NONE__";
        अचिन्हित col = 0;
        साठी (स्वतः it = byName.cbegin(); it != byName.cend(); ++it) {
            स्थिरांक V3Statistic* repp = it->second;
            जर (lastName != repp->name()) {
                lastName = repp->name();
                {
                    string commaName = lastName;
                    string::size_type pos = commaName.find(',');
                    जर (pos != string::npos) commaName.erase(pos);
                    जर (lastCommaName != commaName) {
                        lastCommaName = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::डावा << std::setw(maxWidth) << repp->name();
            }
            पर्यंत (col < stages.size() && stages.at(col) != repp->stage()) {
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
    स्थिर शून्य addStat(स्थिरांक V3Statistic& stat) { s_allStats.ढकला_back(stat); }

    स्थिर द्विगुण getStatSum(स्थिरांक string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        साठी (स्थिरांक V3Statistic& itr : s_allStats) {
            स्थिरांक V3Statistic* स्थिरांक repp = &itr;
            जर (repp->name() == name) परत repp->मूल्य();
        }
        परत 0.0;
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
    ~StatsReport() = पूर्वनिर्धारित;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

शून्य V3Statistic::dump(std::ofstream& os) स्थिरांक {
    os << " " << std::उजवा << std::fixed << std::setprecision(precision()) << std::setw(9)
       << मूल्य();
}

//######################################################################
// Top Stats class

शून्य V3Stats::addStat(स्थिरांक V3Statistic& stat) { StatsReport::addStat(stat); }

द्विगुण V3Stats::getStatSum(स्थिरांक string& name) { परत StatsReport::getStatSum(name); }

शून्य V3Stats::statsStage(स्थिरांक string& name) {
    स्थिर द्विगुण s_lastWallTime = -1;
    स्थिर पूर्णांक s_file_संख्या = 0;

    स्थिरांक string digitName = V3Global::digitsFilename(++s_file_संख्या) + "_" + name;

    स्थिरांक द्विगुण wallTime = V3Os::timeUsecs() / 1.0e6;
    जर (s_lastWallTime < 0) s_lastWallTime = wallTime;
    स्थिरांक द्विगुण wallTimeDelta = wallTime - s_lastWallTime;
    s_lastWallTime = wallTime;
    V3Stats::addStatPerf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::addStatPerf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::addStatPerf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::addStatPerf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

शून्य V3Stats::infoHeader(std::ofstream& os, स्थिरांक string& prefix) {
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
    स्थिरांक द्विगुण memory = memPeak / 1024.0 / 1024.0;
    V3Stats::addStat("Peak Memory Usage (MB)", memory);

    // Open stats file
    स्थिरांक string filename
        = v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    जर (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->बंद();
    VL_DO_DANGLING(delete ofp, ofp);
}

शून्य V3Stats::summaryReport() {
    StatsReport::calculate();
    std::cout << "- V e r i l a t i o n   R e p o r t: " << V3Options::version() << "\n";

    std::cout << std::setprecision(3) << std::fixed;
    स्थिरांक द्विगुण sourceCharsMB = V3Stats::getStatSumQ(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    स्थिरांक द्विगुण cppCharsMB = V3Stats::getStatSumQ(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    स्थिरांक uint64_t sourceModules = V3Stats::getStatSumQ(STAT_SOURCE_MODULES);
    स्थिरांक uint64_t cppFiles = V3Stats::getStatSumQ(STAT_CPP_FILES);
    स्थिरांक द्विगुण modelMB = V3Stats::getStatSum(STAT_MODEL_SIZE) / 1024.0 / 1024.0;
    std::cout << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    स्थिरांक द्विगुण walltime = V3Stats::getStatSum(STAT_WALLTIME);
    स्थिरांक द्विगुण walltimeElab = V3Stats::getStatSum(STAT_WALLTIME_ELAB);
    स्थिरांक द्विगुण walltimeCvt = V3Stats::getStatSum(STAT_WALLTIME_CVT);
    स्थिरांक द्विगुण walltimeBuild = V3Stats::getStatSum(STAT_WALLTIME_BUILD);
    स्थिरांक द्विगुण cputime = V3Stats::getStatSum(STAT_CPUTIME);
    std::cout << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::memUsageBytes(memPeak /*ref*/, memCurrent /*ref*/);
    स्थिरांक द्विगुण memory = memPeak / 1024.0 / 1024.0;
    जर (VL_UNCOVERABLE(memory != 0.0)) std::cout << "; allocated " << memory << " MB";
    std::cout << "\n";
}
