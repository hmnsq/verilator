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
صنف Emit_گروه final {
عام:
    بنية FileOrConcatenatedFilesList final {
        ثابت std::string m_filename;  // Filename or output group filename if grouping
        std::وکتور<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        منطقي isConcatenatingFile() ثابت { أرجع !m_concatenatedFilenames.خالی(); }
    };

    بنية FilenameWithScore final {
        ثابت std::string m_filename;  // Input filename
        ثابت uint64_t m_score;  // Input file complexity
        ثابت uint64_t m_اندازه_بایتها;  // Input file size
    };

خاص:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    بنية WorkList final {
        uint64_t m_مجموع_Score = 0;  // Sum of scores of included files
        std::وکتور<FilenameWithScore> m_files;  // Included filenames
        صحيح m_buckets_رقم
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        منطقي m_isConcatenable = درست;  // Indicated whether files on this list can be concatenated.
        ثابت صحيح m_dbgId;  // Work list ID for debugging.

        WorkList() = حذف;
        صریح WorkList(صحيح id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    ساكن constexpr اندازه_t MIN_FILES_شمار = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    ساكن constexpr اندازه_t MIN_FILES_در_هر_سطل = 4;

    // MEMBERS

    ثابت std::وکتور<FilenameWithScore>
        m_درون_داد_Files;  // List of filenames from initial AstCFile list
    std::وکتور<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ثابت uint64_t m_مجموع_Score;  // Sum of file scores
    ثابت std::string m_گروه_File_پیشوند;  // Prefix for output group filenames
    ثابت uint64_t m_max_گروه_بایتها;  // Maximum sum of file sizes in an output group
    std::وکتور<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::وکتور<WorkList*> m_concatenable_Lists_By_Desc_اندازه;  // Lists sorted by size, descending
    صحيح m_گروه_رقم = 0;  // Number of output groups created

    Emit_گروه(std::وکتور<FilenameWithScore> درون_داد_Files, uint64_t مجموع_Score,
              ثابت std::string& گروه_File_پیشوند, uint64_t max_گروه_بایتها)
        : m_درون_داد_Files{std::move(درون_داد_Files)}
        , m_مجموع_Score{مجموع_Score}
        , m_گروه_File_پیشوند{گروه_File_پیشوند}
        , m_max_گروه_بایتها{max_گروه_بایتها} {}

    string گروه_Filename(صحيح id) ثابت {
        أرجع v3Global.opt.پیشوند() + "_" + m_گروه_File_پیشوند + std::مقصد_string(id);
    }

    // Debug logging: prints scores histogram
    فراغ dumpLogScoreHistogram(std::ostream& os) {
        constexpr صحيح MAX_BAR_درازی = 80;
        constexpr صحيح MAX_INTERVALS_رقم = 60;

        // All scores arranged in ascending order.
        std::وکتور<uint64_t> sortedScores;
        sortedScores.reserve(m_درون_داد_Files.اندازه());
        std::transform(m_درون_داد_Files.begin(), m_درون_داد_Files.انتها(), std::پشت_inserter(sortedScores),
                       [](ثابت FilenameWithScore& درون_داد_File) { أرجع درون_داد_File.m_score; });
        std::مرتب_سازی(sortedScores.begin(), sortedScores.انتها());

        ثابت int64_t بالاترین_Score = sortedScores.پشت();
        os << "Top score: " << بالاترین_Score << '\n';
        ثابت صحيح max_Score_عرض = std::مقصد_string(بالاترین_Score).درازی();

        ثابت int64_t intervals_رقم = std::min<int64_t>(بالاترین_Score + 1, MAX_INTERVALS_رقم);

        بنية Interval final {
            uint64_t m_پایینی_Bound = 0;  // Lowest score included in this histogram bucket
            صحيح m_اندازه = 0;  // Number of scores that fell into this bucket
        };

        std::وکتور<Interval> intervals;
        intervals.resize(intervals_رقم);

        intervals[0].m_پایینی_Bound = 0;
        لكل (صحيح i = 1; i < intervals_رقم; i++) {
            intervals[i].m_پایینی_Bound = (بالاترین_Score + 1) * i / intervals_رقم + 1;
        }

        لكل (ثابت uint64_t score : sortedScores) {
            ثابت بیعلامت صحيح ivIdx = score * intervals_رقم / (بالاترین_Score + 1);
            ++intervals[ivIdx].m_اندازه;
        }

        صحيح بالاترین_Interval_اندازه = 0;
        لكل (ثابت Interval& iv : intervals)
            بالاترین_Interval_اندازه = std::max(بالاترین_Interval_اندازه, iv.m_اندازه);

        os << "Input files' scores histogram:\n";

        لكل (ثابت Interval& iv : intervals) {
            ثابت صحيح scaled_اندازه = iv.m_اندازه * (MAX_BAR_درازی + 1) / بالاترین_Interval_اندازه;
            ثابت std::string سطر = " |" + std::string(scaled_اندازه, '#');

            os << std::setw(max_Score_عرض) << iv.m_پایینی_Bound << سطر << "  " << iv.m_اندازه << '\n';
        }
        os << std::setw(max_Score_عرض) << (بالاترین_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    فراغ dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        لكل (ثابت WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.اندازه()
               << "; total score: " << list.m_مجموع_Score << ")\n";
            إذا (debug() >= 6 || list.m_files.اندازه() < 4) {
                // Log all files
                لكل (ثابت FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } وإلا {
                // Log only first and last file
                ثابت FilenameWithScore& اولی = list.m_files.front();
                ثابت FilenameWithScore& آخرین = list.m_files.پشت();
                os << "| + " << اولی.m_filename << "  (score: " << اولی.m_score << ")\n";
                os << "| | (... " << (list.m_files.اندازه() - 2) << " files ...)\n";
                os << "| + " << آخرین.m_filename << "  (score: " << آخرین.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    فراغ dumpOutputList(std::ostream& os) ثابت {
        os << "List of output files after execution of concatenation:\n";

        لكل (ثابت FileOrConcatenatedFilesList& مدخل : m_outputFiles) {
            إذا (مدخل.isConcatenatingFile()) {
                os << "+ " << مدخل.m_filename << "  (concatenating file)\n";
                لكل (ثابت string& f : مدخل.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } وإلا {
                os << "+ " << مدخل.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    منطقي fallbackNoGrouping(اندازه_t درون_داد_Files_شمار) {
        ثابت صحيح مجموع_Buckets_رقم = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        منطقي groupingRedundant = خطأ;
        إذا (درون_داد_Files_شمار < MIN_FILES_شمار
            && درون_داد_Files_شمار <= static_cast<اندازه_t>(مجموع_Buckets_رقم)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_درون_داد_Files.اندازه() << " < "
                                                                   << MIN_FILES_شمار << ")");
            groupingRedundant = درست;
        }
        إذا (درون_داد_Files_شمار < (MIN_FILES_در_هر_سطل * مجموع_Buckets_رقم)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_درون_داد_Files.اندازه() << " < " << MIN_FILES_در_هر_سطل << " - "
                         << مجموع_Buckets_رقم << ")");
            groupingRedundant = درست;
        }
        إذا (!groupingRedundant) أرجع خطأ;

        m_outputFiles.reserve(m_درون_داد_Files.اندازه());
        لكل (ثابت FilenameWithScore& filename : m_درون_داد_Files) {
            m_outputFiles.افزودن_پشت({std::move(filename.m_filename), {}});
        }
        أرجع درست;
    }

    فراغ createWorkLists() {
        // Create initial Work Lists.

        ثابت صحيح مجموع_Buckets_رقم = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ثابت uint64_t concatenableFileMaxScore = m_مجموع_Score / مجموع_Buckets_رقم / 2;

        V3Stats::جمع_کن_Stat("Concatenation max score", concatenableFileMaxScore);
        صحيح بعدی_Work_List_Id = 0;

        إذا (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        لكل (ثابت FilenameWithScore& درون_داد_File : m_درون_داد_Files) {
            ثابت منطقي fileIsConcatenable = (درون_داد_File.m_score <= concatenableFileMaxScore);
            إذا (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << درون_داد_File.m_filename << "  (score: " << درون_داد_File.m_score << ")\n";
            V3Stats::جمع_کن_Stat_جمع(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                درون_داد_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            إذا (m_workLists.خالی() || m_workLists.پشت().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_پشت(WorkList{بعدی_Work_List_Id++});
                m_workLists.پشت().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.پشت();
            list.m_files.افزودن_پشت(
                {درون_داد_File.m_filename, درون_داد_File.m_score, درون_داد_File.m_اندازه_بایتها});
            list.m_مجموع_Score += درون_داد_File.m_score;
        }
        إذا (m_logp) *m_logp << '\n';
    }

    فراغ تخصیص_بده_Buckets(uint64_t concatenable_Files_مجموع_Score) {
        // Assign buckets to lists

        ثابت اندازه_t مجموع_Buckets_رقم = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        إذا (m_concatenable_Lists_By_Desc_اندازه.اندازه() > مجموع_Buckets_رقم) {
            // Debugging: Log which work lists will be kept
            إذا (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_اندازه.begin(),
                              m_concatenable_Lists_By_Desc_اندازه.begin() + مجموع_Buckets_رقم,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.اندازه()
                                          << "; total score: " << listp->m_مجموع_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_اندازه.begin() + مجموع_Buckets_رقم,
                          m_concatenable_Lists_By_Desc_اندازه.انتها(), [&](WorkList* listp) {
                              listp->m_isConcatenable = خطأ;

                              إذا (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.اندازه()
                                          << "; total score: " << listp->m_مجموع_Score << ")\n";
                          });
            إذا (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_اندازه.resize(مجموع_Buckets_رقم);
            // Recalculate stats
            concatenable_Files_مجموع_Score = 0;
            لكل (ثابت WorkList* ثابت listp : m_concatenable_Lists_By_Desc_اندازه) {
                concatenable_Files_مجموع_Score += listp->m_مجموع_Score;
            }
        }

        ثابت uint64_t ideal_سطل_Score = concatenable_Files_مجموع_Score / مجموع_Buckets_رقم;

        V3Stats::جمع_کن_Stat("Concatenation ideal bucket score", ideal_سطل_Score);

        إذا (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        صحيح موجود_Buckets = v3Global.opt.outputGroups();
        لكل (WorkList* listp : m_concatenable_Lists_By_Desc_اندازه) {
            إذا (موجود_Buckets > 0 && ideal_سطل_Score > 0) {
                listp->m_buckets_رقم = std::min(
                    موجود_Buckets, std::max<صحيح>(1, listp->m_مجموع_Score / ideal_سطل_Score));
                موجود_Buckets -= listp->m_buckets_رقم;
                إذا (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_رقم << "] Work List #"
                            << listp->m_dbgId << '\n';
            } وإلا {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = خطأ;
                إذا (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::چپ << std::setw(4) << listp->m_dbgId
                            << std::راست << " (excluding from concatenation)\n";
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
                    m_outputFiles.افزودن_پشت({std::move(file.m_filename), {}});
                }
                استمر;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ثابت uint64_t list_Ideal_سطل_Score = list.m_مجموع_Score / list.m_buckets_رقم;

            std::وکتور<std::وکتور<ثابت FilenameWithScore*>> buckets;
            خودکار fileIt = list.m_files.cbegin();
            لكل (صحيح i = 0; i < list.m_buckets_رقم; ++i) {
                std::وکتور<ثابت FilenameWithScore*> سطل;
                uint64_t سطل_Score = 0;

                لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                    ثابت uint64_t diffNow
                        = std::قدر_مطلق(static_cast<int64_t>(list_Ideal_سطل_Score - سطل_Score));
                    ثابت uint64_t diffIfAdded = std::قدر_مطلق(static_cast<int64_t>(
                        list_Ideal_سطل_Score - سطل_Score - fileIt->m_score));
                    إذا (سطل_Score == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        سطل_Score += fileIt->m_score;
                        سطل.افزودن_پشت(&*fileIt);
                    } وإلا {
                        // Best possible bucket score reached, process next bucket.
                        اكسر;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.افزودن_پشت(std::move(سطل));
            }
            لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.پشت().افزودن_پشت(&*fileIt);
            }
            لكل (ثابت std::وکتور<ثابت FilenameWithScore*>& سطل : buckets) {
                جمع_کن_سطل_Groups(سطل);
            }
        }
    }

    فراغ جمع_کن_سطل_Groups(ثابت std::وکتور<ثابت FilenameWithScore*>& سطل) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::وکتور<std::وکتور<std::string>> groups;
        uint64_t گروه_بایتها = 0;
        لكل (ثابت FilenameWithScore* ثابت filep : سطل) {
            إذا (groups.خالی() || گروه_بایتها + filep->m_اندازه_بایتها > m_max_گروه_بایتها) {
                groups.emplace_پشت();
                گروه_بایتها = 0;
            }
            گروه_بایتها += filep->m_اندازه_بایتها;
            groups.پشت().افزودن_پشت(filep->m_filename);
        }
        لكل (std::وکتور<std::string>& گروه : groups) {
            إذا (گروه.اندازه() == 1) {
                m_outputFiles.افزودن_پشت({std::move(گروه.front()), {}});
            } وإلا {
                m_outputFiles.افزودن_پشت({گروه_Filename(m_گروه_رقم++), std::move(گروه)});
            }
        }
    }

    فراغ assertFilesSame() ثابت {
        خودکار ifIt = m_درون_داد_Files.begin();
        خودکار از_It = m_outputFiles.begin();
        طالما (ifIt != m_درون_داد_Files.انتها() && از_It != m_outputFiles.انتها()) {
            إذا (از_It->isConcatenatingFile()) {
                لكل (ثابت string& ocf : از_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_درون_داد_Files.انتها(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++از_It;
            } وإلا {
                UASSERT(ifIt->m_filename == از_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << از_It->m_filename);
                ++ifIt;
                ++از_It;
            }
        }
        UASSERT(ifIt == m_درون_داد_Files.انتها(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(از_It == m_outputFiles.انتها(),
                "More output files than input files. First extra file: " << از_It->m_filename);
    }

    فراغ process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_گروه_File_پیشوند);
        UINFO(5, "Number of input files: " << m_درون_داد_Files.اندازه());
        UINFO(5, "Total score: " << m_مجموع_Score);

        ثابت صحيح مجموع_Buckets_رقم = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << مجموع_Buckets_رقم);
        UASSERT(مجموع_Buckets_رقم > 0, "More than 0 buckets required");

        إذا (fallbackNoGrouping(m_درون_داد_Files.اندازه())) أرجع;

        إذا (debug() >= 6 || dumpLevel() >= 6) {
            ثابت string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            إذا (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        إذا (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        اندازه_t concatenable_Files_شمار = 0;
        int64_t concatenable_Files_مجموع_Score = 0;

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

            concatenable_Files_شمار += list.m_files.اندازه();
            concatenable_Files_مجموع_Score += list.m_مجموع_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_اندازه.افزودن_پشت(&list);
        }

        إذا (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        إذا (fallbackNoGrouping(concatenable_Files_شمار)) أرجع;

        std::مرتب_سازی(m_concatenable_Lists_By_Desc_اندازه.begin(), m_concatenable_Lists_By_Desc_اندازه.انتها(),
                  [](ثابت WorkList* ap, ثابت WorkList* bp) {
                      // Sort in descending order by number of files
                      إذا (ap->m_files.اندازه() != bp->m_files.اندازه())
                          أرجع (ap->m_files.اندازه() > bp->m_files.اندازه());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      أرجع bp->m_مجموع_Score > ap->m_مجموع_Score;
                  });

        تخصیص_بده_Buckets(concatenable_Files_مجموع_Score);

        buildOutputList();

        إذا (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

عام:
    ساكن std::وکتور<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::وکتور<FilenameWithScore> درون_داد_Files, uint64_t مجموع_Score,
                                ثابت std::string& گروه_File_پیشوند, uint64_t max_گروه_بایتها) {
        Emit_گروه گروه{std::move(درون_داد_Files), مجموع_Score, گروه_File_پیشوند, max_گروه_بایتها};
        گروه.process();
        أرجع گروه.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

صنف EmitMk final {
    استفاده FileOrConcatenatedFilesList = Emit_گروه::FileOrConcatenatedFilesList;
    استفاده FilenameWithScore = Emit_گروه::FilenameWithScore;

    // MEMBERS
    مزدوج m_put_Class_شمار = 0;  // Number of classEntries printed

عام:
    // METHODS

    ساكن فراغ emitConcatenatingFile(ثابت FileOrConcatenatedFilesList& مدخل) {
        UASSERT(مدخل.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_بیرون_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + مدخل.m_filename + ".cpp"};
        concatenatingFile.puts_سرآیند();
        لكل (ثابت string& file : مدخل.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    ساكن uint64_t file_اندازه(ثابت AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::دودویی | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        أرجع static_cast<uint64_t>(ifs.tellg());
    }

    فراغ put_Make_Class_مدخل(V3_بیرون_Mk_File& از, ثابت string& name) {
        از.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_شمار;
    }

    فراغ emitClassMake() {
        std::وکتور<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::وکتور<FileOrConcatenatedFilesList> vmClassesFastList;
        إذا (v3Global.opt.outputGroups() > 0) {
            std::وکتور<FilenameWithScore> slowFiles;
            std::وکتور<FilenameWithScore> fastFiles;
            uint64_t slow_مجموع_Score = 0;
            uint64_t fast_مجموع_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t سرآیند_بایتها = 0;

            لكل (Ast_گره_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), گره_File)) {
                ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                إذا (cfilep && VString::دوسر_With(cfilep->name(), ".h")) {
                    سرآیند_بایتها += file_اندازه(cfilep);
                }
                إذا (cfilep && cfilep->منبع() && cfilep->support() == خطأ) {
                    std::وکتور<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& مجموع_Score = cfilep->slow() ? slow_مجموع_Score : fast_مجموع_Score;

                    مجموع_Score += cfilep->complexityScore();
                    files.افزودن_پشت({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_اندازه(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_گروه_بایتها = std::numeric_limits<uint64_t>::max();
            إذا (v3Global.opt.output_Groups_اندازه()) {
                ثابت uint64_t max_Tu_بایتها = static_cast<uint64_t>(v3Global.opt.output_Groups_اندازه());
                max_گروه_بایتها = max_Tu_بایتها > سرآیند_بایتها ? max_Tu_بایتها - سرآیند_بایتها : 0;
            }

            vmClassesSlowList = Emit_گروه::singleConcatenatedFilesList(
                std::move(slowFiles), slow_مجموع_Score, "vm_classes_Slow_", max_گروه_بایتها);
            vmClassesFastList = Emit_گروه::singleConcatenatedFilesList(
                std::move(fastFiles), fast_مجموع_Score, "vm_classes_", max_گروه_بایتها);
        }

        // Generate the makefile
        V3_بیرون_Mk_File از{v3Global.opt.makeDir() + "/" + v3Global.opt.پیشوند() + "_classes.mk"};
        از.puts_سرآیند();
        از.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        از.puts("#\n");
        از.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        از.puts("# See " + v3Global.opt.پیشوند() + ".mk" + " for the caller.\n");

        از.puts("\n### Switches...\n");
        از.puts("# C11 constructs required?  0/1 (always on now)\n");
        از.put_مجموعه("VM_C11", "1");
        از.puts("# Timing enabled?  0/1\n");
        از.put_مجموعه("VM_TIMING", v3Global.استفاده_میکند_Timing() ? "1" : "0");
        از.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        از.put_مجموعه("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        از.puts("# Parallel builds?  0/1 (from --output-split)\n");
        از.put_مجموعه("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        از.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        از.put_مجموعه("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        از.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        از.put_مجموعه("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        از.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        از.put_مجموعه("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        از.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        از.put_مجموعه("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        از.puts("# VPI enabled?  0/1 (from --vpi)\n");
        از.put_مجموعه("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        از.puts("\n### Object file lists...\n");
        لكل (صحيح support = 0; support < 3; ++support) {
            لكل (ثابت منطقي& slow : {خطأ, درست}) {
                إذا (support == 2) {
                    از.puts("# Global classes, need linked once per executable");
                } وإلا إذا (support) {
                    از.puts("# Generated support classes");
                } وإلا {
                    از.puts("# Generated module classes");
                }
                إذا (slow) {
                    از.puts(", non-fast-path, compile with low/medium optimization\n");
                } وإلا {
                    از.puts(", fast-path, compile with highest optimization\n");
                }
                ثابت string هدف_Var = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_شمار = 0;
                از.puts(هدف_Var + " += \\\n");
                إذا (support == 2 && v3Global.opt.hier_فرزند()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } وإلا إذا (support == 2 && !slow) {
                    لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_مدخل(از, cpp);
                } وإلا إذا (support == 2 && slow) {
                } وإلا إذا (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ثابت std::وکتور<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    لكل (ثابت FileOrConcatenatedFilesList& مدخل : list) {
                        إذا (مدخل.isConcatenatingFile()) emitConcatenatingFile(مدخل);
                        put_Make_Class_مدخل(از, مدخل.m_filename);
                    }
                } وإلا {
                    لكل (Ast_گره_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), گره_File)) {
                        ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                        إذا (cfilep && cfilep->منبع() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_مدخل(از, cfilep->name());
                        }
                    }
                }
                از.puts("\n");
                V3Stats::جمع_کن_Stat("Makefile targets, " + هدف_Var, m_put_Class_شمار);
            }
        }

        از.puts_سرآیند();
    }

    فراغ emitOverallMake() {
        // Generate the makefile
        V3_بیرون_Mk_File از{v3Global.opt.makeDir() + "/" + v3Global.opt.پیشوند() + ".mk"};
        از.puts_سرآیند();
        از.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        از.puts("#\n");
        از.puts("# Execute this makefile from the object directory:\n");
        از.puts("#    make -f " + v3Global.opt.پیشوند() + ".mk" + "\n");
        از.puts("\n");

        إذا (v3Global.opt.exe()) {
            از.puts("default: " + v3Global.opt.exeName() + "\n");
        } وإلا إذا (!v3Global.opt.libCreate().خالی()) {
            از.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } وإلا {
            از.puts("default: lib" + v3Global.opt.پیشوند() + "\n");
        }
        از.puts("\n### Constants...\n");
        از.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        از.put_مجموعه("PERL", V3_بیرون_Formatter::quoteNameControls(V3Options::getenvPERL()));
        از.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        از.put_مجموعه("PYTHON3", V3_بیرون_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        از.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        از.put_مجموعه("VERILATOR_ROOT",
                  V3_بیرون_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_ریشه()));
        از.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        از.put_مجموعه_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        از.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        از.put_مجموعه_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        إذا (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        از.puts("\n### Switches...\n");
        از.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        از.put_مجموعه("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        از.puts("# SystemC output mode?  0/1 (from --sc)\n");
        از.put_مجموعه("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        از.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        از.put_مجموعه("VM_SP_OR_SC", "$(VM_SC)");
        از.puts("# Deprecated\n");
        از.put_مجموعه("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        از.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        از.put_مجموعه("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        از.puts("\n### Vars...\n");
        از.puts("# Module prefix (from --prefix)\n");
        از.put_مجموعه("VM_MODPREFIX", v3Global.opt.mod_پیشوند());
        از.puts("# Design prefix (from --prefix)\n");
        از.put_مجموعه("VM_PREFIX", v3Global.opt.پیشوند());
        از.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        از.put_مجموعه("VM_PREFIX_INCLUDE", "<" + v3Global.opt.پیشوند() + ".h>");
        إذا (v3Global.dpi()) {
            از.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            از.put_مجموعه("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.پیشوند() + "__Dpi.h>");
        }

        از.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        از.puts("VM_USER_CFLAGS = \\\n");
        ثابت std::string solver = V3Options::getenvVERILATOR_SOLVER();
        إذا (v3Global.useRandomizeMethods() && solver != "")
            از.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3_بیرون_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        إذا (!v3Global.opt.libCreate().خالی()) از.puts("\t-fPIC \\\n");
        ثابت VStringList& cFlags = v3Global.opt.cFlags();
        لكل (ثابت string& i : cFlags) از.puts("  " + i + " \\\n");
        از.puts("\n");

        از.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        از.puts("VM_USER_LDLIBS = \\\n");
        ثابت VStringList& ldLibs = v3Global.opt.ldLibs();
        لكل (ثابت string& i : ldLibs) از.puts("  " + i + " \\\n");
        از.puts("\n");

        از.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        از.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        از.puts("VM_USER_LDLIBS_FILES = \\\n");
        لكل (ثابت string& i : v3Global.opt.ldLibFiles()) از.puts("  " + i + " \\\n");
        از.puts("\n");

        VString_مجموعه dirs;
        از.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        از.puts("VM_USER_CLASSES = \\\n");
        ثابت VString_مجموعه& cppFiles = v3Global.opt.cppFiles();
        لكل (ثابت خودکار& cppfile : cppFiles) {
            از.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ثابت string dir
                = V3Os::filename_Relative_مسیر(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.درج(dir);
        }
        dirs.درج(V3Os::filename_Relative_مسیر(".", v3Global.opt.makeDir()));

        از.puts("\n");

        از.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        از.puts("VM_USER_DIR = \\\n");
        لكل (ثابت خودکار& i : dirs) از.puts("  " + i + " \\\n");
        از.puts("\n");

        از.puts("### Default rules...\n");
        از.puts("# Include list of all generated classes\n");
        از.puts("include " + v3Global.opt.پیشوند() + "_classes.mk\n");
        إذا (v3Global.opt.hier_بالاترین()) {
            از.puts("# Include rules for hierarchical blocks\n");
            از.puts("include " + v3Global.opt.پیشوند() + "_hier.mk\n");
        }
        از.puts("# Include global rules\n");
        از.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        إذا (v3Global.opt.exe()) {
            از.puts("\n### Executable rules... (from --exe)\n");
            از.puts("VPATH += $(VM_USER_DIR)\n");
            از.puts("\n");

            إذا (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                از.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                از.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                از.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                از.puts("\n");
            }
        }

        ثابت string compilerIncludePch
            = v3Global.opt.compilerIncludes().خالی() ? "" : "$(VK_PCH_H).fast.gch";
        ثابت string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().خالی() ? "" : "$(VK_PCH_I_FAST)";
        لكل (ثابت string& cppfile : cppFiles) {
            ثابت string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            از.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            از.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        إذا (v3Global.opt.exe()) {
            از.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            از.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            از.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            از.puts("\n");
        } وإلا إذا (!v3Global.opt.libCreate().خالی()) {
            ثابت string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            از.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            از.puts(v3Global.opt.libCreateName(خطأ) + ": " + libCreateDeps + "\n");
            از.puts("\n");
            إذا (v3Global.opt.hier_فرزند()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                از.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + "\n");
            } وإلا {
                از.puts(v3Global.opt.libCreateName(درست) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                از.puts("ifeq ($(shell uname -s),Darwin)\n");
                از.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                از.puts("else\n");
                از.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                از.puts("endif\n");
                از.puts("\n");
                از.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + " " + v3Global.opt.libCreateName(درست) + "\n");
            }
        } وإلا {
            ثابت string libname = "lib" + v3Global.opt.پیشوند() + ".a";
            از.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            از.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            از.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            از.puts("lib" + v3Global.opt.پیشوند() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        از.puts_سرآیند();
    }

    صریح EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    مجازی ~EmitMk() = افتراضي;
};

//######################################################################

صنف EmitMkHierVerilation final {
    ثابت V3_Hier_گراف* ثابت m_graphp;
    ثابت string m_makefile;  // path of this makefile
    فراغ emitCommonOpts(V3_بیرون_Mk_File& از) ثابت {
        از.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        از.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        از.puts("# Common options for hierarchical blocks\n");
        ثابت string fullpath_bin = V3Os::filename_Real_مسیر(v3Global.opt.buildDepBin());
        ثابت string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        از.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        از.puts("VM_HIER_INPUT_FILES := \\\n");
        لكل (ثابت خودکار& i : v3Global.opt.vFiles())
            از.puts("  " + V3Os::filename_Real_مسیر(i.filename()) + " \\\n");
        از.puts("\n");
        از.puts("VM_HIER_VERILOG_LIBS := \\\n");
        لكل (ثابت خودکار& i : v3Global.opt.libraryFiles()) {
            از.puts("  " + V3Os::filename_Real_مسیر(i.filename()) + " \\\n");
        }
        از.puts("\n");
    }
    فراغ emitLaunchVerilator(V3_بیرون_Mk_File& از, ثابت string& آرگومانها_File) ثابت {
        از.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        از.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + آرگومانها_File + "\"\n");
    }
    فراغ emit(V3_بیرون_Mk_File& از) ثابت {
        از.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        از.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        از.puts("#\n");
        از.puts("# The main makefile " + v3Global.opt.پیشوند() + ".mk calls this makefile\n");
        از.puts("\n");
        از.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        از.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        از.puts(".SUFFIXES:\n");
        از.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        از.puts("# Libraries of hierarchical blocks\n");
        از.puts("VM_HIER_LIBS := \\\n");
        لكل (ثابت V3_گراف_رأس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلوک* ثابت blockp = vtx.as<V3_Hier_بلوک>();
            از.puts("  " + blockp->hierLibFilename(درست) + " \\\n");
        }
        از.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        از.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.پیشوند() + ".mk\n");
        از.puts("\t$(MAKE) -f " + v3Global.opt.پیشوند() + ".mk\n");
        از.puts("hier_verilation: " + v3Global.opt.پیشوند() + ".mk\n");
        emitCommonOpts(از);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        از.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        از.puts("hier_launch_verilator:\n");
        از.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ثابت string آرگومانها_File = v3Global.hierGraphp()->بالاترین_Command_آرگومانها_Filename(خطأ);
            از.puts("\n# Verilate the top module\n");
            از.puts(v3Global.opt.پیشوند()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            از.puts(V3Os::filenameNonDir(آرگومانها_File) + " ");
            لكل (ثابت V3_گراف_رأس& vtx : m_graphp->vertices()) {
                ثابت V3_Hier_بلوک* ثابت blockp = vtx.as<V3_Hier_بلوک>();
                از.puts(blockp->hierWrapperFilename(درست) + " ");
            }
            از.puts("\n");
            emitLaunchVerilator(از, آرگومانها_File);
        }

        // Rules to process hierarchical blocks
        از.puts("\n# Verilate hierarchical blocks\n");
        لكل (ثابت V3_گراف_رأس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلوک* ثابت blockp = vtx.as<V3_Hier_بلوک>();
            ثابت string پیشوند = blockp->hier_پیشوند();
            ثابت string آرگومانها_Filename = blockp->command_آرگومانها_Filename(خطأ);
            از.puts(blockp->hierGeneratedFilenames(درست));
            از.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            از.puts(V3Os::filenameNonDir(آرگومانها_Filename) + " ");
            لكل (ثابت V3_گراف_یال& یال : blockp->بیرون_یالها()) {
                ثابت V3_Hier_بلوک* ثابت dependencyp = یال.بالاترین()->as<V3_Hier_بلوک>();
                از.puts(dependencyp->hierWrapperFilename(درست) + " ");
            }
            از.puts("\n");
            emitLaunchVerilator(از, آرگومانها_Filename);

            // Rule to build lib*.a
            از.puts(blockp->hierLibFilename(درست));
            از.puts(": ");
            از.puts(blockp->hierMkFilename(درست));
            از.puts(" ");
            لكل (ثابت V3_گراف_یال& یال : blockp->بیرون_یالها()) {
                ثابت V3_Hier_بلوک* ثابت dependencyp = یال.بالاترین()->as<V3_Hier_بلوک>();
                از.puts(dependencyp->hierLibFilename(درست));
                از.puts(" ");
            }
            از.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(خطأ) + " -C " + پیشوند);
            از.puts(" VM_PREFIX=" + پیشوند);
            از.puts("\n\n");
        }
        از.puts("endif  # Guard\n");
    }

عام:
    صریح EmitMkHierVerilation(ثابت V3_Hier_گراف* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.پیشوند() + "_hier.mk"} {
        V3_بیرون_Mk_File از{m_makefile};
        emit(از);
    }
};

//######################################################################
// Gate class functions

فراغ V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت EmitMk emitter;
}

فراغ V3EmitMk::emitHierVerilation(ثابت V3_Hier_گراف* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
