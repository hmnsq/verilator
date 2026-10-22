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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <iomanip>
#అన్తర్భావ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Stats dumping

తరగతి StatsReport final {
    // TYPES
    ఉపయోగించి StatColl = std::సదిశ<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    స్థిర StatColl s_allStats;  ///< All statistics

    స్థిర శూన్య sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        కొరకు (స్వయంచాలక& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : byName) {
            V3Statistic* repp = itr.రెండవ;
            ఒకవేళ (lastp && lastp->sumit() && lastp->printit() && repp->printit()
                && lastp->name() == repp->name() && lastp->stage() == repp->stage()) {
                lastp->combineWith(repp);
            } లేకపోతే {
                lastp = repp;
            }
        }
    }

    శూన్య stars() {
        // Find all stages
        కొలత_t max_వెడల్పు = 0;
        std::multimap<std::string, స్థిరరాశి V3Statistic*> byName;
        // * is always first
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : s_allStats) {
            స్థిరరాశి V3Statistic* repp = &itr;
            ఒకవేళ (repp->stage() == "*" && repp->printit()) {
                ఒకవేళ (max_వెడల్పు < repp->name().నిడివి()) max_వెడల్పు = repp->name().నిడివి();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : byName) {
            స్థిరరాశి V3Statistic* repp = itr.రెండవ;
            ఒకవేళ (repp->perf()) కొనసాగు;
            os << "  " << std::ఎడమ << std::setw(max_వెడల్పు) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : byName) {
            స్థిరరాశి V3Statistic* repp = itr.రెండవ;
            ఒకవేళ (!repp->perf()) కొనసాగు;
            os << "  " << std::ఎడమ << std::setw(max_వెడల్పు) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    శూన్య stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        పూర్ణాంకం stage = 0;
        కొలత_t max_వెడల్పు = 0;
        std::సదిశ<std::string> stages;
        std::unordered_map<string, పూర్ణాంకం> stageInt;
        std::multimap<std::string, స్థిరరాశి V3Statistic*> byName;
        // * is always first
        కొరకు (స్వయంచాలక it = s_allStats.begin(); it != s_allStats.చివర(); ++it) {
            స్థిరరాశి V3Statistic* repp = &(*it);
            ఒకవేళ (repp->stage() != "*" && repp->printit()) {
                ఒకవేళ (max_వెడల్పు < repp->name().నిడివి()) max_వెడల్పు = repp->name().నిడివి();
                స్థిరరాశి స్వయంచాలక it_దొరికింది_Pair = stageInt.emplace(repp->stage(), stage);
                ఒకవేళ (it_దొరికింది_Pair.రెండవ) {
                    ++stage;
                    stages.నెట్టు_వెనుక(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //శీర్షిక
        os << "  Stat     " << std::ఎడమ << std::setw(max_వెడల్పు - 5 - 2) << "";
        కొరకు (స్థిరరాశి string& i : stages) os << " " << std::ఎడమ << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::ఎడమ << std::setw(max_వెడల్పు - 5 - 2) << "";
        కొరకు (స్వయంచాలక it = stages.begin(); it != stages.చివర(); ++it) {
            os << " " << std::ఎడమ << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string చివరి_Name = "__NONE__";
        string చివరి_Comma_Name = "__NONE__";
        గుర్తులేని నిలువు = 0;
        కొరకు (స్వయంచాలక it = byName.cbegin(); it != byName.cend(); ++it) {
            స్థిరరాశి V3Statistic* repp = it->రెండవ;
            ఒకవేళ (చివరి_Name != repp->name()) {
                చివరి_Name = repp->name();
                {
                    string commaName = చివరి_Name;
                    string::కొలత_type pos = commaName.కనుగొను(',');
                    ఒకవేళ (pos != string::npos) commaName.erase(pos);
                    ఒకవేళ (చివరి_Comma_Name != commaName) {
                        చివరి_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                నిలువు = 0;
                os << "  " << std::ఎడమ << std::setw(max_వెడల్పు) << repp->name();
            }
            అయితే (నిలువు < stages.కొలత() && stages.at(నిలువు) != repp->stage()) {
                os << std::setw(10) << "";
                నిలువు++;
            }
            repp->dump(os);
            నిలువు++;
        }
        os << '\n';
    }

బహిరంగ:
    // METHODS
    స్థిర శూన్య కూడు_Stat(స్థిరరాశి V3Statistic& stat) { s_allStats.నెట్టు_వెనుక(stat); }

    స్థిర ద్విగుణ get_Stat_కూడిక(స్థిరరాశి string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        కొరకు (స్థిరరాశి V3Statistic& itr : s_allStats) {
            స్థిరరాశి V3Statistic* స్థిరరాశి repp = &itr;
            ఒకవేళ (repp->name() == name) తిరిగి repp->విలువ();
        }
        తిరిగి 0.0;
    }

    స్థిర శూన్య calculate() { sumit(); }

    // CONSTRUCTORS
    స్పష్టమైన StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::info_శీర్షిక(os, "");
        sumit();
        stars();
        stages();
    }
    ~StatsReport() = అప్రమేయ;
};

StatsReport::StatColl StatsReport::s_allStats;

//######################################################################
// V3Statstic class

శూన్య V3Statistic::dump(std::ofstream& os) స్థిరరాశి {
    os << " " << std::కుడి << std::fixed << std::setprecision(precision()) << std::setw(9)
       << విలువ();
}

//######################################################################
// Top Stats class

శూన్య V3Stats::కూడు_Stat(స్థిరరాశి V3Statistic& stat) { StatsReport::కూడు_Stat(stat); }

ద్విగుణ V3Stats::get_Stat_కూడిక(స్థిరరాశి string& name) { తిరిగి StatsReport::get_Stat_కూడిక(name); }

శూన్య V3Stats::statsStage(స్థిరరాశి string& name) {
    స్థిర ద్విగుణ s_చివరి_Wall_సమయం = -1;
    స్థిర పూర్ణాంకం s_file_సంఖ్య = 0;

    స్థిరరాశి string digitName = V3Global::digitsFilename(++s_file_సంఖ్య) + "_" + name;

    స్థిరరాశి ద్విగుణ wall_సమయం = V3Os::సమయం_Usecs() / 1.0e6;
    ఒకవేళ (s_చివరి_Wall_సమయం < 0) s_చివరి_Wall_సమయం = wall_సమయం;
    స్థిరరాశి ద్విగుణ wall_సమయం_Delta = wall_సమయం - s_చివరి_Wall_సమయం;
    s_చివరి_Wall_సమయం = wall_సమయం;
    V3Stats::కూడు_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wall_సమయం_Delta);
    V3Stats::కూడు_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wall_సమయం_Delta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_బైట్లు(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::కూడు_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::కూడు_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

శూన్య V3Stats::info_శీర్షిక(std::ofstream& os, స్థిరరాశి string& ఉపసర్గ) {
    os << ఉపసర్గ << "Information:\n";
    os << ఉపసర్గ << "  Version: " << V3Options::సంస్కరణ() << '\n';
    os << ఉపసర్గ << "  Arguments: " << v3Global.opt.all_వాదనలు_String() << '\n';
    os << ఉపసర్గ << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << ఉపసర్గ << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

శూన్య V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_బైట్లు(memPeak /*ref*/, memCurrent /*ref*/);
    స్థిరరాశి ద్విగుణ జ్ఞాపకం = memPeak / 1024.0 / 1024.0;
    V3Stats::కూడు_Stat("Peak Memory Usage (MB)", జ్ఞాపకం);

    // Open stats file
    స్థిరరాశి string filename
        = v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir() + "/" + v3Global.opt.ఉపసర్గ() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    ఒకవేళ (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(తొలగించు ofp, ofp);
}

శూన్య V3Stats::summaryReport() {
    StatsReport::calculate();
    std::బయటకు << "- V e r i l a t i o n   R e p o r t: " << V3Options::సంస్కరణ() << "\n";

    std::బయటకు << std::setprecision(3) << std::fixed;
    స్థిరరాశి ద్విగుణ ఆధారం_Chars_MB = V3Stats::get_Stat_కూడిక_Q(STAT_ఆధారం_CHARS) / 1024.0 / 1024.0;
    స్థిరరాశి ద్విగుణ cppCharsMB = V3Stats::get_Stat_కూడిక_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    స్థిరరాశి uint64_t ఆధారం_Modules = V3Stats::get_Stat_కూడిక_Q(STAT_ఆధారం_MODULES);
    స్థిరరాశి uint64_t cppFiles = V3Stats::get_Stat_కూడిక_Q(STAT_CPP_FILES);
    స్థిరరాశి ద్విగుణ modelMB = V3Stats::get_Stat_కూడిక(STAT_MODEL_కొలత) / 1024.0 / 1024.0;
    std::బయటకు << "- Verilator: Built from " << ఆధారం_Chars_MB << " MB sources in " << ఆధారం_Modules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    స్థిరరాశి ద్విగుణ walltime = V3Stats::get_Stat_కూడిక(STAT_WALLTIME);
    స్థిరరాశి ద్విగుణ walltimeElab = V3Stats::get_Stat_కూడిక(STAT_WALLTIME_ELAB);
    స్థిరరాశి ద్విగుణ walltimeCvt = V3Stats::get_Stat_కూడిక(STAT_WALLTIME_CVT);
    స్థిరరాశి ద్విగుణ walltimeBuild = V3Stats::get_Stat_కూడిక(STAT_WALLTIME_BUILD);
    స్థిరరాశి ద్విగుణ cputime = V3Stats::get_Stat_కూడిక(STAT_CPUTIME);
    std::బయటకు << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_బైట్లు(memPeak /*ref*/, memCurrent /*ref*/);
    స్థిరరాశి ద్విగుణ జ్ఞాపకం = memPeak / 1024.0 / 1024.0;
    ఒకవేళ (VL_UNCOVERABLE(జ్ఞాపకం != 0.0)) std::బయటకు << "; allocated " << జ్ఞాపకం << " MB";
    std::బయటకు << "\n";
}
