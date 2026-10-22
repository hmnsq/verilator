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
صنف Emit_گروہ final {
عام:
    بنية FileOrConcatenatedFilesList final {
        ثابت std::string m_filename;  // Filename or output group filename if grouping
        std::سمتیہ<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        منطقي isConcatenatingFile() ثابت { أرجع !m_concatenatedFilenames.خالی_تھاواں(); }
    };

    بنية FilenameWithScore final {
        ثابت std::string m_filename;  // Input filename
        ثابت uint64_t m_score;  // Input file complexity
        ثابت uint64_t m_ناپ_بائٹاں;  // Input file size
    };

خاص:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    بنية WorkList final {
        uint64_t m_کل_Score = 0;  // Sum of scores of included files
        std::سمتیہ<FilenameWithScore> m_files;  // Included filenames
        صحيح m_buckets_عدد
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        منطقي m_isConcatenable = سچ;  // Indicated whether files on this list can be concatenated.
        ثابت صحيح m_dbgId;  // Work list ID for debugging.

        WorkList() = مٹاؤ;
        واضح WorkList(صحيح id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    ساكن constexpr ناپ_t MIN_FILES_گنتی = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    ساكن constexpr ناپ_t MIN_FILES_فی_بالٹی = 4;

    // MEMBERS

    ثابت std::سمتیہ<FilenameWithScore>
        m_داخلہ_Files;  // List of filenames from initial AstCFile list
    std::سمتیہ<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ثابت uint64_t m_کل_Score;  // Sum of file scores
    ثابت std::string m_گروہ_File_اگیتر;  // Prefix for output group filenames
    ثابت uint64_t m_max_گروہ_بائٹاں;  // Maximum sum of file sizes in an output group
    std::سمتیہ<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::سمتیہ<WorkList*> m_concatenable_Lists_By_Desc_ناپ;  // Lists sorted by size, descending
    صحيح m_گروہ_عدد = 0;  // Number of output groups created

    Emit_گروہ(std::سمتیہ<FilenameWithScore> داخلہ_Files, uint64_t کل_Score,
              ثابت std::string& گروہ_File_اگیتر, uint64_t max_گروہ_بائٹاں)
        : m_داخلہ_Files{std::move(داخلہ_Files)}
        , m_کل_Score{کل_Score}
        , m_گروہ_File_اگیتر{گروہ_File_اگیتر}
        , m_max_گروہ_بائٹاں{max_گروہ_بائٹاں} {}

    string گروہ_Filename(صحيح id) ثابت {
        أرجع v3Global.opt.اگیتر() + "_" + m_گروہ_File_اگیتر + std::منزل_string(id);
    }

    // Debug logging: prints scores histogram
    فراغ dumpLogScoreHistogram(std::ostream& os) {
        constexpr صحيح MAX_BAR_لمائی = 80;
        constexpr صحيح MAX_INTERVALS_عدد = 60;

        // All scores arranged in ascending order.
        std::سمتیہ<uint64_t> sortedScores;
        sortedScores.reserve(m_داخلہ_Files.ناپ());
        std::transform(m_داخلہ_Files.begin(), m_داخلہ_Files.سرا(), std::پٹھ_inserter(sortedScores),
                       [](ثابت FilenameWithScore& داخلہ_File) { أرجع داخلہ_File.m_score; });
        std::ترتیب_دیو(sortedScores.begin(), sortedScores.سرا());

        ثابت int64_t چوٹی_Score = sortedScores.پٹھ();
        os << "Top score: " << چوٹی_Score << '\n';
        ثابت صحيح max_Score_چوڑائی = std::منزل_string(چوٹی_Score).لمائی();

        ثابت int64_t intervals_عدد = std::min<int64_t>(چوٹی_Score + 1, MAX_INTERVALS_عدد);

        بنية Interval final {
            uint64_t m_ہیٹھلا_Bound = 0;  // Lowest score included in this histogram bucket
            صحيح m_ناپ = 0;  // Number of scores that fell into this bucket
        };

        std::سمتیہ<Interval> intervals;
        intervals.resize(intervals_عدد);

        intervals[0].m_ہیٹھلا_Bound = 0;
        لكل (صحيح i = 1; i < intervals_عدد; i++) {
            intervals[i].m_ہیٹھلا_Bound = (چوٹی_Score + 1) * i / intervals_عدد + 1;
        }

        لكل (ثابت uint64_t score : sortedScores) {
            ثابت بنانشانی صحيح ivIdx = score * intervals_عدد / (چوٹی_Score + 1);
            ++intervals[ivIdx].m_ناپ;
        }

        صحيح چوٹی_Interval_ناپ = 0;
        لكل (ثابت Interval& iv : intervals)
            چوٹی_Interval_ناپ = std::max(چوٹی_Interval_ناپ, iv.m_ناپ);

        os << "Input files' scores histogram:\n";

        لكل (ثابت Interval& iv : intervals) {
            ثابت صحيح scaled_ناپ = iv.m_ناپ * (MAX_BAR_لمائی + 1) / چوٹی_Interval_ناپ;
            ثابت std::string سطر = " |" + std::string(scaled_ناپ, '#');

            os << std::setw(max_Score_چوڑائی) << iv.m_ہیٹھلا_Bound << سطر << "  " << iv.m_ناپ << '\n';
        }
        os << std::setw(max_Score_چوڑائی) << (چوٹی_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    فراغ dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        لكل (ثابت WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.ناپ()
               << "; total score: " << list.m_کل_Score << ")\n";
            إذا (debug() >= 6 || list.m_files.ناپ() < 4) {
                // Log all files
                لكل (ثابت FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } وإلا {
                // Log only first and last file
                ثابت FilenameWithScore& پہلا = list.m_files.front();
                ثابت FilenameWithScore& آخری = list.m_files.پٹھ();
                os << "| + " << پہلا.m_filename << "  (score: " << پہلا.m_score << ")\n";
                os << "| | (... " << (list.m_files.ناپ() - 2) << " files ...)\n";
                os << "| + " << آخری.m_filename << "  (score: " << آخری.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    فراغ dumpOutputList(std::ostream& os) ثابت {
        os << "List of output files after execution of concatenation:\n";

        لكل (ثابت FileOrConcatenatedFilesList& اندراج : m_outputFiles) {
            إذا (اندراج.isConcatenatingFile()) {
                os << "+ " << اندراج.m_filename << "  (concatenating file)\n";
                لكل (ثابت string& f : اندراج.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } وإلا {
                os << "+ " << اندراج.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    منطقي fallbackNoGrouping(ناپ_t داخلہ_Files_گنتی) {
        ثابت صحيح کل_Buckets_عدد = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        منطقي groupingRedundant = خطأ;
        إذا (داخلہ_Files_گنتی < MIN_FILES_گنتی
            && داخلہ_Files_گنتی <= static_cast<ناپ_t>(کل_Buckets_عدد)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_داخلہ_Files.ناپ() << " < "
                                                                   << MIN_FILES_گنتی << ")");
            groupingRedundant = سچ;
        }
        إذا (داخلہ_Files_گنتی < (MIN_FILES_فی_بالٹی * کل_Buckets_عدد)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_داخلہ_Files.ناپ() << " < " << MIN_FILES_فی_بالٹی << " - "
                         << کل_Buckets_عدد << ")");
            groupingRedundant = سچ;
        }
        إذا (!groupingRedundant) أرجع خطأ;

        m_outputFiles.reserve(m_داخلہ_Files.ناپ());
        لكل (ثابت FilenameWithScore& filename : m_داخلہ_Files) {
            m_outputFiles.ڈالو_پٹھ({std::move(filename.m_filename), {}});
        }
        أرجع سچ;
    }

    فراغ createWorkLists() {
        // Create initial Work Lists.

        ثابت صحيح کل_Buckets_عدد = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ثابت uint64_t concatenableFileMaxScore = m_کل_Score / کل_Buckets_عدد / 2;

        V3Stats::جوڑو_Stat("Concatenation max score", concatenableFileMaxScore);
        صحيح اگلا_Work_List_Id = 0;

        إذا (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        لكل (ثابت FilenameWithScore& داخلہ_File : m_داخلہ_Files) {
            ثابت منطقي fileIsConcatenable = (داخلہ_File.m_score <= concatenableFileMaxScore);
            إذا (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << داخلہ_File.m_filename << "  (score: " << داخلہ_File.m_score << ")\n";
            V3Stats::جوڑو_Stat_جوڑ(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                داخلہ_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            إذا (m_workLists.خالی_تھاواں() || m_workLists.پٹھ().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_پٹھ(WorkList{اگلا_Work_List_Id++});
                m_workLists.پٹھ().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.پٹھ();
            list.m_files.ڈالو_پٹھ(
                {داخلہ_File.m_filename, داخلہ_File.m_score, داخلہ_File.m_ناپ_بائٹاں});
            list.m_کل_Score += داخلہ_File.m_score;
        }
        إذا (m_logp) *m_logp << '\n';
    }

    فراغ سونپو_Buckets(uint64_t concatenable_Files_کل_Score) {
        // Assign buckets to lists

        ثابت ناپ_t کل_Buckets_عدد = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        إذا (m_concatenable_Lists_By_Desc_ناپ.ناپ() > کل_Buckets_عدد) {
            // Debugging: Log which work lists will be kept
            إذا (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_ناپ.begin(),
                              m_concatenable_Lists_By_Desc_ناپ.begin() + کل_Buckets_عدد,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.ناپ()
                                          << "; total score: " << listp->m_کل_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_ناپ.begin() + کل_Buckets_عدد,
                          m_concatenable_Lists_By_Desc_ناپ.سرا(), [&](WorkList* listp) {
                              listp->m_isConcatenable = خطأ;

                              إذا (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.ناپ()
                                          << "; total score: " << listp->m_کل_Score << ")\n";
                          });
            إذا (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_ناپ.resize(کل_Buckets_عدد);
            // Recalculate stats
            concatenable_Files_کل_Score = 0;
            لكل (ثابت WorkList* ثابت listp : m_concatenable_Lists_By_Desc_ناپ) {
                concatenable_Files_کل_Score += listp->m_کل_Score;
            }
        }

        ثابت uint64_t ideal_بالٹی_Score = concatenable_Files_کل_Score / کل_Buckets_عدد;

        V3Stats::جوڑو_Stat("Concatenation ideal bucket score", ideal_بالٹی_Score);

        إذا (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        صحيح دستیاب_Buckets = v3Global.opt.outputGroups();
        لكل (WorkList* listp : m_concatenable_Lists_By_Desc_ناپ) {
            إذا (دستیاب_Buckets > 0 && ideal_بالٹی_Score > 0) {
                listp->m_buckets_عدد = std::min(
                    دستیاب_Buckets, std::max<صحيح>(1, listp->m_کل_Score / ideal_بالٹی_Score));
                دستیاب_Buckets -= listp->m_buckets_عدد;
                إذا (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_عدد << "] Work List #"
                            << listp->m_dbgId << '\n';
            } وإلا {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = خطأ;
                إذا (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::کھبے << std::setw(4) << listp->m_dbgId
                            << std::سجے << " (excluding from concatenation)\n";
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
                    m_outputFiles.ڈالو_پٹھ({std::move(file.m_filename), {}});
                }
                استمر;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ثابت uint64_t list_Ideal_بالٹی_Score = list.m_کل_Score / list.m_buckets_عدد;

            std::سمتیہ<std::سمتیہ<ثابت FilenameWithScore*>> buckets;
            خودکار fileIt = list.m_files.cbegin();
            لكل (صحيح i = 0; i < list.m_buckets_عدد; ++i) {
                std::سمتیہ<ثابت FilenameWithScore*> بالٹی;
                uint64_t بالٹی_Score = 0;

                لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                    ثابت uint64_t diffNow
                        = std::مطلق_قدر(static_cast<int64_t>(list_Ideal_بالٹی_Score - بالٹی_Score));
                    ثابت uint64_t diffIfAdded = std::مطلق_قدر(static_cast<int64_t>(
                        list_Ideal_بالٹی_Score - بالٹی_Score - fileIt->m_score));
                    إذا (بالٹی_Score == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        بالٹی_Score += fileIt->m_score;
                        بالٹی.ڈالو_پٹھ(&*fileIt);
                    } وإلا {
                        // Best possible bucket score reached, process next bucket.
                        اكسر;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.ڈالو_پٹھ(std::move(بالٹی));
            }
            لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.پٹھ().ڈالو_پٹھ(&*fileIt);
            }
            لكل (ثابت std::سمتیہ<ثابت FilenameWithScore*>& بالٹی : buckets) {
                جوڑو_بالٹی_Groups(بالٹی);
            }
        }
    }

    فراغ جوڑو_بالٹی_Groups(ثابت std::سمتیہ<ثابت FilenameWithScore*>& بالٹی) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::سمتیہ<std::سمتیہ<std::string>> groups;
        uint64_t گروہ_بائٹاں = 0;
        لكل (ثابت FilenameWithScore* ثابت filep : بالٹی) {
            إذا (groups.خالی_تھاواں() || گروہ_بائٹاں + filep->m_ناپ_بائٹاں > m_max_گروہ_بائٹاں) {
                groups.emplace_پٹھ();
                گروہ_بائٹاں = 0;
            }
            گروہ_بائٹاں += filep->m_ناپ_بائٹاں;
            groups.پٹھ().ڈالو_پٹھ(filep->m_filename);
        }
        لكل (std::سمتیہ<std::string>& گروہ : groups) {
            إذا (گروہ.ناپ() == 1) {
                m_outputFiles.ڈالو_پٹھ({std::move(گروہ.front()), {}});
            } وإلا {
                m_outputFiles.ڈالو_پٹھ({گروہ_Filename(m_گروہ_عدد++), std::move(گروہ)});
            }
        }
    }

    فراغ assertFilesSame() ثابت {
        خودکار ifIt = m_داخلہ_Files.begin();
        خودکار دا_It = m_outputFiles.begin();
        طالما (ifIt != m_داخلہ_Files.سرا() && دا_It != m_outputFiles.سرا()) {
            إذا (دا_It->isConcatenatingFile()) {
                لكل (ثابت string& ocf : دا_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_داخلہ_Files.سرا(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++دا_It;
            } وإلا {
                UASSERT(ifIt->m_filename == دا_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << دا_It->m_filename);
                ++ifIt;
                ++دا_It;
            }
        }
        UASSERT(ifIt == m_داخلہ_Files.سرا(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(دا_It == m_outputFiles.سرا(),
                "More output files than input files. First extra file: " << دا_It->m_filename);
    }

    فراغ process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_گروہ_File_اگیتر);
        UINFO(5, "Number of input files: " << m_داخلہ_Files.ناپ());
        UINFO(5, "Total score: " << m_کل_Score);

        ثابت صحيح کل_Buckets_عدد = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << کل_Buckets_عدد);
        UASSERT(کل_Buckets_عدد > 0, "More than 0 buckets required");

        إذا (fallbackNoGrouping(m_داخلہ_Files.ناپ())) أرجع;

        إذا (debug() >= 6 || dumpLevel() >= 6) {
            ثابت string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            إذا (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        إذا (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        ناپ_t concatenable_Files_گنتی = 0;
        int64_t concatenable_Files_کل_Score = 0;

        لكل (WorkList& list : m_workLists) {
            إذا (!list.m_isConcatenable) استمر;

            // "Concatenation" of a single file is pointless
            إذا (list.m_files.ناپ() == 1) {
                list.m_isConcatenable = خطأ;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                استمر;
            }

            concatenable_Files_گنتی += list.m_files.ناپ();
            concatenable_Files_کل_Score += list.m_کل_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_ناپ.ڈالو_پٹھ(&list);
        }

        إذا (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        إذا (fallbackNoGrouping(concatenable_Files_گنتی)) أرجع;

        std::ترتیب_دیو(m_concatenable_Lists_By_Desc_ناپ.begin(), m_concatenable_Lists_By_Desc_ناپ.سرا(),
                  [](ثابت WorkList* ap, ثابت WorkList* bp) {
                      // Sort in descending order by number of files
                      إذا (ap->m_files.ناپ() != bp->m_files.ناپ())
                          أرجع (ap->m_files.ناپ() > bp->m_files.ناپ());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      أرجع bp->m_کل_Score > ap->m_کل_Score;
                  });

        سونپو_Buckets(concatenable_Files_کل_Score);

        buildOutputList();

        إذا (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

عام:
    ساكن std::سمتیہ<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::سمتیہ<FilenameWithScore> داخلہ_Files, uint64_t کل_Score,
                                ثابت std::string& گروہ_File_اگیتر, uint64_t max_گروہ_بائٹاں) {
        Emit_گروہ گروہ{std::move(داخلہ_Files), کل_Score, گروہ_File_اگیتر, max_گروہ_بائٹاں};
        گروہ.process();
        أرجع گروہ.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

صنف EmitMk final {
    ورتوں FileOrConcatenatedFilesList = Emit_گروہ::FileOrConcatenatedFilesList;
    ورتوں FilenameWithScore = Emit_گروہ::FilenameWithScore;

    // MEMBERS
    مزدوج m_put_Class_گنتی = 0;  // Number of classEntries printed

عام:
    // METHODS

    ساكن فراغ emitConcatenatingFile(ثابت FileOrConcatenatedFilesList& اندراج) {
        UASSERT(اندراج.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_باہرلا_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + اندراج.m_filename + ".cpp"};
        concatenatingFile.puts_سرخی();
        لكل (ثابت string& file : اندراج.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    ساكن uint64_t file_ناپ(ثابت AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::ثنائی | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        أرجع static_cast<uint64_t>(ifs.tellg());
    }

    فراغ put_Make_Class_اندراج(V3_باہرلا_Mk_File& دا, ثابت string& name) {
        دا.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_گنتی;
    }

    فراغ emitClassMake() {
        std::سمتیہ<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::سمتیہ<FileOrConcatenatedFilesList> vmClassesFastList;
        إذا (v3Global.opt.outputGroups() > 0) {
            std::سمتیہ<FilenameWithScore> slowFiles;
            std::سمتیہ<FilenameWithScore> fastFiles;
            uint64_t slow_کل_Score = 0;
            uint64_t fast_کل_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t سرخی_بائٹاں = 0;

            لكل (Ast_گرہ_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), گرہ_File)) {
                ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                إذا (cfilep && VString::سرے_With(cfilep->name(), ".h")) {
                    سرخی_بائٹاں += file_ناپ(cfilep);
                }
                إذا (cfilep && cfilep->سروت() && cfilep->support() == خطأ) {
                    std::سمتیہ<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& کل_Score = cfilep->slow() ? slow_کل_Score : fast_کل_Score;

                    کل_Score += cfilep->complexityScore();
                    files.ڈالو_پٹھ({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_ناپ(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_گروہ_بائٹاں = std::numeric_limits<uint64_t>::max();
            إذا (v3Global.opt.output_Groups_ناپ()) {
                ثابت uint64_t max_Tu_بائٹاں = static_cast<uint64_t>(v3Global.opt.output_Groups_ناپ());
                max_گروہ_بائٹاں = max_Tu_بائٹاں > سرخی_بائٹاں ? max_Tu_بائٹاں - سرخی_بائٹاں : 0;
            }

            vmClassesSlowList = Emit_گروہ::singleConcatenatedFilesList(
                std::move(slowFiles), slow_کل_Score, "vm_classes_Slow_", max_گروہ_بائٹاں);
            vmClassesFastList = Emit_گروہ::singleConcatenatedFilesList(
                std::move(fastFiles), fast_کل_Score, "vm_classes_", max_گروہ_بائٹاں);
        }

        // Generate the makefile
        V3_باہرلا_Mk_File دا{v3Global.opt.makeDir() + "/" + v3Global.opt.اگیتر() + "_classes.mk"};
        دا.puts_سرخی();
        دا.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        دا.puts("#\n");
        دا.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        دا.puts("# See " + v3Global.opt.اگیتر() + ".mk" + " for the caller.\n");

        دا.puts("\n### Switches...\n");
        دا.puts("# C11 constructs required?  0/1 (always on now)\n");
        دا.put_سیٹ("VM_C11", "1");
        دا.puts("# Timing enabled?  0/1\n");
        دا.put_سیٹ("VM_TIMING", v3Global.ورتدا_Timing() ? "1" : "0");
        دا.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        دا.put_سیٹ("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        دا.puts("# Parallel builds?  0/1 (from --output-split)\n");
        دا.put_سیٹ("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        دا.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        دا.put_سیٹ("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        دا.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        دا.put_سیٹ("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        دا.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        دا.put_سیٹ("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        دا.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        دا.put_سیٹ("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        دا.puts("# VPI enabled?  0/1 (from --vpi)\n");
        دا.put_سیٹ("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        دا.puts("\n### Object file lists...\n");
        لكل (صحيح support = 0; support < 3; ++support) {
            لكل (ثابت منطقي& slow : {خطأ, سچ}) {
                إذا (support == 2) {
                    دا.puts("# Global classes, need linked once per executable");
                } وإلا إذا (support) {
                    دا.puts("# Generated support classes");
                } وإلا {
                    دا.puts("# Generated module classes");
                }
                إذا (slow) {
                    دا.puts(", non-fast-path, compile with low/medium optimization\n");
                } وإلا {
                    دا.puts(", fast-path, compile with highest optimization\n");
                }
                ثابت string نشانہ_Var = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_گنتی = 0;
                دا.puts(نشانہ_Var + " += \\\n");
                إذا (support == 2 && v3Global.opt.hier_بچہ()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } وإلا إذا (support == 2 && !slow) {
                    لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_اندراج(دا, cpp);
                } وإلا إذا (support == 2 && slow) {
                } وإلا إذا (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ثابت std::سمتیہ<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    لكل (ثابت FileOrConcatenatedFilesList& اندراج : list) {
                        إذا (اندراج.isConcatenatingFile()) emitConcatenatingFile(اندراج);
                        put_Make_Class_اندراج(دا, اندراج.m_filename);
                    }
                } وإلا {
                    لكل (Ast_گرہ_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), گرہ_File)) {
                        ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                        إذا (cfilep && cfilep->سروت() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_اندراج(دا, cfilep->name());
                        }
                    }
                }
                دا.puts("\n");
                V3Stats::جوڑو_Stat("Makefile targets, " + نشانہ_Var, m_put_Class_گنتی);
            }
        }

        دا.puts_سرخی();
    }

    فراغ emitOverallMake() {
        // Generate the makefile
        V3_باہرلا_Mk_File دا{v3Global.opt.makeDir() + "/" + v3Global.opt.اگیتر() + ".mk"};
        دا.puts_سرخی();
        دا.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        دا.puts("#\n");
        دا.puts("# Execute this makefile from the object directory:\n");
        دا.puts("#    make -f " + v3Global.opt.اگیتر() + ".mk" + "\n");
        دا.puts("\n");

        إذا (v3Global.opt.exe()) {
            دا.puts("default: " + v3Global.opt.exeName() + "\n");
        } وإلا إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
            دا.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } وإلا {
            دا.puts("default: lib" + v3Global.opt.اگیتر() + "\n");
        }
        دا.puts("\n### Constants...\n");
        دا.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        دا.put_سیٹ("PERL", V3_باہرلا_Formatter::quoteNameControls(V3Options::getenvPERL()));
        دا.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        دا.put_سیٹ("PYTHON3", V3_باہرلا_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        دا.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        دا.put_سیٹ("VERILATOR_ROOT",
                  V3_باہرلا_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_جڑ()));
        دا.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        دا.put_سیٹ_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        دا.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        دا.put_سیٹ_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        إذا (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        دا.puts("\n### Switches...\n");
        دا.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        دا.put_سیٹ("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        دا.puts("# SystemC output mode?  0/1 (from --sc)\n");
        دا.put_سیٹ("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        دا.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        دا.put_سیٹ("VM_SP_OR_SC", "$(VM_SC)");
        دا.puts("# Deprecated\n");
        دا.put_سیٹ("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        دا.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        دا.put_سیٹ("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        دا.puts("\n### Vars...\n");
        دا.puts("# Module prefix (from --prefix)\n");
        دا.put_سیٹ("VM_MODPREFIX", v3Global.opt.mod_اگیتر());
        دا.puts("# Design prefix (from --prefix)\n");
        دا.put_سیٹ("VM_PREFIX", v3Global.opt.اگیتر());
        دا.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        دا.put_سیٹ("VM_PREFIX_INCLUDE", "<" + v3Global.opt.اگیتر() + ".h>");
        إذا (v3Global.dpi()) {
            دا.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            دا.put_سیٹ("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.اگیتر() + "__Dpi.h>");
        }

        دا.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        دا.puts("VM_USER_CFLAGS = \\\n");
        ثابت std::string solver = V3Options::getenvVERILATOR_SOLVER();
        إذا (v3Global.useRandomizeMethods() && solver != "")
            دا.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3_باہرلا_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        إذا (!v3Global.opt.libCreate().خالی_تھاواں()) دا.puts("\t-fPIC \\\n");
        ثابت VStringList& cFlags = v3Global.opt.cFlags();
        لكل (ثابت string& i : cFlags) دا.puts("  " + i + " \\\n");
        دا.puts("\n");

        دا.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        دا.puts("VM_USER_LDLIBS = \\\n");
        ثابت VStringList& ldLibs = v3Global.opt.ldLibs();
        لكل (ثابت string& i : ldLibs) دا.puts("  " + i + " \\\n");
        دا.puts("\n");

        دا.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        دا.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        دا.puts("VM_USER_LDLIBS_FILES = \\\n");
        لكل (ثابت string& i : v3Global.opt.ldLibFiles()) دا.puts("  " + i + " \\\n");
        دا.puts("\n");

        VString_سیٹ dirs;
        دا.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        دا.puts("VM_USER_CLASSES = \\\n");
        ثابت VString_سیٹ& cppFiles = v3Global.opt.cppFiles();
        لكل (ثابت خودکار& cppfile : cppFiles) {
            دا.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ثابت string dir
                = V3Os::filename_Relative_رستہ(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.داخل_کرو(dir);
        }
        dirs.داخل_کرو(V3Os::filename_Relative_رستہ(".", v3Global.opt.makeDir()));

        دا.puts("\n");

        دا.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        دا.puts("VM_USER_DIR = \\\n");
        لكل (ثابت خودکار& i : dirs) دا.puts("  " + i + " \\\n");
        دا.puts("\n");

        دا.puts("### Default rules...\n");
        دا.puts("# Include list of all generated classes\n");
        دا.puts("include " + v3Global.opt.اگیتر() + "_classes.mk\n");
        إذا (v3Global.opt.hier_چوٹی()) {
            دا.puts("# Include rules for hierarchical blocks\n");
            دا.puts("include " + v3Global.opt.اگیتر() + "_hier.mk\n");
        }
        دا.puts("# Include global rules\n");
        دا.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        إذا (v3Global.opt.exe()) {
            دا.puts("\n### Executable rules... (from --exe)\n");
            دا.puts("VPATH += $(VM_USER_DIR)\n");
            دا.puts("\n");

            إذا (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                دا.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                دا.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                دا.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                دا.puts("\n");
            }
        }

        ثابت string compilerIncludePch
            = v3Global.opt.compilerIncludes().خالی_تھاواں() ? "" : "$(VK_PCH_H).fast.gch";
        ثابت string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().خالی_تھاواں() ? "" : "$(VK_PCH_I_FAST)";
        لكل (ثابت string& cppfile : cppFiles) {
            ثابت string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            دا.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            دا.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        إذا (v3Global.opt.exe()) {
            دا.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            دا.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            دا.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            دا.puts("\n");
        } وإلا إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
            ثابت string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            دا.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            دا.puts(v3Global.opt.libCreateName(خطأ) + ": " + libCreateDeps + "\n");
            دا.puts("\n");
            إذا (v3Global.opt.hier_بچہ()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                دا.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + "\n");
            } وإلا {
                دا.puts(v3Global.opt.libCreateName(سچ) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                دا.puts("ifeq ($(shell uname -s),Darwin)\n");
                دا.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                دا.puts("else\n");
                دا.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                دا.puts("endif\n");
                دا.puts("\n");
                دا.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + " " + v3Global.opt.libCreateName(سچ) + "\n");
            }
        } وإلا {
            ثابت string libname = "lib" + v3Global.opt.اگیتر() + ".a";
            دا.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            دا.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            دا.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            دا.puts("lib" + v3Global.opt.اگیتر() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        دا.puts_سرخی();
    }

    واضح EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    مجازی ~EmitMk() = افتراضي;
};

//######################################################################

صنف EmitMkHierVerilation final {
    ثابت V3_Hier_گراف* ثابت m_graphp;
    ثابت string m_makefile;  // path of this makefile
    فراغ emitCommonOpts(V3_باہرلا_Mk_File& دا) ثابت {
        دا.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        دا.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        دا.puts("# Common options for hierarchical blocks\n");
        ثابت string fullpath_bin = V3Os::filename_Real_رستہ(v3Global.opt.buildDepBin());
        ثابت string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        دا.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        دا.puts("VM_HIER_INPUT_FILES := \\\n");
        لكل (ثابت خودکار& i : v3Global.opt.vFiles())
            دا.puts("  " + V3Os::filename_Real_رستہ(i.filename()) + " \\\n");
        دا.puts("\n");
        دا.puts("VM_HIER_VERILOG_LIBS := \\\n");
        لكل (ثابت خودکار& i : v3Global.opt.libraryFiles()) {
            دا.puts("  " + V3Os::filename_Real_رستہ(i.filename()) + " \\\n");
        }
        دا.puts("\n");
    }
    فراغ emitLaunchVerilator(V3_باہرلا_Mk_File& دا, ثابت string& دلیلاں_File) ثابت {
        دا.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        دا.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + دلیلاں_File + "\"\n");
    }
    فراغ emit(V3_باہرلا_Mk_File& دا) ثابت {
        دا.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        دا.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        دا.puts("#\n");
        دا.puts("# The main makefile " + v3Global.opt.اگیتر() + ".mk calls this makefile\n");
        دا.puts("\n");
        دا.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        دا.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        دا.puts(".SUFFIXES:\n");
        دا.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        دا.puts("# Libraries of hierarchical blocks\n");
        دا.puts("VM_HIER_LIBS := \\\n");
        لكل (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
            دا.puts("  " + blockp->hierLibFilename(سچ) + " \\\n");
        }
        دا.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        دا.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.اگیتر() + ".mk\n");
        دا.puts("\t$(MAKE) -f " + v3Global.opt.اگیتر() + ".mk\n");
        دا.puts("hier_verilation: " + v3Global.opt.اگیتر() + ".mk\n");
        emitCommonOpts(دا);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        دا.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        دا.puts("hier_launch_verilator:\n");
        دا.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ثابت string دلیلاں_File = v3Global.hierGraphp()->چوٹی_Command_دلیلاں_Filename(خطأ);
            دا.puts("\n# Verilate the top module\n");
            دا.puts(v3Global.opt.اگیتر()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            دا.puts(V3Os::filenameNonDir(دلیلاں_File) + " ");
            لكل (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
                ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
                دا.puts(blockp->hierWrapperFilename(سچ) + " ");
            }
            دا.puts("\n");
            emitLaunchVerilator(دا, دلیلاں_File);
        }

        // Rules to process hierarchical blocks
        دا.puts("\n# Verilate hierarchical blocks\n");
        لكل (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلاک* ثابت blockp = vtx.as<V3_Hier_بلاک>();
            ثابت string اگیتر = blockp->hier_اگیتر();
            ثابت string دلیلاں_Filename = blockp->command_دلیلاں_Filename(خطأ);
            دا.puts(blockp->hierGeneratedFilenames(سچ));
            دا.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            دا.puts(V3Os::filenameNonDir(دلیلاں_Filename) + " ");
            لكل (ثابت V3_گراف_کنارا& کنارا : blockp->باہرلا_کنارے()) {
                ثابت V3_Hier_بلاک* ثابت dependencyp = کنارا.چوٹی()->as<V3_Hier_بلاک>();
                دا.puts(dependencyp->hierWrapperFilename(سچ) + " ");
            }
            دا.puts("\n");
            emitLaunchVerilator(دا, دلیلاں_Filename);

            // Rule to build lib*.a
            دا.puts(blockp->hierLibFilename(سچ));
            دا.puts(": ");
            دا.puts(blockp->hierMkFilename(سچ));
            دا.puts(" ");
            لكل (ثابت V3_گراف_کنارا& کنارا : blockp->باہرلا_کنارے()) {
                ثابت V3_Hier_بلاک* ثابت dependencyp = کنارا.چوٹی()->as<V3_Hier_بلاک>();
                دا.puts(dependencyp->hierLibFilename(سچ));
                دا.puts(" ");
            }
            دا.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(خطأ) + " -C " + اگیتر);
            دا.puts(" VM_PREFIX=" + اگیتر);
            دا.puts("\n\n");
        }
        دا.puts("endif  # Guard\n");
    }

عام:
    واضح EmitMkHierVerilation(ثابت V3_Hier_گراف* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.اگیتر() + "_hier.mk"} {
        V3_باہرلا_Mk_File دا{m_makefile};
        emit(دا);
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
