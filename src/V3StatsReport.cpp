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
    استعمال StatColl = std::ويڪٽر<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ساكن StatColl s_allStats;  ///< All statistics

    ساكن فراغ sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        لكل (خودڪار& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = خالي_اشارو;
        لكل (ثابت خودڪار& itr : byName) {
            V3Statistic* repp = itr.ٻيو;
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
        سائيز_t max_ويڪر = 0;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        لكل (ثابت خودڪار& itr : s_allStats) {
            ثابت V3Statistic* repp = &itr;
            إذا (repp->stage() == "*" && repp->printit()) {
                إذا (max_ويڪر < repp->name().ڊيگهائي()) max_ويڪر = repp->name().ڊيگهائي();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        لكل (ثابت خودڪار& itr : byName) {
            ثابت V3Statistic* repp = itr.ٻيو;
            إذا (repp->perf()) استمر;
            os << "  " << std::کاٻي << std::setw(max_ويڪر) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        لكل (ثابت خودڪار& itr : byName) {
            ثابت V3Statistic* repp = itr.ٻيو;
            إذا (!repp->perf()) استمر;
            os << "  " << std::کاٻي << std::setw(max_ويڪر) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    فراغ stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        صحيح stage = 0;
        سائيز_t max_ويڪر = 0;
        std::ويڪٽر<std::string> stages;
        std::unordered_map<string, صحيح> stageInt;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        لكل (خودڪار it = s_allStats.begin(); it != s_allStats.پڇاڙي(); ++it) {
            ثابت V3Statistic* repp = &(*it);
            إذا (repp->stage() != "*" && repp->printit()) {
                إذا (max_ويڪر < repp->name().ڊيگهائي()) max_ويڪر = repp->name().ڊيگهائي();
                ثابت خودڪار it_مليو_Pair = stageInt.emplace(repp->stage(), stage);
                إذا (it_مليو_Pair.ٻيو) {
                    ++stage;
                    stages.ڌڪو_پٺ(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //سرو
        os << "  Stat     " << std::کاٻي << std::setw(max_ويڪر - 5 - 2) << "";
        لكل (ثابت string& i : stages) os << " " << std::کاٻي << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::کاٻي << std::setw(max_ويڪر - 5 - 2) << "";
        لكل (خودڪار it = stages.begin(); it != stages.پڇاڙي(); ++it) {
            os << " " << std::کاٻي << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string آخري_Name = "__NONE__";
        string آخري_Comma_Name = "__NONE__";
        بنانشان کانو = 0;
        لكل (خودڪار it = byName.cbegin(); it != byName.cend(); ++it) {
            ثابت V3Statistic* repp = it->ٻيو;
            إذا (آخري_Name != repp->name()) {
                آخري_Name = repp->name();
                {
                    string commaName = آخري_Name;
                    string::سائيز_type pos = commaName.ڳوليو(',');
                    إذا (pos != string::npos) commaName.erase(pos);
                    إذا (آخري_Comma_Name != commaName) {
                        آخري_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                کانو = 0;
                os << "  " << std::کاٻي << std::setw(max_ويڪر) << repp->name();
            }
            طالما (کانو < stages.سائيز() && stages.at(کانو) != repp->stage()) {
                os << std::setw(10) << "";
                کانو++;
            }
            repp->dump(os);
            کانو++;
        }
        os << '\n';
    }

عام:
    // METHODS
    ساكن فراغ جوڙيو_Stat(ثابت V3Statistic& stat) { s_allStats.ڌڪو_پٺ(stat); }

    ساكن مزدوج get_Stat_جوڙ(ثابت string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        لكل (ثابت V3Statistic& itr : s_allStats) {
            ثابت V3Statistic* ثابت repp = &itr;
            إذا (repp->name() == name) أرجع repp->قدر();
        }
        أرجع 0.0;
    }

    ساكن فراغ calculate() { sumit(); }

    // CONSTRUCTORS
    واضح StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::info_سرو(os, "");
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
    os << " " << std::ساڄي << std::fixed << std::setprecision(precision()) << std::setw(9)
       << قدر();
}

//######################################################################
// Top Stats class

فراغ V3Stats::جوڙيو_Stat(ثابت V3Statistic& stat) { StatsReport::جوڙيو_Stat(stat); }

مزدوج V3Stats::get_Stat_جوڙ(ثابت string& name) { أرجع StatsReport::get_Stat_جوڙ(name); }

فراغ V3Stats::statsStage(ثابت string& name) {
    ساكن مزدوج s_آخري_Wall_وقت = -1;
    ساكن صحيح s_file_عدد = 0;

    ثابت string digitName = V3Global::digitsFilename(++s_file_عدد) + "_" + name;

    ثابت مزدوج wall_وقت = V3Os::وقت_Usecs() / 1.0e6;
    إذا (s_آخري_Wall_وقت < 0) s_آخري_Wall_وقت = wall_وقت;
    ثابت مزدوج wall_وقت_Delta = wall_وقت - s_آخري_Wall_وقت;
    s_آخري_Wall_وقت = wall_وقت;
    V3Stats::جوڙيو_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wall_وقت_Delta);
    V3Stats::جوڙيو_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wall_وقت_Delta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائيٽون(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::جوڙيو_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::جوڙيو_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

فراغ V3Stats::info_سرو(std::ofstream& os, ثابت string& اڳياڙي) {
    os << اڳياڙي << "Information:\n";
    os << اڳياڙي << "  Version: " << V3Options::نسخو() << '\n';
    os << اڳياڙي << "  Arguments: " << v3Global.opt.all_دليلون_String() << '\n';
    os << اڳياڙي << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << اڳياڙي << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

فراغ V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائيٽون(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت مزدوج يادگيري = memPeak / 1024.0 / 1024.0;
    V3Stats::جوڙيو_Stat("Peak Memory Usage (MB)", يادگيري);

    // Open stats file
    ثابت string filename
        = v3Global.opt.hier_چوٽي_معلومات_Dir() + "/" + v3Global.opt.اڳياڙي() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(ميٽيو ofp, ofp);
}

فراغ V3Stats::summaryReport() {
    StatsReport::calculate();
    std::ٻاهر << "- V e r i l a t i o n   R e p o r t: " << V3Options::نسخو() << "\n";

    std::ٻاهر << std::setprecision(3) << std::fixed;
    ثابت مزدوج ذريعو_Chars_MB = V3Stats::get_Stat_جوڙ_Q(STAT_ذريعو_CHARS) / 1024.0 / 1024.0;
    ثابت مزدوج cppCharsMB = V3Stats::get_Stat_جوڙ_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ثابت uint64_t ذريعو_Modules = V3Stats::get_Stat_جوڙ_Q(STAT_ذريعو_MODULES);
    ثابت uint64_t cppFiles = V3Stats::get_Stat_جوڙ_Q(STAT_CPP_FILES);
    ثابت مزدوج modelMB = V3Stats::get_Stat_جوڙ(STAT_MODEL_سائيز) / 1024.0 / 1024.0;
    std::ٻاهر << "- Verilator: Built from " << ذريعو_Chars_MB << " MB sources in " << ذريعو_Modules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ثابت مزدوج walltime = V3Stats::get_Stat_جوڙ(STAT_WALLTIME);
    ثابت مزدوج walltimeElab = V3Stats::get_Stat_جوڙ(STAT_WALLTIME_ELAB);
    ثابت مزدوج walltimeCvt = V3Stats::get_Stat_جوڙ(STAT_WALLTIME_CVT);
    ثابت مزدوج walltimeBuild = V3Stats::get_Stat_جوڙ(STAT_WALLTIME_BUILD);
    ثابت مزدوج cputime = V3Stats::get_Stat_جوڙ(STAT_CPUTIME);
    std::ٻاهر << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائيٽون(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت مزدوج يادگيري = memPeak / 1024.0 / 1024.0;
    إذا (VL_UNCOVERABLE(يادگيري != 0.0)) std::ٻاهر << "; allocated " << يادگيري << " MB";
    std::ٻاهر << "\n";
}
