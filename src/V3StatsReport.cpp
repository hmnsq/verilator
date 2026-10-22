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

#ضمن "V3PchAstMT.h"

#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3Os.h"
#ضمن "V3Stats.h"

#ضمن <iomanip>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

صنف StatsReport final {
    // TYPES
    کارول StatColl = std::ویکټور<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ساكن StatColl s_allStats;  ///< All statistics

    ساكن فراغ sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        لكل (خپلکاری& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = تش_اشاره;
        لكل (ثابت خپلکاری& itr : byName) {
            V3Statistic* repp = itr.دوهم;
            إذا (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } وإلا {
                lastp = repp;
            }
        }
    }

    فراغ stars() {
        // Find all stages
        اندازه_t max_پلنوالی = 0;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        لكل (ثابت خپلکاری& itr : s_allStats) {
            ثابت V3Statistic* repp = &itr;
            إذا (repp->stage() == "*" && repp->printit()) {
                إذا (max_پلنوالی < repp->name().اوږدتیا()) max_پلنوالی = repp->name().اوږدتیا();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        لكل (ثابت خپلکاری& itr : byName) {
            ثابت V3Statistic* repp = itr.دوهم;
            إذا (repp->perf()) استمر;
            os << "  " << std::کیڼ << std::setw(max_پلنوالی) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        لكل (ثابت خپلکاری& itr : byName) {
            ثابت V3Statistic* repp = itr.دوهم;
            إذا (!repp->perf()) استمر;
            os << "  " << std::کیڼ << std::setw(max_پلنوالی) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    فراغ stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        صحيح stage = 0;
        اندازه_t max_پلنوالی = 0;
        std::ویکټور<std::string> stages;
        std::unordered_map<string, صحيح> stageInt;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        لكل (خپلکاری it = s_allStats.begin(); it != s_allStats.پای_ټکی(); ++it) {
            ثابت V3Statistic* repp = &(*it);
            إذا (repp->stage() != "*" && repp->printit()) {
                إذا (max_پلنوالی < repp->name().اوږدتیا()) max_پلنوالی = repp->name().اوږدتیا();
                ثابت خپلکاری it_وموندل_Pair = stageInt.emplace(repp->stage(), stage);
                إذا (it_وموندل_Pair.دوهم) {
                    ++stage;
                    stages.ټېل_شا(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //سرلیک
        os << "  Stat     " << std::کیڼ << std::setw(max_پلنوالی - 5 - 2) << "";
        لكل (ثابت string& i : stages) os << " " << std::کیڼ << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::کیڼ << std::setw(max_پلنوالی - 5 - 2) << "";
        لكل (خپلکاری it = stages.begin(); it != stages.پای_ټکی(); ++it) {
            os << " " << std::کیڼ << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string وروستی_Name = "__NONE__";
        string وروستی_Comma_Name = "__NONE__";
        بینښه ستون = 0;
        لكل (خپلکاری it = byName.cbegin(); it != byName.cend(); ++it) {
            ثابت V3Statistic* repp = it->دوهم;
            إذا (وروستی_Name != repp->name()) {
                وروستی_Name = repp->name();
                {
                    string commaName = وروستی_Name;
                    string::اندازه_type pos = commaName.پیدا_کړه(',');
                    إذا (pos != string::npos) commaName.erase(pos);
                    إذا (وروستی_Comma_Name != commaName) {
                        وروستی_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                ستون = 0;
                os << "  " << std::کیڼ << std::setw(max_پلنوالی) << repp->name();
            }
            طالما (ستون < stages.اندازه() && stages.at(ستون) != repp->stage()) {
                os << std::setw(10) << "";
                ستون++;
            }
            repp->dump(os);
            ستون++;
        }
        os << '\n';
    }

عام:
    // METHODS
    ساكن فراغ جمع_کړه_Stat(ثابت V3Statistic& stat) { s_allStats.ټېل_شا(stat); }

    ساكن مزدوج get_Stat_مجموعه(ثابت string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        لكل (ثابت V3Statistic& itr : s_allStats) {
            ثابت V3Statistic* ثابت repp = &itr;
            إذا (repp->name() == name) أرجع repp->ارزښت();
        }
        أرجع 0.0;
    }

    ساكن فراغ calculate() { sumit(); }

    // CONSTRUCTORS
    څرګند StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::info_سرلیک(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = افتراضي;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

فراغ V3Statistic::dump(std::ofstream& os) ثابت {
    os << " " << std::ښی << std::fixed << std::setprecision(precision()) << std::setw(9)
       << ارزښت();
}

//######################################################################
// Top Stats class

فراغ V3Stats::جمع_کړه_Stat(ثابت V3Statistic& stat) { StatsReport::جمع_کړه_Stat(stat); }

مزدوج V3Stats::get_Stat_مجموعه(ثابت string& name) { أرجع StatsReport::get_Stat_مجموعه(name); }

فراغ V3Stats::statsStage(ثابت string& name) {
    ساكن مزدوج s_وروستی_Wall_وخت = -1;
    ساكن صحيح s_file_شمېره = 0;

    ثابت string digitName = V3Global::digitsFilename(++s_file_شمېره) + "_" + name;

    ثابت مزدوج wall_وخت = V3Os::وخت_Usecs() / 1.0e6;
    إذا (s_وروستی_Wall_وخت < 0) s_وروستی_Wall_وخت = wall_وخت;
    ثابت مزدوج wall_وخت_Delta = wall_وخت - s_وروستی_Wall_وخت;
    s_وروستی_Wall_وخت = wall_وخت;
    V3Stats::جمع_کړه_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wall_وخت_Delta);
    V3Stats::جمع_کړه_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wall_وخت_Delta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بایټونه(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::جمع_کړه_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::جمع_کړه_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

فراغ V3Stats::info_سرلیک(std::ofstream& os, ثابت string& مختاړی) {
    os << مختاړی << "Information:\n";
    os << مختاړی << "  Version: " << V3Options::نسخه() << '\n';
    os << مختاړی << "  Arguments: " << v3Global.opt.all_دلیلونه_String() << '\n';
    os << مختاړی << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << مختاړی << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

فراغ V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بایټونه(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت مزدوج حافظه = memPeak / 1024.0 / 1024.0;
    V3Stats::جمع_کړه_Stat("Peak Memory Usage (MB)", حافظه);

    // Open stats file
    ثابت string filename
        = v3Global.opt.hier_سر_معلومات_Dir() + "/" + v3Global.opt.مختاړی() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(ړنګول ofp, ofp);
}

فراغ V3Stats::summaryReport() {
    StatsReport::calculate();
    std::وتنه << "- V e r i l a t i o n   R e p o r t: " << V3Options::نسخه() << "\n";

    std::وتنه << std::setprecision(3) << std::fixed;
    ثابت مزدوج سرچینه_Chars_MB = V3Stats::get_Stat_مجموعه_Q(STAT_سرچینه_CHARS) / 1024.0 / 1024.0;
    ثابت مزدوج cppCharsMB = V3Stats::get_Stat_مجموعه_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ثابت uint64_t سرچینه_Modules = V3Stats::get_Stat_مجموعه_Q(STAT_سرچینه_MODULES);
    ثابت uint64_t cppFiles = V3Stats::get_Stat_مجموعه_Q(STAT_CPP_FILES);
    ثابت مزدوج modelMB = V3Stats::get_Stat_مجموعه(STAT_MODEL_اندازه) / 1024.0 / 1024.0;
    std::وتنه << "- Verilator: Built from " << سرچینه_Chars_MB << " MB sources in " << سرچینه_Modules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ثابت مزدوج walltime = V3Stats::get_Stat_مجموعه(STAT_WALLTIME);
    ثابت مزدوج walltimeElab = V3Stats::get_Stat_مجموعه(STAT_WALLTIME_ELAB);
    ثابت مزدوج walltimeCvt = V3Stats::get_Stat_مجموعه(STAT_WALLTIME_CVT);
    ثابت مزدوج walltimeBuild = V3Stats::get_Stat_مجموعه(STAT_WALLTIME_BUILD);
    ثابت مزدوج cputime = V3Stats::get_Stat_مجموعه(STAT_CPUTIME);
    std::وتنه << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بایټونه(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت مزدوج حافظه = memPeak / 1024.0 / 1024.0;
    إذا (VL_UNCOVERABLE(حافظه != 0.0)) std::وتنه << "; allocated " << حافظه << " MB";
    std::وتنه << "\n";
}
