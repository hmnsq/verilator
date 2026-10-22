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
    उपयुज्य StatColl = std::सदिशम्<V3Statistic>;

    // STATE
    std::ofstream& os;  ///< Output stream
    स्थिर StatColl s_allStats;  ///< All statistics

    स्थिर शून्य sumit() {
        // If sumit is set on a statistic, combine with others of same name
        std::multimap<std::string, V3Statistic*> byName;
        // * is always first
        प्रति (स्वचालित& itr : s_allStats) {
            V3Statistic* repp = &itr;
            byName.emplace(repp->name(), repp);
        }

        // Process duplicates
        V3Statistic* lastp = शून्यसूचकः;
        प्रति (ध्रुव स्वचालित& itr : byName) {
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
        आकार_t max_विस्तारः = 0;
        std::multimap<std::string, ध्रुव V3Statistic*> byName;
        // * is always first
        प्रति (ध्रुव स्वचालित& itr : s_allStats) {
            ध्रुव V3Statistic* repp = &itr;
            यदि (repp->stage() == "*" && repp->printit()) {
                यदि (max_विस्तारः < repp->name().दीर्घता_2()) max_विस्तारः = repp->name().दीर्घता_2();
                byName.emplace(repp->name(), repp);
            }
        }

        // Print organized by stage
        os << "\nGlobal Statistics:\n\n";
        प्रति (ध्रुव स्वचालित& itr : byName) {
            ध्रुव V3Statistic* repp = itr.second;
            यदि (repp->perf()) अनुवर्त;
            os << "  " << std::वामतः << std::setw(max_विस्तारः) << repp->name();
            repp->dump(os);
            os << '\n';
        }

        // Print organized by stage
        os << "\nPerformance Statistics:\n\n";
        प्रति (ध्रुव स्वचालित& itr : byName) {
            ध्रुव V3Statistic* repp = itr.second;
            यदि (!repp->perf()) अनुवर्त;
            os << "  " << std::वामतः << std::setw(max_विस्तारः) << repp->name();
            repp->dump(os);
            os << '\n';
        }
        os << '\n';
    }

    शून्य stages() {
        os << "Stage Statistics:\n";

        // Find all stages
        पूर्णाङ्क stage = 0;
        आकार_t max_विस्तारः = 0;
        std::सदिशम्<std::string> stages;
        std::unordered_map<string, पूर्णाङ्क> stageInt;
        std::multimap<std::string, ध्रुव V3Statistic*> byName;
        // * is always first
        प्रति (स्वचालित it = s_allStats.begin(); it != s_allStats.अन्तः_2(); ++it) {
            ध्रुव V3Statistic* repp = &(*it);
            यदि (repp->stage() != "*" && repp->printit()) {
                यदि (max_विस्तारः < repp->name().दीर्घता_2()) max_विस्तारः = repp->name().दीर्घता_2();
                ध्रुव स्वचालित it_प्राप्तम्_Pair = stageInt.emplace(repp->stage(), stage);
                यदि (it_प्राप्तम्_Pair.second) {
                    ++stage;
                    stages.प्रक्षिप_पृष्ठभागः(repp->stage());
                }
                byName.emplace(repp->name(), repp);
            }
        }

        //शीर्षकम्
        os << "  Stat     " << std::वामतः << std::setw(max_विस्तारः - 5 - 2) << "";
        प्रति (ध्रुव string& i : stages) os << " " << std::वामतः << std::setw(9) << i;
        os << '\n';
        os << "  -------- " << std::वामतः << std::setw(max_विस्तारः - 5 - 2) << "";
        प्रति (स्वचालित it = stages.begin(); it != stages.अन्तः_2(); ++it) {
            os << " " << std::वामतः << std::setw(9) << "-------";
        }
        // os<<endl;

        // Print organized by stage
        string चरमः_Name = "__NONE__";
        string चरमः_Comma_Name = "__NONE__";
        अचिह्नित col = 0;
        प्रति (स्वचालित it = byName.cbegin(); it != byName.cend(); ++it) {
            ध्रुव V3Statistic* repp = it->second;
            यदि (चरमः_Name != repp->name()) {
                चरमः_Name = repp->name();
                {
                    string commaName = चरमः_Name;
                    string::आकार_type pos = commaName.find(',');
                    यदि (pos != string::npos) commaName.erase(pos);
                    यदि (चरमः_Comma_Name != commaName) {
                        चरमः_Comma_Name = commaName;
                        os << '\n';
                    }
                }
                os << '\n';
                col = 0;
                os << "  " << std::वामतः << std::setw(max_विस्तारः) << repp->name();
            }
            यावत् (col < stages.आकार() && stages.at(col) != repp->stage()) {
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
    स्थिर शून्य सङ्कलय_Stat(ध्रुव V3Statistic& stat) { s_allStats.प्रक्षिप_पृष्ठभागः(stat); }

    स्थिर द्विगुण get_Stat_योग(ध्रुव string& name) {
        // O(n^2) if called a lot; present assumption is only a small call count
        प्रति (ध्रुव V3Statistic& itr : s_allStats) {
            ध्रुव V3Statistic* ध्रुव repp = &itr;
            यदि (repp->name() == name) प्रत्यागम repp->मानम्();
        }
        प्रत्यागम 0.0;
    }

    स्थिर शून्य calculate() { sumit(); }

    // CONSTRUCTORS
    स्पष्ट StatsReport(std::ofstream* aofp)
        : os(*aofp) {  // Need () or GCC 4.8 false warning
        os << "Verilator Statistics Report\n\n";
        V3Stats::info_शीर्षकम्(os, "");
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
    os << " " << std::दक्षिणतः << std::fixed << std::setprecision(precision()) << std::setw(9)
       << मानम्();
}

//######################################################################
// Top Stats class

शून्य V3Stats::सङ्कलय_Stat(ध्रुव V3Statistic& stat) { StatsReport::सङ्कलय_Stat(stat); }

द्विगुण V3Stats::get_Stat_योग(ध्रुव string& name) { प्रत्यागम StatsReport::get_Stat_योग(name); }

शून्य V3Stats::statsStage(ध्रुव string& name) {
    स्थिर द्विगुण s_चरमः_Wall_Time = -1;
    स्थिर पूर्णाङ्क s_file_सङ्ख्या = 0;

    ध्रुव string digitName = V3Global::digitsFilename(++s_file_सङ्ख्या) + "_" + name;

    ध्रुव द्विगुण wallTime = V3Os::timeUsecs() / 1.0e6;
    यदि (s_चरमः_Wall_Time < 0) s_चरमः_Wall_Time = wallTime;
    ध्रुव द्विगुण wallTimeDelta = wallTime - s_चरमः_Wall_Time;
    s_चरमः_Wall_Time = wallTime;
    V3Stats::सङ्कलय_Stat_Perf("Stage, Elapsed time (sec), " + digitName, wallTimeDelta);
    V3Stats::सङ्कलय_Stat_Perf("Stage, Elapsed time (sec), TOTAL", wallTimeDelta);

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_अष्टकानि(memPeak /*ref*/, memCurrent /*ref*/);
    V3Stats::सङ्कलय_Stat_Perf("Stage, Memory current (MB), " + digitName, memCurrent / 1024.0 / 1024.0);
    V3Stats::सङ्कलय_Stat_Perf("Stage, Memory peak (MB), " + digitName, memPeak / 1024.0 / 1024.0);
}

शून्य V3Stats::info_शीर्षकम्(std::ofstream& os, ध्रुव string& उपसर्गः) {
    os << उपसर्गः << "Information:\n";
    os << उपसर्गः << "  Version: " << V3Options::संस्करणम्() << '\n';
    os << उपसर्गः << "  Arguments: " << v3Global.opt.all_तर्काः_String() << '\n';
    os << उपसर्गः << "  Build jobs: " << v3Global.opt.buildJobs() << '\n';
    os << उपसर्गः << "  Verilate jobs: " << v3Global.opt.verilateJobs() << '\n';
}

शून्य V3Stats::statsReport() {
    UINFO(2, __FUNCTION__ << ":");

    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_अष्टकानि(memPeak /*ref*/, memCurrent /*ref*/);
    ध्रुव द्विगुण स्मृतिः = memPeak / 1024.0 / 1024.0;
    V3Stats::सङ्कलय_Stat("Peak Memory Usage (MB)", स्मृतिः);

    // Open stats file
    ध्रुव string filename
        = v3Global.opt.hier_शिखरम्_दत्त_Dir() + "/" + v3Global.opt.उपसर्गः() + "__stats.txt";
    std::ofstream* ofp{V3File::new_ofstream(filename)};
    यदि (ofp->fail()) v3fatal("Can't write file: " << filename);

    { StatsReport{ofp}; }  // Destruct before cleanup

    // Cleanup
    ofp->close();
    VL_DO_DANGLING(विलोपय ofp, ofp);
}

शून्य V3Stats::summaryReport() {
    StatsReport::calculate();
    std::निर्गमः << "- V e r i l a t i o n   R e p o r t: " << V3Options::संस्करणम्() << "\n";

    std::निर्गमः << std::setprecision(3) << std::fixed;
    ध्रुव द्विगुण sourceCharsMB = V3Stats::get_Stat_योग_Q(STAT_SOURCE_CHARS) / 1024.0 / 1024.0;
    ध्रुव द्विगुण cppCharsMB = V3Stats::get_Stat_योग_Q(STAT_CPP_CHARS) / 1024.0 / 1024.0;
    ध्रुव uint64_t sourceModules = V3Stats::get_Stat_योग_Q(STAT_SOURCE_MODULES);
    ध्रुव uint64_t cppFiles = V3Stats::get_Stat_योग_Q(STAT_CPP_FILES);
    ध्रुव द्विगुण modelMB = V3Stats::get_Stat_योग(STAT_MODEL_आकार) / 1024.0 / 1024.0;
    std::निर्गमः << "- Verilator: Built from " << sourceCharsMB << " MB sources in " << sourceModules
              << " modules, into " << cppCharsMB << " MB in " << cppFiles << " C++ files needing "
              << modelMB << " MB\n";

    ध्रुव द्विगुण walltime = V3Stats::get_Stat_योग(STAT_WALLTIME);
    ध्रुव द्विगुण walltimeElab = V3Stats::get_Stat_योग(STAT_WALLTIME_ELAB);
    ध्रुव द्विगुण walltimeCvt = V3Stats::get_Stat_योग(STAT_WALLTIME_CVT);
    ध्रुव द्विगुण walltimeBuild = V3Stats::get_Stat_योग(STAT_WALLTIME_BUILD);
    ध्रुव द्विगुण cputime = V3Stats::get_Stat_योग(STAT_CPUTIME);
    std::निर्गमः << "- Verilator: Walltime " << walltime << " s (elab=" << walltimeElab
              << ", cvt=" << walltimeCvt << ", bld=" << walltimeBuild << "); cpu " << cputime
              << " s on " << std::max(v3Global.opt.verilateJobs(), v3Global.opt.buildJobs())
              << " threads";
    uint64_t memPeak;
    uint64_t memCurrent;
    VlOs::mem_Usage_अष्टकानि(memPeak /*ref*/, memCurrent /*ref*/);
    ध्रुव द्विगुण स्मृतिः = memPeak / 1024.0 / 1024.0;
    यदि (VL_UNCOVERABLE(स्मृतिः != 0.0)) std::निर्गमः << "; allocated " << स्मृतिः << " MB";
    std::निर्गमः << "\n";
}
