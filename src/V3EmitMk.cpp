// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Makefile
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3EmitMk.h"

#ضمن "V3EmitCBase.h"
#ضمن "V3HierBlock.h"
#ضمن "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Groups adjacent files in a list, evenly distributing sum of scores
صنف Emit_ډله final {
عام:
    بنية FileOrConcatenatedFilesList final {
        ثابت std::string m_filename;  // Filename or output group filename if grouping
        std::ویکټور<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        منطقي isConcatenatingFile() ثابت { أرجع !m_concatenatedFilenames.تشه(); }
    };

    بنية FilenameWithScore final {
        ثابت std::string m_filename;  // Input filename
        ثابت uint64_t m_score;  // Input file complexity
        ثابت uint64_t m_اندازه_بایټونه;  // Input file size
    };

خاص:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    بنية WorkList final {
        uint64_t m_ټول_Score = 0;  // Sum of scores of included files
        std::ویکټور<FilenameWithScore> m_files;  // Included filenames
        صحيح m_buckets_عدد
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        منطقي m_isConcatenable = ریښتیا;  // Indicated whether files on this list can be concatenated.
        ثابت صحيح m_dbgId;  // Work list ID for debugging.

        WorkList() = ړنګول;
        څرګند WorkList(صحيح id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    ساكن constexpr اندازه_t MIN_FILES_شمېر = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    ساكن constexpr اندازه_t MIN_FILES_فی_لوښی = 4;

    // MEMBERS

    ثابت std::ویکټور<FilenameWithScore>
        m_ننوت_Files;  // List of filenames from initial AstCFile list
    std::ویکټور<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ثابت uint64_t m_ټول_Score;  // Sum of file scores
    ثابت std::string m_ډله_File_مختاړی;  // Prefix for output group filenames
    ثابت uint64_t m_max_ډله_بایټونه;  // Maximum sum of file sizes in an output group
    std::ویکټور<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::ویکټور<WorkList*> m_concatenable_Lists_By_Desc_اندازه;  // Lists sorted by size, descending
    صحيح m_ډله_عدد = 0;  // Number of output groups created

    Emit_ډله(std::ویکټور<FilenameWithScore> ننوت_Files, uint64_t ټول_Score,
              ثابت std::string& ډله_File_مختاړی, uint64_t max_ډله_بایټونه)
        : m_ننوت_Files{std::move(ننوت_Files)}
        , m_ټول_Score{ټول_Score}
        , m_ډله_File_مختاړی{ډله_File_مختاړی}
        , m_max_ډله_بایټونه{max_ډله_بایټونه} {}

    string ډله_Filename(صحيح id) ثابت {
        أرجع v3Global.opt.مختاړی() + "_" + m_ډله_File_مختاړی + std::منزل_string(id);
    }

    // Debug logging: prints scores histogram
    فراغ dumpLogScoreHistogram(std::ostream& os) {
        constexpr صحيح MAX_BAR_اوږدتیا = 80;
        constexpr صحيح MAX_INTERVALS_عدد = 60;

        // All scores arranged in ascending order.
        std::ویکټور<uint64_t> sortedScores;
        sortedScores.reserve(m_ننوت_Files.اندازه());
        std::transform(m_ننوت_Files.begin(), m_ننوت_Files.پای_ټکی(), std::شا_inserter(sortedScores),
                       [](ثابت FilenameWithScore& ننوت_File) { أرجع ننوت_File.m_score; });
        std::ترتیبول(sortedScores.begin(), sortedScores.پای_ټکی());

        ثابت int64_t سر_Score = sortedScores.شا();
        os << "Top score: " << سر_Score << '\n';
        ثابت صحيح max_Score_پلنوالی = std::منزل_string(سر_Score).اوږدتیا();

        ثابت int64_t intervals_عدد = std::min<int64_t>(سر_Score + 1, MAX_INTERVALS_عدد);

        بنية Interval final {
            uint64_t m_ښکتنی_Bound = 0;  // Lowest score included in this histogram bucket
            صحيح m_اندازه = 0;  // Number of scores that fell into this bucket
        };

        std::ویکټور<Interval> intervals;
        intervals.resize(intervals_عدد);

        intervals[0].m_ښکتنی_Bound = 0;
        لكل (صحيح i = 1; i < intervals_عدد; i++) {
            intervals[i].m_ښکتنی_Bound = (سر_Score + 1) * i / intervals_عدد + 1;
        }

        لكل (ثابت uint64_t score : sortedScores) {
            ثابت بینښه صحيح ivIdx = score * intervals_عدد / (سر_Score + 1);
            ++intervals[ivIdx].m_اندازه;
        }

        صحيح سر_Interval_اندازه = 0;
        لكل (ثابت Interval& iv : intervals)
            سر_Interval_اندازه = std::max(سر_Interval_اندازه, iv.m_اندازه);

        os << "Input files' scores histogram:\n";

        لكل (ثابت Interval& iv : intervals) {
            ثابت صحيح scaled_اندازه = iv.m_اندازه * (MAX_BAR_اوږدتیا + 1) / سر_Interval_اندازه;
            ثابت std::string کرښه = " |" + std::string(scaled_اندازه, '#');

            os << std::setw(max_Score_پلنوالی) << iv.m_ښکتنی_Bound << کرښه << "  " << iv.m_اندازه << '\n';
        }
        os << std::setw(max_Score_پلنوالی) << (سر_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    فراغ dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        لكل (ثابت WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.اندازه()
               << "; total score: " << list.m_ټول_Score << ")\n";
            إذا (debug() >= 6 || list.m_files.اندازه() < 4) {
                // Log all files
                لكل (ثابت FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } وإلا {
                // Log only first and last file
                ثابت FilenameWithScore& لومړی = list.m_files.front();
                ثابت FilenameWithScore& وروستی = list.m_files.شا();
                os << "| + " << لومړی.m_filename << "  (score: " << لومړی.m_score << ")\n";
                os << "| | (... " << (list.m_files.اندازه() - 2) << " files ...)\n";
                os << "| + " << وروستی.m_filename << "  (score: " << وروستی.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    فراغ dumpOutputList(std::ostream& os) ثابت {
        os << "List of output files after execution of concatenation:\n";

        لكل (ثابت FileOrConcatenatedFilesList& ثبت : m_outputFiles) {
            إذا (ثبت.isConcatenatingFile()) {
                os << "+ " << ثبت.m_filename << "  (concatenating file)\n";
                لكل (ثابت string& f : ثبت.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } وإلا {
                os << "+ " << ثبت.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    منطقي fallbackNoGrouping(اندازه_t ننوت_Files_شمېر) {
        ثابت صحيح ټول_Buckets_عدد = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        منطقي groupingRedundant = خطأ;
        إذا (ننوت_Files_شمېر < MIN_FILES_شمېر
            && ننوت_Files_شمېر <= static_cast<اندازه_t>(ټول_Buckets_عدد)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_ننوت_Files.اندازه() << " < "
                                                                   << MIN_FILES_شمېر << ")");
            groupingRedundant = ریښتیا;
        }
        إذا (ننوت_Files_شمېر < (MIN_FILES_فی_لوښی * ټول_Buckets_عدد)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_ننوت_Files.اندازه() << " < " << MIN_FILES_فی_لوښی << " - "
                         << ټول_Buckets_عدد << ")");
            groupingRedundant = ریښتیا;
        }
        إذا (!groupingRedundant) أرجع خطأ;

        m_outputFiles.reserve(m_ننوت_Files.اندازه());
        لكل (ثابت FilenameWithScore& filename : m_ننوت_Files) {
            m_outputFiles.ټېل_شا({std::move(filename.m_filename), {}});
        }
        أرجع ریښتیا;
    }

    فراغ createWorkLists() {
        // Create initial Work Lists.

        ثابت صحيح ټول_Buckets_عدد = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ثابت uint64_t concatenableFileMaxScore = m_ټول_Score / ټول_Buckets_عدد / 2;

        V3Stats::جمع_کړه_Stat("Concatenation max score", concatenableFileMaxScore);
        صحيح بل_Work_List_Id = 0;

        إذا (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        لكل (ثابت FilenameWithScore& ننوت_File : m_ننوت_Files) {
            ثابت منطقي fileIsConcatenable = (ننوت_File.m_score <= concatenableFileMaxScore);
            إذا (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << ننوت_File.m_filename << "  (score: " << ننوت_File.m_score << ")\n";
            V3Stats::جمع_کړه_Stat_مجموعه(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                ننوت_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            إذا (m_workLists.تشه() || m_workLists.شا().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_شا(WorkList{بل_Work_List_Id++});
                m_workLists.شا().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.شا();
            list.m_files.ټېل_شا(
                {ننوت_File.m_filename, ننوت_File.m_score, ننوت_File.m_اندازه_بایټونه});
            list.m_ټول_Score += ننوت_File.m_score;
        }
        إذا (m_logp) *m_logp << '\n';
    }

    فراغ ټاکل_Buckets(uint64_t concatenable_Files_ټول_Score) {
        // Assign buckets to lists

        ثابت اندازه_t ټول_Buckets_عدد = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        إذا (m_concatenable_Lists_By_Desc_اندازه.اندازه() > ټول_Buckets_عدد) {
            // Debugging: Log which work lists will be kept
            إذا (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_اندازه.begin(),
                              m_concatenable_Lists_By_Desc_اندازه.begin() + ټول_Buckets_عدد,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.اندازه()
                                          << "; total score: " << listp->m_ټول_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_اندازه.begin() + ټول_Buckets_عدد,
                          m_concatenable_Lists_By_Desc_اندازه.پای_ټکی(), [&](WorkList* listp) {
                              listp->m_isConcatenable = خطأ;

                              إذا (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.اندازه()
                                          << "; total score: " << listp->m_ټول_Score << ")\n";
                          });
            إذا (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_اندازه.resize(ټول_Buckets_عدد);
            // Recalculate stats
            concatenable_Files_ټول_Score = 0;
            لكل (ثابت WorkList* ثابت listp : m_concatenable_Lists_By_Desc_اندازه) {
                concatenable_Files_ټول_Score += listp->m_ټول_Score;
            }
        }

        ثابت uint64_t ideal_لوښی_Score = concatenable_Files_ټول_Score / ټول_Buckets_عدد;

        V3Stats::جمع_کړه_Stat("Concatenation ideal bucket score", ideal_لوښی_Score);

        إذا (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        صحيح شته_Buckets = v3Global.opt.outputGroups();
        لكل (WorkList* listp : m_concatenable_Lists_By_Desc_اندازه) {
            إذا (شته_Buckets > 0 && ideal_لوښی_Score > 0) {
                listp->m_buckets_عدد = std::min(
                    شته_Buckets, std::max<صحيح>(1, listp->m_ټول_Score / ideal_لوښی_Score));
                شته_Buckets -= listp->m_buckets_عدد;
                إذا (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_عدد << "] Work List #"
                            << listp->m_dbgId << '\n';
            } وإلا {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = خطأ;
                إذا (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::کیڼ << std::setw(4) << listp->m_dbgId
                            << std::ښی << " (excluding from concatenation)\n";
            }
        }
        إذا (m_logp) *m_logp << '\n';
    }

    فراغ buildOutputList() {
        // Assign files to buckets and build final list of files

        // At this point the workLists contains concatenatable file lists separated by one or more
        // non-concatenable file lists. Each concatenatable list has N buckets (where N > 0)
        // assigned to it, which have to be filled with files from this list. Ideally, sum of file
        // scores in every bucket should be the same.
        لكل (WorkList& list : m_workLists) {
            إذا (!list.m_isConcatenable) {
                لكل (ثابت FilenameWithScore& file : list.m_files) {
                    m_outputFiles.ټېل_شا({std::move(file.m_filename), {}});
                }
                استمر;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ثابت uint64_t list_Ideal_لوښی_Score = list.m_ټول_Score / list.m_buckets_عدد;

            std::ویکټور<std::ویکټور<ثابت FilenameWithScore*>> buckets;
            خپلکاری fileIt = list.m_files.cbegin();
            لكل (صحيح i = 0; i < list.m_buckets_عدد; ++i) {
                std::ویکټور<ثابت FilenameWithScore*> لوښی;
                uint64_t لوښی_Score = 0;

                لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                    ثابت uint64_t diffNow
                        = std::مطلق_قیمت(static_cast<int64_t>(list_Ideal_لوښی_Score - لوښی_Score));
                    ثابت uint64_t diffIfAdded = std::مطلق_قیمت(static_cast<int64_t>(
                        list_Ideal_لوښی_Score - لوښی_Score - fileIt->m_score));
                    إذا (لوښی_Score == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        لوښی_Score += fileIt->m_score;
                        لوښی.ټېل_شا(&*fileIt);
                    } وإلا {
                        // Best possible bucket score reached, process next bucket.
                        اكسر;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.ټېل_شا(std::move(لوښی));
            }
            لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.شا().ټېل_شا(&*fileIt);
            }
            لكل (ثابت std::ویکټور<ثابت FilenameWithScore*>& لوښی : buckets) {
                جمع_کړه_لوښی_Groups(لوښی);
            }
        }
    }

    فراغ جمع_کړه_لوښی_Groups(ثابت std::ویکټور<ثابت FilenameWithScore*>& لوښی) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::ویکټور<std::ویکټور<std::string>> groups;
        uint64_t ډله_بایټونه = 0;
        لكل (ثابت FilenameWithScore* ثابت filep : لوښی) {
            إذا (groups.تشه() || ډله_بایټونه + filep->m_اندازه_بایټونه > m_max_ډله_بایټونه) {
                groups.emplace_شا();
                ډله_بایټونه = 0;
            }
            ډله_بایټونه += filep->m_اندازه_بایټونه;
            groups.شا().ټېل_شا(filep->m_filename);
        }
        لكل (std::ویکټور<std::string>& ډله : groups) {
            إذا (ډله.اندازه() == 1) {
                m_outputFiles.ټېل_شا({std::move(ډله.front()), {}});
            } وإلا {
                m_outputFiles.ټېل_شا({ډله_Filename(m_ډله_عدد++), std::move(ډله)});
            }
        }
    }

    فراغ assertFilesSame() ثابت {
        خپلکاری ifIt = m_ننوت_Files.begin();
        خپلکاری د_It = m_outputFiles.begin();
        طالما (ifIt != m_ننوت_Files.پای_ټکی() && د_It != m_outputFiles.پای_ټکی()) {
            إذا (د_It->isConcatenatingFile()) {
                لكل (ثابت string& ocf : د_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_ننوت_Files.پای_ټکی(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++د_It;
            } وإلا {
                UASSERT(ifIt->m_filename == د_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << د_It->m_filename);
                ++ifIt;
                ++د_It;
            }
        }
        UASSERT(ifIt == m_ننوت_Files.پای_ټکی(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(د_It == m_outputFiles.پای_ټکی(),
                "More output files than input files. First extra file: " << د_It->m_filename);
    }

    فراغ process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_ډله_File_مختاړی);
        UINFO(5, "Number of input files: " << m_ننوت_Files.اندازه());
        UINFO(5, "Total score: " << m_ټول_Score);

        ثابت صحيح ټول_Buckets_عدد = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << ټول_Buckets_عدد);
        UASSERT(ټول_Buckets_عدد > 0, "More than 0 buckets required");

        إذا (fallbackNoGrouping(m_ننوت_Files.اندازه())) أرجع;

        إذا (debug() >= 6 || dumpLevel() >= 6) {
            ثابت string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            إذا (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        إذا (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        اندازه_t concatenable_Files_شمېر = 0;
        int64_t concatenable_Files_ټول_Score = 0;

        لكل (WorkList& list : m_workLists) {
            إذا (!list.m_isConcatenable) استمر;

            // "Concatenation" of a single file is pointless
            إذا (list.m_files.اندازه() == 1) {
                list.m_isConcatenable = خطأ;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                استمر;
            }

            concatenable_Files_شمېر += list.m_files.اندازه();
            concatenable_Files_ټول_Score += list.m_ټول_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_اندازه.ټېل_شا(&list);
        }

        إذا (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        إذا (fallbackNoGrouping(concatenable_Files_شمېر)) أرجع;

        std::ترتیبول(m_concatenable_Lists_By_Desc_اندازه.begin(), m_concatenable_Lists_By_Desc_اندازه.پای_ټکی(),
                  [](ثابت WorkList* ap, ثابت WorkList* bp) {
                      // Sort in descending order by number of files
                      إذا (ap->m_files.اندازه() != bp->m_files.اندازه())
                          أرجع (ap->m_files.اندازه() > bp->m_files.اندازه());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      أرجع bp->m_ټول_Score > ap->m_ټول_Score;
                  });

        ټاکل_Buckets(concatenable_Files_ټول_Score);

        buildOutputList();

        إذا (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

عام:
    ساكن std::ویکټور<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::ویکټور<FilenameWithScore> ننوت_Files, uint64_t ټول_Score,
                                ثابت std::string& ډله_File_مختاړی, uint64_t max_ډله_بایټونه) {
        Emit_ډله ډله{std::move(ننوت_Files), ټول_Score, ډله_File_مختاړی, max_ډله_بایټونه};
        ډله.process();
        أرجع ډله.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

صنف EmitMk final {
    کارول FileOrConcatenatedFilesList = Emit_ډله::FileOrConcatenatedFilesList;
    کارول FilenameWithScore = Emit_ډله::FilenameWithScore;

    // MEMBERS
    مزدوج m_put_Class_شمېر = 0;  // Number of classEntries printed

عام:
    // METHODS

    ساكن فراغ emitConcatenatingFile(ثابت FileOrConcatenatedFilesList& ثبت) {
        UASSERT(ثبت.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_بهر_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + ثبت.m_filename + ".cpp"};
        concatenatingFile.puts_سرلیک();
        لكل (ثابت string& file : ثبت.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    ساكن uint64_t file_اندازه(ثابت AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::دوګونی | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        أرجع static_cast<uint64_t>(ifs.tellg());
    }

    فراغ put_Make_Class_ثبت(V3_بهر_Mk_File& د, ثابت string& name) {
        د.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_شمېر;
    }

    فراغ emitClassMake() {
        std::ویکټور<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::ویکټور<FileOrConcatenatedFilesList> vmClassesFastList;
        إذا (v3Global.opt.outputGroups() > 0) {
            std::ویکټور<FilenameWithScore> slowFiles;
            std::ویکټور<FilenameWithScore> fastFiles;
            uint64_t slow_ټول_Score = 0;
            uint64_t fast_ټول_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t سرلیک_بایټونه = 0;

            لكل (Ast_غوټه_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), غوټه_File)) {
                ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                إذا (cfilep && VString::پایونه_With(cfilep->name(), ".h")) {
                    سرلیک_بایټونه += file_اندازه(cfilep);
                }
                إذا (cfilep && cfilep->سرچینه() && cfilep->support() == خطأ) {
                    std::ویکټور<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& ټول_Score = cfilep->slow() ? slow_ټول_Score : fast_ټول_Score;

                    ټول_Score += cfilep->complexityScore();
                    files.ټېل_شا({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_اندازه(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_ډله_بایټونه = std::numeric_limits<uint64_t>::max();
            إذا (v3Global.opt.output_Groups_اندازه()) {
                ثابت uint64_t max_Tu_بایټونه = static_cast<uint64_t>(v3Global.opt.output_Groups_اندازه());
                max_ډله_بایټونه = max_Tu_بایټونه > سرلیک_بایټونه ? max_Tu_بایټونه - سرلیک_بایټونه : 0;
            }

            vmClassesSlowList = Emit_ډله::singleConcatenatedFilesList(
                std::move(slowFiles), slow_ټول_Score, "vm_classes_Slow_", max_ډله_بایټونه);
            vmClassesFastList = Emit_ډله::singleConcatenatedFilesList(
                std::move(fastFiles), fast_ټول_Score, "vm_classes_", max_ډله_بایټونه);
        }

        // Generate the makefile
        V3_بهر_Mk_File د{v3Global.opt.makeDir() + "/" + v3Global.opt.مختاړی() + "_classes.mk"};
        د.puts_سرلیک();
        د.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        د.puts("#\n");
        د.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        د.puts("# See " + v3Global.opt.مختاړی() + ".mk" + " for the caller.\n");

        د.puts("\n### Switches...\n");
        د.puts("# C11 constructs required?  0/1 (always on now)\n");
        د.put_سېټ("VM_C11", "1");
        د.puts("# Timing enabled?  0/1\n");
        د.put_سېټ("VM_TIMING", v3Global.کاروي_Timing() ? "1" : "0");
        د.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        د.put_سېټ("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        د.puts("# Parallel builds?  0/1 (from --output-split)\n");
        د.put_سېټ("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        د.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        د.put_سېټ("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        د.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        د.put_سېټ("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        د.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        د.put_سېټ("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        د.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        د.put_سېټ("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        د.puts("# VPI enabled?  0/1 (from --vpi)\n");
        د.put_سېټ("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        د.puts("\n### Object file lists...\n");
        لكل (صحيح support = 0; support < 3; ++support) {
            لكل (ثابت منطقي& slow : {خطأ, ریښتیا}) {
                إذا (support == 2) {
                    د.puts("# Global classes, need linked once per executable");
                } وإلا إذا (support) {
                    د.puts("# Generated support classes");
                } وإلا {
                    د.puts("# Generated module classes");
                }
                إذا (slow) {
                    د.puts(", non-fast-path, compile with low/medium optimization\n");
                } وإلا {
                    د.puts(", fast-path, compile with highest optimization\n");
                }
                ثابت string موخه_Var = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_شمېر = 0;
                د.puts(موخه_Var + " += \\\n");
                إذا (support == 2 && v3Global.opt.hier_ماشوم()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } وإلا إذا (support == 2 && !slow) {
                    لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_ثبت(د, cpp);
                } وإلا إذا (support == 2 && slow) {
                } وإلا إذا (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ثابت std::ویکټور<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    لكل (ثابت FileOrConcatenatedFilesList& ثبت : list) {
                        إذا (ثبت.isConcatenatingFile()) emitConcatenatingFile(ثبت);
                        put_Make_Class_ثبت(د, ثبت.m_filename);
                    }
                } وإلا {
                    لكل (Ast_غوټه_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), غوټه_File)) {
                        ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                        إذا (cfilep && cfilep->سرچینه() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_ثبت(د, cfilep->name());
                        }
                    }
                }
                د.puts("\n");
                V3Stats::جمع_کړه_Stat("Makefile targets, " + موخه_Var, m_put_Class_شمېر);
            }
        }

        د.puts_سرلیک();
    }

    فراغ emitOverallMake() {
        // Generate the makefile
        V3_بهر_Mk_File د{v3Global.opt.makeDir() + "/" + v3Global.opt.مختاړی() + ".mk"};
        د.puts_سرلیک();
        د.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        د.puts("#\n");
        د.puts("# Execute this makefile from the object directory:\n");
        د.puts("#    make -f " + v3Global.opt.مختاړی() + ".mk" + "\n");
        د.puts("\n");

        إذا (v3Global.opt.exe()) {
            د.puts("default: " + v3Global.opt.exeName() + "\n");
        } وإلا إذا (!v3Global.opt.libCreate().تشه()) {
            د.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } وإلا {
            د.puts("default: lib" + v3Global.opt.مختاړی() + "\n");
        }
        د.puts("\n### Constants...\n");
        د.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        د.put_سېټ("PERL", V3_بهر_Formatter::quoteNameControls(V3Options::getenvPERL()));
        د.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        د.put_سېټ("PYTHON3", V3_بهر_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        د.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        د.put_سېټ("VERILATOR_ROOT",
                  V3_بهر_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_ریښه()));
        د.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        د.put_سېټ_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        د.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        د.put_سېټ_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        إذا (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        د.puts("\n### Switches...\n");
        د.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        د.put_سېټ("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        د.puts("# SystemC output mode?  0/1 (from --sc)\n");
        د.put_سېټ("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        د.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        د.put_سېټ("VM_SP_OR_SC", "$(VM_SC)");
        د.puts("# Deprecated\n");
        د.put_سېټ("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        د.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        د.put_سېټ("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        د.puts("\n### Vars...\n");
        د.puts("# Module prefix (from --prefix)\n");
        د.put_سېټ("VM_MODPREFIX", v3Global.opt.mod_مختاړی());
        د.puts("# Design prefix (from --prefix)\n");
        د.put_سېټ("VM_PREFIX", v3Global.opt.مختاړی());
        د.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        د.put_سېټ("VM_PREFIX_INCLUDE", "<" + v3Global.opt.مختاړی() + ".h>");
        إذا (v3Global.dpi()) {
            د.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            د.put_سېټ("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.مختاړی() + "__Dpi.h>");
        }

        د.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        د.puts("VM_USER_CFLAGS = \\\n");
        ثابت std::string solver = V3Options::getenvVERILATOR_SOLVER();
        إذا (v3Global.useRandomizeMethods() && solver != "")
            د.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3_بهر_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        إذا (!v3Global.opt.libCreate().تشه()) د.puts("\t-fPIC \\\n");
        ثابت VStringList& cFlags = v3Global.opt.cFlags();
        لكل (ثابت string& i : cFlags) د.puts("  " + i + " \\\n");
        د.puts("\n");

        د.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        د.puts("VM_USER_LDLIBS = \\\n");
        ثابت VStringList& ldLibs = v3Global.opt.ldLibs();
        لكل (ثابت string& i : ldLibs) د.puts("  " + i + " \\\n");
        د.puts("\n");

        د.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        د.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        د.puts("VM_USER_LDLIBS_FILES = \\\n");
        لكل (ثابت string& i : v3Global.opt.ldLibFiles()) د.puts("  " + i + " \\\n");
        د.puts("\n");

        VString_سېټ dirs;
        د.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        د.puts("VM_USER_CLASSES = \\\n");
        ثابت VString_سېټ& cppFiles = v3Global.opt.cppFiles();
        لكل (ثابت خپلکاری& cppfile : cppFiles) {
            د.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ثابت string dir
                = V3Os::filename_Relative_لار(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.ورننباسه(dir);
        }
        dirs.ورننباسه(V3Os::filename_Relative_لار(".", v3Global.opt.makeDir()));

        د.puts("\n");

        د.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        د.puts("VM_USER_DIR = \\\n");
        لكل (ثابت خپلکاری& i : dirs) د.puts("  " + i + " \\\n");
        د.puts("\n");

        د.puts("### Default rules...\n");
        د.puts("# Include list of all generated classes\n");
        د.puts("include " + v3Global.opt.مختاړی() + "_classes.mk\n");
        إذا (v3Global.opt.hier_سر()) {
            د.puts("# Include rules for hierarchical blocks\n");
            د.puts("include " + v3Global.opt.مختاړی() + "_hier.mk\n");
        }
        د.puts("# Include global rules\n");
        د.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        إذا (v3Global.opt.exe()) {
            د.puts("\n### Executable rules... (from --exe)\n");
            د.puts("VPATH += $(VM_USER_DIR)\n");
            د.puts("\n");

            إذا (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                د.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                د.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                د.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                د.puts("\n");
            }
        }

        ثابت string compilerIncludePch
            = v3Global.opt.compilerIncludes().تشه() ? "" : "$(VK_PCH_H).fast.gch";
        ثابت string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().تشه() ? "" : "$(VK_PCH_I_FAST)";
        لكل (ثابت string& cppfile : cppFiles) {
            ثابت string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            د.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            د.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        إذا (v3Global.opt.exe()) {
            د.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            د.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            د.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            د.puts("\n");
        } وإلا إذا (!v3Global.opt.libCreate().تشه()) {
            ثابت string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            د.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            د.puts(v3Global.opt.libCreateName(خطأ) + ": " + libCreateDeps + "\n");
            د.puts("\n");
            إذا (v3Global.opt.hier_ماشوم()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                د.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + "\n");
            } وإلا {
                د.puts(v3Global.opt.libCreateName(ریښتیا) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                د.puts("ifeq ($(shell uname -s),Darwin)\n");
                د.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                د.puts("else\n");
                د.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                د.puts("endif\n");
                د.puts("\n");
                د.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + " " + v3Global.opt.libCreateName(ریښتیا) + "\n");
            }
        } وإلا {
            ثابت string libname = "lib" + v3Global.opt.مختاړی() + ".a";
            د.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            د.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            د.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            د.puts("lib" + v3Global.opt.مختاړی() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        د.puts_سرلیک();
    }

    څرګند EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    مجازي ~EmitMk() = افتراضي;
};

//######################################################################

صنف EmitMkHierVerilation final {
    ثابت V3_Hier_ګراف* ثابت m_graphp;
    ثابت string m_makefile;  // path of this makefile
    فراغ emitCommonOpts(V3_بهر_Mk_File& د) ثابت {
        د.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        د.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        د.puts("# Common options for hierarchical blocks\n");
        ثابت string fullpath_bin = V3Os::filename_Real_لار(v3Global.opt.buildDepBin());
        ثابت string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        د.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        د.puts("VM_HIER_INPUT_FILES := \\\n");
        لكل (ثابت خپلکاری& i : v3Global.opt.vFiles())
            د.puts("  " + V3Os::filename_Real_لار(i.filename()) + " \\\n");
        د.puts("\n");
        د.puts("VM_HIER_VERILOG_LIBS := \\\n");
        لكل (ثابت خپلکاری& i : v3Global.opt.libraryFiles()) {
            د.puts("  " + V3Os::filename_Real_لار(i.filename()) + " \\\n");
        }
        د.puts("\n");
    }
    فراغ emitLaunchVerilator(V3_بهر_Mk_File& د, ثابت string& دلیلونه_File) ثابت {
        د.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        د.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + دلیلونه_File + "\"\n");
    }
    فراغ emit(V3_بهر_Mk_File& د) ثابت {
        د.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        د.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        د.puts("#\n");
        د.puts("# The main makefile " + v3Global.opt.مختاړی() + ".mk calls this makefile\n");
        د.puts("\n");
        د.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        د.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        د.puts(".SUFFIXES:\n");
        د.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        د.puts("# Libraries of hierarchical blocks\n");
        د.puts("VM_HIER_LIBS := \\\n");
        لكل (ثابت V3_ګراف_راس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
            د.puts("  " + blockp->hierLibFilename(ریښتیا) + " \\\n");
        }
        د.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        د.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.مختاړی() + ".mk\n");
        د.puts("\t$(MAKE) -f " + v3Global.opt.مختاړی() + ".mk\n");
        د.puts("hier_verilation: " + v3Global.opt.مختاړی() + ".mk\n");
        emitCommonOpts(د);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        د.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        د.puts("hier_launch_verilator:\n");
        د.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ثابت string دلیلونه_File = v3Global.hierGraphp()->سر_Command_دلیلونه_Filename(خطأ);
            د.puts("\n# Verilate the top module\n");
            د.puts(v3Global.opt.مختاړی()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            د.puts(V3Os::filenameNonDir(دلیلونه_File) + " ");
            لكل (ثابت V3_ګراف_راس& vtx : m_graphp->vertices()) {
                ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
                د.puts(blockp->hierWrapperFilename(ریښتیا) + " ");
            }
            د.puts("\n");
            emitLaunchVerilator(د, دلیلونه_File);
        }

        // Rules to process hierarchical blocks
        د.puts("\n# Verilate hierarchical blocks\n");
        لكل (ثابت V3_ګراف_راس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
            ثابت string مختاړی = blockp->hier_مختاړی();
            ثابت string دلیلونه_Filename = blockp->command_دلیلونه_Filename(خطأ);
            د.puts(blockp->hierGeneratedFilenames(ریښتیا));
            د.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            د.puts(V3Os::filenameNonDir(دلیلونه_Filename) + " ");
            لكل (ثابت V3_ګراف_څنډه& څنډه : blockp->بهر_څنډې()) {
                ثابت V3_Hier_بلاک* ثابت dependencyp = څنډه.سر()->as<V3_Hier_بلاک>();
                د.puts(dependencyp->hierWrapperFilename(ریښتیا) + " ");
            }
            د.puts("\n");
            emitLaunchVerilator(د, دلیلونه_Filename);

            // Rule to build lib*.a
            د.puts(blockp->hierLibFilename(ریښتیا));
            د.puts(": ");
            د.puts(blockp->hierMkFilename(ریښتیا));
            د.puts(" ");
            لكل (ثابت V3_ګراف_څنډه& څنډه : blockp->بهر_څنډې()) {
                ثابت V3_Hier_بلاک* ثابت dependencyp = څنډه.سر()->as<V3_Hier_بلاک>();
                د.puts(dependencyp->hierLibFilename(ریښتیا));
                د.puts(" ");
            }
            د.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(خطأ) + " -C " + مختاړی);
            د.puts(" VM_PREFIX=" + مختاړی);
            د.puts("\n\n");
        }
        د.puts("endif  # Guard\n");
    }

عام:
    څرګند EmitMkHierVerilation(ثابت V3_Hier_ګراف* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.مختاړی() + "_hier.mk"} {
        V3_بهر_Mk_File د{m_makefile};
        emit(د);
    }
};

//######################################################################
// Gate class functions

فراغ V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت EmitMk emitter;
}

فراغ V3EmitMk::emitHierVerilation(ثابت V3_Hier_ګراف* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
