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
صنف Emit_گروهه final {
عام:
    بنية FileOrConcatenatedFilesList final {
        ثابت std::string m_filename;  // Filename or output group filename if grouping
        std::ويڪٽر<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        منطقي isConcatenatingFile() ثابت { أرجع !m_concatenatedFilenames.خالي_جايون(); }
    };

    بنية FilenameWithScore final {
        ثابت std::string m_filename;  // Input filename
        ثابت uint64_t m_score;  // Input file complexity
        ثابت uint64_t m_سائيز_بائيٽون;  // Input file size
    };

خاص:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    بنية WorkList final {
        uint64_t m_ڪل_Score = 0;  // Sum of scores of included files
        std::ويڪٽر<FilenameWithScore> m_files;  // Included filenames
        صحيح m_buckets_انگ
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        منطقي m_isConcatenable = سچ;  // Indicated whether files on this list can be concatenated.
        ثابت صحيح m_dbgId;  // Work list ID for debugging.

        WorkList() = ميٽيو;
        واضح WorkList(صحيح id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    ساكن constexpr سائيز_t MIN_FILES_ڳڻپ = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    ساكن constexpr سائيز_t MIN_FILES_في_ٻالٽي = 4;

    // MEMBERS

    ثابت std::ويڪٽر<FilenameWithScore>
        m_داخلا_Files;  // List of filenames from initial AstCFile list
    std::ويڪٽر<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ثابت uint64_t m_ڪل_Score;  // Sum of file scores
    ثابت std::string m_گروهه_File_اڳياڙي;  // Prefix for output group filenames
    ثابت uint64_t m_max_گروهه_بائيٽون;  // Maximum sum of file sizes in an output group
    std::ويڪٽر<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::ويڪٽر<WorkList*> m_concatenable_Lists_By_Desc_سائيز;  // Lists sorted by size, descending
    صحيح m_گروهه_انگ = 0;  // Number of output groups created

    Emit_گروهه(std::ويڪٽر<FilenameWithScore> داخلا_Files, uint64_t ڪل_Score,
              ثابت std::string& گروهه_File_اڳياڙي, uint64_t max_گروهه_بائيٽون)
        : m_داخلا_Files{std::move(داخلا_Files)}
        , m_ڪل_Score{ڪل_Score}
        , m_گروهه_File_اڳياڙي{گروهه_File_اڳياڙي}
        , m_max_گروهه_بائيٽون{max_گروهه_بائيٽون} {}

    string گروهه_Filename(صحيح id) ثابت {
        أرجع v3Global.opt.اڳياڙي() + "_" + m_گروهه_File_اڳياڙي + std::منزل_string(id);
    }

    // Debug logging: prints scores histogram
    فراغ dumpLogScoreHistogram(std::ostream& os) {
        constexpr صحيح MAX_BAR_ڊيگهائي = 80;
        constexpr صحيح MAX_INTERVALS_انگ = 60;

        // All scores arranged in ascending order.
        std::ويڪٽر<uint64_t> sortedScores;
        sortedScores.reserve(m_داخلا_Files.سائيز());
        std::transform(m_داخلا_Files.begin(), m_داخلا_Files.پڇاڙي(), std::پٺ_inserter(sortedScores),
                       [](ثابت FilenameWithScore& داخلا_File) { أرجع داخلا_File.m_score; });
        std::ترتيب_ڏيو(sortedScores.begin(), sortedScores.پڇاڙي());

        ثابت int64_t چوٽي_Score = sortedScores.پٺ();
        os << "Top score: " << چوٽي_Score << '\n';
        ثابت صحيح max_Score_ويڪر = std::منزل_string(چوٽي_Score).ڊيگهائي();

        ثابت int64_t intervals_انگ = std::min<int64_t>(چوٽي_Score + 1, MAX_INTERVALS_انگ);

        بنية Interval final {
            uint64_t m_هيٺاهون_Bound = 0;  // Lowest score included in this histogram bucket
            صحيح m_سائيز = 0;  // Number of scores that fell into this bucket
        };

        std::ويڪٽر<Interval> intervals;
        intervals.resize(intervals_انگ);

        intervals[0].m_هيٺاهون_Bound = 0;
        لكل (صحيح i = 1; i < intervals_انگ; i++) {
            intervals[i].m_هيٺاهون_Bound = (چوٽي_Score + 1) * i / intervals_انگ + 1;
        }

        لكل (ثابت uint64_t score : sortedScores) {
            ثابت بنانشان صحيح ivIdx = score * intervals_انگ / (چوٽي_Score + 1);
            ++intervals[ivIdx].m_سائيز;
        }

        صحيح چوٽي_Interval_سائيز = 0;
        لكل (ثابت Interval& iv : intervals)
            چوٽي_Interval_سائيز = std::max(چوٽي_Interval_سائيز, iv.m_سائيز);

        os << "Input files' scores histogram:\n";

        لكل (ثابت Interval& iv : intervals) {
            ثابت صحيح scaled_سائيز = iv.m_سائيز * (MAX_BAR_ڊيگهائي + 1) / چوٽي_Interval_سائيز;
            ثابت std::string سٽ = " |" + std::string(scaled_سائيز, '#');

            os << std::setw(max_Score_ويڪر) << iv.m_هيٺاهون_Bound << سٽ << "  " << iv.m_سائيز << '\n';
        }
        os << std::setw(max_Score_ويڪر) << (چوٽي_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    فراغ dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        لكل (ثابت WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.سائيز()
               << "; total score: " << list.m_ڪل_Score << ")\n";
            إذا (debug() >= 6 || list.m_files.سائيز() < 4) {
                // Log all files
                لكل (ثابت FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } وإلا {
                // Log only first and last file
                ثابت FilenameWithScore& پهريون = list.m_files.front();
                ثابت FilenameWithScore& آخري = list.m_files.پٺ();
                os << "| + " << پهريون.m_filename << "  (score: " << پهريون.m_score << ")\n";
                os << "| | (... " << (list.m_files.سائيز() - 2) << " files ...)\n";
                os << "| + " << آخري.m_filename << "  (score: " << آخري.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    فراغ dumpOutputList(std::ostream& os) ثابت {
        os << "List of output files after execution of concatenation:\n";

        لكل (ثابت FileOrConcatenatedFilesList& داخلا_2 : m_outputFiles) {
            إذا (داخلا_2.isConcatenatingFile()) {
                os << "+ " << داخلا_2.m_filename << "  (concatenating file)\n";
                لكل (ثابت string& f : داخلا_2.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } وإلا {
                os << "+ " << داخلا_2.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    منطقي fallbackNoGrouping(سائيز_t داخلا_Files_ڳڻپ) {
        ثابت صحيح ڪل_Buckets_انگ = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        منطقي groupingRedundant = خطأ;
        إذا (داخلا_Files_ڳڻپ < MIN_FILES_ڳڻپ
            && داخلا_Files_ڳڻپ <= static_cast<سائيز_t>(ڪل_Buckets_انگ)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_داخلا_Files.سائيز() << " < "
                                                                   << MIN_FILES_ڳڻپ << ")");
            groupingRedundant = سچ;
        }
        إذا (داخلا_Files_ڳڻپ < (MIN_FILES_في_ٻالٽي * ڪل_Buckets_انگ)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_داخلا_Files.سائيز() << " < " << MIN_FILES_في_ٻالٽي << " - "
                         << ڪل_Buckets_انگ << ")");
            groupingRedundant = سچ;
        }
        إذا (!groupingRedundant) أرجع خطأ;

        m_outputFiles.reserve(m_داخلا_Files.سائيز());
        لكل (ثابت FilenameWithScore& filename : m_داخلا_Files) {
            m_outputFiles.ڌڪو_پٺ({std::move(filename.m_filename), {}});
        }
        أرجع سچ;
    }

    فراغ createWorkLists() {
        // Create initial Work Lists.

        ثابت صحيح ڪل_Buckets_انگ = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ثابت uint64_t concatenableFileMaxScore = m_ڪل_Score / ڪل_Buckets_انگ / 2;

        V3Stats::جوڙيو_Stat("Concatenation max score", concatenableFileMaxScore);
        صحيح ايندڙ_Work_List_Id = 0;

        إذا (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        لكل (ثابت FilenameWithScore& داخلا_File : m_داخلا_Files) {
            ثابت منطقي fileIsConcatenable = (داخلا_File.m_score <= concatenableFileMaxScore);
            إذا (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << داخلا_File.m_filename << "  (score: " << داخلا_File.m_score << ")\n";
            V3Stats::جوڙيو_Stat_جوڙ(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                داخلا_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            إذا (m_workLists.خالي_جايون() || m_workLists.پٺ().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_پٺ(WorkList{ايندڙ_Work_List_Id++});
                m_workLists.پٺ().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.پٺ();
            list.m_files.ڌڪو_پٺ(
                {داخلا_File.m_filename, داخلا_File.m_score, داخلا_File.m_سائيز_بائيٽون});
            list.m_ڪل_Score += داخلا_File.m_score;
        }
        إذا (m_logp) *m_logp << '\n';
    }

    فراغ سونپيو_Buckets(uint64_t concatenable_Files_ڪل_Score) {
        // Assign buckets to lists

        ثابت سائيز_t ڪل_Buckets_انگ = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        إذا (m_concatenable_Lists_By_Desc_سائيز.سائيز() > ڪل_Buckets_انگ) {
            // Debugging: Log which work lists will be kept
            إذا (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_سائيز.begin(),
                              m_concatenable_Lists_By_Desc_سائيز.begin() + ڪل_Buckets_انگ,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.سائيز()
                                          << "; total score: " << listp->m_ڪل_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_سائيز.begin() + ڪل_Buckets_انگ,
                          m_concatenable_Lists_By_Desc_سائيز.پڇاڙي(), [&](WorkList* listp) {
                              listp->m_isConcatenable = خطأ;

                              إذا (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.سائيز()
                                          << "; total score: " << listp->m_ڪل_Score << ")\n";
                          });
            إذا (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_سائيز.resize(ڪل_Buckets_انگ);
            // Recalculate stats
            concatenable_Files_ڪل_Score = 0;
            لكل (ثابت WorkList* ثابت listp : m_concatenable_Lists_By_Desc_سائيز) {
                concatenable_Files_ڪل_Score += listp->m_ڪل_Score;
            }
        }

        ثابت uint64_t ideal_ٻالٽي_Score = concatenable_Files_ڪل_Score / ڪل_Buckets_انگ;

        V3Stats::جوڙيو_Stat("Concatenation ideal bucket score", ideal_ٻالٽي_Score);

        إذا (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        صحيح موجود_Buckets = v3Global.opt.outputGroups();
        لكل (WorkList* listp : m_concatenable_Lists_By_Desc_سائيز) {
            إذا (موجود_Buckets > 0 && ideal_ٻالٽي_Score > 0) {
                listp->m_buckets_انگ = std::min(
                    موجود_Buckets, std::max<صحيح>(1, listp->m_ڪل_Score / ideal_ٻالٽي_Score));
                موجود_Buckets -= listp->m_buckets_انگ;
                إذا (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_انگ << "] Work List #"
                            << listp->m_dbgId << '\n';
            } وإلا {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = خطأ;
                إذا (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::کاٻي << std::setw(4) << listp->m_dbgId
                            << std::ساڄي << " (excluding from concatenation)\n";
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
                    m_outputFiles.ڌڪو_پٺ({std::move(file.m_filename), {}});
                }
                استمر;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ثابت uint64_t list_Ideal_ٻالٽي_Score = list.m_ڪل_Score / list.m_buckets_انگ;

            std::ويڪٽر<std::ويڪٽر<ثابت FilenameWithScore*>> buckets;
            خودڪار fileIt = list.m_files.cbegin();
            لكل (صحيح i = 0; i < list.m_buckets_انگ; ++i) {
                std::ويڪٽر<ثابت FilenameWithScore*> ٻالٽي;
                uint64_t ٻالٽي_Score = 0;

                لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                    ثابت uint64_t diffNow
                        = std::مطلق_قدر(static_cast<int64_t>(list_Ideal_ٻالٽي_Score - ٻالٽي_Score));
                    ثابت uint64_t diffIfAdded = std::مطلق_قدر(static_cast<int64_t>(
                        list_Ideal_ٻالٽي_Score - ٻالٽي_Score - fileIt->m_score));
                    إذا (ٻالٽي_Score == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        ٻالٽي_Score += fileIt->m_score;
                        ٻالٽي.ڌڪو_پٺ(&*fileIt);
                    } وإلا {
                        // Best possible bucket score reached, process next bucket.
                        اكسر;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.ڌڪو_پٺ(std::move(ٻالٽي));
            }
            لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.پٺ().ڌڪو_پٺ(&*fileIt);
            }
            لكل (ثابت std::ويڪٽر<ثابت FilenameWithScore*>& ٻالٽي : buckets) {
                جوڙيو_ٻالٽي_Groups(ٻالٽي);
            }
        }
    }

    فراغ جوڙيو_ٻالٽي_Groups(ثابت std::ويڪٽر<ثابت FilenameWithScore*>& ٻالٽي) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::ويڪٽر<std::ويڪٽر<std::string>> groups;
        uint64_t گروهه_بائيٽون = 0;
        لكل (ثابت FilenameWithScore* ثابت filep : ٻالٽي) {
            إذا (groups.خالي_جايون() || گروهه_بائيٽون + filep->m_سائيز_بائيٽون > m_max_گروهه_بائيٽون) {
                groups.emplace_پٺ();
                گروهه_بائيٽون = 0;
            }
            گروهه_بائيٽون += filep->m_سائيز_بائيٽون;
            groups.پٺ().ڌڪو_پٺ(filep->m_filename);
        }
        لكل (std::ويڪٽر<std::string>& گروهه : groups) {
            إذا (گروهه.سائيز() == 1) {
                m_outputFiles.ڌڪو_پٺ({std::move(گروهه.front()), {}});
            } وإلا {
                m_outputFiles.ڌڪو_پٺ({گروهه_Filename(m_گروهه_انگ++), std::move(گروهه)});
            }
        }
    }

    فراغ assertFilesSame() ثابت {
        خودڪار ifIt = m_داخلا_Files.begin();
        خودڪار جو_It = m_outputFiles.begin();
        طالما (ifIt != m_داخلا_Files.پڇاڙي() && جو_It != m_outputFiles.پڇاڙي()) {
            إذا (جو_It->isConcatenatingFile()) {
                لكل (ثابت string& ocf : جو_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_داخلا_Files.پڇاڙي(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++جو_It;
            } وإلا {
                UASSERT(ifIt->m_filename == جو_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << جو_It->m_filename);
                ++ifIt;
                ++جو_It;
            }
        }
        UASSERT(ifIt == m_داخلا_Files.پڇاڙي(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(جو_It == m_outputFiles.پڇاڙي(),
                "More output files than input files. First extra file: " << جو_It->m_filename);
    }

    فراغ process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_گروهه_File_اڳياڙي);
        UINFO(5, "Number of input files: " << m_داخلا_Files.سائيز());
        UINFO(5, "Total score: " << m_ڪل_Score);

        ثابت صحيح ڪل_Buckets_انگ = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << ڪل_Buckets_انگ);
        UASSERT(ڪل_Buckets_انگ > 0, "More than 0 buckets required");

        إذا (fallbackNoGrouping(m_داخلا_Files.سائيز())) أرجع;

        إذا (debug() >= 6 || dumpLevel() >= 6) {
            ثابت string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            إذا (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        إذا (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        سائيز_t concatenable_Files_ڳڻپ = 0;
        int64_t concatenable_Files_ڪل_Score = 0;

        لكل (WorkList& list : m_workLists) {
            إذا (!list.m_isConcatenable) استمر;

            // "Concatenation" of a single file is pointless
            إذا (list.m_files.سائيز() == 1) {
                list.m_isConcatenable = خطأ;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                استمر;
            }

            concatenable_Files_ڳڻپ += list.m_files.سائيز();
            concatenable_Files_ڪل_Score += list.m_ڪل_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_سائيز.ڌڪو_پٺ(&list);
        }

        إذا (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        إذا (fallbackNoGrouping(concatenable_Files_ڳڻپ)) أرجع;

        std::ترتيب_ڏيو(m_concatenable_Lists_By_Desc_سائيز.begin(), m_concatenable_Lists_By_Desc_سائيز.پڇاڙي(),
                  [](ثابت WorkList* ap, ثابت WorkList* bp) {
                      // Sort in descending order by number of files
                      إذا (ap->m_files.سائيز() != bp->m_files.سائيز())
                          أرجع (ap->m_files.سائيز() > bp->m_files.سائيز());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      أرجع bp->m_ڪل_Score > ap->m_ڪل_Score;
                  });

        سونپيو_Buckets(concatenable_Files_ڪل_Score);

        buildOutputList();

        إذا (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

عام:
    ساكن std::ويڪٽر<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::ويڪٽر<FilenameWithScore> داخلا_Files, uint64_t ڪل_Score,
                                ثابت std::string& گروهه_File_اڳياڙي, uint64_t max_گروهه_بائيٽون) {
        Emit_گروهه گروهه{std::move(داخلا_Files), ڪل_Score, گروهه_File_اڳياڙي, max_گروهه_بائيٽون};
        گروهه.process();
        أرجع گروهه.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

صنف EmitMk final {
    استعمال FileOrConcatenatedFilesList = Emit_گروهه::FileOrConcatenatedFilesList;
    استعمال FilenameWithScore = Emit_گروهه::FilenameWithScore;

    // MEMBERS
    مزدوج m_put_Class_ڳڻپ = 0;  // Number of classEntries printed

عام:
    // METHODS

    ساكن فراغ emitConcatenatingFile(ثابت FileOrConcatenatedFilesList& داخلا_2) {
        UASSERT(داخلا_2.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_ٻاهريون_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + داخلا_2.m_filename + ".cpp"};
        concatenatingFile.puts_سرو();
        لكل (ثابت string& file : داخلا_2.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    ساكن uint64_t file_سائيز(ثابت AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::ٻٽو | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        أرجع static_cast<uint64_t>(ifs.tellg());
    }

    فراغ put_Make_Class_داخلا(V3_ٻاهريون_Mk_File& جو, ثابت string& name) {
        جو.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_ڳڻپ;
    }

    فراغ emitClassMake() {
        std::ويڪٽر<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::ويڪٽر<FileOrConcatenatedFilesList> vmClassesFastList;
        إذا (v3Global.opt.outputGroups() > 0) {
            std::ويڪٽر<FilenameWithScore> slowFiles;
            std::ويڪٽر<FilenameWithScore> fastFiles;
            uint64_t slow_ڪل_Score = 0;
            uint64_t fast_ڪل_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t سرو_بائيٽون = 0;

            لكل (Ast_ڳنڍ_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), ڳنڍ_File)) {
                ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                إذا (cfilep && VString::پڇاڙيون_With(cfilep->name(), ".h")) {
                    سرو_بائيٽون += file_سائيز(cfilep);
                }
                إذا (cfilep && cfilep->ذريعو() && cfilep->support() == خطأ) {
                    std::ويڪٽر<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& ڪل_Score = cfilep->slow() ? slow_ڪل_Score : fast_ڪل_Score;

                    ڪل_Score += cfilep->complexityScore();
                    files.ڌڪو_پٺ({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_سائيز(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_گروهه_بائيٽون = std::numeric_limits<uint64_t>::max();
            إذا (v3Global.opt.output_Groups_سائيز()) {
                ثابت uint64_t max_Tu_بائيٽون = static_cast<uint64_t>(v3Global.opt.output_Groups_سائيز());
                max_گروهه_بائيٽون = max_Tu_بائيٽون > سرو_بائيٽون ? max_Tu_بائيٽون - سرو_بائيٽون : 0;
            }

            vmClassesSlowList = Emit_گروهه::singleConcatenatedFilesList(
                std::move(slowFiles), slow_ڪل_Score, "vm_classes_Slow_", max_گروهه_بائيٽون);
            vmClassesFastList = Emit_گروهه::singleConcatenatedFilesList(
                std::move(fastFiles), fast_ڪل_Score, "vm_classes_", max_گروهه_بائيٽون);
        }

        // Generate the makefile
        V3_ٻاهريون_Mk_File جو{v3Global.opt.makeDir() + "/" + v3Global.opt.اڳياڙي() + "_classes.mk"};
        جو.puts_سرو();
        جو.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        جو.puts("#\n");
        جو.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        جو.puts("# See " + v3Global.opt.اڳياڙي() + ".mk" + " for the caller.\n");

        جو.puts("\n### Switches...\n");
        جو.puts("# C11 constructs required?  0/1 (always on now)\n");
        جو.put_سيٽ("VM_C11", "1");
        جو.puts("# Timing enabled?  0/1\n");
        جو.put_سيٽ("VM_TIMING", v3Global.استعمال_ڪري_Timing() ? "1" : "0");
        جو.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        جو.put_سيٽ("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        جو.puts("# Parallel builds?  0/1 (from --output-split)\n");
        جو.put_سيٽ("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        جو.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        جو.put_سيٽ("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        جو.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        جو.put_سيٽ("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        جو.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        جو.put_سيٽ("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        جو.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        جو.put_سيٽ("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        جو.puts("# VPI enabled?  0/1 (from --vpi)\n");
        جو.put_سيٽ("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        جو.puts("\n### Object file lists...\n");
        لكل (صحيح support = 0; support < 3; ++support) {
            لكل (ثابت منطقي& slow : {خطأ, سچ}) {
                إذا (support == 2) {
                    جو.puts("# Global classes, need linked once per executable");
                } وإلا إذا (support) {
                    جو.puts("# Generated support classes");
                } وإلا {
                    جو.puts("# Generated module classes");
                }
                إذا (slow) {
                    جو.puts(", non-fast-path, compile with low/medium optimization\n");
                } وإلا {
                    جو.puts(", fast-path, compile with highest optimization\n");
                }
                ثابت string هدف_Var = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_ڳڻپ = 0;
                جو.puts(هدف_Var + " += \\\n");
                إذا (support == 2 && v3Global.opt.hier_ٻار()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } وإلا إذا (support == 2 && !slow) {
                    لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_داخلا(جو, cpp);
                } وإلا إذا (support == 2 && slow) {
                } وإلا إذا (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ثابت std::ويڪٽر<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    لكل (ثابت FileOrConcatenatedFilesList& داخلا_2 : list) {
                        إذا (داخلا_2.isConcatenatingFile()) emitConcatenatingFile(داخلا_2);
                        put_Make_Class_داخلا(جو, داخلا_2.m_filename);
                    }
                } وإلا {
                    لكل (Ast_ڳنڍ_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), ڳنڍ_File)) {
                        ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                        إذا (cfilep && cfilep->ذريعو() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_داخلا(جو, cfilep->name());
                        }
                    }
                }
                جو.puts("\n");
                V3Stats::جوڙيو_Stat("Makefile targets, " + هدف_Var, m_put_Class_ڳڻپ);
            }
        }

        جو.puts_سرو();
    }

    فراغ emitOverallMake() {
        // Generate the makefile
        V3_ٻاهريون_Mk_File جو{v3Global.opt.makeDir() + "/" + v3Global.opt.اڳياڙي() + ".mk"};
        جو.puts_سرو();
        جو.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        جو.puts("#\n");
        جو.puts("# Execute this makefile from the object directory:\n");
        جو.puts("#    make -f " + v3Global.opt.اڳياڙي() + ".mk" + "\n");
        جو.puts("\n");

        إذا (v3Global.opt.exe()) {
            جو.puts("default: " + v3Global.opt.exeName() + "\n");
        } وإلا إذا (!v3Global.opt.libCreate().خالي_جايون()) {
            جو.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } وإلا {
            جو.puts("default: lib" + v3Global.opt.اڳياڙي() + "\n");
        }
        جو.puts("\n### Constants...\n");
        جو.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        جو.put_سيٽ("PERL", V3_ٻاهريون_Formatter::quoteNameControls(V3Options::getenvPERL()));
        جو.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        جو.put_سيٽ("PYTHON3", V3_ٻاهريون_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        جو.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        جو.put_سيٽ("VERILATOR_ROOT",
                  V3_ٻاهريون_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_پاڙ()));
        جو.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        جو.put_سيٽ_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        جو.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        جو.put_سيٽ_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        إذا (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        جو.puts("\n### Switches...\n");
        جو.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        جو.put_سيٽ("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        جو.puts("# SystemC output mode?  0/1 (from --sc)\n");
        جو.put_سيٽ("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        جو.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        جو.put_سيٽ("VM_SP_OR_SC", "$(VM_SC)");
        جو.puts("# Deprecated\n");
        جو.put_سيٽ("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        جو.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        جو.put_سيٽ("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        جو.puts("\n### Vars...\n");
        جو.puts("# Module prefix (from --prefix)\n");
        جو.put_سيٽ("VM_MODPREFIX", v3Global.opt.mod_اڳياڙي());
        جو.puts("# Design prefix (from --prefix)\n");
        جو.put_سيٽ("VM_PREFIX", v3Global.opt.اڳياڙي());
        جو.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        جو.put_سيٽ("VM_PREFIX_INCLUDE", "<" + v3Global.opt.اڳياڙي() + ".h>");
        إذا (v3Global.dpi()) {
            جو.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            جو.put_سيٽ("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.اڳياڙي() + "__Dpi.h>");
        }

        جو.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        جو.puts("VM_USER_CFLAGS = \\\n");
        ثابت std::string solver = V3Options::getenvVERILATOR_SOLVER();
        إذا (v3Global.useRandomizeMethods() && solver != "")
            جو.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3_ٻاهريون_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        إذا (!v3Global.opt.libCreate().خالي_جايون()) جو.puts("\t-fPIC \\\n");
        ثابت VStringList& cFlags = v3Global.opt.cFlags();
        لكل (ثابت string& i : cFlags) جو.puts("  " + i + " \\\n");
        جو.puts("\n");

        جو.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        جو.puts("VM_USER_LDLIBS = \\\n");
        ثابت VStringList& ldLibs = v3Global.opt.ldLibs();
        لكل (ثابت string& i : ldLibs) جو.puts("  " + i + " \\\n");
        جو.puts("\n");

        جو.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        جو.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        جو.puts("VM_USER_LDLIBS_FILES = \\\n");
        لكل (ثابت string& i : v3Global.opt.ldLibFiles()) جو.puts("  " + i + " \\\n");
        جو.puts("\n");

        VString_سيٽ dirs;
        جو.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        جو.puts("VM_USER_CLASSES = \\\n");
        ثابت VString_سيٽ& cppFiles = v3Global.opt.cppFiles();
        لكل (ثابت خودڪار& cppfile : cppFiles) {
            جو.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ثابت string dir
                = V3Os::filename_Relative_رستو(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.وجهو(dir);
        }
        dirs.وجهو(V3Os::filename_Relative_رستو(".", v3Global.opt.makeDir()));

        جو.puts("\n");

        جو.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        جو.puts("VM_USER_DIR = \\\n");
        لكل (ثابت خودڪار& i : dirs) جو.puts("  " + i + " \\\n");
        جو.puts("\n");

        جو.puts("### Default rules...\n");
        جو.puts("# Include list of all generated classes\n");
        جو.puts("include " + v3Global.opt.اڳياڙي() + "_classes.mk\n");
        إذا (v3Global.opt.hier_چوٽي()) {
            جو.puts("# Include rules for hierarchical blocks\n");
            جو.puts("include " + v3Global.opt.اڳياڙي() + "_hier.mk\n");
        }
        جو.puts("# Include global rules\n");
        جو.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        إذا (v3Global.opt.exe()) {
            جو.puts("\n### Executable rules... (from --exe)\n");
            جو.puts("VPATH += $(VM_USER_DIR)\n");
            جو.puts("\n");

            إذا (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                جو.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                جو.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                جو.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                جو.puts("\n");
            }
        }

        ثابت string compilerIncludePch
            = v3Global.opt.compilerIncludes().خالي_جايون() ? "" : "$(VK_PCH_H).fast.gch";
        ثابت string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().خالي_جايون() ? "" : "$(VK_PCH_I_FAST)";
        لكل (ثابت string& cppfile : cppFiles) {
            ثابت string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            جو.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            جو.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        إذا (v3Global.opt.exe()) {
            جو.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            جو.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            جو.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            جو.puts("\n");
        } وإلا إذا (!v3Global.opt.libCreate().خالي_جايون()) {
            ثابت string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            جو.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            جو.puts(v3Global.opt.libCreateName(خطأ) + ": " + libCreateDeps + "\n");
            جو.puts("\n");
            إذا (v3Global.opt.hier_ٻار()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                جو.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + "\n");
            } وإلا {
                جو.puts(v3Global.opt.libCreateName(سچ) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                جو.puts("ifeq ($(shell uname -s),Darwin)\n");
                جو.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                جو.puts("else\n");
                جو.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                جو.puts("endif\n");
                جو.puts("\n");
                جو.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + " " + v3Global.opt.libCreateName(سچ) + "\n");
            }
        } وإلا {
            ثابت string libname = "lib" + v3Global.opt.اڳياڙي() + ".a";
            جو.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            جو.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            جو.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            جو.puts("lib" + v3Global.opt.اڳياڙي() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        جو.puts_سرو();
    }

    واضح EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    مجازي ~EmitMk() = افتراضي;
};

//######################################################################

صنف EmitMkHierVerilation final {
    ثابت V3_Hier_گراف* ثابت m_graphp;
    ثابت string m_makefile;  // path of this makefile
    فراغ emitCommonOpts(V3_ٻاهريون_Mk_File& جو) ثابت {
        جو.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        جو.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        جو.puts("# Common options for hierarchical blocks\n");
        ثابت string fullpath_bin = V3Os::filename_Real_رستو(v3Global.opt.buildDepBin());
        ثابت string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        جو.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        جو.puts("VM_HIER_INPUT_FILES := \\\n");
        لكل (ثابت خودڪار& i : v3Global.opt.vFiles())
            جو.puts("  " + V3Os::filename_Real_رستو(i.filename()) + " \\\n");
        جو.puts("\n");
        جو.puts("VM_HIER_VERILOG_LIBS := \\\n");
        لكل (ثابت خودڪار& i : v3Global.opt.libraryFiles()) {
            جو.puts("  " + V3Os::filename_Real_رستو(i.filename()) + " \\\n");
        }
        جو.puts("\n");
    }
    فراغ emitLaunchVerilator(V3_ٻاهريون_Mk_File& جو, ثابت string& دليلون_File) ثابت {
        جو.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        جو.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + دليلون_File + "\"\n");
    }
    فراغ emit(V3_ٻاهريون_Mk_File& جو) ثابت {
        جو.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        جو.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        جو.puts("#\n");
        جو.puts("# The main makefile " + v3Global.opt.اڳياڙي() + ".mk calls this makefile\n");
        جو.puts("\n");
        جو.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        جو.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        جو.puts(".SUFFIXES:\n");
        جو.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        جو.puts("# Libraries of hierarchical blocks\n");
        جو.puts("VM_HIER_LIBS := \\\n");
        لكل (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلاڪ* ثابت blockp = vtx.as<V3_Hier_بلاڪ>();
            جو.puts("  " + blockp->hierLibFilename(سچ) + " \\\n");
        }
        جو.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        جو.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.اڳياڙي() + ".mk\n");
        جو.puts("\t$(MAKE) -f " + v3Global.opt.اڳياڙي() + ".mk\n");
        جو.puts("hier_verilation: " + v3Global.opt.اڳياڙي() + ".mk\n");
        emitCommonOpts(جو);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        جو.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        جو.puts("hier_launch_verilator:\n");
        جو.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ثابت string دليلون_File = v3Global.hierGraphp()->چوٽي_Command_دليلون_Filename(خطأ);
            جو.puts("\n# Verilate the top module\n");
            جو.puts(v3Global.opt.اڳياڙي()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            جو.puts(V3Os::filenameNonDir(دليلون_File) + " ");
            لكل (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
                ثابت V3_Hier_بلاڪ* ثابت blockp = vtx.as<V3_Hier_بلاڪ>();
                جو.puts(blockp->hierWrapperFilename(سچ) + " ");
            }
            جو.puts("\n");
            emitLaunchVerilator(جو, دليلون_File);
        }

        // Rules to process hierarchical blocks
        جو.puts("\n# Verilate hierarchical blocks\n");
        لكل (ثابت V3_گراف_راس& vtx : m_graphp->vertices()) {
            ثابت V3_Hier_بلاڪ* ثابت blockp = vtx.as<V3_Hier_بلاڪ>();
            ثابت string اڳياڙي = blockp->hier_اڳياڙي();
            ثابت string دليلون_Filename = blockp->command_دليلون_Filename(خطأ);
            جو.puts(blockp->hierGeneratedFilenames(سچ));
            جو.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            جو.puts(V3Os::filenameNonDir(دليلون_Filename) + " ");
            لكل (ثابت V3_گراف_ڪنارو& ڪنارو : blockp->ٻاهريون_ڪنارا()) {
                ثابت V3_Hier_بلاڪ* ثابت dependencyp = ڪنارو.چوٽي()->as<V3_Hier_بلاڪ>();
                جو.puts(dependencyp->hierWrapperFilename(سچ) + " ");
            }
            جو.puts("\n");
            emitLaunchVerilator(جو, دليلون_Filename);

            // Rule to build lib*.a
            جو.puts(blockp->hierLibFilename(سچ));
            جو.puts(": ");
            جو.puts(blockp->hierMkFilename(سچ));
            جو.puts(" ");
            لكل (ثابت V3_گراف_ڪنارو& ڪنارو : blockp->ٻاهريون_ڪنارا()) {
                ثابت V3_Hier_بلاڪ* ثابت dependencyp = ڪنارو.چوٽي()->as<V3_Hier_بلاڪ>();
                جو.puts(dependencyp->hierLibFilename(سچ));
                جو.puts(" ");
            }
            جو.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(خطأ) + " -C " + اڳياڙي);
            جو.puts(" VM_PREFIX=" + اڳياڙي);
            جو.puts("\n\n");
        }
        جو.puts("endif  # Guard\n");
    }

عام:
    واضح EmitMkHierVerilation(ثابت V3_Hier_گراف* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.اڳياڙي() + "_hier.mk"} {
        V3_ٻاهريون_Mk_File جو{m_makefile};
        emit(جو);
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
