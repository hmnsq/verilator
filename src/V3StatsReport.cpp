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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

ਵਰਗ StatsReport final {
    // TYPES
    ਵਰਤੋਂ StatColl = std::ਸਦਿਸ਼<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    ਸਥਿਰ StatColl s_allStats;  ///< All statistics

    ਸਥਿਰ ਖਾਲੀ sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        ਲਈ (ਸਵੈਚਾਲਿਤ& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : byName) {
            V3Statistic* repp = itr.ਦੂਜਾ;
            ਜੇ (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } ਨਹੀਂਤਾਂ {
                lastp = repp;
            }
        }
    }

    ਖਾਲੀ stars() {
        // Find all stages
        ਮਾਪ_t max_ਚੌੜਾਈ = 0;
        std::multimap<std::string, ਅਚਲ V3Statistic*> byName;
        // * is always first
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : s_allStats) {
            ਅਚਲ V3Statistic* repp = &itr;
            ਜੇ (repp->stage() == "*" && repp->printit()) {
                ਜੇ (max_ਚੌੜਾਈ < repp->name().ਲੰਮਾਈ()) max_ਚੌੜਾਈ = repp->name().ਲੰਮਾਈ();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : byName) {
            ਅਚਲ V3Statistic* repp = itr.ਦੂਜਾ;
            ਜੇ (repp->perf()) ਜਾਰੀ;
            os << "  " << std::ਖੱਬੇ << std::setw(max_ਚੌੜਾਈ) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : byName) {
            ਅਚਲ V3Statistic* repp = itr.ਦੂਜਾ;
            ਜੇ (!repp->perf()) ਜਾਰੀ;
            os << "  " << std::ਖੱਬੇ << std::setw(max_ਚੌੜਾਈ) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    ਖਾਲੀ stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        ਪੂਰਨ stage = 0;
        ਮਾਪ_t max_ਚੌੜਾਈ = 0;
        std::ਸਦਿਸ਼<std::string> stages;
        std::unordered_map<string, ਪੂਰਨ> stageInt;
        std::multimap<std::string, ਅਚਲ V3Statistic*> byName;
        // * is always first
        ਲਈ (ਸਵੈਚਾਲਿਤ it = s_allStats.begin(); it != s_allStats.ਸਿਰਾ(); ++it) {
            ਅਚਲ V3Statistic* repp = &(*it);
            ਜੇ (repp->stage() != "*" && repp->printit()) {
                ਜੇ (max_ਚੌੜਾਈ < repp->name().ਲੰਮਾਈ()) max_ਚੌੜਾਈ = repp->name().ਲੰਮਾਈ();
                ਅਚਲ ਸਵੈਚਾਲਿਤ it_ਮਿਲਿਆ_Pair = stageInt.emplace(repp->stage(), stage);
                ਜੇ (it_ਮਿਲਿਆ_Pair.ਦੂਜਾ) {
                    ++stage;
                    stages.ਧੱਕੋ_ਪਿੱਠ(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //ਸਿਰਲੇਖ
        os << "  Stat     " << std::ਖੱਬੇ << std::setw(max_ਚੌੜਾਈ - 5 - 2) << "";
        ਲਈ (ਅਚਲ string& i : stages) os << " " << std::ਖੱਬੇ << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::ਖੱਬੇ << std::setw(max_ਚੌੜਾਈ - 5 - 2) << "";
        ਲਈ (ਸਵੈਚਾਲਿਤ it = stages.begin(); it != stages.ਸਿਰਾ(); ++it) {
            os << " " << std::ਖੱਬੇ << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string ਆਖਰੀ_Name = "__NONE__";
        string ਆਖਰੀ_Comma_Name = "__NONE__";
        ਬਿਨਾਨਿਸ਼ਾਨੀ ਖਾਨਾ = 0;
        ਲਈ (ਸਵੈਚਾਲਿਤ it = byName.cbegin(); it != byName.cend(); ++it) {
            ਅਚਲ V3Statistic* repp = it->ਦੂਜਾ;
            ਜੇ (ਆਖਰੀ_Name != repp->name()) {
                ਆਖਰੀ_Name = repp->name();
                {
                    string commaName = ਆਖਰੀ_Name;
                    string::ਮਾਪ_type pos = commaName.ਲੱਭੋ(',');
                    ਜੇ (pos != string::npos) commaName.erase(pos);
                    ਜੇ (ਆਖਰੀ_Comma_Name != commaName) {
                        ਆਖਰੀ_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                ਖਾਨਾ = 0;
                os << "  " << std::ਖੱਬੇ << std::setw(max_ਚੌੜਾਈ) << repp->name();
            }
            ਜਦੋਂ (ਖਾਨਾ < stages.ਮਾਪ() && stages.at(ਖਾਨਾ) != repp->stage()) {
                os << std::setw(10) << "";
                ਖਾਨਾ++;
            }
            repp->dump(os);
            ਖਾਨਾ++;
        }
        os << '\n';
    }

ਜਨਤਕ:
    // METHODS
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Stat(ਅਚਲ V3Statistic& stat) { s_allStats.ਧੱਕੋ_ਪਿੱਠ(stat); }

    ਸਥਿਰ ਦੁੱਗਣਾ get_Stat_ਜੋੜ(ਅਚਲ string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        ਲਈ (ਅਚਲ V3Statistic& itr : s_allStats) {
            ਅਚਲ V3Statistic* ਅਚਲ repp = &itr;
            ਜੇ (repp->name() == name) ਵਾਪਸ repp->ਮੁੱਲ();
        }
        ਵਾਪਸ 0.0;
    }

    ਸਥਿਰ ਖਾਲੀ calculate() { sumit(); }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::info_ਸਿਰਲੇਖ(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = ਮੂਲ;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

ਖਾਲੀ V3Statistic::dump(std::ofstream& os) ਅਚਲ {
    os << " " << std::ਸੱਜੇ << std::fixed << std::setprecision(precision()) << std::setw(9)
       << ਮੁੱਲ();
}

//######################################################################
// Top Stats class

ਖਾਲੀ V3Stats::ਜੋੜੋ_Stat(ਅਚਲ V3Statistic& stat) { StatsReport::ਜੋੜੋ_Stat(stat); }

ਦੁੱਗਣਾ V3Stats::get_Stat_ਜੋੜ(ਅਚਲ string& name) { ਵਾਪਸ StatsReport::get_Stat_ਜੋੜ(name); }

ਖਾਲੀ V3Stats::statsStage(ਅਚਲ string& name) {
    ਸਥਿਰ ਦੁੱਗਣਾ s_ਆਖਰੀ_Wall_ਸਮਾਂ = -1;
    ਸਥਿਰ ਪੂਰਨ s_file_ਸੰਖਿਆ = 0;

    ਅਚਲ string digitName = V3Global::digitsFilename(++s_file_ਸੰਖਿਆ) + "_" + name;

    ਅਚਲ ਦੁੱਗਣਾ wall_ਸਮਾਂ = V3Os::ਸਮਾਂ_Usecs() / 1.0e6;
    ਜੇ (s_ਆਖਰੀ_Wall_ਸਮਾਂ < 0) s_ਆਖਰੀ_Wall_ਸਮਾਂ = wall_ਸਮਾਂ;
    ਅਚਲ ਦੁੱਗਣਾ wall_ਸਮਾਂ_Delta = wall_ਸਮਾਂ - s_ਆਖਰੀ_Wall_ਸਮਾਂ;
    s_ਆਖਰੀ_Wall_ਸਮਾਂ = wall_ਸਮਾਂ;
    V3Stats::ਜੋੜੋ_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wall_ਸਮਾਂ_Delta);
    V3Stats::ਜੋੜੋ_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wall_ਸਮਾਂ_Delta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_ਬਾਈਟਾਂ(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::ਜੋੜੋ_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::ਜੋੜੋ_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

ਖਾਲੀ V3Stats::info_ਸਿਰਲੇਖ(std::ofstream& os, ਅਚਲ string& ਅਗੇਤਰ) {
    os << ਅਗੇਤਰ << "Information:\n";
    os << ਅਗੇਤਰ << "  Version: " << V3Options::ਸੰਸਕਰਣ() << '\n';
    os << ਅਗੇਤਰ << "  Arguments: " << v3Global.opt.all_ਦਲੀਲਾਂ_String() << '\n';
    os << ਅਗੇਤਰ << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << ਅਗੇਤਰ << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

ਖਾਲੀ V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_ਬਾਈਟਾਂ(memPeak /*ref*/, memCurrent /*ref*/);
    ਅਚਲ ਦੁੱਗਣਾ ਸਮ੍ਰਿਤੀ = memPeak / 1024.0 / 1024.0;
    V3Stats::ਜੋੜੋ_Stat("Peak Memory Usage (MB)", ਸਮ੍ਰਿਤੀ);

    // Open stats file
    ਅਚਲ string filename
        = v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir() + "/" + v3Global.opt.ਅਗੇਤਰ() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    ਜੇ (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(ਮਿਟਾਓ ofp, ofp);
}

ਖਾਲੀ V3Stats::summaryReport() {
    StatsReport::calculate();
    std::ਬਾਹਰ << "- V e r i l a t i o n   R e p o r t: " << V3Options::ਸੰਸਕਰਣ() << "\n";

    std::ਬਾਹਰ << std::setprecision(3) << std::fixed;
    ਅਚਲ ਦੁੱਗਣਾ ਸਰੋਤ_Chars_MB = V3Stats::get_Stat_ਜੋੜ_Q(STAT_ਸਰੋਤ_CHARS) / 1024.0 / 1024.0;
    ਅਚਲ ਦੁੱਗਣਾ cppCharsMB = V3Stats::get_Stat_ਜੋੜ_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ਅਚਲ uint64_t ਸਰੋਤ_Modules = V3Stats::get_Stat_ਜੋੜ_Q(STAT_ਸਰੋਤ_MODULES);
    ਅਚਲ uint64_t cppFiles = V3Stats::get_Stat_ਜੋੜ_Q(STAT_CPP_FILES);
    ਅਚਲ ਦੁੱਗਣਾ modelMB = V3Stats::get_Stat_ਜੋੜ(STAT_MODEL_ਮਾਪ) / 1024.0 / 1024.0;
    std::ਬਾਹਰ << "- Verilator: Built from " << ਸਰੋਤ_Chars_MB << " MB sources in " << ਸਰੋਤ_Modules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ਅਚਲ ਦੁੱਗਣਾ walltime = V3Stats::get_Stat_ਜੋੜ(STAT_WALLTIME);
    ਅਚਲ ਦੁੱਗਣਾ walltimeElab = V3Stats::get_Stat_ਜੋੜ(STAT_WALLTIME_ELAB);
    ਅਚਲ ਦੁੱਗਣਾ walltimeCvt = V3Stats::get_Stat_ਜੋੜ(STAT_WALLTIME_CVT);
    ਅਚਲ ਦੁੱਗਣਾ walltimeBuild = V3Stats::get_Stat_ਜੋੜ(STAT_WALLTIME_BUILD);
    ਅਚਲ ਦੁੱਗਣਾ cputime = V3Stats::get_Stat_ਜੋੜ(STAT_CPUTIME);
    std::ਬਾਹਰ << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_ਬਾਈਟਾਂ(memPeak /*ref*/, memCurrent /*ref*/);
    ਅਚਲ ਦੁੱਗਣਾ ਸਮ੍ਰਿਤੀ = memPeak / 1024.0 / 1024.0;
    ਜੇ (VL_UNCOVERABLE(ਸਮ੍ਰਿਤੀ != 0.0)) std::ਬਾਹਰ << "; allocated " << ਸਮ੍ਰਿਤੀ << " MB";
    std::ਬਾਹਰ << "\n";
}
