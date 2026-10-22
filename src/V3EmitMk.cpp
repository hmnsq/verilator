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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3EmitMk.h"

#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3HierBlock.h"
#अन्तर्भाव "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Groups adjacent files in a list, evenly distributing sum of scores
वर्ग EmitGroup final {
सार्वजनिक:
    संरचना FileOrConcatenatedFilesList final {
        स्थिरांक std::string m_filename;  // Filename or output group filename if grouping
        std::vector<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        तार्किक isConcatenatingFile() स्थिरांक { परत !m_concatenatedFilenames.empty(); }
    };

    संरचना FilenameWithScore final {
        स्थिरांक std::string m_filename;  // Input filename
        स्थिरांक uint64_t m_score;  // Input file complexity
        स्थिरांक uint64_t m_sizeBytes;  // Input file size
    };

खाजगी:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    संरचना WorkList final {
        uint64_t m_totalScore = 0;  // Sum of scores of included files
        std::vector<FilenameWithScore> m_files;  // Included filenames
        पूर्णांक m_buckets_अंक
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        तार्किक m_isConcatenable = सत्य;  // Indicated whether files on this list can be concatenated.
        स्थिरांक पूर्णांक m_dbgId;  // Work list ID for debugging.

        WorkList() = delete;
        explicit WorkList(पूर्णांक id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    स्थिर constexpr माप_प्रकार MIN_FILES_मोजणी = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    स्थिर constexpr माप_प्रकार MIN_FILES_PER_BUCKET = 4;

    // MEMBERS

    स्थिरांक std::vector<FilenameWithScore>
        m_आदान_Files;  // List of filenames from initial AstCFile list
    std::vector<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    स्थिरांक uint64_t m_totalScore;  // Sum of file scores
    स्थिरांक std::string m_groupFilePrefix;  // Prefix for output group filenames
    स्थिरांक uint64_t m_maxGroupBytes;  // Maximum sum of file sizes in an output group
    std::vector<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::vector<WorkList*> m_concatenableListsByDescSize;  // Lists sorted by size, descending
    पूर्णांक m_group_अंक = 0;  // Number of output groups created

    EmitGroup(std::vector<FilenameWithScore> आदान_Files, uint64_t totalScore,
              स्थिरांक std::string& groupFilePrefix, uint64_t maxGroupBytes)
        : m_आदान_Files{std::move(आदान_Files)}
        , m_totalScore{totalScore}
        , m_groupFilePrefix{groupFilePrefix}
        , m_maxGroupBytes{maxGroupBytes} {}

    string groupFilename(पूर्णांक id) स्थिरांक {
        परत v3Global.opt.prefix() + "_" + m_groupFilePrefix + std::to_string(id);
    }

    // Debug logging: prints scores histogram
    शून्य dumpLogScoreHistogram(std::ostream& os) {
        constexpr पूर्णांक MAX_BAR_LENGTH = 80;
        constexpr पूर्णांक MAX_INTERVALS_अंक = 60;

        // All scores arranged in ascending order.
        std::vector<uint64_t> sortedScores;
        sortedScores.reserve(m_आदान_Files.size());
        std::transform(m_आदान_Files.begin(), m_आदान_Files.end(), std::back_inserter(sortedScores),
                       [](स्थिरांक FilenameWithScore& आदान_File) { परत आदान_File.m_score; });
        std::sort(sortedScores.begin(), sortedScores.end());

        स्थिरांक int64_t शीर्ष_Score = sortedScores.back();
        os << "Top score: " << शीर्ष_Score << '\n';
        स्थिरांक पूर्णांक maxScoreWidth = std::to_string(शीर्ष_Score).length();

        स्थिरांक int64_t intervals_अंक = std::min<int64_t>(शीर्ष_Score + 1, MAX_INTERVALS_अंक);

        संरचना Interval final {
            uint64_t m_lowerBound = 0;  // Lowest score included in this histogram bucket
            पूर्णांक m_size = 0;  // Number of scores that fell into this bucket
        };

        std::vector<Interval> intervals;
        intervals.resize(intervals_अंक);

        intervals[0].m_lowerBound = 0;
        साठी (पूर्णांक i = 1; i < intervals_अंक; i++) {
            intervals[i].m_lowerBound = (शीर्ष_Score + 1) * i / intervals_अंक + 1;
        }

        साठी (स्थिरांक uint64_t score : sortedScores) {
            स्थिरांक अचिन्हित पूर्णांक ivIdx = score * intervals_अंक / (शीर्ष_Score + 1);
            ++intervals[ivIdx].m_size;
        }

        पूर्णांक शीर्ष_Interval_Size = 0;
        साठी (स्थिरांक Interval& iv : intervals)
            शीर्ष_Interval_Size = std::max(शीर्ष_Interval_Size, iv.m_size);

        os << "Input files' scores histogram:\n";

        साठी (स्थिरांक Interval& iv : intervals) {
            स्थिरांक पूर्णांक scaledSize = iv.m_size * (MAX_BAR_LENGTH + 1) / शीर्ष_Interval_Size;
            स्थिरांक std::string ओळ = " |" + std::string(scaledSize, '#');

            os << std::setw(maxScoreWidth) << iv.m_lowerBound << ओळ << "  " << iv.m_size << '\n';
        }
        os << std::setw(maxScoreWidth) << (शीर्ष_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    शून्य dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        साठी (स्थिरांक WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.size()
               << "; total score: " << list.m_totalScore << ")\n";
            जर (debug() >= 6 || list.m_files.size() < 4) {
                // Log all files
                साठी (स्थिरांक FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } अन्यथा {
                // Log only first and last file
                स्थिरांक FilenameWithScore& first = list.m_files.front();
                स्थिरांक FilenameWithScore& last = list.m_files.back();
                os << "| + " << first.m_filename << "  (score: " << first.m_score << ")\n";
                os << "| | (... " << (list.m_files.size() - 2) << " files ...)\n";
                os << "| + " << last.m_filename << "  (score: " << last.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    शून्य dumpOutputList(std::ostream& os) स्थिरांक {
        os << "List of output files after execution of concatenation:\n";

        साठी (स्थिरांक FileOrConcatenatedFilesList& entry : m_outputFiles) {
            जर (entry.isConcatenatingFile()) {
                os << "+ " << entry.m_filename << "  (concatenating file)\n";
                साठी (स्थिरांक string& f : entry.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } अन्यथा {
                os << "+ " << entry.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    तार्किक fallbackNoGrouping(माप_प्रकार आदान_Files_मोजणी) {
        स्थिरांक पूर्णांक total_Buckets_अंक = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        तार्किक groupingRedundant = असत्य;
        जर (आदान_Files_मोजणी < MIN_FILES_मोजणी
            && आदान_Files_मोजणी <= static_cast<माप_प्रकार>(total_Buckets_अंक)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_आदान_Files.size() << " < "
                                                                   << MIN_FILES_मोजणी << ")");
            groupingRedundant = सत्य;
        }
        जर (आदान_Files_मोजणी < (MIN_FILES_PER_BUCKET * total_Buckets_अंक)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_आदान_Files.size() << " < " << MIN_FILES_PER_BUCKET << " - "
                         << total_Buckets_अंक << ")");
            groupingRedundant = सत्य;
        }
        जर (!groupingRedundant) परत असत्य;

        m_outputFiles.reserve(m_आदान_Files.size());
        साठी (स्थिरांक FilenameWithScore& filename : m_आदान_Files) {
            m_outputFiles.ढकला_back({std::move(filename.m_filename), {}});
        }
        परत सत्य;
    }

    शून्य createWorkLists() {
        // Create initial Work Lists.

        स्थिरांक पूर्णांक total_Buckets_अंक = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        स्थिरांक uint64_t concatenableFileMaxScore = m_totalScore / total_Buckets_अंक / 2;

        V3Stats::addStat("Concatenation max score", concatenableFileMaxScore);
        पूर्णांक nextWorkListId = 0;

        जर (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        साठी (स्थिरांक FilenameWithScore& आदान_File : m_आदान_Files) {
            स्थिरांक तार्किक fileIsConcatenable = (आदान_File.m_score <= concatenableFileMaxScore);
            जर (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << आदान_File.m_filename << "  (score: " << आदान_File.m_score << ")\n";
            V3Stats::addStatSum(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                आदान_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            जर (m_workLists.empty() || m_workLists.back().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_back(WorkList{nextWorkListId++});
                m_workLists.back().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.back();
            list.m_files.ढकला_back(
                {आदान_File.m_filename, आदान_File.m_score, आदान_File.m_sizeBytes});
            list.m_totalScore += आदान_File.m_score;
        }
        जर (m_logp) *m_logp << '\n';
    }

    शून्य assignBuckets(uint64_t concatenableFilesTotalScore) {
        // Assign buckets to lists

        स्थिरांक माप_प्रकार total_Buckets_अंक = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        जर (m_concatenableListsByDescSize.size() > total_Buckets_अंक) {
            // Debugging: Log which work lists will be kept
            जर (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenableListsByDescSize.begin(),
                              m_concatenableListsByDescSize.begin() + total_Buckets_अंक,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.size()
                                          << "; total score: " << listp->m_totalScore << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenableListsByDescSize.begin() + total_Buckets_अंक,
                          m_concatenableListsByDescSize.end(), [&](WorkList* listp) {
                              listp->m_isConcatenable = असत्य;

                              जर (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.size()
                                          << "; total score: " << listp->m_totalScore << ")\n";
                          });
            जर (m_logp) *m_logp << '\n';

            m_concatenableListsByDescSize.resize(total_Buckets_अंक);
            // Recalculate stats
            concatenableFilesTotalScore = 0;
            साठी (स्थिरांक WorkList* स्थिरांक listp : m_concatenableListsByDescSize) {
                concatenableFilesTotalScore += listp->m_totalScore;
            }
        }

        स्थिरांक uint64_t idealBucketScore = concatenableFilesTotalScore / total_Buckets_अंक;

        V3Stats::addStat("Concatenation ideal bucket score", idealBucketScore);

        जर (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        पूर्णांक availableBuckets = v3Global.opt.outputGroups();
        साठी (WorkList* listp : m_concatenableListsByDescSize) {
            जर (availableBuckets > 0 && idealBucketScore > 0) {
                listp->m_buckets_अंक = std::min(
                    availableBuckets, std::max<पूर्णांक>(1, listp->m_totalScore / idealBucketScore));
                availableBuckets -= listp->m_buckets_अंक;
                जर (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_अंक << "] Work List #"
                            << listp->m_dbgId << '\n';
            } अन्यथा {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = असत्य;
                जर (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::डावा << std::setw(4) << listp->m_dbgId
                            << std::उजवा << " (excluding from concatenation)\n";
            }
        }
        जर (m_logp) *m_logp << '\n';
    }

    शून्य buildOutputList() {
        // Assign files to buckets and build final list of files

        // At this point the workLists contains concatenatable file lists separated by one or more
        // non-concatenable file lists. Each concatenatable list has N buckets (where N > 0)
        // assigned to it, which have to be filled with files from this list. Ideally, sum of file
        // scores in every bucket should be the same.
        साठी (WorkList& list : m_workLists) {
            जर (!list.m_isConcatenable) {
                साठी (स्थिरांक FilenameWithScore& file : list.m_files) {
                    m_outputFiles.ढकला_back({std::move(file.m_filename), {}});
                }
                सुरूठेवा;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            स्थिरांक uint64_t listIdealBucketScore = list.m_totalScore / list.m_buckets_अंक;

            std::vector<std::vector<स्थिरांक FilenameWithScore*>> buckets;
            स्वतः fileIt = list.m_files.cbegin();
            साठी (पूर्णांक i = 0; i < list.m_buckets_अंक; ++i) {
                std::vector<स्थिरांक FilenameWithScore*> bucket;
                uint64_t bucketScore = 0;

                साठी (; fileIt != list.m_files.cend(); ++fileIt) {
                    स्थिरांक uint64_t diffNow
                        = std::असल(static_cast<int64_t>(listIdealBucketScore - bucketScore));
                    स्थिरांक uint64_t diffIfAdded = std::असल(static_cast<int64_t>(
                        listIdealBucketScore - bucketScore - fileIt->m_score));
                    जर (bucketScore == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        bucketScore += fileIt->m_score;
                        bucket.ढकला_back(&*fileIt);
                    } अन्यथा {
                        // Best possible bucket score reached, process next bucket.
                        थांबा;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.ढकला_back(std::move(bucket));
            }
            साठी (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.back().ढकला_back(&*fileIt);
            }
            साठी (स्थिरांक std::vector<स्थिरांक FilenameWithScore*>& bucket : buckets) {
                addBucketGroups(bucket);
            }
        }
    }

    शून्य addBucketGroups(स्थिरांक std::vector<स्थिरांक FilenameWithScore*>& bucket) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::vector<std::vector<std::string>> groups;
        uint64_t groupBytes = 0;
        साठी (स्थिरांक FilenameWithScore* स्थिरांक filep : bucket) {
            जर (groups.empty() || groupBytes + filep->m_sizeBytes > m_maxGroupBytes) {
                groups.emplace_back();
                groupBytes = 0;
            }
            groupBytes += filep->m_sizeBytes;
            groups.back().ढकला_back(filep->m_filename);
        }
        साठी (std::vector<std::string>& group : groups) {
            जर (group.size() == 1) {
                m_outputFiles.ढकला_back({std::move(group.front()), {}});
            } अन्यथा {
                m_outputFiles.ढकला_back({groupFilename(m_group_अंक++), std::move(group)});
            }
        }
    }

    शून्य assertFilesSame() स्थिरांक {
        स्वतः ifIt = m_आदान_Files.begin();
        स्वतः ofIt = m_outputFiles.begin();
        पर्यंत (ifIt != m_आदान_Files.end() && ofIt != m_outputFiles.end()) {
            जर (ofIt->isConcatenatingFile()) {
                साठी (स्थिरांक string& ocf : ofIt->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_आदान_Files.end(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++ofIt;
            } अन्यथा {
                UASSERT(ifIt->m_filename == ofIt->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << ofIt->m_filename);
                ++ifIt;
                ++ofIt;
            }
        }
        UASSERT(ifIt == m_आदान_Files.end(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(ofIt == m_outputFiles.end(),
                "More output files than input files. First extra file: " << ofIt->m_filename);
    }

    शून्य process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_groupFilePrefix);
        UINFO(5, "Number of input files: " << m_आदान_Files.size());
        UINFO(5, "Total score: " << m_totalScore);

        स्थिरांक पूर्णांक total_Buckets_अंक = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << total_Buckets_अंक);
        UASSERT(total_Buckets_अंक > 0, "More than 0 buckets required");

        जर (fallbackNoGrouping(m_आदान_Files.size())) परत;

        जर (debug() >= 6 || dumpLevel() >= 6) {
            स्थिरांक string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            जर (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        जर (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        माप_प्रकार concatenable_Files_मोजणी = 0;
        int64_t concatenableFilesTotalScore = 0;

        साठी (WorkList& list : m_workLists) {
            जर (!list.m_isConcatenable) सुरूठेवा;

            // "Concatenation" of a single file is pointless
            जर (list.m_files.size() == 1) {
                list.m_isConcatenable = असत्य;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                सुरूठेवा;
            }

            concatenable_Files_मोजणी += list.m_files.size();
            concatenableFilesTotalScore += list.m_totalScore;
            // This vector is sorted below
            m_concatenableListsByDescSize.ढकला_back(&list);
        }

        जर (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        जर (fallbackNoGrouping(concatenable_Files_मोजणी)) परत;

        std::sort(m_concatenableListsByDescSize.begin(), m_concatenableListsByDescSize.end(),
                  [](स्थिरांक WorkList* ap, स्थिरांक WorkList* bp) {
                      // Sort in descending order by number of files
                      जर (ap->m_files.size() != bp->m_files.size())
                          परत (ap->m_files.size() > bp->m_files.size());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      परत bp->m_totalScore > ap->m_totalScore;
                  });

        assignBuckets(concatenableFilesTotalScore);

        buildOutputList();

        जर (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

सार्वजनिक:
    स्थिर std::vector<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::vector<FilenameWithScore> आदान_Files, uint64_t totalScore,
                                स्थिरांक std::string& groupFilePrefix, uint64_t maxGroupBytes) {
        EmitGroup group{std::move(आदान_Files), totalScore, groupFilePrefix, maxGroupBytes};
        group.process();
        परत group.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

वर्ग EmitMk final {
    using FileOrConcatenatedFilesList = EmitGroup::FileOrConcatenatedFilesList;
    using FilenameWithScore = EmitGroup::FilenameWithScore;

    // MEMBERS
    द्विगुण m_put_Class_मोजणी = 0;  // Number of classEntries printed

सार्वजनिक:
    // METHODS

    स्थिर शून्य emitConcatenatingFile(स्थिरांक FileOrConcatenatedFilesList& entry) {
        UASSERT(entry.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3OutCFile concatenatingFile{v3Global.opt.makeDir() + "/" + entry.m_filename + ".cpp"};
        concatenatingFile.putsHeader();
        साठी (स्थिरांक string& file : entry.m_concatenatedFilenames) {
            concatenatingFile.माला_दो("#include \"" + file + ".cpp\"\n");
        }
    }

    स्थिर uint64_t fileSize(स्थिरांक AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::द्विमान | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        परत static_cast<uint64_t>(ifs.tellg());
    }

    शून्य putMakeClassEntry(V3OutMkFile& of, स्थिरांक string& name) {
        of.माला_दो("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_मोजणी;
    }

    शून्य emitClassMake() {
        std::vector<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::vector<FileOrConcatenatedFilesList> vmClassesFastList;
        जर (v3Global.opt.outputGroups() > 0) {
            std::vector<FilenameWithScore> slowFiles;
            std::vector<FilenameWithScore> fastFiles;
            uint64_t slowTotalScore = 0;
            uint64_t fastTotalScore = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t headerBytes = 0;

            साठी (Ast_गाठ_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), गाठ_File)) {
                स्थिरांक AstCFile* स्थिरांक cfilep = VN_CAST(nodep, CFile);
                जर (cfilep && VString::endsWith(cfilep->name(), ".h")) {
                    headerBytes += fileSize(cfilep);
                }
                जर (cfilep && cfilep->source() && cfilep->support() == असत्य) {
                    std::vector<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& totalScore = cfilep->slow() ? slowTotalScore : fastTotalScore;

                    totalScore += cfilep->complexityScore();
                    files.ढकला_back({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), fileSize(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t maxGroupBytes = std::numeric_limits<uint64_t>::max();
            जर (v3Global.opt.outputGroupsSize()) {
                स्थिरांक uint64_t maxTuBytes = static_cast<uint64_t>(v3Global.opt.outputGroupsSize());
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
        of.माला_दो("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        of.माला_दो("#\n");
        of.माला_दो("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        of.माला_दो("# See " + v3Global.opt.prefix() + ".mk" + " for the caller.\n");

        of.माला_दो("\n### Switches...\n");
        of.माला_दो("# C11 constructs required?  0/1 (always on now)\n");
        of.putSet("VM_C11", "1");
        of.माला_दो("# Timing enabled?  0/1\n");
        of.putSet("VM_TIMING", v3Global.usesTiming() ? "1" : "0");
        of.माला_दो("# Coverage output mode?  0/1 (from --coverage)\n");
        of.putSet("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        of.माला_दो("# Parallel builds?  0/1 (from --output-split)\n");
        of.putSet("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        of.माला_दो("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        of.putSet("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        of.माला_दो("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        of.putSet("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        of.माला_दो("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        of.putSet("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        of.माला_दो("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        of.putSet("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        of.माला_दो("# VPI enabled?  0/1 (from --vpi)\n");
        of.putSet("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        of.माला_दो("\n### Object file lists...\n");
        साठी (पूर्णांक support = 0; support < 3; ++support) {
            साठी (स्थिरांक तार्किक& slow : {असत्य, सत्य}) {
                जर (support == 2) {
                    of.माला_दो("# Global classes, need linked once per executable");
                } अन्यथा जर (support) {
                    of.माला_दो("# Generated support classes");
                } अन्यथा {
                    of.माला_दो("# Generated module classes");
                }
                जर (slow) {
                    of.माला_दो(", non-fast-path, compile with low/medium optimization\n");
                } अन्यथा {
                    of.माला_दो(", fast-path, compile with highest optimization\n");
                }
                स्थिरांक string targetVar = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_मोजणी = 0;
                of.माला_दो(targetVar + " += \\\n");
                जर (support == 2 && v3Global.opt.hierChild()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } अन्यथा जर (support == 2 && !slow) {
                    साठी (स्थिरांक string& cpp : v3Global.verilatedCppFiles())
                        putMakeClassEntry(of, cpp);
                } अन्यथा जर (support == 2 && slow) {
                } अन्यथा जर (support == 0 && v3Global.opt.outputGroups() > 0) {
                    स्थिरांक std::vector<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    साठी (स्थिरांक FileOrConcatenatedFilesList& entry : list) {
                        जर (entry.isConcatenatingFile()) emitConcatenatingFile(entry);
                        putMakeClassEntry(of, entry.m_filename);
                    }
                } अन्यथा {
                    साठी (Ast_गाठ_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), गाठ_File)) {
                        स्थिरांक AstCFile* स्थिरांक cfilep = VN_CAST(nodep, CFile);
                        जर (cfilep && cfilep->source() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            putMakeClassEntry(of, cfilep->name());
                        }
                    }
                }
                of.माला_दो("\n");
                V3Stats::addStat("Makefile targets, " + targetVar, m_put_Class_मोजणी);
            }
        }

        of.putsHeader();
    }

    शून्य emitOverallMake() {
        // Generate the makefile
        V3OutMkFile of{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".mk"};
        of.putsHeader();
        of.माला_दो("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        of.माला_दो("#\n");
        of.माला_दो("# Execute this makefile from the object directory:\n");
        of.माला_दो("#    make -f " + v3Global.opt.prefix() + ".mk" + "\n");
        of.माला_दो("\n");

        जर (v3Global.opt.exe()) {
            of.माला_दो("default: " + v3Global.opt.exeName() + "\n");
        } अन्यथा जर (!v3Global.opt.libCreate().empty()) {
            of.माला_दो("default: lib" + v3Global.opt.libCreate() + "\n");
        } अन्यथा {
            of.माला_दो("default: lib" + v3Global.opt.prefix() + "\n");
        }
        of.माला_दो("\n### Constants...\n");
        of.माला_दो("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        of.putSet("PERL", V3OutFormatter::quoteNameControls(V3Options::getenvPERL()));
        of.माला_दो("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        of.putSet("PYTHON3", V3OutFormatter::quoteNameControls(V3Options::getenvPYTHON3()));
        of.माला_दो("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        of.putSet("VERILATOR_ROOT",
                  V3OutFormatter::quoteNameControls(V3Options::getenv_VERILATOR_मूळ()));
        of.माला_दो("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        of.putSetQ("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        of.माला_दो("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        of.putSetQ("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        जर (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        of.माला_दो("\n### Switches...\n");
        of.माला_दो("# C++ code coverage  0/1 (from --prof-c)\n");
        of.putSet("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        of.माला_दो("# SystemC output mode?  0/1 (from --sc)\n");
        of.putSet("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        of.माला_दो("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        of.putSet("VM_SP_OR_SC", "$(VM_SC)");
        of.माला_दो("# Deprecated\n");
        of.putSet("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        of.माला_दो(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        of.putSet("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        of.माला_दो("\n### Vars...\n");
        of.माला_दो("# Module prefix (from --prefix)\n");
        of.putSet("VM_MODPREFIX", v3Global.opt.modPrefix());
        of.माला_दो("# Design prefix (from --prefix)\n");
        of.putSet("VM_PREFIX", v3Global.opt.prefix());
        of.माला_दो("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        of.putSet("VM_PREFIX_INCLUDE", "<" + v3Global.opt.prefix() + ".h>");
        जर (v3Global.dpi()) {
            of.माला_दो("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            of.putSet("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.prefix() + "__Dpi.h>");
        }

        of.माला_दो("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        of.माला_दो("VM_USER_CFLAGS = \\\n");
        स्थिरांक std::string solver = V3Options::getenvVERILATOR_SOLVER();
        जर (v3Global.useRandomizeMethods() && solver != "")
            of.माला_दो("\t-DVM_SOLVER_DEFAULT='\"" + V3OutFormatter::quoteNameControls(solver)
                    + "\"' \\\n");
        जर (!v3Global.opt.libCreate().empty()) of.माला_दो("\t-fPIC \\\n");
        स्थिरांक VStringList& cFlags = v3Global.opt.cFlags();
        साठी (स्थिरांक string& i : cFlags) of.माला_दो("  " + i + " \\\n");
        of.माला_दो("\n");

        of.माला_दो("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        of.माला_दो("VM_USER_LDLIBS = \\\n");
        स्थिरांक VStringList& ldLibs = v3Global.opt.ldLibs();
        साठी (स्थिरांक string& i : ldLibs) of.माला_दो("  " + i + " \\\n");
        of.माला_दो("\n");

        of.माला_दो("# User library files (from .a/.o/.so on Verilator command line)\n");
        of.माला_दो("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        of.माला_दो("VM_USER_LDLIBS_FILES = \\\n");
        साठी (स्थिरांक string& i : v3Global.opt.ldLibFiles()) of.माला_दो("  " + i + " \\\n");
        of.माला_दो("\n");

        VStringSet dirs;
        of.माला_दो("# User .cpp files (from .cpp's on Verilator command line)\n");
        of.माला_दो("VM_USER_CLASSES = \\\n");
        स्थिरांक VStringSet& cppFiles = v3Global.opt.cppFiles();
        साठी (स्थिरांक स्वतः& cppfile : cppFiles) {
            of.माला_दो("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            स्थिरांक string dir
                = V3Os::filenameRelativePath(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.घाला(dir);
        }
        dirs.घाला(V3Os::filenameRelativePath(".", v3Global.opt.makeDir()));

        of.माला_दो("\n");

        of.माला_दो("# User .cpp directories (from .cpp's on Verilator command line)\n");
        of.माला_दो("VM_USER_DIR = \\\n");
        साठी (स्थिरांक स्वतः& i : dirs) of.माला_दो("  " + i + " \\\n");
        of.माला_दो("\n");

        of.माला_दो("### Default rules...\n");
        of.माला_दो("# Include list of all generated classes\n");
        of.माला_दो("include " + v3Global.opt.prefix() + "_classes.mk\n");
        जर (v3Global.opt.hier_शीर्ष()) {
            of.माला_दो("# Include rules for hierarchical blocks\n");
            of.माला_दो("include " + v3Global.opt.prefix() + "_hier.mk\n");
        }
        of.माला_दो("# Include global rules\n");
        of.माला_दो("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        जर (v3Global.opt.exe()) {
            of.माला_दो("\n### Executable rules... (from --exe)\n");
            of.माला_दो("VPATH += $(VM_USER_DIR)\n");
            of.माला_दो("\n");

            जर (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                of.माला_दो("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                of.माला_दो("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                of.माला_दो("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                of.माला_दो("\n");
            }
        }

        स्थिरांक string compilerIncludePch
            = v3Global.opt.compilerIncludes().empty() ? "" : "$(VK_PCH_H).fast.gch";
        स्थिरांक string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().empty() ? "" : "$(VK_PCH_I_FAST)";
        साठी (स्थिरांक string& cppfile : cppFiles) {
            स्थिरांक string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            of.माला_दो(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            of.माला_दो("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        जर (v3Global.opt.exe()) {
            of.माला_दो("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            of.माला_दो(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            of.माला_दो("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            of.माला_दो("\n");
        } अन्यथा जर (!v3Global.opt.libCreate().empty()) {
            स्थिरांक string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            of.माला_दो("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            of.माला_दो(v3Global.opt.libCreateName(असत्य) + ": " + libCreateDeps + "\n");
            of.माला_दो("\n");
            जर (v3Global.opt.hierChild()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                of.माला_दो("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(असत्य)
                        + "\n");
            } अन्यथा {
                of.माला_दो(v3Global.opt.libCreateName(सत्य) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                of.माला_दो("ifeq ($(shell uname -s),Darwin)\n");
                of.माला_दो("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                of.माला_दो("else\n");
                of.माला_दो("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                of.माला_दो("endif\n");
                of.माला_दो("\n");
                of.माला_दो("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(असत्य)
                        + " " + v3Global.opt.libCreateName(सत्य) + "\n");
            }
        } अन्यथा {
            स्थिरांक string libname = "lib" + v3Global.opt.prefix() + ".a";
            of.माला_दो("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            of.माला_दो(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            of.माला_दो("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            of.माला_दो("lib" + v3Global.opt.prefix() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        of.putsHeader();
    }

    explicit EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    आभासी ~EmitMk() = पूर्वनिर्धारित;
};

//######################################################################

वर्ग EmitMkHierVerilation final {
    स्थिरांक V3HierGraph* स्थिरांक m_graphp;
    स्थिरांक string m_makefile;  // path of this makefile
    शून्य emitCommonOpts(V3OutMkFile& of) स्थिरांक {
        of.माला_दो("# Verilation of hierarchical blocks are executed in this directory\n");
        of.माला_दो("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        of.माला_दो("# Common options for hierarchical blocks\n");
        स्थिरांक string fullpath_bin = V3Os::filenameRealPath(v3Global.opt.buildDepBin());
        स्थिरांक string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        of.माला_दो("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        of.माला_दो("VM_HIER_INPUT_FILES := \\\n");
        साठी (स्थिरांक स्वतः& i : v3Global.opt.vFiles())
            of.माला_दो("  " + V3Os::filenameRealPath(i.filename()) + " \\\n");
        of.माला_दो("\n");
        of.माला_दो("VM_HIER_VERILOG_LIBS := \\\n");
        साठी (स्थिरांक स्वतः& i : v3Global.opt.libraryFiles()) {
            of.माला_दो("  " + V3Os::filenameRealPath(i.filename()) + " \\\n");
        }
        of.माला_दो("\n");
    }
    शून्य emitLaunchVerilator(V3OutMkFile& of, स्थिरांक string& argsFile) स्थिरांक {
        of.माला_दो("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        of.माला_दो("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + argsFile + "\"\n");
    }
    शून्य emit(V3OutMkFile& of) स्थिरांक {
        of.माला_दो("# Hierarchical Verilation -*- Makefile -*-\n");
        of.माला_दो("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        of.माला_दो("#\n");
        of.माला_दो("# The main makefile " + v3Global.opt.prefix() + ".mk calls this makefile\n");
        of.माला_दो("\n");
        of.माला_दो("ifndef VM_HIER_VERILATION_INCLUDED\n");
        of.माला_दो("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        of.माला_दो(".SUFFIXES:\n");
        of.माला_दो(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        of.माला_दो("# Libraries of hierarchical blocks\n");
        of.माला_दो("VM_HIER_LIBS := \\\n");
        साठी (स्थिरांक V3GraphVertex& vtx : m_graphp->vertices()) {
            स्थिरांक V3HierBlock* स्थिरांक blockp = vtx.as<V3HierBlock>();
            of.माला_दो("  " + blockp->hierLibFilename(सत्य) + " \\\n");
        }
        of.माला_दो("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        of.माला_दो("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.prefix() + ".mk\n");
        of.माला_दो("\t$(MAKE) -f " + v3Global.opt.prefix() + ".mk\n");
        of.माला_दो("hier_verilation: " + v3Global.opt.prefix() + ".mk\n");
        emitCommonOpts(of);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        of.माला_दो("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        of.माला_दो("hier_launch_verilator:\n");
        of.माला_दो("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            स्थिरांक string argsFile = v3Global.hierGraphp()->शीर्ष_Command_Args_Filename(असत्य);
            of.माला_दो("\n# Verilate the top module\n");
            of.माला_दो(v3Global.opt.prefix()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            of.माला_दो(V3Os::filenameNonDir(argsFile) + " ");
            साठी (स्थिरांक V3GraphVertex& vtx : m_graphp->vertices()) {
                स्थिरांक V3HierBlock* स्थिरांक blockp = vtx.as<V3HierBlock>();
                of.माला_दो(blockp->hierWrapperFilename(सत्य) + " ");
            }
            of.माला_दो("\n");
            emitLaunchVerilator(of, argsFile);
        }

        // Rules to process hierarchical blocks
        of.माला_दो("\n# Verilate hierarchical blocks\n");
        साठी (स्थिरांक V3GraphVertex& vtx : m_graphp->vertices()) {
            स्थिरांक V3HierBlock* स्थिरांक blockp = vtx.as<V3HierBlock>();
            स्थिरांक string prefix = blockp->hierPrefix();
            स्थिरांक string argsFilename = blockp->commandArgsFilename(असत्य);
            of.माला_दो(blockp->hierGeneratedFilenames(सत्य));
            of.माला_दो(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            of.माला_दो(V3Os::filenameNonDir(argsFilename) + " ");
            साठी (स्थिरांक V3GraphEdge& edge : blockp->outEdges()) {
                स्थिरांक V3HierBlock* स्थिरांक dependencyp = edge.शीर्ष()->as<V3HierBlock>();
                of.माला_दो(dependencyp->hierWrapperFilename(सत्य) + " ");
            }
            of.माला_दो("\n");
            emitLaunchVerilator(of, argsFilename);

            // Rule to build lib*.a
            of.माला_दो(blockp->hierLibFilename(सत्य));
            of.माला_दो(": ");
            of.माला_दो(blockp->hierMkFilename(सत्य));
            of.माला_दो(" ");
            साठी (स्थिरांक V3GraphEdge& edge : blockp->outEdges()) {
                स्थिरांक V3HierBlock* स्थिरांक dependencyp = edge.शीर्ष()->as<V3HierBlock>();
                of.माला_दो(dependencyp->hierLibFilename(सत्य));
                of.माला_दो(" ");
            }
            of.माला_दो("\n\t$(MAKE) -f " + blockp->hierMkFilename(असत्य) + " -C " + prefix);
            of.माला_दो(" VM_PREFIX=" + prefix);
            of.माला_दो("\n\n");
        }
        of.माला_दो("endif  # Guard\n");
    }

सार्वजनिक:
    explicit EmitMkHierVerilation(स्थिरांक V3HierGraph* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + "_hier.mk"} {
        V3OutMkFile of{m_makefile};
        emit(of);
    }
};

//######################################################################
// Gate class functions

शून्य V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    स्थिरांक EmitMk emitter;
}

शून्य V3EmitMk::emitHierVerilation(स्थिरांक V3HierGraph* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
