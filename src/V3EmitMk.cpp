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
صنف EmitGroup final {
عام:
    بنية FileOrConcatenatedFilesList final {
        ثابت std::string m_filename;  // Filename or output group filename if grouping
        std::vector<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        منطقي isConcatenatingFile() ثابت { أرجع !m_concatenatedFilenames.empty(); }
    };

    بنية FilenameWithScore final {
        ثابت std::string m_filename;  // Input filename
        ثابت uint64_t m_score;  // Input file complexity
        ثابت uint64_t m_sizeBytes;  // Input file size
    };

خاص:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    بنية WorkList final {
        uint64_t m_totalScore = 0;  // Sum of scores of included files
        std::vector<FilenameWithScore> m_files;  // Included filenames
        صحيح m_buckets_لمب
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        منطقي m_isConcatenable = true;  // Indicated whether files on this list can be concatenated.
        ثابت صحيح m_dbgId;  // Work list ID for debugging.

        WorkList() = delete;
        explicit WorkList(صحيح id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    ساكن constexpr size_t MIN_FILES_كدي = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    ساكن constexpr size_t MIN_FILES_PER_BUCKET = 4;

    // MEMBERS

    ثابت std::vector<FilenameWithScore>
        m_شغرو_Files;  // List of filenames from initial AstCFile list
    std::vector<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ثابت uint64_t m_totalScore;  // Sum of file scores
    ثابت std::string m_groupFilePrefix;  // Prefix for output group filenames
    ثابت uint64_t m_maxGroupBytes;  // Maximum sum of file sizes in an output group
    std::vector<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::vector<WorkList*> m_concatenableListsByDescSize;  // Lists sorted by size, descending
    صحيح m_group_لمب = 0;  // Number of output groups created

    EmitGroup(std::vector<FilenameWithScore> شغرو_Files, uint64_t totalScore,
              ثابت std::string& groupFilePrefix, uint64_t maxGroupBytes)
        : m_شغرو_Files{std::move(شغرو_Files)}
        , m_totalScore{totalScore}
        , m_groupFilePrefix{groupFilePrefix}
        , m_maxGroupBytes{maxGroupBytes} {}

    string groupFilename(صحيح id) ثابت {
        أرجع v3Global.opt.prefix() + "_" + m_groupFilePrefix + std::to_string(id);
    }

    // Debug logging: prints scores histogram
    فراغ dumpLogScoreHistogram(std::ostream& os) {
        constexpr صحيح MAX_BAR_LENGTH = 80;
        constexpr صحيح MAX_INTERVALS_لمب = 60;

        // All scores arranged in ascending order.
        std::vector<uint64_t> sortedScores;
        sortedScores.reserve(m_شغرو_Files.size());
        std::transform(m_شغرو_Files.begin(), m_شغرو_Files.end(), std::back_inserter(sortedScores),
                       [](ثابت FilenameWithScore& شغرو_File) { أرجع شغرو_File.m_score; });
        std::sort(sortedScores.begin(), sortedScores.end());

        ثابت int64_t كولي_Score = sortedScores.back();
        os << "Top score: " << كولي_Score << '\n';
        ثابت صحيح maxScoreWidth = std::to_string(كولي_Score).length();

        ثابت int64_t intervals_لمب = std::min<int64_t>(كولي_Score + 1, MAX_INTERVALS_لمب);

        بنية Interval final {
            uint64_t m_lowerBound = 0;  // Lowest score included in this histogram bucket
            صحيح m_size = 0;  // Number of scores that fell into this bucket
        };

        std::vector<Interval> intervals;
        intervals.resize(intervals_لمب);

        intervals[0].m_lowerBound = 0;
        لكل (صحيح i = 1; i < intervals_لمب; i++) {
            intervals[i].m_lowerBound = (كولي_Score + 1) * i / intervals_لمب + 1;
        }

        لكل (ثابت uint64_t score : sortedScores) {
            ثابت unsigned صحيح ivIdx = score * intervals_لمب / (كولي_Score + 1);
            ++intervals[ivIdx].m_size;
        }

        صحيح كولي_Interval_Size = 0;
        لكل (ثابت Interval& iv : intervals)
            كولي_Interval_Size = std::max(كولي_Interval_Size, iv.m_size);

        os << "Input files' scores histogram:\n";

        لكل (ثابت Interval& iv : intervals) {
            ثابت صحيح scaledSize = iv.m_size * (MAX_BAR_LENGTH + 1) / كولي_Interval_Size;
            ثابت std::string لي = " |" + std::string(scaledSize, '#');

            os << std::setw(maxScoreWidth) << iv.m_lowerBound << لي << "  " << iv.m_size << '\n';
        }
        os << std::setw(maxScoreWidth) << (كولي_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    فراغ dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        لكل (ثابت WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.size()
               << "; total score: " << list.m_totalScore << ")\n";
            إذا (debug() >= 6 || list.m_files.size() < 4) {
                // Log all files
                لكل (ثابت FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } وإلا {
                // Log only first and last file
                ثابت FilenameWithScore& first = list.m_files.front();
                ثابت FilenameWithScore& last = list.m_files.back();
                os << "| + " << first.m_filename << "  (score: " << first.m_score << ")\n";
                os << "| | (... " << (list.m_files.size() - 2) << " files ...)\n";
                os << "| + " << last.m_filename << "  (score: " << last.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    فراغ dumpOutputList(std::ostream& os) ثابت {
        os << "List of output files after execution of concatenation:\n";

        لكل (ثابت FileOrConcatenatedFilesList& entry : m_outputFiles) {
            إذا (entry.isConcatenatingFile()) {
                os << "+ " << entry.m_filename << "  (concatenating file)\n";
                لكل (ثابت string& f : entry.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } وإلا {
                os << "+ " << entry.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    منطقي fallbackNoGrouping(size_t شغرو_Files_كدي) {
        ثابت صحيح total_Buckets_لمب = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        منطقي groupingRedundant = خطأ;
        إذا (شغرو_Files_كدي < MIN_FILES_كدي
            && شغرو_Files_كدي <= static_cast<size_t>(total_Buckets_لمب)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_شغرو_Files.size() << " < "
                                                                   << MIN_FILES_كدي << ")");
            groupingRedundant = true;
        }
        إذا (شغرو_Files_كدي < (MIN_FILES_PER_BUCKET * total_Buckets_لمب)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_شغرو_Files.size() << " < " << MIN_FILES_PER_BUCKET << " - "
                         << total_Buckets_لمب << ")");
            groupingRedundant = true;
        }
        إذا (!groupingRedundant) أرجع خطأ;

        m_outputFiles.reserve(m_شغرو_Files.size());
        لكل (ثابت FilenameWithScore& filename : m_شغرو_Files) {
            m_outputFiles.تور_back({std::move(filename.m_filename), {}});
        }
        أرجع true;
    }

    فراغ createWorkLists() {
        // Create initial Work Lists.

        ثابت صحيح total_Buckets_لمب = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ثابت uint64_t concatenableFileMaxScore = m_totalScore / total_Buckets_لمب / 2;

        V3Stats::addStat("Concatenation max score", concatenableFileMaxScore);
        صحيح nextWorkListId = 0;

        إذا (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        لكل (ثابت FilenameWithScore& شغرو_File : m_شغرو_Files) {
            ثابت منطقي fileIsConcatenable = (شغرو_File.m_score <= concatenableFileMaxScore);
            إذا (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << شغرو_File.m_filename << "  (score: " << شغرو_File.m_score << ")\n";
            V3Stats::addStatSum(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                شغرو_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            إذا (m_workLists.empty() || m_workLists.back().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_back(WorkList{nextWorkListId++});
                m_workLists.back().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.back();
            list.m_files.تور_back(
                {شغرو_File.m_filename, شغرو_File.m_score, شغرو_File.m_sizeBytes});
            list.m_totalScore += شغرو_File.m_score;
        }
        إذا (m_logp) *m_logp << '\n';
    }

    فراغ assignBuckets(uint64_t concatenableFilesTotalScore) {
        // Assign buckets to lists

        ثابت size_t total_Buckets_لمب = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        إذا (m_concatenableListsByDescSize.size() > total_Buckets_لمب) {
            // Debugging: Log which work lists will be kept
            إذا (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenableListsByDescSize.begin(),
                              m_concatenableListsByDescSize.begin() + total_Buckets_لمب,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.size()
                                          << "; total score: " << listp->m_totalScore << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenableListsByDescSize.begin() + total_Buckets_لمب,
                          m_concatenableListsByDescSize.end(), [&](WorkList* listp) {
                              listp->m_isConcatenable = خطأ;

                              إذا (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.size()
                                          << "; total score: " << listp->m_totalScore << ")\n";
                          });
            إذا (m_logp) *m_logp << '\n';

            m_concatenableListsByDescSize.resize(total_Buckets_لمب);
            // Recalculate stats
            concatenableFilesTotalScore = 0;
            لكل (ثابت WorkList* ثابت listp : m_concatenableListsByDescSize) {
                concatenableFilesTotalScore += listp->m_totalScore;
            }
        }

        ثابت uint64_t idealBucketScore = concatenableFilesTotalScore / total_Buckets_لمب;

        V3Stats::addStat("Concatenation ideal bucket score", idealBucketScore);

        إذا (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        صحيح availableBuckets = v3Global.opt.outputGroups();
        لكل (WorkList* listp : m_concatenableListsByDescSize) {
            إذا (availableBuckets > 0 && idealBucketScore > 0) {
                listp->m_buckets_لمب = std::min(
                    availableBuckets, std::max<صحيح>(1, listp->m_totalScore / idealBucketScore));
                availableBuckets -= listp->m_buckets_لمب;
                إذا (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_لمب << "] Work List #"
                            << listp->m_dbgId << '\n';
            } وإلا {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = خطأ;
                إذا (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::هغو << std::setw(4) << listp->m_dbgId
                            << std::دام << " (excluding from concatenation)\n";
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
                    m_outputFiles.تور_back({std::move(file.m_filename), {}});
                }
                استمر;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ثابت uint64_t listIdealBucketScore = list.m_totalScore / list.m_buckets_لمب;

            std::vector<std::vector<ثابت FilenameWithScore*>> buckets;
            auto fileIt = list.m_files.cbegin();
            لكل (صحيح i = 0; i < list.m_buckets_لمب; ++i) {
                std::vector<ثابت FilenameWithScore*> bucket;
                uint64_t bucketScore = 0;

                لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                    ثابت uint64_t diffNow
                        = std::abs(static_cast<int64_t>(listIdealBucketScore - bucketScore));
                    ثابت uint64_t diffIfAdded = std::abs(static_cast<int64_t>(
                        listIdealBucketScore - bucketScore - fileIt->m_score));
                    إذا (bucketScore == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        bucketScore += fileIt->m_score;
                        bucket.تور_back(&*fileIt);
                    } وإلا {
                        // Best possible bucket score reached, process next bucket.
                        اكسر;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.تور_back(std::move(bucket));
            }
            لكل (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.back().تور_back(&*fileIt);
            }
            لكل (ثابت std::vector<ثابت FilenameWithScore*>& bucket : buckets) {
                addBucketGroups(bucket);
            }
        }
    }

    فراغ addBucketGroups(ثابت std::vector<ثابت FilenameWithScore*>& bucket) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::vector<std::vector<std::string>> groups;
        uint64_t groupBytes = 0;
        لكل (ثابت FilenameWithScore* ثابت filep : bucket) {
            إذا (groups.empty() || groupBytes + filep->m_sizeBytes > m_maxGroupBytes) {
                groups.emplace_back();
                groupBytes = 0;
            }
            groupBytes += filep->m_sizeBytes;
            groups.back().تور_back(filep->m_filename);
        }
        لكل (std::vector<std::string>& group : groups) {
            إذا (group.size() == 1) {
                m_outputFiles.تور_back({std::move(group.front()), {}});
            } وإلا {
                m_outputFiles.تور_back({groupFilename(m_group_لمب++), std::move(group)});
            }
        }
    }

    فراغ assertFilesSame() ثابت {
        auto ifIt = m_شغرو_Files.begin();
        auto ofIt = m_outputFiles.begin();
        طالما (ifIt != m_شغرو_Files.end() && ofIt != m_outputFiles.end()) {
            إذا (ofIt->isConcatenatingFile()) {
                لكل (ثابت string& ocf : ofIt->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_شغرو_Files.end(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++ofIt;
            } وإلا {
                UASSERT(ifIt->m_filename == ofIt->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << ofIt->m_filename);
                ++ifIt;
                ++ofIt;
            }
        }
        UASSERT(ifIt == m_شغرو_Files.end(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(ofIt == m_outputFiles.end(),
                "More output files than input files. First extra file: " << ofIt->m_filename);
    }

    فراغ process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_groupFilePrefix);
        UINFO(5, "Number of input files: " << m_شغرو_Files.size());
        UINFO(5, "Total score: " << m_totalScore);

        ثابت صحيح total_Buckets_لمب = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << total_Buckets_لمب);
        UASSERT(total_Buckets_لمب > 0, "More than 0 buckets required");

        إذا (fallbackNoGrouping(m_شغرو_Files.size())) أرجع;

        إذا (debug() >= 6 || dumpLevel() >= 6) {
            ثابت string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            إذا (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        إذا (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        size_t concatenable_Files_كدي = 0;
        int64_t concatenableFilesTotalScore = 0;

        لكل (WorkList& list : m_workLists) {
            إذا (!list.m_isConcatenable) استمر;

            // "Concatenation" of a single file is pointless
            إذا (list.m_files.size() == 1) {
                list.m_isConcatenable = خطأ;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                استمر;
            }

            concatenable_Files_كدي += list.m_files.size();
            concatenableFilesTotalScore += list.m_totalScore;
            // This vector is sorted below
            m_concatenableListsByDescSize.تور_back(&list);
        }

        إذا (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        إذا (fallbackNoGrouping(concatenable_Files_كدي)) أرجع;

        std::sort(m_concatenableListsByDescSize.begin(), m_concatenableListsByDescSize.end(),
                  [](ثابت WorkList* ap, ثابت WorkList* bp) {
                      // Sort in descending order by number of files
                      إذا (ap->m_files.size() != bp->m_files.size())
                          أرجع (ap->m_files.size() > bp->m_files.size());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      أرجع bp->m_totalScore > ap->m_totalScore;
                  });

        assignBuckets(concatenableFilesTotalScore);

        buildOutputList();

        إذا (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

عام:
    ساكن std::vector<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::vector<FilenameWithScore> شغرو_Files, uint64_t totalScore,
                                ثابت std::string& groupFilePrefix, uint64_t maxGroupBytes) {
        EmitGroup group{std::move(شغرو_Files), totalScore, groupFilePrefix, maxGroupBytes};
        group.process();
        أرجع group.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

صنف EmitMk final {
    using FileOrConcatenatedFilesList = EmitGroup::FileOrConcatenatedFilesList;
    using FilenameWithScore = EmitGroup::FilenameWithScore;

    // MEMBERS
    مزدوج m_put_Class_كدي = 0;  // Number of classEntries printed

عام:
    // METHODS

    ساكن فراغ emitConcatenatingFile(ثابت FileOrConcatenatedFilesList& entry) {
        UASSERT(entry.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3OutCFile concatenatingFile{v3Global.opt.makeDir() + "/" + entry.m_filename + ".cpp"};
        concatenatingFile.putsHeader();
        لكل (ثابت string& file : entry.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    ساكن uint64_t fileSize(ثابت AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::نبيو | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        أرجع static_cast<uint64_t>(ifs.tellg());
    }

    فراغ putMakeClassEntry(V3OutMkFile& of, ثابت string& name) {
        of.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_كدي;
    }

    فراغ emitClassMake() {
        std::vector<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::vector<FileOrConcatenatedFilesList> vmClassesFastList;
        إذا (v3Global.opt.outputGroups() > 0) {
            std::vector<FilenameWithScore> slowFiles;
            std::vector<FilenameWithScore> fastFiles;
            uint64_t slowTotalScore = 0;
            uint64_t fastTotalScore = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t headerBytes = 0;

            لكل (Ast_كلي_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), كلي_File)) {
                ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                إذا (cfilep && VString::endsWith(cfilep->name(), ".h")) {
                    headerBytes += fileSize(cfilep);
                }
                إذا (cfilep && cfilep->source() && cfilep->support() == خطأ) {
                    std::vector<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& totalScore = cfilep->slow() ? slowTotalScore : fastTotalScore;

                    totalScore += cfilep->complexityScore();
                    files.تور_back({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), fileSize(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t maxGroupBytes = std::numeric_limits<uint64_t>::max();
            إذا (v3Global.opt.outputGroupsSize()) {
                ثابت uint64_t maxTuBytes = static_cast<uint64_t>(v3Global.opt.outputGroupsSize());
                maxGroupBytes = maxTuBytes > headerBytes ? maxTuBytes - headerBytes : 0;
            }

            vmClassesSlowList = EmitGroup::singleConcatenatedFilesList(
                std::move(slowFiles), slowTotalScore, "vm_classes_Slow_", maxGroupBytes);
            vmClassesFastList = EmitGroup::singleConcatenatedFilesList(
                std::move(fastFiles), fastTotalScore, "vm_classes_", maxGroupBytes);
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
        لكل (صحيح support = 0; support < 3; ++support) {
            لكل (ثابت منطقي& slow : {خطأ, true}) {
                إذا (support == 2) {
                    of.puts("# Global classes, need linked once per executable");
                } وإلا إذا (support) {
                    of.puts("# Generated support classes");
                } وإلا {
                    of.puts("# Generated module classes");
                }
                إذا (slow) {
                    of.puts(", non-fast-path, compile with low/medium optimization\n");
                } وإلا {
                    of.puts(", fast-path, compile with highest optimization\n");
                }
                ثابت string targetVar = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_كدي = 0;
                of.puts(targetVar + " += \\\n");
                إذا (support == 2 && v3Global.opt.hierChild()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } وإلا إذا (support == 2 && !slow) {
                    لكل (ثابت string& cpp : v3Global.verilatedCppFiles())
                        putMakeClassEntry(of, cpp);
                } وإلا إذا (support == 2 && slow) {
                } وإلا إذا (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ثابت std::vector<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    لكل (ثابت FileOrConcatenatedFilesList& entry : list) {
                        إذا (entry.isConcatenatingFile()) emitConcatenatingFile(entry);
                        putMakeClassEntry(of, entry.m_filename);
                    }
                } وإلا {
                    لكل (Ast_كلي_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), كلي_File)) {
                        ثابت AstCFile* ثابت cfilep = VN_CAST(nodep, CFile);
                        إذا (cfilep && cfilep->source() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            putMakeClassEntry(of, cfilep->name());
                        }
                    }
                }
                of.puts("\n");
                V3Stats::addStat("Makefile targets, " + targetVar, m_put_Class_كدي);
            }
        }

        of.putsHeader();
    }

    فراغ emitOverallMake() {
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

        إذا (v3Global.opt.exe()) {
            of.puts("default: " + v3Global.opt.exeName() + "\n");
        } وإلا إذا (!v3Global.opt.libCreate().empty()) {
            of.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } وإلا {
            of.puts("default: lib" + v3Global.opt.prefix() + "\n");
        }
        of.puts("\n### Constants...\n");
        of.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        of.putSet("PERL", V3OutFormatter::quoteNameControls(V3Options::getenvPERL()));
        of.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        of.putSet("PYTHON3", V3OutFormatter::quoteNameControls(V3Options::getenvPYTHON3()));
        of.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        of.putSet("VERILATOR_ROOT",
                  V3OutFormatter::quoteNameControls(V3Options::getenv_VERILATOR_سيو()));
        of.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        of.putSetQ("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        of.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        of.putSetQ("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        إذا (v3Global.opt.systemC() && !V3Options::systemCFound()) {
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
        إذا (v3Global.dpi()) {
            of.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            of.putSet("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.prefix() + "__Dpi.h>");
        }

        of.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        of.puts("VM_USER_CFLAGS = \\\n");
        ثابت std::string solver = V3Options::getenvVERILATOR_SOLVER();
        إذا (v3Global.useRandomizeMethods() && solver != "")
            of.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3OutFormatter::quoteNameControls(solver)
                    + "\"' \\\n");
        إذا (!v3Global.opt.libCreate().empty()) of.puts("\t-fPIC \\\n");
        ثابت VStringList& cFlags = v3Global.opt.cFlags();
        لكل (ثابت string& i : cFlags) of.puts("  " + i + " \\\n");
        of.puts("\n");

        of.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        of.puts("VM_USER_LDLIBS = \\\n");
        ثابت VStringList& ldLibs = v3Global.opt.ldLibs();
        لكل (ثابت string& i : ldLibs) of.puts("  " + i + " \\\n");
        of.puts("\n");

        of.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        of.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        of.puts("VM_USER_LDLIBS_FILES = \\\n");
        لكل (ثابت string& i : v3Global.opt.ldLibFiles()) of.puts("  " + i + " \\\n");
        of.puts("\n");

        VStringSet dirs;
        of.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        of.puts("VM_USER_CLASSES = \\\n");
        ثابت VStringSet& cppFiles = v3Global.opt.cppFiles();
        لكل (ثابت auto& cppfile : cppFiles) {
            of.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ثابت string dir
                = V3Os::filenameRelativePath(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.سك(dir);
        }
        dirs.سك(V3Os::filenameRelativePath(".", v3Global.opt.makeDir()));

        of.puts("\n");

        of.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        of.puts("VM_USER_DIR = \\\n");
        لكل (ثابت auto& i : dirs) of.puts("  " + i + " \\\n");
        of.puts("\n");

        of.puts("### Default rules...\n");
        of.puts("# Include list of all generated classes\n");
        of.puts("include " + v3Global.opt.prefix() + "_classes.mk\n");
        إذا (v3Global.opt.hier_كولي()) {
            of.puts("# Include rules for hierarchical blocks\n");
            of.puts("include " + v3Global.opt.prefix() + "_hier.mk\n");
        }
        of.puts("# Include global rules\n");
        of.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        إذا (v3Global.opt.exe()) {
            of.puts("\n### Executable rules... (from --exe)\n");
            of.puts("VPATH += $(VM_USER_DIR)\n");
            of.puts("\n");

            إذا (v3Global.opt.vpi()) {
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

        ثابت string compilerIncludePch
            = v3Global.opt.compilerIncludes().empty() ? "" : "$(VK_PCH_H).fast.gch";
        ثابت string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().empty() ? "" : "$(VK_PCH_I_FAST)";
        لكل (ثابت string& cppfile : cppFiles) {
            ثابت string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            of.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            of.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        إذا (v3Global.opt.exe()) {
            of.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            of.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            of.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            of.puts("\n");
        } وإلا إذا (!v3Global.opt.libCreate().empty()) {
            ثابت string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            of.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            of.puts(v3Global.opt.libCreateName(خطأ) + ": " + libCreateDeps + "\n");
            of.puts("\n");
            إذا (v3Global.opt.hierChild()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                of.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + "\n");
            } وإلا {
                of.puts(v3Global.opt.libCreateName(true) + ": " + libCreateDeps + "\n");
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
                of.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(خطأ)
                        + " " + v3Global.opt.libCreateName(true) + "\n");
            }
        } وإلا {
            ثابت string libname = "lib" + v3Global.opt.prefix() + ".a";
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
    virtual ~EmitMk() = افتراضي;
};

//######################################################################

صنف EmitMkHierVerilation final {
    ثابت V3HierGraph* ثابت m_graphp;
    ثابت string m_makefile;  // path of this makefile
    فراغ emitCommonOpts(V3OutMkFile& of) ثابت {
        of.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        of.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        of.puts("# Common options for hierarchical blocks\n");
        ثابت string fullpath_bin = V3Os::filenameRealPath(v3Global.opt.buildDepBin());
        ثابت string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        of.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        of.puts("VM_HIER_INPUT_FILES := \\\n");
        لكل (ثابت auto& i : v3Global.opt.vFiles())
            of.puts("  " + V3Os::filenameRealPath(i.filename()) + " \\\n");
        of.puts("\n");
        of.puts("VM_HIER_VERILOG_LIBS := \\\n");
        لكل (ثابت auto& i : v3Global.opt.libraryFiles()) {
            of.puts("  " + V3Os::filenameRealPath(i.filename()) + " \\\n");
        }
        of.puts("\n");
    }
    فراغ emitLaunchVerilator(V3OutMkFile& of, ثابت string& حجوجي_File) ثابت {
        of.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        of.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + حجوجي_File + "\"\n");
    }
    فراغ emit(V3OutMkFile& of) ثابت {
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
        لكل (ثابت V3GraphVertex& vtx : m_graphp->vertices()) {
            ثابت V3HierBlock* ثابت blockp = vtx.as<V3HierBlock>();
            of.puts("  " + blockp->hierLibFilename(true) + " \\\n");
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
            ثابت string حجوجي_File = v3Global.hierGraphp()->كولي_Command_حجوجي_Filename(خطأ);
            of.puts("\n# Verilate the top module\n");
            of.puts(v3Global.opt.prefix()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            of.puts(V3Os::filenameNonDir(حجوجي_File) + " ");
            لكل (ثابت V3GraphVertex& vtx : m_graphp->vertices()) {
                ثابت V3HierBlock* ثابت blockp = vtx.as<V3HierBlock>();
                of.puts(blockp->hierWrapperFilename(true) + " ");
            }
            of.puts("\n");
            emitLaunchVerilator(of, حجوجي_File);
        }

        // Rules to process hierarchical blocks
        of.puts("\n# Verilate hierarchical blocks\n");
        لكل (ثابت V3GraphVertex& vtx : m_graphp->vertices()) {
            ثابت V3HierBlock* ثابت blockp = vtx.as<V3HierBlock>();
            ثابت string prefix = blockp->hierPrefix();
            ثابت string حجوجي_Filename = blockp->command_حجوجي_Filename(خطأ);
            of.puts(blockp->hierGeneratedFilenames(true));
            of.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            of.puts(V3Os::filenameNonDir(حجوجي_Filename) + " ");
            لكل (ثابت V3GraphEdge& edge : blockp->outEdges()) {
                ثابت V3HierBlock* ثابت dependencyp = edge.كولي()->as<V3HierBlock>();
                of.puts(dependencyp->hierWrapperFilename(true) + " ");
            }
            of.puts("\n");
            emitLaunchVerilator(of, حجوجي_Filename);

            // Rule to build lib*.a
            of.puts(blockp->hierLibFilename(true));
            of.puts(": ");
            of.puts(blockp->hierMkFilename(true));
            of.puts(" ");
            لكل (ثابت V3GraphEdge& edge : blockp->outEdges()) {
                ثابت V3HierBlock* ثابت dependencyp = edge.كولي()->as<V3HierBlock>();
                of.puts(dependencyp->hierLibFilename(true));
                of.puts(" ");
            }
            of.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(خطأ) + " -C " + prefix);
            of.puts(" VM_PREFIX=" + prefix);
            of.puts("\n\n");
        }
        of.puts("endif  # Guard\n");
    }

عام:
    explicit EmitMkHierVerilation(ثابت V3HierGraph* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + "_hier.mk"} {
        V3OutMkFile of{m_makefile};
        emit(of);
    }
};

//######################################################################
// Gate class functions

فراغ V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    ثابت EmitMk emitter;
}

فراغ V3EmitMk::emitHierVerilation(ثابت V3HierGraph* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
