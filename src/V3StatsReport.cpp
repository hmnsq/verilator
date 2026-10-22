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

جماعت StatsReport final {
    // TYPES
    استعمال StatColl = std::سمتیہ<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ساکن StatColl s_allStats;  ///< All statistics

    ساکن خالی sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        برائے (خودکار& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = خالی_اشاریہ;
        برائے (ثابت خودکار& itr : byName) {
            V3Statistic* repp = itr.دوسرا;
            اگر (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } ورنہ {
                lastp = repp;
            }
        }
    }

    خالی stars() {
        // Find all stages
        سائز_t max_چوڑائی = 0;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        برائے (ثابت خودکار& itr : s_allStats) {
            ثابت V3Statistic* repp = &itr;
            اگر (repp->stage() == "*" && repp->printit()) {
                اگر (max_چوڑائی < repp->name().لمبائی_2()) max_چوڑائی = repp->name().لمبائی_2();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        برائے (ثابت خودکار& itr : byName) {
            ثابت V3Statistic* repp = itr.دوسرا;
            اگر (repp->perf()) جاری;
            os << "  " << std::بائیں << std::setw(max_چوڑائی) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        برائے (ثابت خودکار& itr : byName) {
            ثابت V3Statistic* repp = itr.دوسرا;
            اگر (!repp->perf()) جاری;
            os << "  " << std::بائیں << std::setw(max_چوڑائی) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    خالی stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        صحیح stage = 0;
        سائز_t max_چوڑائی = 0;
        std::سمتیہ<std::string> stages;
        std::unordered_map<string, صحیح> stageInt;
        std::multimap<std::string, ثابت V3Statistic*> byName;
        // * is always first
        برائے (خودکار it = s_allStats.begin(); it != s_allStats.انتہا(); ++it) {
            ثابت V3Statistic* repp = &(*it);
            اگر (repp->stage() != "*" && repp->printit()) {
                اگر (max_چوڑائی < repp->name().لمبائی_2()) max_چوڑائی = repp->name().لمبائی_2();
                ثابت خودکار it_ملا_Pair = stageInt.emplace(repp->stage(), stage);
                اگر (it_ملا_Pair.دوسرا) {
                    ++stage;
                    stages.ڈالو_عقب(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //سرخی
        os << "  Stat     " << std::بائیں << std::setw(max_چوڑائی - 5 - 2) << "";
        برائے (ثابت string& i : stages) os << " " << std::بائیں << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::بائیں << std::setw(max_چوڑائی - 5 - 2) << "";
        برائے (خودکار it = stages.begin(); it != stages.انتہا(); ++it) {
            os << " " << std::بائیں << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string آخری_Name = "__NONE__";
        string آخری_Comma_Name = "__NONE__";
        غیرعلامتی ستون = 0;
        برائے (خودکار it = byName.cbegin(); it != byName.cend(); ++it) {
            ثابت V3Statistic* repp = it->دوسرا;
            اگر (آخری_Name != repp->name()) {
                آخری_Name = repp->name();
                {
                    string commaName = آخری_Name;
                    string::سائز_type pos = commaName.ڈھونڈو(',');
                    اگر (pos != string::npos) commaName.erase(pos);
                    اگر (آخری_Comma_Name != commaName) {
                        آخری_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                ستون = 0;
                os << "  " << std::بائیں << std::setw(max_چوڑائی) << repp->name();
            }
            جب (ستون < stages.سائز() && stages.at(ستون) != repp->stage()) {
                os << std::setw(10) << "";
                ستون++;
            }
            repp->dump(os);
            ستون++;
        }
        os << '\n';
    }

عوامی:
    // METHODS
    ساکن خالی جوڑو_Stat(ثابت V3Statistic& stat) { s_allStats.ڈالو_عقب(stat); }

    ساکن دوہرا get_Stat_مجموع(ثابت string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        برائے (ثابت V3Statistic& itr : s_allStats) {
            ثابت V3Statistic* ثابت repp = &itr;
            اگر (repp->name() == name) واپس repp->قدر();
        }
        واپس 0.0;
    }

    ساکن خالی calculate() { sumit(); }

    // CONSTRUCTORS
    صریح StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::info_سرخی(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = طےشدہ;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

خالی V3Statistic::dump(std::ofstream& os) ثابت {
    os << " " << std::دائیں << std::fixed << std::setprecision(precision()) << std::setw(9)
       << قدر();
}

//######################################################################
// Top Stats class

خالی V3Stats::جوڑو_Stat(ثابت V3Statistic& stat) { StatsReport::جوڑو_Stat(stat); }

دوہرا V3Stats::get_Stat_مجموع(ثابت string& name) { واپس StatsReport::get_Stat_مجموع(name); }

خالی V3Stats::statsStage(ثابت string& name) {
    ساکن دوہرا s_آخری_Wall_وقت = -1;
    ساکن صحیح s_file_نمبر = 0;

    ثابت string digitName = V3Global::digitsFilename(++s_file_نمبر) + "_" + name;

    ثابت دوہرا wall_وقت = V3Os::وقت_Usecs() / 1.0e6;
    اگر (s_آخری_Wall_وقت < 0) s_آخری_Wall_وقت = wall_وقت;
    ثابت دوہرا wall_وقت_Delta = wall_وقت - s_آخری_Wall_وقت;
    s_آخری_Wall_وقت = wall_وقت;
    V3Stats::جوڑو_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wall_وقت_Delta);
    V3Stats::جوڑو_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wall_وقت_Delta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائٹیں(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::جوڑو_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::جوڑو_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

خالی V3Stats::info_سرخی(std::ofstream& os, ثابت string& سابقہ) {
    os << سابقہ << "Information:\n";
    os << سابقہ << "  Version: " << V3Options::نسخہ() << '\n';
    os << سابقہ << "  Arguments: " << v3Global.opt.all_دلائل_String() << '\n';
    os << سابقہ << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << سابقہ << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

خالی V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائٹیں(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت دوہرا حافظہ = memPeak / 1024.0 / 1024.0;
    V3Stats::جوڑو_Stat("Peak Memory Usage (MB)", حافظہ);

    // Open stats file
    ثابت string filename
        = v3Global.opt.hier_چوٹی_ڈیٹا_Dir() + "/" + v3Global.opt.سابقہ() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    اگر (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(حذف ofp, ofp);
}

خالی V3Stats::summaryReport() {
    StatsReport::calculate();
    std::خارج << "- V e r i l a t i o n   R e p o r t: " << V3Options::نسخہ() << "\n";

    std::خارج << std::setprecision(3) << std::fixed;
    ثابت دوہرا ماخذ_Chars_MB = V3Stats::get_Stat_مجموع_Q(STAT_ماخذ_CHARS) / 1024.0 / 1024.0;
    ثابت دوہرا cppCharsMB = V3Stats::get_Stat_مجموع_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ثابت uint64_t ماخذ_Modules = V3Stats::get_Stat_مجموع_Q(STAT_ماخذ_MODULES);
    ثابت uint64_t cppFiles = V3Stats::get_Stat_مجموع_Q(STAT_CPP_FILES);
    ثابت دوہرا modelMB = V3Stats::get_Stat_مجموع(STAT_MODEL_سائز) / 1024.0 / 1024.0;
    std::خارج << "- Verilator: Built from " << ماخذ_Chars_MB << " MB sources in " << ماخذ_Modules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ثابت دوہرا walltime = V3Stats::get_Stat_مجموع(STAT_WALLTIME);
    ثابت دوہرا walltimeElab = V3Stats::get_Stat_مجموع(STAT_WALLTIME_ELAB);
    ثابت دوہرا walltimeCvt = V3Stats::get_Stat_مجموع(STAT_WALLTIME_CVT);
    ثابت دوہرا walltimeBuild = V3Stats::get_Stat_مجموع(STAT_WALLTIME_BUILD);
    ثابت دوہرا cputime = V3Stats::get_Stat_مجموع(STAT_CPUTIME);
    std::خارج << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_بائٹیں(memPeak /*ref*/, memCurrent /*ref*/);
    ثابت دوہرا حافظہ = memPeak / 1024.0 / 1024.0;
    اگر (VL_UNCOVERABLE(حافظہ != 0.0)) std::خارج << "; allocated " << حافظہ << " MB";
    std::خارج << "\n";
}
