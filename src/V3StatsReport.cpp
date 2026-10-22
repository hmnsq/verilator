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
    استفاده StatColl = std::وکتور<V3Statistic>;

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
        V3Statistic* lastp = اشاره_تهی;
        لكل (ثابت خودکار& itr : byName) {
            V3Statistic* repp = itr.دومی;
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
        اندازه_t max_عرض = 0;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        لكل (ثابت خودکار& itr : s_allStats) {
            ثابت V3Statistic* repp = &itr;
            إذا (repp->stage() == "*" && repp->printit()) {
                إذا (max_عرض < repp->name().درازی()) max_عرض = repp->name().درازی();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        لكل (ثابت خودکار& itr : byName) {
            ثابت V3Statistic* repp = itr.دومی;
            إذا (repp->perf()) استمر;
            os << "  " << std::چپ << std::setw(max_عرض) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        لكل (ثابت خودکار& itr : byName) {
            ثابت V3Statistic* repp = itr.دومی;
            إذا (!repp->perf()) استمر;
            os << "  " << std::چپ << std::setw(max_عرض) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    فراغ stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        صحيح stage = 0;
        اندازه_t max_عرض = 0;
        std::وکتور<std::string> stages;
        std::unordered_map<string, صحيح> stageInt;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        لكل (خودکار it = s_allStats.begin(); it != s_allStats.انتها(); ++it) {
            ثابت V3Statistic* repp = &(*it);
            إذا (repp->stage() != "*" && repp->printit()) {
                إذا (max_عرض < repp->name().درازی()) max_عرض = repp->name().درازی();
                ثابت خودکار it_یافت_Pair = stageInt.emplace(repp->stage(), stage);
                إذا (it_یافت_Pair.دومی) {
                    ++stage;
                    stages.افزودن_پشت(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //سرآیند
        os << "  Stat     " << std::چپ << std::setw(max_عرض - 5 - 2) << "";
        لكل (ثابت string& i : stages) os << " " << std::چپ << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::چپ << std::setw(max_عرض - 5 - 2) << "";
        لكل (خودکار it = stages.begin(); it != stages.انتها(); ++it) {
            os << " " << std::چپ << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string آخرین_Name = "__NONE__";
        string آخرین_Comma_Name = "__NONE__";
        بیعلامت ستون = 0;
        لكل (خودکار it = byName.cbegin(); it != byName.cend(); ++it) {
            ثابت V3Statistic* repp = it->دومی;
            إذا (آخرین_Name != repp->name()) {
                آخرین_Name = repp->name();
                {
                    string commaName = آخرین_Name;
                    string::اندازه_type pos = commaName.پیدا_کن(',');
                    إذا (pos != string::npos) commaName.erase(pos);
                    إذا (آخرین_Comma_Name != commaName) {
                        آخرین_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                ستون = 0;
                os << "  " << std::چپ << std::setw(max_عرض) << repp->name();
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
    ساكن فراغ جمع_کن_Stat(ثابت V3Statistic& stat) { s_allStats.افزودن_پشت(stat); }

    ساكن مزدوج get_Stat_جمع(ثابت string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        لكل (ثابت V3Statistic& itr : s_allStats) {
            ثابت V3Statistic* ثابت repp = &itr;
            إذا (repp->name() == name) أرجع repp->مقدار();
        }
        أرجع 0.0;
    }

    ساكن فراغ calculate() { sumit(); }

    // CONSTRUCTORS
    صریح StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::info_سرآیند(os, "");
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
    os << " " << std::راست << std::fixed << std::setprecision(precision()) << std::setw(9)
       << مقدار();
}

//######################################################################
// Top Stats class

فراغ V3Stats::جمع_کن_Stat(ثابت V3Statistic& stat) { StatsReport::جمع_کن_Stat(stat); }

مزدوج V3Stats::get_Stat_جمع(ثابت string& name) { أرجع StatsReport::get_Stat_جمع(name); }

فراغ V3Stats::statsStage(ثابت string& name) {
    ساكن مزدوج s_آخرین_Wall_زمان = -1;
    ساكن صحيح s_file_عدد = 0;

    ثابت string digitName = V3Global::digitsFilename(++s_file_عدد) + "_" + name;

    ثابت مزدوج wall_زمان = V3Os::زمان_Usecs() / 1.0e6;
    إذا (s_آخرین_Wall_زمان < 0) s_آخرین_Wall_زمان = wall_زمان;
    ثابت مزدوج wall_زمان_Delta = wall_زمان - s_آخرین_Wall_زمان;
    s_آخرین_Wall_زمان = wall_زمان;
    V3Stats::جمع_کن_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wall_زمان_Delta);
    V3Stats::جمع_کن_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wall_زمان_Delta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بایتها(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::جمع_کن_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::جمع_کن_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

فراغ V3Stats::info_سرآیند(std::ofstream& os, ثابت string& پیشوند) {
    os << پیشوند << "Information:\n";
    os << پیشوند << "  Version: " << V3Options::نسخه() << '\n';
    os << پیشوند << "  Arguments: " << v3Global.opt.all_آرگومانها_String() << '\n';
    os << پیشوند << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << پیشوند << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

فراغ V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بایتها(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت مزدوج حافظه = memPeak / 1024.0 / 1024.0;
    V3Stats::جمع_کن_Stat("Peak Memory Usage (MB)", حافظه);

    // Open stats file
    ثابت string filename
        = v3Global.opt.hier_بالاترین_داده_Dir() + "/" + v3Global.opt.پیشوند() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(حذف ofp, ofp);
}

فراغ V3Stats::summaryReport() {
    StatsReport::calculate();
    std::خروجی << "- V e r i l a t i o n   R e p o r t: " << V3Options::نسخه() << "\n";

    std::خروجی << std::setprecision(3) << std::fixed;
    ثابت مزدوج منبع_Chars_MB = V3Stats::get_Stat_جمع_Q(STAT_منبع_CHARS) / 1024.0 / 1024.0;
    ثابت مزدوج cppCharsMB = V3Stats::get_Stat_جمع_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ثابت uint64_t منبع_Modules = V3Stats::get_Stat_جمع_Q(STAT_منبع_MODULES);
    ثابت uint64_t cppFiles = V3Stats::get_Stat_جمع_Q(STAT_CPP_FILES);
    ثابت مزدوج modelMB = V3Stats::get_Stat_جمع(STAT_MODEL_اندازه) / 1024.0 / 1024.0;
    std::خروجی << "- Verilator: Built from " << منبع_Chars_MB << " MB sources in " << منبع_Modules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ثابت مزدوج walltime = V3Stats::get_Stat_جمع(STAT_WALLTIME);
    ثابت مزدوج walltimeElab = V3Stats::get_Stat_جمع(STAT_WALLTIME_ELAB);
    ثابت مزدوج walltimeCvt = V3Stats::get_Stat_جمع(STAT_WALLTIME_CVT);
    ثابت مزدوج walltimeBuild = V3Stats::get_Stat_جمع(STAT_WALLTIME_BUILD);
    ثابت مزدوج cputime = V3Stats::get_Stat_جمع(STAT_CPUTIME);
    std::خروجی << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بایتها(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت مزدوج حافظه = memPeak / 1024.0 / 1024.0;
    إذا (VL_UNCOVERABLE(حافظه != 0.0)) std::خروجی << "; allocated " << حافظه << " MB";
    std::خروجی << "\n";
}
