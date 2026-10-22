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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3EmitMk.h"

#شامل "V3EmitCBase.h"
#شامل "V3HierBlock.h"
#شامل "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Groups adjacent files in a list, evenly distributing sum of scores
جماعت Emit_گروہ final {
عوامی:
    ساخت FileOrConcatenatedFilesList final {
        ثابت std::string m_filename;  // Filename or output group filename if grouping
        std::سمتیہ<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        منطقی isConcatenatingFile() ثابت { واپس !m_concatenatedFilenames.خالی_خانے(); }
    };

    ساخت FilenameWithScore final {
        ثابت std::string m_filename;  // Input filename
        ثابت uint64_t m_score;  // Input file complexity
        ثابت uint64_t m_سائز_بائٹیں;  // Input file size
    };

نجی:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    ساخت WorkList final {
        uint64_t m_کل_Score = 0;  // Sum of scores of included files
        std::سمتیہ<FilenameWithScore> m_files;  // Included filenames
        صحیح m_buckets_عدد
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        منطقی m_isConcatenable = سچ;  // Indicated whether files on this list can be concatenated.
        ثابت صحیح m_dbgId;  // Work list ID for debugging.

        WorkList() = حذف;
        صریح WorkList(صحیح id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    ساکن constexpr سائز_t MIN_FILES_گنتی = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    ساکن constexpr سائز_t MIN_FILES_فی_بالٹی = 4;

    // MEMBERS

    ثابت std::سمتیہ<FilenameWithScore>
        m_داخلہ_Files;  // List of filenames from initial AstCFile list
    std::سمتیہ<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ثابت uint64_t m_کل_Score;  // Sum of file scores
    ثابت std::string m_گروہ_File_سابقہ;  // Prefix for output group filenames
    ثابت uint64_t m_max_گروہ_بائٹیں;  // Maximum sum of file sizes in an output group
    std::سمتیہ<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::سمتیہ<WorkList*> m_concatenable_Lists_By_Desc_سائز;  // Lists sorted by size, descending
    صحیح m_گروہ_عدد = 0;  // Number of output groups created

    Emit_گروہ(std::سمتیہ<FilenameWithScore> داخلہ_Files, uint64_t کل_Score,
              ثابت std::string& گروہ_File_سابقہ, uint64_t max_گروہ_بائٹیں)
        : m_داخلہ_Files{std::move(داخلہ_Files)}
        , m_کل_Score{کل_Score}
        , m_گروہ_File_سابقہ{گروہ_File_سابقہ}
        , m_max_گروہ_بائٹیں{max_گروہ_بائٹیں} {}

    string گروہ_Filename(صحیح id) ثابت {
        واپس v3Global.opt.سابقہ() + "_" + m_گروہ_File_سابقہ + std::مقصد_string(id);
    }

    // Debug logging: prints scores histogram
    خالی dumpLogScoreHistogram(std::ostream& os) {
        constexpr صحیح MAX_BAR_لمبائی = 80;
        constexpr صحیح MAX_INTERVALS_عدد = 60;

        // All scores arranged in ascending order.
        std::سمتیہ<uint64_t> sortedScores;
        sortedScores.reserve(m_داخلہ_Files.سائز());
        std::transform(m_داخلہ_Files.begin(), m_داخلہ_Files.انتہا(), std::عقب_inserter(sortedScores),
                       [](ثابت FilenameWithScore& داخلہ_File) { واپس داخلہ_File.m_score; });
        std::ترتیب_دو(sortedScores.begin(), sortedScores.انتہا());

        ثابت int64_t چوٹی_Score = sortedScores.عقب();
        os << "Top score: " << چوٹی_Score << '\n';
        ثابت صحیح max_Score_چوڑائی = std::مقصد_string(چوٹی_Score).لمبائی_2();

        ثابت int64_t intervals_عدد = std::min<int64_t>(چوٹی_Score + 1, MAX_INTERVALS_عدد);

        ساخت Interval final {
            uint64_t m_زیریں_Bound = 0;  // Lowest score included in this histogram bucket
            صحیح m_سائز = 0;  // Number of scores that fell into this bucket
        };

        std::سمتیہ<Interval> intervals;
        intervals.resize(intervals_عدد);

        intervals[0].m_زیریں_Bound = 0;
        برائے (صحیح i = 1; i < intervals_عدد; i++) {
            intervals[i].m_زیریں_Bound = (چوٹی_Score + 1) * i / intervals_عدد + 1;
        }

        برائے (ثابت uint64_t score : sortedScores) {
            ثابت غیرعلامتی صحیح ivIdx = score * intervals_عدد / (چوٹی_Score + 1);
            ++intervals[ivIdx].m_سائز;
        }

        صحیح چوٹی_Interval_سائز = 0;
        برائے (ثابت Interval& iv : intervals)
            چوٹی_Interval_سائز = std::max(چوٹی_Interval_سائز, iv.m_سائز);

        os << "Input files' scores histogram:\n";

        برائے (ثابت Interval& iv : intervals) {
            ثابت صحیح scaled_سائز = iv.m_سائز * (MAX_BAR_لمبائی + 1) / چوٹی_Interval_سائز;
            ثابت std::string سطر = " |" + std::string(scaled_سائز, '#');

            os << std::setw(max_Score_چوڑائی) << iv.m_زیریں_Bound << سطر << "  " << iv.m_سائز << '\n';
        }
        os << std::setw(max_Score_چوڑائی) << (چوٹی_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    خالی dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        برائے (ثابت WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.سائز()
               << "; total score: " << list.m_کل_Score << ")\n";
            اگر (debug() >= 6 || list.m_files.سائز() < 4) {
                // Log all files
                برائے (ثابت FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } ورنہ {
                // Log only first and last file
                ثابت FilenameWithScore& پہلا = list.m_files.front();
                ثابت FilenameWithScore& آخری = list.m_files.عقب();
                os << "| + " << پہلا.m_filename << "  (score: " << پہلا.m_score << ")\n";
                os << "| | (... " << (list.m_files.سائز() - 2) << " files ...)\n";
                os << "| + " << آخری.m_filename << "  (score: " << آخری.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    خالی dumpOutputList(std::ostream& os) ثابت {
        os << "List of output files after execution of concatenation:\n";

        برائے (ثابت FileOrConcatenatedFilesList& اندراج_2 : m_outputFiles) {
            اگر (اندراج_2.isConcatenatingFile()) {
                os << "+ " << اندراج_2.m_filename << "  (concatenating file)\n";
                برائے (ثابت string& f : اندراج_2.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } ورنہ {
                os << "+ " << اندراج_2.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    منطقی fallbackNoGrouping(سائز_t داخلہ_Files_گنتی) {
        ثابت صحیح کل_Buckets_عدد = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        منطقی groupingRedundant = جھوٹ;
        اگر (داخلہ_Files_گنتی < MIN_FILES_گنتی
            && داخلہ_Files_گنتی <= static_cast<سائز_t>(کل_Buckets_عدد)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_داخلہ_Files.سائز() << " < "
                                                                   << MIN_FILES_گنتی << ")");
            groupingRedundant = سچ;
        }
        اگر (داخلہ_Files_گنتی < (MIN_FILES_فی_بالٹی * کل_Buckets_عدد)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_داخلہ_Files.سائز() << " < " << MIN_FILES_فی_بالٹی << " - "
                         << کل_Buckets_عدد << ")");
            groupingRedundant = سچ;
        }
        اگر (!groupingRedundant) واپس جھوٹ;

        m_outputFiles.reserve(m_داخلہ_Files.سائز());
        برائے (ثابت FilenameWithScore& filename : m_داخلہ_Files) {
            m_outputFiles.ڈالو_عقب({std::move(filename.m_filename), {}});
        }
        واپس سچ;
    }

    خالی createWorkLists() {
        // Create initial Work Lists.

        ثابت صحیح کل_Buckets_عدد = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ثابت uint64_t concatenableFileMaxScore = m_کل_Score / کل_Buckets_عدد / 2;

        V3Stats::جوڑو_Stat("Concatenation max score", concatenableFileMaxScore);
        صحیح اگلا_Work_List_Id = 0;

        اگر (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        برائے (ثابت FilenameWithScore& داخلہ_File : m_داخلہ_Files) {
            ثابت منطقی fileIsConcatenable = (داخلہ_File.m_score <= concatenableFileMaxScore);
            اگر (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << داخلہ_File.m_filename << "  (score: " << داخلہ_File.m_score << ")\n";
            V3Stats::جوڑو_Stat_مجموع(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                داخلہ_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            اگر (m_workLists.خالی_خانے() || m_workLists.عقب().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_عقب(WorkList{اگلا_Work_List_Id++});
                m_workLists.عقب().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.عقب();
            list.m_files.ڈالو_عقب(
                {داخلہ_File.m_filename, داخلہ_File.m_score, داخلہ_File.m_سائز_بائٹیں});
            list.m_کل_Score += داخلہ_File.m_score;
        }
        اگر (m_logp) *m_logp << '\n';
    }

    خالی تفویض_Buckets(uint64_t concatenable_Files_کل_Score) {
        // Assign buckets to lists

        ثابت سائز_t کل_Buckets_عدد = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        اگر (m_concatenable_Lists_By_Desc_سائز.سائز() > کل_Buckets_عدد) {
            // Debugging: Log which work lists will be kept
            اگر (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_سائز.begin(),
                              m_concatenable_Lists_By_Desc_سائز.begin() + کل_Buckets_عدد,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.سائز()
                                          << "; total score: " << listp->m_کل_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_سائز.begin() + کل_Buckets_عدد,
                          m_concatenable_Lists_By_Desc_سائز.انتہا(), [&](WorkList* listp) {
                              listp->m_isConcatenable = جھوٹ;

                              اگر (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.سائز()
                                          << "; total score: " << listp->m_کل_Score << ")\n";
                          });
            اگر (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_سائز.resize(کل_Buckets_عدد);
            // Recalculate stats
            concatenable_Files_کل_Score = 0;
            برائے (ثابت WorkList* ثابت listp : m_concatenable_Lists_By_Desc_سائز) {
                concatenable_Files_کل_Score += listp->m_کل_Score;
            }
        }

        ثابت uint64_t ideal_بالٹی_Score = concatenable_Files_کل_Score / کل_Buckets_عدد;

        V3Stats::جوڑو_Stat("Concatenation ideal bucket score", ideal_بالٹی_Score);

        اگر (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        صحیح دستیاب_Buckets = v3Global.opt.outputGroups();
        برائے (WorkList* listp : m_concatenable_Lists_By_Desc_سائز) {
            اگر (دستیاب_Buckets > 0 && ideal_بالٹی_Score > 0) {
                listp->m_buckets_عدد = std::min(
                    دستیاب_Buckets, std::max<صحیح>(1, listp->m_کل_Score / ideal_بالٹی_Score));
                دستیاب_Buckets -= listp->m_buckets_عدد;
                اگر (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_عدد << "] Work List #"
                            << listp->m_dbgId << '\n';
            } ورنہ {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = جھوٹ;
                اگر (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::بائیں << std::setw(4) << listp->m_dbgId
                            << std::دائیں << " (excluding from concatenation)\n";
            }
        }
        اگر (m_logp) *m_logp << '\n';
    }

    خالی buildOutputList() {
        // Assign files to buckets and build final list of files

        // At this point the workLists contains concatenatable file lists separated by one or more
        // non-concatenable file lists. Each concatenatable list has N buckets (where N > 0)
        // assigned to it, which have to be filled with files from this list. Ideally, sum of file
        // scores in every bucket should be the same.
        برائے (WorkList& list : m_workLists) {
            اگر (!list.m_isConcatenable) {
                برائے (ثابت FilenameWithScore& file : list.m_files) {
                    m_outputFiles.ڈالو_عقب({std::move(file.m_filename), {}});
                }
                جاری;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ثابت uint64_t list_Ideal_بالٹی_Score = list.m_کل_Score / list.m_buckets_عدد;

            std::سمتیہ<std::سمتیہ<ثابت FilenameWithScore*>> buckets;
            خودکار fileIt = list.m_files.cbegin();
            برائے (صحیح i = 0; i < list.m_buckets_عدد; ++i) {
                std::سمتیہ<ثابت FilenameWithScore*> بالٹی;
                uint64_t بالٹی_Score = 0;

                برائے (; fileIt != list.m_files.cend(); ++fileIt) {
                    ثابت uint64_t diffNow
                        = std::مطلق_قدر(static_cast<int64_t>(list_Ideal_بالٹی_Score - بالٹی_Score));
                    ثابت uint64_t diffIfAdded = std::مطلق_قدر(static_cast<int64_t>(
                        list_Ideal_بالٹی_Score - بالٹی_Score - fileIt->m_score));
                    اگر (بالٹی_Score == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        بالٹی_Score += fileIt->m_score;
                        بالٹی.ڈالو_عقب(&*fileIt);
                    } ورنہ {
                        // Best possible bucket score reached, process next bucket.
                        توڑو;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.ڈالو_عقب(std::move(بالٹی));
            }
            برائے (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.عقب().ڈالو_عقب(&*fileIt);
            }
            برائے (ثابت std::سمتیہ<ثابت FilenameWithScore*>& بالٹی : buckets) {
                جوڑو_بالٹی_Groups(بالٹی);
            }
        }
    }

    خالی جوڑو_بالٹی_Groups(ثابت std::سمتیہ<ثابت FilenameWithScore*>& بالٹی) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::سمتیہ<std::سمتیہ<std::string>> groups;
        uint64_t گروہ_بائٹیں = 0;
        برائے (ثابت FilenameWithScore* ثابت filep : بالٹی) {
            اگر (groups.خالی_خانے() || گروہ_بائٹیں + filep->m_سائز_بائٹیں > m_max_گروہ_بائٹیں) {
                groups.emplace_عقب();
                گروہ_بائٹیں = 0;
            }
            گروہ_بائٹیں += filep->m_سائز_بائٹیں;
            groups.عقب().ڈالو_عقب(filep->m_filename);
        }
        برائے (std::سمتیہ<std::string>& گروہ : groups) {
            اگر (گروہ.سائز() == 1) {
                m_outputFiles.ڈالو_عقب({std::move(گروہ.front()), {}});
            } ورنہ {
                m_outputFiles.ڈالو_عقب({گروہ_Filename(m_گروہ_عدد++), std::move(گروہ)});
            }
        }
    }

    خالی assertFilesSame() ثابت {
        خودکار ifIt = m_داخلہ_Files.begin();
        خودکار کا_It = m_outputFiles.begin();
        جب (ifIt != m_داخلہ_Files.انتہا() && کا_It != m_outputFiles.انتہا()) {
            اگر (کا_It->isConcatenatingFile()) {
                برائے (ثابت string& ocf : کا_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_داخلہ_Files.انتہا(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++کا_It;
            } ورنہ {
                UASSERT(ifIt->m_filename == کا_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << کا_It->m_filename);
                ++ifIt;
                ++کا_It;
            }
        }
        UASSERT(ifIt == m_داخلہ_Files.انتہا(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(کا_It == m_outputFiles.انتہا(),
                "More output files than input files. First extra file: " << کا_It->m_filename);
    }

    خالی process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_گروہ_File_سابقہ);
        UINFO(5, "Number of input files: " << m_داخلہ_Files.سائز());
        UINFO(5, "Total score: " << m_کل_Score);

        ثابت صحیح کل_Buckets_عدد = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << کل_Buckets_عدد);
        UASSERT(کل_Buckets_عدد > 0, "More than 0 buckets required");

        اگر (fallbackNoGrouping(m_داخلہ_Files.سائز())) واپس;

        اگر (debug() >= 6 || dumpLevel() >= 6) {
            ثابت string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            اگر (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        اگر (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        سائز_t concatenable_Files_گنتی = 0;
        int64_t concatenable_Files_کل_Score = 0;

        برائے (WorkList& list : m_workLists) {
            اگر (!list.m_isConcatenable) جاری;

            // "Concatenation" of a single file is pointless
            اگر (list.m_files.سائز() == 1) {
                list.m_isConcatenable = جھوٹ;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                جاری;
            }

            concatenable_Files_گنتی += list.m_files.سائز();
            concatenable_Files_کل_Score += list.m_کل_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_سائز.ڈالو_عقب(&list);
        }

        اگر (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        اگر (fallbackNoGrouping(concatenable_Files_گنتی)) واپس;

        std::ترتیب_دو(m_concatenable_Lists_By_Desc_سائز.begin(), m_concatenable_Lists_By_Desc_سائز.انتہا(),
                  [](ثابت WorkList* ap, ثابت WorkList* bp) {
                      // Sort in descending order by number of files
                      اگر (ap->m_files.سائز() != bp->m_files.سائز())
                          واپس (ap->m_files.سائز() > bp->m_files.سائز());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      واپس bp->m_کل_Score > ap->m_کل_Score;
                  });

        تفویض_Buckets(concatenable_Files_کل_Score);

        buildOutputList();

        اگر (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

عوامی:
    ساکن std::سمتیہ<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::سمتیہ<FilenameWithScore> داخلہ_Files, uint64_t کل_Score,
                                ثابت std::string& گروہ_File_سابقہ, uint64_t max_گروہ_بائٹیں) {
        Emit_گروہ گروہ{std::move(داخلہ_Files), کل_Score, گروہ_File_سابقہ, max_گروہ_بائٹیں};
        گروہ.process();
        واپس گروہ.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

جماعت EmitMk final {
    استعمال FileOrConcatenatedFilesList = Emit_گروہ::FileOrConcatenatedFilesList;
    استعمال FilenameWithScore = Emit_گروہ::FilenameWithScore;

    // MEMBERS
    دوہرا m_put_Class_گنتی = 0;  // Number of classEntries printed

عوامی:
    // METHODS

    ساکن خالی emitConcatenatingFile(ثابت FileOrConcatenatedFilesList& اندراج_2) {
        UASSERT(اندراج_2.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_برآمد_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + اندراج_2.m_filename + ".cpp"};
        concatenatingFile.puts_سرخی();
        برائے (ثابت string& file : اندراج_2.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    ساکن uint64_t file_سائز(ثابت AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::ثنائی | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        واپس static_cast<uint64_t>(ifs.tellg());
    }

    خالی put_Make_Class_اندراج(V3_برآمد_Mk_File& کا, ثابت string& name) {
        کا.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_گنتی;
    }

    خالی emitClassMake() {
        std::سمتیہ<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::سمتیہ<FileOrConcatenatedFilesList> vmClassesFastList;
        اگر (v3Global.opt.outputGroups() > 0) {
            std::سمتیہ<FilenameWithScore> slowFiles;
            std::سمتیہ<FilenameWithScore> fastFiles;
            uint64_t slow_کل_Score = 0;
            uint64_t fast_کل_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t سرخی_بائٹیں = 0;

            برائے (Ast_گرہ_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), گرہ_File)) {
                ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                اگر (cfilep && VString::سرے_With(cfilep->name(), ".h")) {
                    سرخی_بائٹیں += file_سائز(cfilep);
                }
                اگر (cfilep && cfilep->ماخذ() && cfilep->support() == جھوٹ) {
                    std::سمتیہ<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& کل_Score = cfilep->slow() ? slow_کل_Score : fast_کل_Score;

                    کل_Score += cfilep->complexityScore();
                    files.ڈالو_عقب({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_سائز(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_گروہ_بائٹیں = std::numeric_limits<uint64_t>::max();
            اگر (v3Global.opt.output_Groups_سائز()) {
                ثابت uint64_t max_Tu_بائٹیں = static_cast<uint64_t>(v3Global.opt.output_Groups_سائز());
                max_گروہ_بائٹیں = max_Tu_بائٹیں > سرخی_بائٹیں ? max_Tu_بائٹیں - سرخی_بائٹیں : 0;
            }

            vmClassesSlowList = Emit_گروہ::singleConcatenatedFilesList(
                std::move(slowFiles), slow_کل_Score, "vm_classes_Slow_", max_گروہ_بائٹیں);
            vmClassesFastList = Emit_گروہ::singleConcatenatedFilesList(
                std::move(fastFiles), fast_کل_Score, "vm_classes_", max_گروہ_بائٹیں);
        }

        // Generate the makefile
        V3_برآمد_Mk_File کا{v3Global.opt.makeDir() + "/" + v3Global.opt.سابقہ() + "_classes.mk"};
        کا.puts_سرخی();
        کا.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        کا.puts("#\n");
        کا.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        کا.puts("# See " + v3Global.opt.سابقہ() + ".mk" + " for the caller.\n");

        کا.puts("\n### Switches...\n");
        کا.puts("# C11 constructs required?  0/1 (always on now)\n");
        کا.put_سیٹ("VM_C11", "1");
        کا.puts("# Timing enabled?  0/1\n");
        کا.put_سیٹ("VM_TIMING", v3Global.استعمال_کرتا_Timing() ? "1" : "0");
        کا.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        کا.put_سیٹ("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        کا.puts("# Parallel builds?  0/1 (from --output-split)\n");
        کا.put_سیٹ("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        کا.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        کا.put_سیٹ("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        کا.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        کا.put_سیٹ("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        کا.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        کا.put_سیٹ("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        کا.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        کا.put_سیٹ("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        کا.puts("# VPI enabled?  0/1 (from --vpi)\n");
        کا.put_سیٹ("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        کا.puts("\n### Object file lists...\n");
        برائے (صحیح support = 0; support < 3; ++support) {
            برائے (ثابت منطقی& slow : {جھوٹ, سچ}) {
                اگر (support == 2) {
                    کا.puts("# Global classes, need linked once per executable");
                } ورنہ اگر (support) {
                    کا.puts("# Generated support classes");
                } ورنہ {
                    کا.puts("# Generated module classes");
                }
                اگر (slow) {
                    کا.puts(", non-fast-path, compile with low/medium optimization\n");
                } ورنہ {
                    کا.puts(", fast-path, compile with highest optimization\n");
                }
                ثابت string ہدف_Var = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_گنتی = 0;
                کا.puts(ہدف_Var + " += \\\n");
                اگر (support == 2 && v3Global.opt.hier_بچہ()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } ورنہ اگر (support == 2 && !slow) {
                    برائے (ثابت string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_اندراج(کا, cpp);
                } ورنہ اگر (support == 2 && slow) {
                } ورنہ اگر (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ثابت std::سمتیہ<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    برائے (ثابت FileOrConcatenatedFilesList& اندراج_2 : list) {
                        اگر (اندراج_2.isConcatenatingFile()) emitConcatenatingFile(اندراج_2);
                        put_Make_Class_اندراج(کا, اندراج_2.m_filename);
                    }
                } ورنہ {
                    برائے (Ast_گرہ_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), گرہ_File)) {
                        ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                        اگر (cfilep && cfilep->ماخذ() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_اندراج(کا, cfilep->name());
                        }
                    }
                }
                کا.puts("\n");
                V3Stats::جوڑو_Stat("Makefile targets, " + ہدف_Var, m_put_Class_گنتی);
            }
        }

        کا.puts_سرخی();
    }

    خالی emitOverallMake() {
        // Generate the makefile
        V3_برآمد_Mk_File کا{v3Global.opt.makeDir() + "/" + v3Global.opt.سابقہ() + ".mk"};
        کا.puts_سرخی();
        کا.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        کا.puts("#\n");
        کا.puts("# Execute this makefile from the object directory:\n");
        کا.puts("#    make -f " + v3Global.opt.سابقہ() + ".mk" + "\n");
        کا.puts("\n");

        اگر (v3Global.opt.exe()) {
            کا.puts("default: " + v3Global.opt.exeName() + "\n");
        } ورنہ اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            کا.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } ورنہ {
            کا.puts("default: lib" + v3Global.opt.سابقہ() + "\n");
        }
        کا.puts("\n### Constants...\n");
        کا.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        کا.put_سیٹ("PERL", V3_برآمد_Formatter::quoteNameControls(V3Options::getenvPERL()));
        کا.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        کا.put_سیٹ("PYTHON3", V3_برآمد_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        کا.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        کا.put_سیٹ("VERILATOR_ROOT",
                  V3_برآمد_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_جڑ()));
        کا.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        کا.put_سیٹ_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        کا.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        کا.put_سیٹ_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        اگر (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        کا.puts("\n### Switches...\n");
        کا.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        کا.put_سیٹ("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        کا.puts("# SystemC output mode?  0/1 (from --sc)\n");
        کا.put_سیٹ("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        کا.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        کا.put_سیٹ("VM_SP_OR_SC", "$(VM_SC)");
        کا.puts("# Deprecated\n");
        کا.put_سیٹ("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        کا.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        کا.put_سیٹ("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        کا.puts("\n### Vars...\n");
        کا.puts("# Module prefix (from --prefix)\n");
        کا.put_سیٹ("VM_MODPREFIX", v3Global.opt.mod_سابقہ());
        کا.puts("# Design prefix (from --prefix)\n");
        کا.put_سیٹ("VM_PREFIX", v3Global.opt.سابقہ());
        کا.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        کا.put_سیٹ("VM_PREFIX_INCLUDE", "<" + v3Global.opt.سابقہ() + ".h>");
        اگر (v3Global.dpi()) {
            کا.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            کا.put_سیٹ("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.سابقہ() + "__Dpi.h>");
        }

        کا.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        کا.puts("VM_USER_CFLAGS = \\\n");
        ثابت std::string solver = V3Options::getenvVERILATOR_SOLVER();
        اگر (v3Global.useRandomizeMethods() && solver != "")
            کا.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3_برآمد_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        اگر (!v3Global.opt.libCreate().خالی_خانے()) کا.puts("\t-fPIC \\\n");
        ثابت VStringList& cFlags = v3Global.opt.cFlags();
        برائے (ثابت string& i : cFlags) کا.puts("  " + i + " \\\n");
        کا.puts("\n");

        کا.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        کا.puts("VM_USER_LDLIBS = \\\n");
        ثابت VStringList& ldLibs = v3Global.opt.ldLibs();
        برائے (ثابت string& i : ldLibs) کا.puts("  " + i + " \\\n");
        کا.puts("\n");

        کا.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        کا.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        کا.puts("VM_USER_LDLIBS_FILES = \\\n");
        برائے (ثابت string& i : v3Global.opt.ldLibFiles()) کا.puts("  " + i + " \\\n");
        کا.puts("\n");

        VString_سیٹ dirs;
        کا.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        کا.puts("VM_USER_CLASSES = \\\n");
        ثابت VString_سیٹ& cppFiles = v3Global.opt.cppFiles();
        برائے (ثابت خودکار& cppfile : cppFiles) {
            کا.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ثابت string dir
                = V3Os::filename_Relative_راستہ(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.داخل_کرو(dir);
        }
        dirs.داخل_کرو(V3Os::filename_Relative_راستہ(".", v3Global.opt.makeDir()));

        کا.puts("\n");

        کا.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        کا.puts("VM_USER_DIR = \\\n");
        برائے (ثابت خودکار& i : dirs) کا.puts("  " + i + " \\\n");
        کا.puts("\n");

        کا.puts("### Default rules...\n");
        کا.puts("# Include list of all generated classes\n");
        کا.puts("include " + v3Global.opt.سابقہ() + "_classes.mk\n");
        اگر (v3Global.opt.hier_چوٹی()) {
            کا.puts("# Include rules for hierarchical blocks\n");
            کا.puts("include " + v3Global.opt.سابقہ() + "_hier.mk\n");
        }
        کا.puts("# Include global rules\n");
        کا.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        اگر (v3Global.opt.exe()) {
            کا.puts("\n### Executable rules... (from --exe)\n");
            کا.puts("VPATH += $(VM_USER_DIR)\n");
            کا.puts("\n");

            اگر (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                کا.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                کا.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                کا.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                کا.puts("\n");
            }
        }

        ثابت string compilerIncludePch
            = v3Global.opt.compilerIncludes().خالی_خانے() ? "" : "$(VK_PCH_H).fast.gch";
        ثابت string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().خالی_خانے() ? "" : "$(VK_PCH_I_FAST)";
        برائے (ثابت string& cppfile : cppFiles) {
            ثابت string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            کا.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            کا.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        اگر (v3Global.opt.exe()) {
            کا.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            کا.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            کا.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            کا.puts("\n");
        } ورنہ اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            ثابت string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            کا.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            کا.puts(v3Global.opt.libCreateName(جھوٹ) + ": " + libCreateDeps + "\n");
            کا.puts("\n");
            اگر (v3Global.opt.hier_بچہ()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                کا.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(جھوٹ)
                        + "\n");
            } ورنہ {
                کا.puts(v3Global.opt.libCreateName(سچ) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                کا.puts("ifeq ($(shell uname -s),Darwin)\n");
                کا.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                کا.puts("else\n");
                کا.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                کا.puts("endif\n");
                کا.puts("\n");
                کا.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(جھوٹ)
                        + " " + v3Global.opt.libCreateName(سچ) + "\n");
            }
        } ورنہ {
            ثابت string libname = "lib" + v3Global.opt.سابقہ() + ".a";
            کا.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            کا.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            کا.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            کا.puts("lib" + v3Global.opt.سابقہ() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        کا.puts_سرخی();
    }

    صریح EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    مجازی ~EmitMk() = طےشدہ;
};

//######################################################################

جماعت EmitMkHierVerilation final {
    ثابت V3_Hier_گراف* ثابت m_graphp;
    ثابت string m_makefile;  // path of this makefile
    خالی emitCommonOpts(V3_برآمد_Mk_File& کا) ثابت {
        کا.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        کا.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        کا.puts("# Common options for hierarchical blocks\n");
        ثابت string fullpath_bin = V3Os::filename_Real_راستہ(v3Global.opt.buildDepBin());
        ثابت string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        کا.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        کا.puts("VM_HIER_INPUT_FILES := \\\n");
        برائے (ثابت خودکار& i : v3Global.opt.vFiles())
            کا.puts("  " + V3Os::filename_Real_راستہ(i.filename()) + " \\\n");
        کا.puts("\n");
        کا.puts("VM_HIER_VERILOG_LIBS := \\\n");
        برائے (ثابت خودکار& i : v3Global.opt.libraryFiles()) {
            کا.puts("  " + V3Os::filename_Real_راستہ(i.filename()) + " \\\n");
        }
        کا.puts("\n");
    }
    خالی emitLaunchVerilator(V3_برآمد_Mk_File& کا, ثابت string& دلائل_File) ثابت {
        کا.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        کا.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + دلائل_File + "\"\n");
    }
    خالی emit(V3_برآمد_Mk_File& کا) ثابت {
        کا.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        کا.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        کا.puts("#\n");
        کا.puts("# The main makefile " + v3Global.opt.سابقہ() + ".mk calls this makefile\n");
        کا.puts("\n");
        کا.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        کا.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        کا.puts(".SUFFIXES:\n");
        کا.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        کا.puts("# Libraries of hierarchical blocks\n");
        کا.puts("VM_HIER_LIBS := \\\n");
        برائے (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
            کا.puts("  " + blockp->hierLibFilename(سچ) + " \\\n");
        }
        کا.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        کا.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.سابقہ() + ".mk\n");
        کا.puts("\t$(MAKE) -f " + v3Global.opt.سابقہ() + ".mk\n");
        کا.puts("hier_verilation: " + v3Global.opt.سابقہ() + ".mk\n");
        emitCommonOpts(کا);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        کا.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        کا.puts("hier_launch_verilator:\n");
        کا.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ثابت string دلائل_File = v3Global.hierGraphp()->چوٹی_Command_دلائل_Filename(جھوٹ);
            کا.puts("\n# Verilate the top module\n");
            کا.puts(v3Global.opt.سابقہ()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            کا.puts(V3Os::filenameNonDir(دلائل_File) + " ");
            برائے (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
                ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
                کا.puts(blockp->hierWrapperFilename(سچ) + " ");
            }
            کا.puts("\n");
            emitLaunchVerilator(کا, دلائل_File);
        }

        // Rules to process hierarchical blocks
        کا.puts("\n# Verilate hierarchical blocks\n");
        برائے (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
            ثابت string سابقہ = blockp->hier_سابقہ();
            ثابت string دلائل_Filename = blockp->command_دلائل_Filename(جھوٹ);
            کا.puts(blockp->hierGeneratedFilenames(سچ));
            کا.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            کا.puts(V3Os::filenameNonDir(دلائل_Filename) + " ");
            برائے (ثابت V3_گراف_کنارہ& کنارہ : blockp->برآمد_کنارے()) {
                ثابت V3_Hier_بلاک* ثابت dependencyp = کنارہ.چوٹی()->as<V3_Hier_بلاک>();
                کا.puts(dependencyp->hierWrapperFilename(سچ) + " ");
            }
            کا.puts("\n");
            emitLaunchVerilator(کا, دلائل_Filename);

            // Rule to build lib*.a
            کا.puts(blockp->hierLibFilename(سچ));
            کا.puts(": ");
            کا.puts(blockp->hierMkFilename(سچ));
            کا.puts(" ");
            برائے (ثابت V3_گراف_کنارہ& کنارہ : blockp->برآمد_کنارے()) {
                ثابت V3_Hier_بلاک* ثابت dependencyp = کنارہ.چوٹی()->as<V3_Hier_بلاک>();
                کا.puts(dependencyp->hierLibFilename(سچ));
                کا.puts(" ");
            }
            کا.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(جھوٹ) + " -C " + سابقہ);
            کا.puts(" VM_PREFIX=" + سابقہ);
            کا.puts("\n\n");
        }
        کا.puts("endif  # Guard\n");
    }

عوامی:
    صریح EmitMkHierVerilation(ثابت V3_Hier_گراف* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.سابقہ() + "_hier.mk"} {
        V3_برآمد_Mk_File کا{m_makefile};
        emit(کا);
    }
};

//######################################################################
// Gate class functions

خالی V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت EmitMk emitter;
}

خالی V3EmitMk::emitHierVerilation(ثابت V3_Hier_گراف* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
