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
    ಬಳಸಿ StatColl = std::ಸದಿಶ<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ಸ್ಥಿರ StatColl s_allStats;  ///< All statistics

    ಸ್ಥಿರ ಶೂನ್ಯ sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : byName) {
            V3Statistic* repp = itr.ಎರಡನೇ;
            ಆದರೆ (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                lastp = repp;
            }
        }
    }

    ಶೂನ್ಯ stars() {
        // Find all stages
        ಗಾತ್ರ_t max_ಅಗಲ = 0;
        std::multimap<std::string, ಸ್ಥಿರಾಂಕ V3Statistic*> byName;
        // * is always first
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : s_allStats) {
            ಸ್ಥಿರಾಂಕ V3Statistic* repp = &itr;
            ಆದರೆ (repp->stage() == "*" && repp->printit()) {
                ಆದರೆ (max_ಅಗಲ < repp->name().ಉದ್ದಳತೆ()) max_ಅಗಲ = repp->name().ಉದ್ದಳತೆ();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : byName) {
            ಸ್ಥಿರಾಂಕ V3Statistic* repp = itr.ಎರಡನೇ;
            ಆದರೆ (repp->perf()) ಮುಂದುವರಿ;
            os << "  " << std::ಎಡ << std::setw(max_ಅಗಲ) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : byName) {
            ಸ್ಥಿರಾಂಕ V3Statistic* repp = itr.ಎರಡನೇ;
            ಆದರೆ (!repp->perf()) ಮುಂದುವರಿ;
            os << "  " << std::ಎಡ << std::setw(max_ಅಗಲ) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    ಶೂನ್ಯ stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        ಪೂರ್ಣಾಂಕ stage = 0;
        ಗಾತ್ರ_t max_ಅಗಲ = 0;
        std::ಸದಿಶ<std::string> stages;
        std::unordered_map<string, ಪೂರ್ಣಾಂಕ> stageInt;
        std::multimap<std::string, ಸ್ಥಿರಾಂಕ V3Statistic*> byName;
        // * is always first
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = s_allStats.begin(); it != s_allStats.ಕೊನೆ(); ++it) {
            ಸ್ಥಿರಾಂಕ V3Statistic* repp = &(*it);
            ಆದರೆ (repp->stage() != "*" && repp->printit()) {
                ಆದರೆ (max_ಅಗಲ < repp->name().ಉದ್ದಳತೆ()) max_ಅಗಲ = repp->name().ಉದ್ದಳತೆ();
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it_ಸಿಕ್ಕಿತು_Pair = stageInt.emplace(repp->stage(), stage);
                ಆದರೆ (it_ಸಿಕ್ಕಿತು_Pair.ಎರಡನೇ) {
                    ++stage;
                    stages.ತಳ್ಳು_ಹಿಂಭಾಗ(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //ಶೀರ್ಷಿಕೆ
        os << "  Stat     " << std::ಎಡ << std::setw(max_ಅಗಲ - 5 - 2) << "";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : stages) os << " " << std::ಎಡ << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::ಎಡ << std::setw(max_ಅಗಲ - 5 - 2) << "";
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = stages.begin(); it != stages.ಕೊನೆ(); ++it) {
            os << " " << std::ಎಡ << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string ಕೊನೆಯ_Name = "__NONE__";
        string ಕೊನೆಯ_Comma_Name = "__NONE__";
        ಚಿಹ್ನೆರಹಿತ ಲಂಬಸಾಲು = 0;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = byName.cbegin(); it != byName.cend(); ++it) {
            ಸ್ಥಿರಾಂಕ V3Statistic* repp = it->ಎರಡನೇ;
            ಆದರೆ (ಕೊನೆಯ_Name != repp->name()) {
                ಕೊನೆಯ_Name = repp->name();
                {
                    string commaName = ಕೊನೆಯ_Name;
                    string::ಗಾತ್ರ_type pos = commaName.ಕಂಡುಹಿಡಿ(',');
                    ಆದರೆ (pos != string::npos) commaName.erase(pos);
                    ಆದರೆ (ಕೊನೆಯ_Comma_Name != commaName) {
                        ಕೊನೆಯ_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                ಲಂಬಸಾಲು = 0;
                os << "  " << std::ಎಡ << std::setw(max_ಅಗಲ) << repp->name();
            }
            ಆಗ (ಲಂಬಸಾಲು < stages.ಗಾತ್ರ_2() && stages.at(ಲಂಬಸಾಲು) != repp->stage()) {
                os << std::setw(10) << "";
                ಲಂಬಸಾಲು++;
            }
            repp->dump(os);
            ಲಂಬಸಾಲು++;
        }
        os << '\n';
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Stat(ಸ್ಥಿರಾಂಕ V3Statistic& stat) { s_allStats.ತಳ್ಳು_ಹಿಂಭಾಗ(stat); }

    ಸ್ಥಿರ ದ್ವಿಗುಣ get_Stat_ಮೊತ್ತ(ಸ್ಥಿರಾಂಕ string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3Statistic& itr : s_allStats) {
            ಸ್ಥಿರಾಂಕ V3Statistic* ಸ್ಥಿರಾಂಕ repp = &itr;
            ಆದರೆ (repp->name() == name) ಹಿಂತಿರುಗಿ repp->ಮೌಲ್ಯ();
        }
        ಹಿಂತಿರುಗಿ 0.0;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ calculate() { sumit(); }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::info_ಶೀರ್ಷಿಕೆ(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = ಪೂರ್ವನಿಯೋಜಿತ;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

ಶೂನ್ಯ V3Statistic::dump(std::ofstream& os) ಸ್ಥಿರಾಂಕ {
    os << " " << std::ಬಲ << std::fixed << std::setprecision(precision()) << std::setw(9)
       << ಮೌಲ್ಯ();
}

//######################################################################
// Top Stats class

ಶೂನ್ಯ V3Stats::ಕೂಡಿಸು_Stat(ಸ್ಥಿರಾಂಕ V3Statistic& stat) { StatsReport::ಕೂಡಿಸು_Stat(stat); }

ದ್ವಿಗುಣ V3Stats::get_Stat_ಮೊತ್ತ(ಸ್ಥಿರಾಂಕ string& name) { ಹಿಂತಿರುಗಿ StatsReport::get_Stat_ಮೊತ್ತ(name); }

ಶೂನ್ಯ V3Stats::statsStage(ಸ್ಥಿರಾಂಕ string& name) {
    ಸ್ಥಿರ ದ್ವಿಗುಣ s_ಕೊನೆಯ_Wall_ಸಮಯ = -1;
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_file_ಸಂಖ್ಯೆ = 0;

    ಸ್ಥಿರಾಂಕ string digitName = V3Global::digitsFilename(++s_file_ಸಂಖ್ಯೆ) + "_" + name;

    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ wall_ಸಮಯ = V3Os::ಸಮಯ_Usecs() / 1.0e6;
    ಆದರೆ (s_ಕೊನೆಯ_Wall_ಸಮಯ < 0) s_ಕೊನೆಯ_Wall_ಸಮಯ = wall_ಸಮಯ;
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ wall_ಸಮಯ_Delta = wall_ಸಮಯ - s_ಕೊನೆಯ_Wall_ಸಮಯ;
    s_ಕೊನೆಯ_Wall_ಸಮಯ = wall_ಸಮಯ;
    V3Stats::ಕೂಡಿಸು_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wall_ಸಮಯ_Delta);
    V3Stats::ಕೂಡಿಸು_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wall_ಸಮಯ_Delta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_ಬೈಟುಗಳು(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::ಕೂಡಿಸು_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::ಕೂಡಿಸು_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

ಶೂನ್ಯ V3Stats::info_ಶೀರ್ಷಿಕೆ(std::ofstream& os, ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ) {
    os << ಪೂರ್ವಪ್ರತ್ಯಯ << "Information:\n";
    os << ಪೂರ್ವಪ್ರತ್ಯಯ << "  Version: " << V3Options::ಆವೃತ್ತಿ() << '\n';
    os << ಪೂರ್ವಪ್ರತ್ಯಯ << "  Arguments: " << v3Global.opt.all_ವಾದಗಳು_String() << '\n';
    os << ಪೂರ್ವಪ್ರತ್ಯಯ << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << ಪೂರ್ವಪ್ರತ್ಯಯ << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

ಶೂನ್ಯ V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_ಬೈಟುಗಳು(memPeak /*ref*/, memCurrent /*ref*/);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಸ್ಮರಣೆ = memPeak / 1024.0 / 1024.0;
    V3Stats::ಕೂಡಿಸು_Stat("Peak Memory Usage (MB)", ಸ್ಮರಣೆ);

    // Open stats file
    ಸ್ಥಿರಾಂಕ string filename
        = v3Global.opt.hier_ತುದಿ_ದತ್ತಾಂಶ_Dir() + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    ಆದರೆ (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(ಅಳಿಸು ofp, ofp);
}

ಶೂನ್ಯ V3Stats::summaryReport() {
    StatsReport::calculate();
    std::ಹೊರಗೆ << "- V e r i l a t i o n   R e p o r t: " << V3Options::ಆವೃತ್ತಿ() << "\n";

    std::ಹೊರಗೆ << std::setprecision(3) << std::fixed;
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಮೂಲ_Chars_MB = V3Stats::get_Stat_ಮೊತ್ತ_Q(STAT_ಮೂಲ_CHARS) / 1024.0 / 1024.0;
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ cppCharsMB = V3Stats::get_Stat_ಮೊತ್ತ_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ಸ್ಥಿರಾಂಕ uint64_t ಮೂಲ_Modules = V3Stats::get_Stat_ಮೊತ್ತ_Q(STAT_ಮೂಲ_MODULES);
    ಸ್ಥಿರಾಂಕ uint64_t cppFiles = V3Stats::get_Stat_ಮೊತ್ತ_Q(STAT_CPP_FILES);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ modelMB = V3Stats::get_Stat_ಮೊತ್ತ(STAT_MODEL_ಗಾತ್ರ) / 1024.0 / 1024.0;
    std::ಹೊರಗೆ << "- Verilator: Built from " << ಮೂಲ_Chars_MB << " MB sources in " << ಮೂಲ_Modules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ walltime = V3Stats::get_Stat_ಮೊತ್ತ(STAT_WALLTIME);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ walltimeElab = V3Stats::get_Stat_ಮೊತ್ತ(STAT_WALLTIME_ELAB);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ walltimeCvt = V3Stats::get_Stat_ಮೊತ್ತ(STAT_WALLTIME_CVT);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ walltimeBuild = V3Stats::get_Stat_ಮೊತ್ತ(STAT_WALLTIME_BUILD);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ cputime = V3Stats::get_Stat_ಮೊತ್ತ(STAT_CPUTIME);
    std::ಹೊರಗೆ << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_ಬೈಟುಗಳು(memPeak /*ref*/, memCurrent /*ref*/);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಸ್ಮರಣೆ = memPeak / 1024.0 / 1024.0;
    ಆದರೆ (VL_UNCOVERABLE(ಸ್ಮರಣೆ != 0.0)) std::ಹೊರಗೆ << "; allocated " << ಸ್ಮರಣೆ << " MB";
    std::ಹೊರಗೆ << "\n";
}
