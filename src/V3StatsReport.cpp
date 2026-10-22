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
    ورتوں StatColl = std::سمتیہ<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ساكن StatColl s_allStats;  ///< All statistics

    ساكن فراغ sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        لكل (خودکار& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = خالی_سوچک;
        لكل (ثابت خودکار& itr : byName) {
            V3Statistic* repp = itr.دوجا;
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
        ناپ_t max_چوڑائی = 0;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        لكل (ثابت خودکار& itr : s_allStats) {
            ثابت V3Statistic* repp = &itr;
            إذا (repp->stage() == "*" && repp->printit()) {
                إذا (max_چوڑائی < repp->name().لمائی()) max_چوڑائی = repp->name().لمائی();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        لكل (ثابت خودکار& itr : byName) {
            ثابت V3Statistic* repp = itr.دوجا;
            إذا (repp->perf()) استمر;
            os << "  " << std::کھبے << std::setw(max_چوڑائی) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        لكل (ثابت خودکار& itr : byName) {
            ثابت V3Statistic* repp = itr.دوجا;
            إذا (!repp->perf()) استمر;
            os << "  " << std::کھبے << std::setw(max_چوڑائی) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    فراغ stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        صحيح stage = 0;
        ناپ_t max_چوڑائی = 0;
        std::سمتیہ<std::string> stages;
        std::unordered_map<string, صحيح> stageInt;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        لكل (خودکار it = s_allStats.begin(); it != s_allStats.سرا(); ++it) {
            ثابت V3Statistic* repp = &(*it);
            إذا (repp->stage() != "*" && repp->printit()) {
                إذا (max_چوڑائی < repp->name().لمائی()) max_چوڑائی = repp->name().لمائی();
                ثابت خودکار it_لبھیا_Pair = stageInt.emplace(repp->stage(), stage);
                إذا (it_لبھیا_Pair.دوجا) {
                    ++stage;
                    stages.ڈالو_پٹھ(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //سرخی
        os << "  Stat     " << std::کھبے << std::setw(max_چوڑائی - 5 - 2) << "";
        لكل (ثابت string& i : stages) os << " " << std::کھبے << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::کھبے << std::setw(max_چوڑائی - 5 - 2) << "";
        لكل (خودکار it = stages.begin(); it != stages.سرا(); ++it) {
            os << " " << std::کھبے << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string آخری_Name = "__NONE__";
        string آخری_Comma_Name = "__NONE__";
        بنانشانی خانہ = 0;
        لكل (خودکار it = byName.cbegin(); it != byName.cend(); ++it) {
            ثابت V3Statistic* repp = it->دوجا;
            إذا (آخری_Name != repp->name()) {
                آخری_Name = repp->name();
                {
                    string commaName = آخری_Name;
                    string::ناپ_type pos = commaName.لبھو(',');
                    إذا (pos != string::npos) commaName.erase(pos);
                    إذا (آخری_Comma_Name != commaName) {
                        آخری_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                خانہ = 0;
                os << "  " << std::کھبے << std::setw(max_چوڑائی) << repp->name();
            }
            طالما (خانہ < stages.ناپ() && stages.at(خانہ) != repp->stage()) {
                os << std::setw(10) << "";
                خانہ++;
            }
            repp->dump(os);
            خانہ++;
        }
        os << '\n';
    }

عام:
    // METHODS
    ساكن فراغ جوڑو_Stat(ثابت V3Statistic& stat) { s_allStats.ڈالو_پٹھ(stat); }

    ساكن مزدوج get_Stat_جوڑ(ثابت string& name) {
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
        V3Stats::info_سرخی(os, "");
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
    os << " " << std::سجے << std::fixed << std::setprecision(precision()) << std::setw(9)
       << قدر();
}

//######################################################################
// Top Stats class

فراغ V3Stats::جوڑو_Stat(ثابت V3Statistic& stat) { StatsReport::جوڑو_Stat(stat); }

مزدوج V3Stats::get_Stat_جوڑ(ثابت string& name) { أرجع StatsReport::get_Stat_جوڑ(name); }

فراغ V3Stats::statsStage(ثابت string& name) {
    ساكن مزدوج s_آخری_Wall_ویلا = -1;
    ساكن صحيح s_file_نمبر = 0;

    ثابت string digitName = V3Global::digitsFilename(++s_file_نمبر) + "_" + name;

    ثابت مزدوج wall_ویلا = V3Os::ویلا_Usecs() / 1.0e6;
    إذا (s_آخری_Wall_ویلا < 0) s_آخری_Wall_ویلا = wall_ویلا;
    ثابت مزدوج wall_ویلا_Delta = wall_ویلا - s_آخری_Wall_ویلا;
    s_آخری_Wall_ویلا = wall_ویلا;
    V3Stats::جوڑو_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wall_ویلا_Delta);
    V3Stats::جوڑو_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wall_ویلا_Delta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائٹاں(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::جوڑو_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::جوڑو_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

فراغ V3Stats::info_سرخی(std::ofstream& os, ثابت string& اگیتر) {
    os << اگیتر << "Information:\n";
    os << اگیتر << "  Version: " << V3Options::نسخہ() << '\n';
    os << اگیتر << "  Arguments: " << v3Global.opt.all_دلیلاں_String() << '\n';
    os << اگیتر << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << اگیتر << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

فراغ V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائٹاں(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت مزدوج حافظہ = memPeak / 1024.0 / 1024.0;
    V3Stats::جوڑو_Stat("Peak Memory Usage (MB)", حافظہ);

    // Open stats file
    ثابت string filename
        = v3Global.opt.hier_چوٹی_انکڑے_Dir() + "/" + v3Global.opt.اگیتر() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(مٹاؤ ofp, ofp);
}

فراغ V3Stats::summaryReport() {
    StatsReport::calculate();
    std::باہر << "- V e r i l a t i o n   R e p o r t: " << V3Options::نسخہ() << "\n";

    std::باہر << std::setprecision(3) << std::fixed;
    ثابت مزدوج سروت_Chars_MB = V3Stats::get_Stat_جوڑ_Q(STAT_سروت_CHARS) / 1024.0 / 1024.0;
    ثابت مزدوج cppCharsMB = V3Stats::get_Stat_جوڑ_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ثابت uint64_t سروت_Modules = V3Stats::get_Stat_جوڑ_Q(STAT_سروت_MODULES);
    ثابت uint64_t cppFiles = V3Stats::get_Stat_جوڑ_Q(STAT_CPP_FILES);
    ثابت مزدوج modelMB = V3Stats::get_Stat_جوڑ(STAT_MODEL_ناپ) / 1024.0 / 1024.0;
    std::باہر << "- Verilator: Built from " << سروت_Chars_MB << " MB sources in " << سروت_Modules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ثابت مزدوج walltime = V3Stats::get_Stat_جوڑ(STAT_WALLTIME);
    ثابت مزدوج walltimeElab = V3Stats::get_Stat_جوڑ(STAT_WALLTIME_ELAB);
    ثابت مزدوج walltimeCvt = V3Stats::get_Stat_جوڑ(STAT_WALLTIME_CVT);
    ثابت مزدوج walltimeBuild = V3Stats::get_Stat_جوڑ(STAT_WALLTIME_BUILD);
    ثابت مزدوج cputime = V3Stats::get_Stat_جوڑ(STAT_CPUTIME);
    std::باہر << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائٹاں(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت مزدوج حافظہ = memPeak / 1024.0 / 1024.0;
    إذا (VL_UNCOVERABLE(حافظہ != 0.0)) std::باہر << "; allocated " << حافظہ << " MB";
    std::باہر << "\n";
}
