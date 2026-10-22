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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3EmitMk.h"

#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3HierBlock.h"
#অন্তর্ভাব "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Groups adjacent files in a list, evenly distributing sum of scores
শ্রেণী EmitGroup final {
সর্বজনীন:
    কাঠামো FileOrConcatenatedFilesList final {
        ধ্রুবক std::string m_filename;  // Filename or output group filename if grouping
        std::vector<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        যৌক্তিক isConcatenatingFile() ধ্রুবক { ফেরত !m_concatenatedFilenames.empty(); }
    };

    কাঠামো FilenameWithScore final {
        ধ্রুবক std::string m_filename;  // Input filename
        ধ্রুবক uint64_t m_score;  // Input file complexity
        ধ্রুবক uint64_t m_আকার_Bytes;  // Input file size
    };

ব্যক্তিগত:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    কাঠামো WorkList final {
        uint64_t m_যোগ_Score = 0;  // Sum of scores of included files
        std::vector<FilenameWithScore> m_files;  // Included filenames
        পূর্ণসংখ্যা m_buckets_অঙ্ক
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        যৌক্তিক m_isConcatenable = সত্য;  // Indicated whether files on this list can be concatenated.
        ধ্রুবক পূর্ণসংখ্যা m_dbgId;  // Work list ID for debugging.

        WorkList() = delete;
        explicit WorkList(পূর্ণসংখ্যা id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    স্থির constexpr আকার_t MIN_FILES_গুনতি = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    স্থির constexpr আকার_t MIN_FILES_PER_BUCKET = 4;

    // MEMBERS

    ধ্রুবক std::vector<FilenameWithScore>
        m_প্রবেশ_Files;  // List of filenames from initial AstCFile list
    std::vector<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ধ্রুবক uint64_t m_যোগ_Score;  // Sum of file scores
    ধ্রুবক std::string m_groupFilePrefix;  // Prefix for output group filenames
    ধ্রুবক uint64_t m_maxGroupBytes;  // Maximum sum of file sizes in an output group
    std::vector<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::vector<WorkList*> m_concatenable_Lists_By_Desc_আকার;  // Lists sorted by size, descending
    পূর্ণসংখ্যা m_group_অঙ্ক = 0;  // Number of output groups created

    EmitGroup(std::vector<FilenameWithScore> প্রবেশ_Files, uint64_t যোগ_Score,
              ধ্রুবক std::string& groupFilePrefix, uint64_t maxGroupBytes)
        : m_প্রবেশ_Files{std::move(প্রবেশ_Files)}
        , m_যোগ_Score{যোগ_Score}
        , m_groupFilePrefix{groupFilePrefix}
        , m_maxGroupBytes{maxGroupBytes} {}

    string groupFilename(পূর্ণসংখ্যা id) ধ্রুবক {
        ফেরত v3Global.opt.prefix() + "_" + m_groupFilePrefix + std::to_string(id);
    }

    // Debug logging: prints scores histogram
    শূন্য dumpLogScoreHistogram(std::ostream& os) {
        constexpr পূর্ণসংখ্যা MAX_BAR_দৈর্ঘ্য = 80;
        constexpr পূর্ণসংখ্যা MAX_INTERVALS_অঙ্ক = 60;

        // All scores arranged in ascending order.
        std::vector<uint64_t> sortedScores;
        sortedScores.reserve(m_প্রবেশ_Files.আকার_2());
        std::transform(m_প্রবেশ_Files.begin(), m_প্রবেশ_Files.end(), std::back_inserter(sortedScores),
                       [](ধ্রুবক FilenameWithScore& প্রবেশ_File) { ফেরত প্রবেশ_File.m_score; });
        std::sort(sortedScores.begin(), sortedScores.end());

        ধ্রুবক int64_t শীর্ষ_Score = sortedScores.back();
        os << "Top score: " << শীর্ষ_Score << '\n';
        ধ্রুবক পূর্ণসংখ্যা maxScoreWidth = std::to_string(শীর্ষ_Score).দৈর্ঘ্য();

        ধ্রুবক int64_t intervals_অঙ্ক = std::min<int64_t>(শীর্ষ_Score + 1, MAX_INTERVALS_অঙ্ক);

        কাঠামো Interval final {
            uint64_t m_lowerBound = 0;  // Lowest score included in this histogram bucket
            পূর্ণসংখ্যা m_আকার = 0;  // Number of scores that fell into this bucket
        };

        std::vector<Interval> intervals;
        intervals.resize(intervals_অঙ্ক);

        intervals[0].m_lowerBound = 0;
        জন্য (পূর্ণসংখ্যা i = 1; i < intervals_অঙ্ক; i++) {
            intervals[i].m_lowerBound = (শীর্ষ_Score + 1) * i / intervals_অঙ্ক + 1;
        }

        জন্য (ধ্রুবক uint64_t score : sortedScores) {
            ধ্রুবক unsigned পূর্ণসংখ্যা ivIdx = score * intervals_অঙ্ক / (শীর্ষ_Score + 1);
            ++intervals[ivIdx].m_আকার;
        }

        পূর্ণসংখ্যা শীর্ষ_Interval_আকার = 0;
        জন্য (ধ্রুবক Interval& iv : intervals)
            শীর্ষ_Interval_আকার = std::max(শীর্ষ_Interval_আকার, iv.m_আকার);

        os << "Input files' scores histogram:\n";

        জন্য (ধ্রুবক Interval& iv : intervals) {
            ধ্রুবক পূর্ণসংখ্যা scaled_আকার = iv.m_আকার * (MAX_BAR_দৈর্ঘ্য + 1) / শীর্ষ_Interval_আকার;
            ধ্রুবক std::string সারি = " |" + std::string(scaled_আকার, '#');

            os << std::setw(maxScoreWidth) << iv.m_lowerBound << সারি << "  " << iv.m_আকার << '\n';
        }
        os << std::setw(maxScoreWidth) << (শীর্ষ_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    শূন্য dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        জন্য (ধ্রুবক WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.আকার_2()
               << "; total score: " << list.m_যোগ_Score << ")\n";
            যদি (debug() >= 6 || list.m_files.আকার_2() < 4) {
                // Log all files
                জন্য (ধ্রুবক FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } অন্যথা {
                // Log only first and last file
                ধ্রুবক FilenameWithScore& first = list.m_files.front();
                ধ্রুবক FilenameWithScore& last = list.m_files.back();
                os << "| + " << first.m_filename << "  (score: " << first.m_score << ")\n";
                os << "| | (... " << (list.m_files.আকার_2() - 2) << " files ...)\n";
                os << "| + " << last.m_filename << "  (score: " << last.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    শূন্য dumpOutputList(std::ostream& os) ধ্রুবক {
        os << "List of output files after execution of concatenation:\n";

        জন্য (ধ্রুবক FileOrConcatenatedFilesList& entry : m_outputFiles) {
            যদি (entry.isConcatenatingFile()) {
                os << "+ " << entry.m_filename << "  (concatenating file)\n";
                জন্য (ধ্রুবক string& f : entry.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } অন্যথা {
                os << "+ " << entry.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    যৌক্তিক fallbackNoGrouping(আকার_t প্রবেশ_Files_গুনতি) {
        ধ্রুবক পূর্ণসংখ্যা যোগ_Buckets_অঙ্ক = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        যৌক্তিক groupingRedundant = মিথ্যা;
        যদি (প্রবেশ_Files_গুনতি < MIN_FILES_গুনতি
            && প্রবেশ_Files_গুনতি <= static_cast<আকার_t>(যোগ_Buckets_অঙ্ক)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_প্রবেশ_Files.আকার_2() << " < "
                                                                   << MIN_FILES_গুনতি << ")");
            groupingRedundant = সত্য;
        }
        যদি (প্রবেশ_Files_গুনতি < (MIN_FILES_PER_BUCKET * যোগ_Buckets_অঙ্ক)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_প্রবেশ_Files.আকার_2() << " < " << MIN_FILES_PER_BUCKET << " - "
                         << যোগ_Buckets_অঙ্ক << ")");
            groupingRedundant = সত্য;
        }
        যদি (!groupingRedundant) ফেরত মিথ্যা;

        m_outputFiles.reserve(m_প্রবেশ_Files.আকার_2());
        জন্য (ধ্রুবক FilenameWithScore& filename : m_প্রবেশ_Files) {
            m_outputFiles.ঠেলো_back({std::move(filename.m_filename), {}});
        }
        ফেরত সত্য;
    }

    শূন্য createWorkLists() {
        // Create initial Work Lists.

        ধ্রুবক পূর্ণসংখ্যা যোগ_Buckets_অঙ্ক = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ধ্রুবক uint64_t concatenableFileMaxScore = m_যোগ_Score / যোগ_Buckets_অঙ্ক / 2;

        V3Stats::addStat("Concatenation max score", concatenableFileMaxScore);
        পূর্ণসংখ্যা পরবর্তী_Work_List_Id = 0;

        যদি (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        জন্য (ধ্রুবক FilenameWithScore& প্রবেশ_File : m_প্রবেশ_Files) {
            ধ্রুবক যৌক্তিক fileIsConcatenable = (প্রবেশ_File.m_score <= concatenableFileMaxScore);
            যদি (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << প্রবেশ_File.m_filename << "  (score: " << প্রবেশ_File.m_score << ")\n";
            V3Stats::add_Stat_যোগ(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                প্রবেশ_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            যদি (m_workLists.empty() || m_workLists.back().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_back(WorkList{পরবর্তী_Work_List_Id++});
                m_workLists.back().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.back();
            list.m_files.ঠেলো_back(
                {প্রবেশ_File.m_filename, প্রবেশ_File.m_score, প্রবেশ_File.m_আকার_Bytes});
            list.m_যোগ_Score += প্রবেশ_File.m_score;
        }
        যদি (m_logp) *m_logp << '\n';
    }

    শূন্য assignBuckets(uint64_t concatenable_Files_যোগ_Score) {
        // Assign buckets to lists

        ধ্রুবক আকার_t যোগ_Buckets_অঙ্ক = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        যদি (m_concatenable_Lists_By_Desc_আকার.আকার_2() > যোগ_Buckets_অঙ্ক) {
            // Debugging: Log which work lists will be kept
            যদি (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_আকার.begin(),
                              m_concatenable_Lists_By_Desc_আকার.begin() + যোগ_Buckets_অঙ্ক,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.আকার_2()
                                          << "; total score: " << listp->m_যোগ_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_আকার.begin() + যোগ_Buckets_অঙ্ক,
                          m_concatenable_Lists_By_Desc_আকার.end(), [&](WorkList* listp) {
                              listp->m_isConcatenable = মিথ্যা;

                              যদি (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.আকার_2()
                                          << "; total score: " << listp->m_যোগ_Score << ")\n";
                          });
            যদি (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_আকার.resize(যোগ_Buckets_অঙ্ক);
            // Recalculate stats
            concatenable_Files_যোগ_Score = 0;
            জন্য (ধ্রুবক WorkList* ধ্রুবক listp : m_concatenable_Lists_By_Desc_আকার) {
                concatenable_Files_যোগ_Score += listp->m_যোগ_Score;
            }
        }

        ধ্রুবক uint64_t idealBucketScore = concatenable_Files_যোগ_Score / যোগ_Buckets_অঙ্ক;

        V3Stats::addStat("Concatenation ideal bucket score", idealBucketScore);

        যদি (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        পূর্ণসংখ্যা availableBuckets = v3Global.opt.outputGroups();
        জন্য (WorkList* listp : m_concatenable_Lists_By_Desc_আকার) {
            যদি (availableBuckets > 0 && idealBucketScore > 0) {
                listp->m_buckets_অঙ্ক = std::min(
                    availableBuckets, std::max<পূর্ণসংখ্যা>(1, listp->m_যোগ_Score / idealBucketScore));
                availableBuckets -= listp->m_buckets_অঙ্ক;
                যদি (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_অঙ্ক << "] Work List #"
                            << listp->m_dbgId << '\n';
            } অন্যথা {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = মিথ্যা;
                যদি (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::বাম << std::setw(4) << listp->m_dbgId
                            << std::ডান << " (excluding from concatenation)\n";
            }
        }
        যদি (m_logp) *m_logp << '\n';
    }

    শূন্য buildOutputList() {
        // Assign files to buckets and build final list of files

        // At this point the workLists contains concatenatable file lists separated by one or more
        // non-concatenable file lists. Each concatenatable list has N buckets (where N > 0)
        // assigned to it, which have to be filled with files from this list. Ideally, sum of file
        // scores in every bucket should be the same.
        জন্য (WorkList& list : m_workLists) {
            যদি (!list.m_isConcatenable) {
                জন্য (ধ্রুবক FilenameWithScore& file : list.m_files) {
                    m_outputFiles.ঠেলো_back({std::move(file.m_filename), {}});
                }
                চালু;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ধ্রুবক uint64_t listIdealBucketScore = list.m_যোগ_Score / list.m_buckets_অঙ্ক;

            std::vector<std::vector<ধ্রুবক FilenameWithScore*>> buckets;
            auto fileIt = list.m_files.cbegin();
            জন্য (পূর্ণসংখ্যা i = 0; i < list.m_buckets_অঙ্ক; ++i) {
                std::vector<ধ্রুবক FilenameWithScore*> bucket;
                uint64_t bucketScore = 0;

                জন্য (; fileIt != list.m_files.cend(); ++fileIt) {
                    ধ্রুবক uint64_t diffNow
                        = std::abs(static_cast<int64_t>(listIdealBucketScore - bucketScore));
                    ধ্রুবক uint64_t diffIfAdded = std::abs(static_cast<int64_t>(
                        listIdealBucketScore - bucketScore - fileIt->m_score));
                    যদি (bucketScore == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        bucketScore += fileIt->m_score;
                        bucket.ঠেলো_back(&*fileIt);
                    } অন্যথা {
                        // Best possible bucket score reached, process next bucket.
                        বিরতি;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.ঠেলো_back(std::move(bucket));
            }
            জন্য (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.back().ঠেলো_back(&*fileIt);
            }
            জন্য (ধ্রুবক std::vector<ধ্রুবক FilenameWithScore*>& bucket : buckets) {
                addBucketGroups(bucket);
            }
        }
    }

    শূন্য addBucketGroups(ধ্রুবক std::vector<ধ্রুবক FilenameWithScore*>& bucket) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::vector<std::vector<std::string>> groups;
        uint64_t groupBytes = 0;
        জন্য (ধ্রুবক FilenameWithScore* ধ্রুবক filep : bucket) {
            যদি (groups.empty() || groupBytes + filep->m_আকার_Bytes > m_maxGroupBytes) {
                groups.emplace_back();
                groupBytes = 0;
            }
            groupBytes += filep->m_আকার_Bytes;
            groups.back().ঠেলো_back(filep->m_filename);
        }
        জন্য (std::vector<std::string>& group : groups) {
            যদি (group.আকার_2() == 1) {
                m_outputFiles.ঠেলো_back({std::move(group.front()), {}});
            } অন্যথা {
                m_outputFiles.ঠেলো_back({groupFilename(m_group_অঙ্ক++), std::move(group)});
            }
        }
    }

    শূন্য assertFilesSame() ধ্রুবক {
        auto ifIt = m_প্রবেশ_Files.begin();
        auto ofIt = m_outputFiles.begin();
        যখন (ifIt != m_প্রবেশ_Files.end() && ofIt != m_outputFiles.end()) {
            যদি (ofIt->isConcatenatingFile()) {
                জন্য (ধ্রুবক string& ocf : ofIt->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_প্রবেশ_Files.end(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++ofIt;
            } অন্যথা {
                UASSERT(ifIt->m_filename == ofIt->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << ofIt->m_filename);
                ++ifIt;
                ++ofIt;
            }
        }
        UASSERT(ifIt == m_প্রবেশ_Files.end(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(ofIt == m_outputFiles.end(),
                "More output files than input files. First extra file: " << ofIt->m_filename);
    }

    শূন্য process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_groupFilePrefix);
        UINFO(5, "Number of input files: " << m_প্রবেশ_Files.আকার_2());
        UINFO(5, "Total score: " << m_যোগ_Score);

        ধ্রুবক পূর্ণসংখ্যা যোগ_Buckets_অঙ্ক = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << যোগ_Buckets_অঙ্ক);
        UASSERT(যোগ_Buckets_অঙ্ক > 0, "More than 0 buckets required");

        যদি (fallbackNoGrouping(m_প্রবেশ_Files.আকার_2())) ফেরত;

        যদি (debug() >= 6 || dumpLevel() >= 6) {
            ধ্রুবক string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            যদি (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        যদি (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        আকার_t concatenable_Files_গুনতি = 0;
        int64_t concatenable_Files_যোগ_Score = 0;

        জন্য (WorkList& list : m_workLists) {
            যদি (!list.m_isConcatenable) চালু;

            // "Concatenation" of a single file is pointless
            যদি (list.m_files.আকার_2() == 1) {
                list.m_isConcatenable = মিথ্যা;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                চালু;
            }

            concatenable_Files_গুনতি += list.m_files.আকার_2();
            concatenable_Files_যোগ_Score += list.m_যোগ_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_আকার.ঠেলো_back(&list);
        }

        যদি (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        যদি (fallbackNoGrouping(concatenable_Files_গুনতি)) ফেরত;

        std::sort(m_concatenable_Lists_By_Desc_আকার.begin(), m_concatenable_Lists_By_Desc_আকার.end(),
                  [](ধ্রুবক WorkList* ap, ধ্রুবক WorkList* bp) {
                      // Sort in descending order by number of files
                      যদি (ap->m_files.আকার_2() != bp->m_files.আকার_2())
                          ফেরত (ap->m_files.আকার_2() > bp->m_files.আকার_2());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      ফেরত bp->m_যোগ_Score > ap->m_যোগ_Score;
                  });

        assignBuckets(concatenable_Files_যোগ_Score);

        buildOutputList();

        যদি (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

সর্বজনীন:
    স্থির std::vector<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::vector<FilenameWithScore> প্রবেশ_Files, uint64_t যোগ_Score,
                                ধ্রুবক std::string& groupFilePrefix, uint64_t maxGroupBytes) {
        EmitGroup group{std::move(প্রবেশ_Files), যোগ_Score, groupFilePrefix, maxGroupBytes};
        group.process();
        ফেরত group.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

শ্রেণী EmitMk final {
    using FileOrConcatenatedFilesList = EmitGroup::FileOrConcatenatedFilesList;
    using FilenameWithScore = EmitGroup::FilenameWithScore;

    // MEMBERS
    দ্বিগুণ m_put_Class_গুনতি = 0;  // Number of classEntries printed

সর্বজনীন:
    // METHODS

    স্থির শূন্য emitConcatenatingFile(ধ্রুবক FileOrConcatenatedFilesList& entry) {
        UASSERT(entry.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3OutCFile concatenatingFile{v3Global.opt.makeDir() + "/" + entry.m_filename + ".cpp"};
        concatenatingFile.putsHeader();
        জন্য (ধ্রুবক string& file : entry.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    স্থির uint64_t file_আকার(ধ্রুবক AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::দ্বিমিক | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        ফেরত static_cast<uint64_t>(ifs.tellg());
    }

    শূন্য putMakeClassEntry(V3OutMkFile& of, ধ্রুবক string& name) {
        of.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_গুনতি;
    }

    শূন্য emitClassMake() {
        std::vector<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::vector<FileOrConcatenatedFilesList> vmClassesFastList;
        যদি (v3Global.opt.outputGroups() > 0) {
            std::vector<FilenameWithScore> slowFiles;
            std::vector<FilenameWithScore> fastFiles;
            uint64_t slow_যোগ_Score = 0;
            uint64_t fast_যোগ_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t headerBytes = 0;

            জন্য (Ast_গ্রন্থি_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), গ্রন্থি_File)) {
                ধ্রুবক AstCFile* ধ্রুবক cfilep = VN_CAST(nodep, CFile);
                যদি (cfilep && VString::endsWith(cfilep->name(), ".h")) {
                    headerBytes += file_আকার(cfilep);
                }
                যদি (cfilep && cfilep->source() && cfilep->support() == মিথ্যা) {
                    std::vector<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& যোগ_Score = cfilep->slow() ? slow_যোগ_Score : fast_যোগ_Score;

                    যোগ_Score += cfilep->complexityScore();
                    files.ঠেলো_back({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_আকার(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t maxGroupBytes = std::numeric_limits<uint64_t>::max();
            যদি (v3Global.opt.output_Groups_আকার()) {
                ধ্রুবক uint64_t maxTuBytes = static_cast<uint64_t>(v3Global.opt.output_Groups_আকার());
                maxGroupBytes = maxTuBytes > headerBytes ? maxTuBytes - headerBytes : 0;
            }

            vmClassesSlowList = EmitGroup::singleConcatenatedFilesList(
                std::move(slowFiles), slow_যোগ_Score, "vm_classes_Slow_", maxGroupBytes);
            vmClassesFastList = EmitGroup::singleConcatenatedFilesList(
                std::move(fastFiles), fast_যোগ_Score, "vm_classes_", maxGroupBytes);
        }

        // Generate the makefile
        V3OutMkFile of{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + "_classes.mk"};
        of.putsHeader();
        of.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        of.puts("#\n");
        of.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        of.puts("# See " + v3Global.opt.prefix() + ".mk" + " for the caller.\n");

        of.puts("\n### Switches...\n");
        of.puts("# C11 constructs required?  0/1 (always on now)\n");
        of.putSet("VM_C11", "1");
        of.puts("# Timing enabled?  0/1\n");
        of.putSet("VM_TIMING", v3Global.usesTiming() ? "1" : "0");
        of.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        of.putSet("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        of.puts("# Parallel builds?  0/1 (from --output-split)\n");
        of.putSet("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        of.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        of.putSet("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        of.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        of.putSet("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        of.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        of.putSet("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        of.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        of.putSet("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        of.puts("# VPI enabled?  0/1 (from --vpi)\n");
        of.putSet("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        of.puts("\n### Object file lists...\n");
        জন্য (পূর্ণসংখ্যা support = 0; support < 3; ++support) {
            জন্য (ধ্রুবক যৌক্তিক& slow : {মিথ্যা, সত্য}) {
                যদি (support == 2) {
                    of.puts("# Global classes, need linked once per executable");
                } অন্যথা যদি (support) {
                    of.puts("# Generated support classes");
                } অন্যথা {
                    of.puts("# Generated module classes");
                }
                যদি (slow) {
                    of.puts(", non-fast-path, compile with low/medium optimization\n");
                } অন্যথা {
                    of.puts(", fast-path, compile with highest optimization\n");
                }
                ধ্রুবক string targetVar = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_গুনতি = 0;
                of.puts(targetVar + " += \\\n");
                যদি (support == 2 && v3Global.opt.hierChild()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } অন্যথা যদি (support == 2 && !slow) {
                    জন্য (ধ্রুবক string& cpp : v3Global.verilatedCppFiles())
                        putMakeClassEntry(of, cpp);
                } অন্যথা যদি (support == 2 && slow) {
                } অন্যথা যদি (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ধ্রুবক std::vector<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    জন্য (ধ্রুবক FileOrConcatenatedFilesList& entry : list) {
                        যদি (entry.isConcatenatingFile()) emitConcatenatingFile(entry);
                        putMakeClassEntry(of, entry.m_filename);
                    }
                } অন্যথা {
                    জন্য (Ast_গ্রন্থি_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), গ্রন্থি_File)) {
                        ধ্রুবক AstCFile* ধ্রুবক cfilep = VN_CAST(nodep, CFile);
                        যদি (cfilep && cfilep->source() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            putMakeClassEntry(of, cfilep->name());
                        }
                    }
                }
                of.puts("\n");
                V3Stats::addStat("Makefile targets, " + targetVar, m_put_Class_গুনতি);
            }
        }

        of.putsHeader();
    }

    শূন্য emitOverallMake() {
        // Generate the makefile
        V3OutMkFile of{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".mk"};
        of.putsHeader();
        of.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        of.puts("#\n");
        of.puts("# Execute this makefile from the object directory:\n");
        of.puts("#    make -f " + v3Global.opt.prefix() + ".mk" + "\n");
        of.puts("\n");

        যদি (v3Global.opt.exe()) {
            of.puts("default: " + v3Global.opt.exeName() + "\n");
        } অন্যথা যদি (!v3Global.opt.libCreate().empty()) {
            of.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } অন্যথা {
            of.puts("default: lib" + v3Global.opt.prefix() + "\n");
        }
        of.puts("\n### Constants...\n");
        of.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        of.putSet("PERL", V3OutFormatter::quoteNameControls(V3Options::getenvPERL()));
        of.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        of.putSet("PYTHON3", V3OutFormatter::quoteNameControls(V3Options::getenvPYTHON3()));
        of.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        of.putSet("VERILATOR_ROOT",
                  V3OutFormatter::quoteNameControls(V3Options::getenv_VERILATOR_মূল()));
        of.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        of.putSetQ("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        of.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        of.putSetQ("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        যদি (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        of.puts("\n### Switches...\n");
        of.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        of.putSet("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        of.puts("# SystemC output mode?  0/1 (from --sc)\n");
        of.putSet("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        of.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        of.putSet("VM_SP_OR_SC", "$(VM_SC)");
        of.puts("# Deprecated\n");
        of.putSet("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        of.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        of.putSet("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        of.puts("\n### Vars...\n");
        of.puts("# Module prefix (from --prefix)\n");
        of.putSet("VM_MODPREFIX", v3Global.opt.modPrefix());
        of.puts("# Design prefix (from --prefix)\n");
        of.putSet("VM_PREFIX", v3Global.opt.prefix());
        of.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        of.putSet("VM_PREFIX_INCLUDE", "<" + v3Global.opt.prefix() + ".h>");
        যদি (v3Global.dpi()) {
            of.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            of.putSet("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.prefix() + "__Dpi.h>");
        }

        of.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        of.puts("VM_USER_CFLAGS = \\\n");
        ধ্রুবক std::string solver = V3Options::getenvVERILATOR_SOLVER();
        যদি (v3Global.useRandomizeMethods() && solver != "")
            of.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3OutFormatter::quoteNameControls(solver)
                    + "\"' \\\n");
        যদি (!v3Global.opt.libCreate().empty()) of.puts("\t-fPIC \\\n");
        ধ্রুবক VStringList& cFlags = v3Global.opt.cFlags();
        জন্য (ধ্রুবক string& i : cFlags) of.puts("  " + i + " \\\n");
        of.puts("\n");

        of.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        of.puts("VM_USER_LDLIBS = \\\n");
        ধ্রুবক VStringList& ldLibs = v3Global.opt.ldLibs();
        জন্য (ধ্রুবক string& i : ldLibs) of.puts("  " + i + " \\\n");
        of.puts("\n");

        of.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        of.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        of.puts("VM_USER_LDLIBS_FILES = \\\n");
        জন্য (ধ্রুবক string& i : v3Global.opt.ldLibFiles()) of.puts("  " + i + " \\\n");
        of.puts("\n");

        VStringSet dirs;
        of.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        of.puts("VM_USER_CLASSES = \\\n");
        ধ্রুবক VStringSet& cppFiles = v3Global.opt.cppFiles();
        জন্য (ধ্রুবক auto& cppfile : cppFiles) {
            of.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ধ্রুবক string dir
                = V3Os::filenameRelativePath(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.ঢোকাও(dir);
        }
        dirs.ঢোকাও(V3Os::filenameRelativePath(".", v3Global.opt.makeDir()));

        of.puts("\n");

        of.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        of.puts("VM_USER_DIR = \\\n");
        জন্য (ধ্রুবক auto& i : dirs) of.puts("  " + i + " \\\n");
        of.puts("\n");

        of.puts("### Default rules...\n");
        of.puts("# Include list of all generated classes\n");
        of.puts("include " + v3Global.opt.prefix() + "_classes.mk\n");
        যদি (v3Global.opt.hier_শীর্ষ()) {
            of.puts("# Include rules for hierarchical blocks\n");
            of.puts("include " + v3Global.opt.prefix() + "_hier.mk\n");
        }
        of.puts("# Include global rules\n");
        of.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        যদি (v3Global.opt.exe()) {
            of.puts("\n### Executable rules... (from --exe)\n");
            of.puts("VPATH += $(VM_USER_DIR)\n");
            of.puts("\n");

            যদি (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                of.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                of.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                of.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                of.puts("\n");
            }
        }

        ধ্রুবক string compilerIncludePch
            = v3Global.opt.compilerIncludes().empty() ? "" : "$(VK_PCH_H).fast.gch";
        ধ্রুবক string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().empty() ? "" : "$(VK_PCH_I_FAST)";
        জন্য (ধ্রুবক string& cppfile : cppFiles) {
            ধ্রুবক string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            of.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            of.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        যদি (v3Global.opt.exe()) {
            of.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            of.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            of.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            of.puts("\n");
        } অন্যথা যদি (!v3Global.opt.libCreate().empty()) {
            ধ্রুবক string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            of.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            of.puts(v3Global.opt.libCreateName(মিথ্যা) + ": " + libCreateDeps + "\n");
            of.puts("\n");
            যদি (v3Global.opt.hierChild()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                of.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(মিথ্যা)
                        + "\n");
            } অন্যথা {
                of.puts(v3Global.opt.libCreateName(সত্য) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                of.puts("ifeq ($(shell uname -s),Darwin)\n");
                of.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                of.puts("else\n");
                of.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                of.puts("endif\n");
                of.puts("\n");
                of.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(মিথ্যা)
                        + " " + v3Global.opt.libCreateName(সত্য) + "\n");
            }
        } অন্যথা {
            ধ্রুবক string libname = "lib" + v3Global.opt.prefix() + ".a";
            of.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            of.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            of.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            of.puts("lib" + v3Global.opt.prefix() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        of.putsHeader();
    }

    explicit EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    আভাসী ~EmitMk() = পূর্বনির্ধারিত;
};

//######################################################################

শ্রেণী EmitMkHierVerilation final {
    ধ্রুবক V3HierGraph* ধ্রুবক m_graphp;
    ধ্রুবক string m_makefile;  // path of this makefile
    শূন্য emitCommonOpts(V3OutMkFile& of) ধ্রুবক {
        of.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        of.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        of.puts("# Common options for hierarchical blocks\n");
        ধ্রুবক string fullpath_bin = V3Os::filenameRealPath(v3Global.opt.buildDepBin());
        ধ্রুবক string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        of.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        of.puts("VM_HIER_INPUT_FILES := \\\n");
        জন্য (ধ্রুবক auto& i : v3Global.opt.vFiles())
            of.puts("  " + V3Os::filenameRealPath(i.filename()) + " \\\n");
        of.puts("\n");
        of.puts("VM_HIER_VERILOG_LIBS := \\\n");
        জন্য (ধ্রুবক auto& i : v3Global.opt.libraryFiles()) {
            of.puts("  " + V3Os::filenameRealPath(i.filename()) + " \\\n");
        }
        of.puts("\n");
    }
    শূন্য emitLaunchVerilator(V3OutMkFile& of, ধ্রুবক string& argsFile) ধ্রুবক {
        of.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        of.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + argsFile + "\"\n");
    }
    শূন্য emit(V3OutMkFile& of) ধ্রুবক {
        of.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        of.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        of.puts("#\n");
        of.puts("# The main makefile " + v3Global.opt.prefix() + ".mk calls this makefile\n");
        of.puts("\n");
        of.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        of.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        of.puts(".SUFFIXES:\n");
        of.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        of.puts("# Libraries of hierarchical blocks\n");
        of.puts("VM_HIER_LIBS := \\\n");
        জন্য (ধ্রুবক V3GraphVertex& vtx : m_graphp->vertices()) {
            ধ্রুবক V3HierBlock* ধ্রুবক blockp = vtx.as<V3HierBlock>();
            of.puts("  " + blockp->hierLibFilename(সত্য) + " \\\n");
        }
        of.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        of.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.prefix() + ".mk\n");
        of.puts("\t$(MAKE) -f " + v3Global.opt.prefix() + ".mk\n");
        of.puts("hier_verilation: " + v3Global.opt.prefix() + ".mk\n");
        emitCommonOpts(of);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        of.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        of.puts("hier_launch_verilator:\n");
        of.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ধ্রুবক string argsFile = v3Global.hierGraphp()->শীর্ষ_Command_Args_Filename(মিথ্যা);
            of.puts("\n# Verilate the top module\n");
            of.puts(v3Global.opt.prefix()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            of.puts(V3Os::filenameNonDir(argsFile) + " ");
            জন্য (ধ্রুবক V3GraphVertex& vtx : m_graphp->vertices()) {
                ধ্রুবক V3HierBlock* ধ্রুবক blockp = vtx.as<V3HierBlock>();
                of.puts(blockp->hierWrapperFilename(সত্য) + " ");
            }
            of.puts("\n");
            emitLaunchVerilator(of, argsFile);
        }

        // Rules to process hierarchical blocks
        of.puts("\n# Verilate hierarchical blocks\n");
        জন্য (ধ্রুবক V3GraphVertex& vtx : m_graphp->vertices()) {
            ধ্রুবক V3HierBlock* ধ্রুবক blockp = vtx.as<V3HierBlock>();
            ধ্রুবক string prefix = blockp->hierPrefix();
            ধ্রুবক string argsFilename = blockp->commandArgsFilename(মিথ্যা);
            of.puts(blockp->hierGeneratedFilenames(সত্য));
            of.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            of.puts(V3Os::filenameNonDir(argsFilename) + " ");
            জন্য (ধ্রুবক V3GraphEdge& edge : blockp->outEdges()) {
                ধ্রুবক V3HierBlock* ধ্রুবক dependencyp = edge.শীর্ষ()->as<V3HierBlock>();
                of.puts(dependencyp->hierWrapperFilename(সত্য) + " ");
            }
            of.puts("\n");
            emitLaunchVerilator(of, argsFilename);

            // Rule to build lib*.a
            of.puts(blockp->hierLibFilename(সত্য));
            of.puts(": ");
            of.puts(blockp->hierMkFilename(সত্য));
            of.puts(" ");
            জন্য (ধ্রুবক V3GraphEdge& edge : blockp->outEdges()) {
                ধ্রুবক V3HierBlock* ধ্রুবক dependencyp = edge.শীর্ষ()->as<V3HierBlock>();
                of.puts(dependencyp->hierLibFilename(সত্য));
                of.puts(" ");
            }
            of.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(মিথ্যা) + " -C " + prefix);
            of.puts(" VM_PREFIX=" + prefix);
            of.puts("\n\n");
        }
        of.puts("endif  # Guard\n");
    }

সর্বজনীন:
    explicit EmitMkHierVerilation(ধ্রুবক V3HierGraph* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + "_hier.mk"} {
        V3OutMkFile of{m_makefile};
        emit(of);
    }
};

//######################################################################
// Gate class functions

শূন্য V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    ধ্রুবক EmitMk emitter;
}

শূন্য V3EmitMk::emitHierVerilation(ধ্রুবক V3HierGraph* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
