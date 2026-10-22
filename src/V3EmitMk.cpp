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
वर्ग Emit_गणः final {
सार्वजनिक:
    संरचना FileOrConcatenatedFilesList final {
        ध्रुव std::string m_filename;  // Filename or output group filename if grouping
        std::सदिशम्<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        तार्किक isConcatenatingFile() ध्रुव { प्रत्यागम !m_concatenatedFilenames.रिक्तानि(); }
    };

    संरचना FilenameWithScore final {
        ध्रुव std::string m_filename;  // Input filename
        ध्रुव uint64_t m_score;  // Input file complexity
        ध्रुव uint64_t m_आकार_अष्टकानि;  // Input file size
    };

गुप्त:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    संरचना WorkList final {
        uint64_t m_योग_Score = 0;  // Sum of scores of included files
        std::सदिशम्<FilenameWithScore> m_files;  // Included filenames
        पूर्णाङ्क m_buckets_अङ्कः
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        तार्किक m_isConcatenable = सत्य;  // Indicated whether files on this list can be concatenated.
        ध्रुव पूर्णाङ्क m_dbgId;  // Work list ID for debugging.

        WorkList() = विलोपय;
        स्पष्ट WorkList(पूर्णाङ्क id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    स्थिर constexpr आकार_t MIN_FILES_गणनम् = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    स्थिर constexpr आकार_t MIN_FILES_प्रत्येकम्_BUCKET = 4;

    // MEMBERS

    ध्रुव std::सदिशम्<FilenameWithScore>
        m_निवेशः_Files;  // List of filenames from initial AstCFile list
    std::सदिशम्<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ध्रुव uint64_t m_योग_Score;  // Sum of file scores
    ध्रुव std::string m_गणः_File_उपसर्गः;  // Prefix for output group filenames
    ध्रुव uint64_t m_max_गणः_अष्टकानि;  // Maximum sum of file sizes in an output group
    std::सदिशम्<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::सदिशम्<WorkList*> m_concatenable_Lists_By_Desc_आकार;  // Lists sorted by size, descending
    पूर्णाङ्क m_गणः_अङ्कः = 0;  // Number of output groups created

    Emit_गणः(std::सदिशम्<FilenameWithScore> निवेशः_Files, uint64_t योग_Score,
              ध्रुव std::string& गणः_File_उपसर्गः, uint64_t max_गणः_अष्टकानि)
        : m_निवेशः_Files{std::move(निवेशः_Files)}
        , m_योग_Score{योग_Score}
        , m_गणः_File_उपसर्गः{गणः_File_उपसर्गः}
        , m_max_गणः_अष्टकानि{max_गणः_अष्टकानि} {}

    string गणः_Filename(पूर्णाङ्क id) ध्रुव {
        प्रत्यागम v3Global.opt.उपसर्गः() + "_" + m_गणः_File_उपसर्गः + std::गन्तव्यम्_string(id);
    }

    // Debug logging: prints scores histogram
    शून्य dumpLogScoreHistogram(std::ostream& os) {
        constexpr पूर्णाङ्क MAX_BAR_दीर्घता = 80;
        constexpr पूर्णाङ्क MAX_INTERVALS_अङ्कः = 60;

        // All scores arranged in ascending order.
        std::सदिशम्<uint64_t> sortedScores;
        sortedScores.reserve(m_निवेशः_Files.आकार());
        std::transform(m_निवेशः_Files.begin(), m_निवेशः_Files.अन्तः_2(), std::पृष्ठभागः_inserter(sortedScores),
                       [](ध्रुव FilenameWithScore& निवेशः_File) { प्रत्यागम निवेशः_File.m_score; });
        std::क्रमय(sortedScores.begin(), sortedScores.अन्तः_2());

        ध्रुव int64_t शिखरम्_Score = sortedScores.पृष्ठभागः();
        os << "Top score: " << शिखरम्_Score << '\n';
        ध्रुव पूर्णाङ्क max_Score_विस्तारः = std::गन्तव्यम्_string(शिखरम्_Score).दीर्घता_2();

        ध्रुव int64_t intervals_अङ्कः = std::min<int64_t>(शिखरम्_Score + 1, MAX_INTERVALS_अङ्कः);

        संरचना Interval final {
            uint64_t m_अधस्तनम्_Bound = 0;  // Lowest score included in this histogram bucket
            पूर्णाङ्क m_आकार = 0;  // Number of scores that fell into this bucket
        };

        std::सदिशम्<Interval> intervals;
        intervals.resize(intervals_अङ्कः);

        intervals[0].m_अधस्तनम्_Bound = 0;
        प्रति (पूर्णाङ्क i = 1; i < intervals_अङ्कः; i++) {
            intervals[i].m_अधस्तनम्_Bound = (शिखरम्_Score + 1) * i / intervals_अङ्कः + 1;
        }

        प्रति (ध्रुव uint64_t score : sortedScores) {
            ध्रुव अचिह्नित पूर्णाङ्क ivIdx = score * intervals_अङ्कः / (शिखरम्_Score + 1);
            ++intervals[ivIdx].m_आकार;
        }

        पूर्णाङ्क शिखरम्_Interval_आकार = 0;
        प्रति (ध्रुव Interval& iv : intervals)
            शिखरम्_Interval_आकार = std::max(शिखरम्_Interval_आकार, iv.m_आकार);

        os << "Input files' scores histogram:\n";

        प्रति (ध्रुव Interval& iv : intervals) {
            ध्रुव पूर्णाङ्क scaled_आकार = iv.m_आकार * (MAX_BAR_दीर्घता + 1) / शिखरम्_Interval_आकार;
            ध्रुव std::string पङ्क्तिः = " |" + std::string(scaled_आकार, '#');

            os << std::setw(max_Score_विस्तारः) << iv.m_अधस्तनम्_Bound << पङ्क्तिः << "  " << iv.m_आकार << '\n';
        }
        os << std::setw(max_Score_विस्तारः) << (शिखरम्_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    शून्य dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        प्रति (ध्रुव WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.आकार()
               << "; total score: " << list.m_योग_Score << ")\n";
            यदि (debug() >= 6 || list.m_files.आकार() < 4) {
                // Log all files
                प्रति (ध्रुव FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } अन्यथा {
                // Log only first and last file
                ध्रुव FilenameWithScore& first = list.m_files.front();
                ध्रुव FilenameWithScore& चरमः = list.m_files.पृष्ठभागः();
                os << "| + " << first.m_filename << "  (score: " << first.m_score << ")\n";
                os << "| | (... " << (list.m_files.आकार() - 2) << " files ...)\n";
                os << "| + " << चरमः.m_filename << "  (score: " << चरमः.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    शून्य dumpOutputList(std::ostream& os) ध्रुव {
        os << "List of output files after execution of concatenation:\n";

        प्रति (ध्रुव FileOrConcatenatedFilesList& प्रविष्टिः : m_outputFiles) {
            यदि (प्रविष्टिः.isConcatenatingFile()) {
                os << "+ " << प्रविष्टिः.m_filename << "  (concatenating file)\n";
                प्रति (ध्रुव string& f : प्रविष्टिः.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } अन्यथा {
                os << "+ " << प्रविष्टिः.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    तार्किक fallbackNoGrouping(आकार_t निवेशः_Files_गणनम्) {
        ध्रुव पूर्णाङ्क योग_Buckets_अङ्कः = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        तार्किक groupingRedundant = असत्य;
        यदि (निवेशः_Files_गणनम् < MIN_FILES_गणनम्
            && निवेशः_Files_गणनम् <= static_cast<आकार_t>(योग_Buckets_अङ्कः)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_निवेशः_Files.आकार() << " < "
                                                                   << MIN_FILES_गणनम् << ")");
            groupingRedundant = सत्य;
        }
        यदि (निवेशः_Files_गणनम् < (MIN_FILES_प्रत्येकम्_BUCKET * योग_Buckets_अङ्कः)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_निवेशः_Files.आकार() << " < " << MIN_FILES_प्रत्येकम्_BUCKET << " - "
                         << योग_Buckets_अङ्कः << ")");
            groupingRedundant = सत्य;
        }
        यदि (!groupingRedundant) प्रत्यागम असत्य;

        m_outputFiles.reserve(m_निवेशः_Files.आकार());
        प्रति (ध्रुव FilenameWithScore& filename : m_निवेशः_Files) {
            m_outputFiles.प्रक्षिप_पृष्ठभागः({std::move(filename.m_filename), {}});
        }
        प्रत्यागम सत्य;
    }

    शून्य createWorkLists() {
        // Create initial Work Lists.

        ध्रुव पूर्णाङ्क योग_Buckets_अङ्कः = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ध्रुव uint64_t concatenableFileMaxScore = m_योग_Score / योग_Buckets_अङ्कः / 2;

        V3Stats::सङ्कलय_Stat("Concatenation max score", concatenableFileMaxScore);
        पूर्णाङ्क अग्र_Work_List_Id = 0;

        यदि (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        प्रति (ध्रुव FilenameWithScore& निवेशः_File : m_निवेशः_Files) {
            ध्रुव तार्किक fileIsConcatenable = (निवेशः_File.m_score <= concatenableFileMaxScore);
            यदि (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << निवेशः_File.m_filename << "  (score: " << निवेशः_File.m_score << ")\n";
            V3Stats::सङ्कलय_Stat_योग(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                निवेशः_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            यदि (m_workLists.रिक्तानि() || m_workLists.पृष्ठभागः().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_पृष्ठभागः(WorkList{अग्र_Work_List_Id++});
                m_workLists.पृष्ठभागः().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.पृष्ठभागः();
            list.m_files.प्रक्षिप_पृष्ठभागः(
                {निवेशः_File.m_filename, निवेशः_File.m_score, निवेशः_File.m_आकार_अष्टकानि});
            list.m_योग_Score += निवेशः_File.m_score;
        }
        यदि (m_logp) *m_logp << '\n';
    }

    शून्य नियोजय_Buckets(uint64_t concatenable_Files_योग_Score) {
        // Assign buckets to lists

        ध्रुव आकार_t योग_Buckets_अङ्कः = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        यदि (m_concatenable_Lists_By_Desc_आकार.आकार() > योग_Buckets_अङ्कः) {
            // Debugging: Log which work lists will be kept
            यदि (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_आकार.begin(),
                              m_concatenable_Lists_By_Desc_आकार.begin() + योग_Buckets_अङ्कः,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.आकार()
                                          << "; total score: " << listp->m_योग_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_आकार.begin() + योग_Buckets_अङ्कः,
                          m_concatenable_Lists_By_Desc_आकार.अन्तः_2(), [&](WorkList* listp) {
                              listp->m_isConcatenable = असत्य;

                              यदि (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.आकार()
                                          << "; total score: " << listp->m_योग_Score << ")\n";
                          });
            यदि (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_आकार.resize(योग_Buckets_अङ्कः);
            // Recalculate stats
            concatenable_Files_योग_Score = 0;
            प्रति (ध्रुव WorkList* ध्रुव listp : m_concatenable_Lists_By_Desc_आकार) {
                concatenable_Files_योग_Score += listp->m_योग_Score;
            }
        }

        ध्रुव uint64_t idealBucketScore = concatenable_Files_योग_Score / योग_Buckets_अङ्कः;

        V3Stats::सङ्कलय_Stat("Concatenation ideal bucket score", idealBucketScore);

        यदि (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        पूर्णाङ्क उपलब्धम्_Buckets = v3Global.opt.outputGroups();
        प्रति (WorkList* listp : m_concatenable_Lists_By_Desc_आकार) {
            यदि (उपलब्धम्_Buckets > 0 && idealBucketScore > 0) {
                listp->m_buckets_अङ्कः = std::min(
                    उपलब्धम्_Buckets, std::max<पूर्णाङ्क>(1, listp->m_योग_Score / idealBucketScore));
                उपलब्धम्_Buckets -= listp->m_buckets_अङ्कः;
                यदि (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_अङ्कः << "] Work List #"
                            << listp->m_dbgId << '\n';
            } अन्यथा {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = असत्य;
                यदि (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::वामतः << std::setw(4) << listp->m_dbgId
                            << std::दक्षिणतः << " (excluding from concatenation)\n";
            }
        }
        यदि (m_logp) *m_logp << '\n';
    }

    शून्य buildOutputList() {
        // Assign files to buckets and build final list of files

        // At this point the workLists contains concatenatable file lists separated by one or more
        // non-concatenable file lists. Each concatenatable list has N buckets (where N > 0)
        // assigned to it, which have to be filled with files from this list. Ideally, sum of file
        // scores in every bucket should be the same.
        प्रति (WorkList& list : m_workLists) {
            यदि (!list.m_isConcatenable) {
                प्रति (ध्रुव FilenameWithScore& file : list.m_files) {
                    m_outputFiles.प्रक्षिप_पृष्ठभागः({std::move(file.m_filename), {}});
                }
                अनुवर्त;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ध्रुव uint64_t listIdealBucketScore = list.m_योग_Score / list.m_buckets_अङ्कः;

            std::सदिशम्<std::सदिशम्<ध्रुव FilenameWithScore*>> buckets;
            स्वचालित fileIt = list.m_files.cbegin();
            प्रति (पूर्णाङ्क i = 0; i < list.m_buckets_अङ्कः; ++i) {
                std::सदिशम्<ध्रुव FilenameWithScore*> bucket;
                uint64_t bucketScore = 0;

                प्रति (; fileIt != list.m_files.cend(); ++fileIt) {
                    ध्रुव uint64_t diffNow
                        = std::निरपेक्षम्(static_cast<int64_t>(listIdealBucketScore - bucketScore));
                    ध्रुव uint64_t diffIfAdded = std::निरपेक्षम्(static_cast<int64_t>(
                        listIdealBucketScore - bucketScore - fileIt->m_score));
                    यदि (bucketScore == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        bucketScore += fileIt->m_score;
                        bucket.प्रक्षिप_पृष्ठभागः(&*fileIt);
                    } अन्यथा {
                        // Best possible bucket score reached, process next bucket.
                        भङ्ग;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.प्रक्षिप_पृष्ठभागः(std::move(bucket));
            }
            प्रति (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.पृष्ठभागः().प्रक्षिप_पृष्ठभागः(&*fileIt);
            }
            प्रति (ध्रुव std::सदिशम्<ध्रुव FilenameWithScore*>& bucket : buckets) {
                सङ्कलय_Bucket_Groups(bucket);
            }
        }
    }

    शून्य सङ्कलय_Bucket_Groups(ध्रुव std::सदिशम्<ध्रुव FilenameWithScore*>& bucket) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::सदिशम्<std::सदिशम्<std::string>> groups;
        uint64_t गणः_अष्टकानि = 0;
        प्रति (ध्रुव FilenameWithScore* ध्रुव filep : bucket) {
            यदि (groups.रिक्तानि() || गणः_अष्टकानि + filep->m_आकार_अष्टकानि > m_max_गणः_अष्टकानि) {
                groups.emplace_पृष्ठभागः();
                गणः_अष्टकानि = 0;
            }
            गणः_अष्टकानि += filep->m_आकार_अष्टकानि;
            groups.पृष्ठभागः().प्रक्षिप_पृष्ठभागः(filep->m_filename);
        }
        प्रति (std::सदिशम्<std::string>& गणः : groups) {
            यदि (गणः.आकार() == 1) {
                m_outputFiles.प्रक्षिप_पृष्ठभागः({std::move(गणः.front()), {}});
            } अन्यथा {
                m_outputFiles.प्रक्षिप_पृष्ठभागः({गणः_Filename(m_गणः_अङ्कः++), std::move(गणः)});
            }
        }
    }

    शून्य assertFilesSame() ध्रुव {
        स्वचालित ifIt = m_निवेशः_Files.begin();
        स्वचालित सम्बन्धि_It = m_outputFiles.begin();
        यावत् (ifIt != m_निवेशः_Files.अन्तः_2() && सम्बन्धि_It != m_outputFiles.अन्तः_2()) {
            यदि (सम्बन्धि_It->isConcatenatingFile()) {
                प्रति (ध्रुव string& ocf : सम्बन्धि_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_निवेशः_Files.अन्तः_2(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++सम्बन्धि_It;
            } अन्यथा {
                UASSERT(ifIt->m_filename == सम्बन्धि_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << सम्बन्धि_It->m_filename);
                ++ifIt;
                ++सम्बन्धि_It;
            }
        }
        UASSERT(ifIt == m_निवेशः_Files.अन्तः_2(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(सम्बन्धि_It == m_outputFiles.अन्तः_2(),
                "More output files than input files. First extra file: " << सम्बन्धि_It->m_filename);
    }

    शून्य process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_गणः_File_उपसर्गः);
        UINFO(5, "Number of input files: " << m_निवेशः_Files.आकार());
        UINFO(5, "Total score: " << m_योग_Score);

        ध्रुव पूर्णाङ्क योग_Buckets_अङ्कः = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << योग_Buckets_अङ्कः);
        UASSERT(योग_Buckets_अङ्कः > 0, "More than 0 buckets required");

        यदि (fallbackNoGrouping(m_निवेशः_Files.आकार())) प्रत्यागम;

        यदि (debug() >= 6 || dumpLevel() >= 6) {
            ध्रुव string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            यदि (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        यदि (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        आकार_t concatenable_Files_गणनम् = 0;
        int64_t concatenable_Files_योग_Score = 0;

        प्रति (WorkList& list : m_workLists) {
            यदि (!list.m_isConcatenable) अनुवर्त;

            // "Concatenation" of a single file is pointless
            यदि (list.m_files.आकार() == 1) {
                list.m_isConcatenable = असत्य;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                अनुवर्त;
            }

            concatenable_Files_गणनम् += list.m_files.आकार();
            concatenable_Files_योग_Score += list.m_योग_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_आकार.प्रक्षिप_पृष्ठभागः(&list);
        }

        यदि (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        यदि (fallbackNoGrouping(concatenable_Files_गणनम्)) प्रत्यागम;

        std::क्रमय(m_concatenable_Lists_By_Desc_आकार.begin(), m_concatenable_Lists_By_Desc_आकार.अन्तः_2(),
                  [](ध्रुव WorkList* ap, ध्रुव WorkList* bp) {
                      // Sort in descending order by number of files
                      यदि (ap->m_files.आकार() != bp->m_files.आकार())
                          प्रत्यागम (ap->m_files.आकार() > bp->m_files.आकार());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      प्रत्यागम bp->m_योग_Score > ap->m_योग_Score;
                  });

        नियोजय_Buckets(concatenable_Files_योग_Score);

        buildOutputList();

        यदि (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

सार्वजनिक:
    स्थिर std::सदिशम्<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::सदिशम्<FilenameWithScore> निवेशः_Files, uint64_t योग_Score,
                                ध्रुव std::string& गणः_File_उपसर्गः, uint64_t max_गणः_अष्टकानि) {
        Emit_गणः गणः{std::move(निवेशः_Files), योग_Score, गणः_File_उपसर्गः, max_गणः_अष्टकानि};
        गणः.process();
        प्रत्यागम गणः.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

वर्ग EmitMk final {
    उपयुज्य FileOrConcatenatedFilesList = Emit_गणः::FileOrConcatenatedFilesList;
    उपयुज्य FilenameWithScore = Emit_गणः::FilenameWithScore;

    // MEMBERS
    द्विगुण m_put_Class_गणनम् = 0;  // Number of classEntries printed

सार्वजनिक:
    // METHODS

    स्थिर शून्य emitConcatenatingFile(ध्रुव FileOrConcatenatedFilesList& प्रविष्टिः) {
        UASSERT(प्रविष्टिः.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_बहिर्गमनम्_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + प्रविष्टिः.m_filename + ".cpp"};
        concatenatingFile.puts_शीर्षकम्();
        प्रति (ध्रुव string& file : प्रविष्टिः.m_concatenatedFilenames) {
            concatenatingFile.पङ्क्तिं_लिख("#include \"" + file + ".cpp\"\n");
        }
    }

    स्थिर uint64_t file_आकार(ध्रुव AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::द्विमान | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        प्रत्यागम static_cast<uint64_t>(ifs.tellg());
    }

    शून्य put_Make_Class_प्रविष्टिः(V3_बहिर्गमनम्_Mk_File& सम्बन्धि, ध्रुव string& name) {
        सम्बन्धि.पङ्क्तिं_लिख("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_गणनम्;
    }

    शून्य emitClassMake() {
        std::सदिशम्<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::सदिशम्<FileOrConcatenatedFilesList> vmClassesFastList;
        यदि (v3Global.opt.outputGroups() > 0) {
            std::सदिशम्<FilenameWithScore> slowFiles;
            std::सदिशम्<FilenameWithScore> fastFiles;
            uint64_t slow_योग_Score = 0;
            uint64_t fast_योग_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t शीर्षकम्_अष्टकानि = 0;

            प्रति (Ast_ग्रन्थिः_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), ग्रन्थिः_File)) {
                ध्रुव AstCFile* ध्रुव cfilep = VN_CAST(nodep, CFile);
                यदि (cfilep && VString::अन्ताः_With(cfilep->name(), ".h")) {
                    शीर्षकम्_अष्टकानि += file_आकार(cfilep);
                }
                यदि (cfilep && cfilep->source() && cfilep->support() == असत्य) {
                    std::सदिशम्<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& योग_Score = cfilep->slow() ? slow_योग_Score : fast_योग_Score;

                    योग_Score += cfilep->complexityScore();
                    files.प्रक्षिप_पृष्ठभागः({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_आकार(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_गणः_अष्टकानि = std::numeric_limits<uint64_t>::max();
            यदि (v3Global.opt.output_Groups_आकार()) {
                ध्रुव uint64_t max_Tu_अष्टकानि = static_cast<uint64_t>(v3Global.opt.output_Groups_आकार());
                max_गणः_अष्टकानि = max_Tu_अष्टकानि > शीर्षकम्_अष्टकानि ? max_Tu_अष्टकानि - शीर्षकम्_अष्टकानि : 0;
            }

            vmClassesSlowList = Emit_गणः::singleConcatenatedFilesList(
                std::move(slowFiles), slow_योग_Score, "vm_classes_Slow_", max_गणः_अष्टकानि);
            vmClassesFastList = Emit_गणः::singleConcatenatedFilesList(
                std::move(fastFiles), fast_योग_Score, "vm_classes_", max_गणः_अष्टकानि);
        }

        // Generate the makefile
        V3_बहिर्गमनम्_Mk_File सम्बन्धि{v3Global.opt.makeDir() + "/" + v3Global.opt.उपसर्गः() + "_classes.mk"};
        सम्बन्धि.puts_शीर्षकम्();
        सम्बन्धि.पङ्क्तिं_लिख("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        सम्बन्धि.पङ्क्तिं_लिख("#\n");
        सम्बन्धि.पङ्क्तिं_लिख("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        सम्बन्धि.पङ्क्तिं_लिख("# See " + v3Global.opt.उपसर्गः() + ".mk" + " for the caller.\n");

        सम्बन्धि.पङ्क्तिं_लिख("\n### Switches...\n");
        सम्बन्धि.पङ्क्तिं_लिख("# C11 constructs required?  0/1 (always on now)\n");
        सम्बन्धि.put_समुच्चयः("VM_C11", "1");
        सम्बन्धि.पङ्क्तिं_लिख("# Timing enabled?  0/1\n");
        सम्बन्धि.put_समुच्चयः("VM_TIMING", v3Global.उपयुङ्क्ते_Timing() ? "1" : "0");
        सम्बन्धि.पङ्क्तिं_लिख("# Coverage output mode?  0/1 (from --coverage)\n");
        सम्बन्धि.put_समुच्चयः("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        सम्बन्धि.पङ्क्तिं_लिख("# Parallel builds?  0/1 (from --output-split)\n");
        सम्बन्धि.put_समुच्चयः("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        सम्बन्धि.पङ्क्तिं_लिख("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        सम्बन्धि.put_समुच्चयः("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        सम्बन्धि.पङ्क्तिं_लिख("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        सम्बन्धि.put_समुच्चयः("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        सम्बन्धि.पङ्क्तिं_लिख("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        सम्बन्धि.put_समुच्चयः("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        सम्बन्धि.पङ्क्तिं_लिख("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        सम्बन्धि.put_समुच्चयः("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        सम्बन्धि.पङ्क्तिं_लिख("# VPI enabled?  0/1 (from --vpi)\n");
        सम्बन्धि.put_समुच्चयः("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        सम्बन्धि.पङ्क्तिं_लिख("\n### Object file lists...\n");
        प्रति (पूर्णाङ्क support = 0; support < 3; ++support) {
            प्रति (ध्रुव तार्किक& slow : {असत्य, सत्य}) {
                यदि (support == 2) {
                    सम्बन्धि.पङ्क्तिं_लिख("# Global classes, need linked once per executable");
                } अन्यथा यदि (support) {
                    सम्बन्धि.पङ्क्तिं_लिख("# Generated support classes");
                } अन्यथा {
                    सम्बन्धि.पङ्क्तिं_लिख("# Generated module classes");
                }
                यदि (slow) {
                    सम्बन्धि.पङ्क्तिं_लिख(", non-fast-path, compile with low/medium optimization\n");
                } अन्यथा {
                    सम्बन्धि.पङ्क्तिं_लिख(", fast-path, compile with highest optimization\n");
                }
                ध्रुव string targetVar = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_गणनम् = 0;
                सम्बन्धि.पङ्क्तिं_लिख(targetVar + " += \\\n");
                यदि (support == 2 && v3Global.opt.hierChild()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } अन्यथा यदि (support == 2 && !slow) {
                    प्रति (ध्रुव string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_प्रविष्टिः(सम्बन्धि, cpp);
                } अन्यथा यदि (support == 2 && slow) {
                } अन्यथा यदि (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ध्रुव std::सदिशम्<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    प्रति (ध्रुव FileOrConcatenatedFilesList& प्रविष्टिः : list) {
                        यदि (प्रविष्टिः.isConcatenatingFile()) emitConcatenatingFile(प्रविष्टिः);
                        put_Make_Class_प्रविष्टिः(सम्बन्धि, प्रविष्टिः.m_filename);
                    }
                } अन्यथा {
                    प्रति (Ast_ग्रन्थिः_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), ग्रन्थिः_File)) {
                        ध्रुव AstCFile* ध्रुव cfilep = VN_CAST(nodep, CFile);
                        यदि (cfilep && cfilep->source() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_प्रविष्टिः(सम्बन्धि, cfilep->name());
                        }
                    }
                }
                सम्बन्धि.पङ्क्तिं_लिख("\n");
                V3Stats::सङ्कलय_Stat("Makefile targets, " + targetVar, m_put_Class_गणनम्);
            }
        }

        सम्बन्धि.puts_शीर्षकम्();
    }

    शून्य emitOverallMake() {
        // Generate the makefile
        V3_बहिर्गमनम्_Mk_File सम्बन्धि{v3Global.opt.makeDir() + "/" + v3Global.opt.उपसर्गः() + ".mk"};
        सम्बन्धि.puts_शीर्षकम्();
        सम्बन्धि.पङ्क्तिं_लिख("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        सम्बन्धि.पङ्क्तिं_लिख("#\n");
        सम्बन्धि.पङ्क्तिं_लिख("# Execute this makefile from the object directory:\n");
        सम्बन्धि.पङ्क्तिं_लिख("#    make -f " + v3Global.opt.उपसर्गः() + ".mk" + "\n");
        सम्बन्धि.पङ्क्तिं_लिख("\n");

        यदि (v3Global.opt.exe()) {
            सम्बन्धि.पङ्क्तिं_लिख("default: " + v3Global.opt.exeName() + "\n");
        } अन्यथा यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            सम्बन्धि.पङ्क्तिं_लिख("default: lib" + v3Global.opt.libCreate() + "\n");
        } अन्यथा {
            सम्बन्धि.पङ्क्तिं_लिख("default: lib" + v3Global.opt.उपसर्गः() + "\n");
        }
        सम्बन्धि.पङ्क्तिं_लिख("\n### Constants...\n");
        सम्बन्धि.पङ्क्तिं_लिख("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        सम्बन्धि.put_समुच्चयः("PERL", V3_बहिर्गमनम्_Formatter::quoteNameControls(V3Options::getenvPERL()));
        सम्बन्धि.पङ्क्तिं_लिख("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        सम्बन्धि.put_समुच्चयः("PYTHON3", V3_बहिर्गमनम्_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        सम्बन्धि.पङ्क्तिं_लिख("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        सम्बन्धि.put_समुच्चयः("VERILATOR_ROOT",
                  V3_बहिर्गमनम्_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_मूलम्()));
        सम्बन्धि.पङ्क्तिं_लिख("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        सम्बन्धि.put_समुच्चयः_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        सम्बन्धि.पङ्क्तिं_लिख("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        सम्बन्धि.put_समुच्चयः_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        यदि (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        सम्बन्धि.पङ्क्तिं_लिख("\n### Switches...\n");
        सम्बन्धि.पङ्क्तिं_लिख("# C++ code coverage  0/1 (from --prof-c)\n");
        सम्बन्धि.put_समुच्चयः("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        सम्बन्धि.पङ्क्तिं_लिख("# SystemC output mode?  0/1 (from --sc)\n");
        सम्बन्धि.put_समुच्चयः("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        सम्बन्धि.पङ्क्तिं_लिख("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        सम्बन्धि.put_समुच्चयः("VM_SP_OR_SC", "$(VM_SC)");
        सम्बन्धि.पङ्क्तिं_लिख("# Deprecated\n");
        सम्बन्धि.put_समुच्चयः("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        सम्बन्धि.पङ्क्तिं_लिख(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        सम्बन्धि.put_समुच्चयः("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        सम्बन्धि.पङ्क्तिं_लिख("\n### Vars...\n");
        सम्बन्धि.पङ्क्तिं_लिख("# Module prefix (from --prefix)\n");
        सम्बन्धि.put_समुच्चयः("VM_MODPREFIX", v3Global.opt.mod_उपसर्गः());
        सम्बन्धि.पङ्क्तिं_लिख("# Design prefix (from --prefix)\n");
        सम्बन्धि.put_समुच्चयः("VM_PREFIX", v3Global.opt.उपसर्गः());
        सम्बन्धि.पङ्क्तिं_लिख("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        सम्बन्धि.put_समुच्चयः("VM_PREFIX_INCLUDE", "<" + v3Global.opt.उपसर्गः() + ".h>");
        यदि (v3Global.dpi()) {
            सम्बन्धि.पङ्क्तिं_लिख("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            सम्बन्धि.put_समुच्चयः("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.उपसर्गः() + "__Dpi.h>");
        }

        सम्बन्धि.पङ्क्तिं_लिख("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_USER_CFLAGS = \\\n");
        ध्रुव std::string solver = V3Options::getenvVERILATOR_SOLVER();
        यदि (v3Global.useRandomizeMethods() && solver != "")
            सम्बन्धि.पङ्क्तिं_लिख("\t-DVM_SOLVER_DEFAULT='\"" + V3_बहिर्गमनम्_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        यदि (!v3Global.opt.libCreate().रिक्तानि()) सम्बन्धि.पङ्क्तिं_लिख("\t-fPIC \\\n");
        ध्रुव VStringList& cFlags = v3Global.opt.cFlags();
        प्रति (ध्रुव string& i : cFlags) सम्बन्धि.पङ्क्तिं_लिख("  " + i + " \\\n");
        सम्बन्धि.पङ्क्तिं_लिख("\n");

        सम्बन्धि.पङ्क्तिं_लिख("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_USER_LDLIBS = \\\n");
        ध्रुव VStringList& ldLibs = v3Global.opt.ldLibs();
        प्रति (ध्रुव string& i : ldLibs) सम्बन्धि.पङ्क्तिं_लिख("  " + i + " \\\n");
        सम्बन्धि.पङ्क्तिं_लिख("\n");

        सम्बन्धि.पङ्क्तिं_लिख("# User library files (from .a/.o/.so on Verilator command line)\n");
        सम्बन्धि.पङ्क्तिं_लिख("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_USER_LDLIBS_FILES = \\\n");
        प्रति (ध्रुव string& i : v3Global.opt.ldLibFiles()) सम्बन्धि.पङ्क्तिं_लिख("  " + i + " \\\n");
        सम्बन्धि.पङ्क्तिं_लिख("\n");

        VString_समुच्चयः dirs;
        सम्बन्धि.पङ्क्तिं_लिख("# User .cpp files (from .cpp's on Verilator command line)\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_USER_CLASSES = \\\n");
        ध्रुव VString_समुच्चयः& cppFiles = v3Global.opt.cppFiles();
        प्रति (ध्रुव स्वचालित& cppfile : cppFiles) {
            सम्बन्धि.पङ्क्तिं_लिख("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ध्रुव string dir
                = V3Os::filename_Relative_मार्गः(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.निवेशय(dir);
        }
        dirs.निवेशय(V3Os::filename_Relative_मार्गः(".", v3Global.opt.makeDir()));

        सम्बन्धि.पङ्क्तिं_लिख("\n");

        सम्बन्धि.पङ्क्तिं_लिख("# User .cpp directories (from .cpp's on Verilator command line)\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_USER_DIR = \\\n");
        प्रति (ध्रुव स्वचालित& i : dirs) सम्बन्धि.पङ्क्तिं_लिख("  " + i + " \\\n");
        सम्बन्धि.पङ्क्तिं_लिख("\n");

        सम्बन्धि.पङ्क्तिं_लिख("### Default rules...\n");
        सम्बन्धि.पङ्क्तिं_लिख("# Include list of all generated classes\n");
        सम्बन्धि.पङ्क्तिं_लिख("include " + v3Global.opt.उपसर्गः() + "_classes.mk\n");
        यदि (v3Global.opt.hier_शिखरम्()) {
            सम्बन्धि.पङ्क्तिं_लिख("# Include rules for hierarchical blocks\n");
            सम्बन्धि.पङ्क्तिं_लिख("include " + v3Global.opt.उपसर्गः() + "_hier.mk\n");
        }
        सम्बन्धि.पङ्क्तिं_लिख("# Include global rules\n");
        सम्बन्धि.पङ्क्तिं_लिख("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        यदि (v3Global.opt.exe()) {
            सम्बन्धि.पङ्क्तिं_लिख("\n### Executable rules... (from --exe)\n");
            सम्बन्धि.पङ्क्तिं_लिख("VPATH += $(VM_USER_DIR)\n");
            सम्बन्धि.पङ्क्तिं_लिख("\n");

            यदि (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                सम्बन्धि.पङ्क्तिं_लिख("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                सम्बन्धि.पङ्क्तिं_लिख("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                सम्बन्धि.पङ्क्तिं_लिख("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                सम्बन्धि.पङ्क्तिं_लिख("\n");
            }
        }

        ध्रुव string compilerIncludePch
            = v3Global.opt.compilerIncludes().रिक्तानि() ? "" : "$(VK_PCH_H).fast.gch";
        ध्रुव string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().रिक्तानि() ? "" : "$(VK_PCH_I_FAST)";
        प्रति (ध्रुव string& cppfile : cppFiles) {
            ध्रुव string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            सम्बन्धि.पङ्क्तिं_लिख(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            सम्बन्धि.पङ्क्तिं_लिख("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        यदि (v3Global.opt.exe()) {
            सम्बन्धि.पङ्क्तिं_लिख("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            सम्बन्धि.पङ्क्तिं_लिख(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            सम्बन्धि.पङ्क्तिं_लिख("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            सम्बन्धि.पङ्क्तिं_लिख("\n");
        } अन्यथा यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            ध्रुव string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            सम्बन्धि.पङ्क्तिं_लिख("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            सम्बन्धि.पङ्क्तिं_लिख(v3Global.opt.libCreateName(असत्य) + ": " + libCreateDeps + "\n");
            सम्बन्धि.पङ्क्तिं_लिख("\n");
            यदि (v3Global.opt.hierChild()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                सम्बन्धि.पङ्क्तिं_लिख("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(असत्य)
                        + "\n");
            } अन्यथा {
                सम्बन्धि.पङ्क्तिं_लिख(v3Global.opt.libCreateName(सत्य) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                सम्बन्धि.पङ्क्तिं_लिख("ifeq ($(shell uname -s),Darwin)\n");
                सम्बन्धि.पङ्क्तिं_लिख("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                सम्बन्धि.पङ्क्तिं_लिख("else\n");
                सम्बन्धि.पङ्क्तिं_लिख("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                सम्बन्धि.पङ्क्तिं_लिख("endif\n");
                सम्बन्धि.पङ्क्तिं_लिख("\n");
                सम्बन्धि.पङ्क्तिं_लिख("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(असत्य)
                        + " " + v3Global.opt.libCreateName(सत्य) + "\n");
            }
        } अन्यथा {
            ध्रुव string libname = "lib" + v3Global.opt.उपसर्गः() + ".a";
            सम्बन्धि.पङ्क्तिं_लिख("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            सम्बन्धि.पङ्क्तिं_लिख(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            सम्बन्धि.पङ्क्तिं_लिख("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            सम्बन्धि.पङ्क्तिं_लिख("lib" + v3Global.opt.उपसर्गः() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        सम्बन्धि.puts_शीर्षकम्();
    }

    स्पष्ट EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    आभास ~EmitMk() = स्वभाव;
};

//######################################################################

वर्ग EmitMkHierVerilation final {
    ध्रुव V3HierGraph* ध्रुव m_graphp;
    ध्रुव string m_makefile;  // path of this makefile
    शून्य emitCommonOpts(V3_बहिर्गमनम्_Mk_File& सम्बन्धि) ध्रुव {
        सम्बन्धि.पङ्क्तिं_लिख("# Verilation of hierarchical blocks are executed in this directory\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        सम्बन्धि.पङ्क्तिं_लिख("# Common options for hierarchical blocks\n");
        ध्रुव string fullpath_bin = V3Os::filename_Real_मार्गः(v3Global.opt.buildDepBin());
        ध्रुव string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        सम्बन्धि.पङ्क्तिं_लिख("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_HIER_INPUT_FILES := \\\n");
        प्रति (ध्रुव स्वचालित& i : v3Global.opt.vFiles())
            सम्बन्धि.पङ्क्तिं_लिख("  " + V3Os::filename_Real_मार्गः(i.filename()) + " \\\n");
        सम्बन्धि.पङ्क्तिं_लिख("\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_HIER_VERILOG_LIBS := \\\n");
        प्रति (ध्रुव स्वचालित& i : v3Global.opt.libraryFiles()) {
            सम्बन्धि.पङ्क्तिं_लिख("  " + V3Os::filename_Real_मार्गः(i.filename()) + " \\\n");
        }
        सम्बन्धि.पङ्क्तिं_लिख("\n");
    }
    शून्य emitLaunchVerilator(V3_बहिर्गमनम्_Mk_File& सम्बन्धि, ध्रुव string& तर्काः_File) ध्रुव {
        सम्बन्धि.पङ्क्तिं_लिख("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        सम्बन्धि.पङ्क्तिं_लिख("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + तर्काः_File + "\"\n");
    }
    शून्य emit(V3_बहिर्गमनम्_Mk_File& सम्बन्धि) ध्रुव {
        सम्बन्धि.पङ्क्तिं_लिख("# Hierarchical Verilation -*- Makefile -*-\n");
        सम्बन्धि.पङ्क्तिं_लिख("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        सम्बन्धि.पङ्क्तिं_लिख("#\n");
        सम्बन्धि.पङ्क्तिं_लिख("# The main makefile " + v3Global.opt.उपसर्गः() + ".mk calls this makefile\n");
        सम्बन्धि.पङ्क्तिं_लिख("\n");
        सम्बन्धि.पङ्क्तिं_लिख("ifndef VM_HIER_VERILATION_INCLUDED\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        सम्बन्धि.पङ्क्तिं_लिख(".SUFFIXES:\n");
        सम्बन्धि.पङ्क्तिं_लिख(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        सम्बन्धि.पङ्क्तिं_लिख("# Libraries of hierarchical blocks\n");
        सम्बन्धि.पङ्क्तिं_लिख("VM_HIER_LIBS := \\\n");
        प्रति (ध्रुव V3GraphVertex& vtx : m_graphp->vertices()) {
            ध्रुव V3_Hier_खण्डम्* ध्रुव blockp = vtx.as<V3_Hier_खण्डम्>();
            सम्बन्धि.पङ्क्तिं_लिख("  " + blockp->hierLibFilename(सत्य) + " \\\n");
        }
        सम्बन्धि.पङ्क्तिं_लिख("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        सम्बन्धि.पङ्क्तिं_लिख("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.उपसर्गः() + ".mk\n");
        सम्बन्धि.पङ्क्तिं_लिख("\t$(MAKE) -f " + v3Global.opt.उपसर्गः() + ".mk\n");
        सम्बन्धि.पङ्क्तिं_लिख("hier_verilation: " + v3Global.opt.उपसर्गः() + ".mk\n");
        emitCommonOpts(सम्बन्धि);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        सम्बन्धि.पङ्क्तिं_लिख("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        सम्बन्धि.पङ्क्तिं_लिख("hier_launch_verilator:\n");
        सम्बन्धि.पङ्क्तिं_लिख("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ध्रुव string तर्काः_File = v3Global.hierGraphp()->शिखरम्_Command_तर्काः_Filename(असत्य);
            सम्बन्धि.पङ्क्तिं_लिख("\n# Verilate the top module\n");
            सम्बन्धि.पङ्क्तिं_लिख(v3Global.opt.उपसर्गः()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            सम्बन्धि.पङ्क्तिं_लिख(V3Os::filenameNonDir(तर्काः_File) + " ");
            प्रति (ध्रुव V3GraphVertex& vtx : m_graphp->vertices()) {
                ध्रुव V3_Hier_खण्डम्* ध्रुव blockp = vtx.as<V3_Hier_खण्डम्>();
                सम्बन्धि.पङ्क्तिं_लिख(blockp->hierWrapperFilename(सत्य) + " ");
            }
            सम्बन्धि.पङ्क्तिं_लिख("\n");
            emitLaunchVerilator(सम्बन्धि, तर्काः_File);
        }

        // Rules to process hierarchical blocks
        सम्बन्धि.पङ्क्तिं_लिख("\n# Verilate hierarchical blocks\n");
        प्रति (ध्रुव V3GraphVertex& vtx : m_graphp->vertices()) {
            ध्रुव V3_Hier_खण्डम्* ध्रुव blockp = vtx.as<V3_Hier_खण्डम्>();
            ध्रुव string उपसर्गः = blockp->hier_उपसर्गः();
            ध्रुव string तर्काः_Filename = blockp->command_तर्काः_Filename(असत्य);
            सम्बन्धि.पङ्क्तिं_लिख(blockp->hierGeneratedFilenames(सत्य));
            सम्बन्धि.पङ्क्तिं_लिख(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            सम्बन्धि.पङ्क्तिं_लिख(V3Os::filenameNonDir(तर्काः_Filename) + " ");
            प्रति (ध्रुव V3GraphEdge& edge : blockp->बहिर्गमनम्_धाराः()) {
                ध्रुव V3_Hier_खण्डम्* ध्रुव dependencyp = edge.शिखरम्()->as<V3_Hier_खण्डम्>();
                सम्बन्धि.पङ्क्तिं_लिख(dependencyp->hierWrapperFilename(सत्य) + " ");
            }
            सम्बन्धि.पङ्क्तिं_लिख("\n");
            emitLaunchVerilator(सम्बन्धि, तर्काः_Filename);

            // Rule to build lib*.a
            सम्बन्धि.पङ्क्तिं_लिख(blockp->hierLibFilename(सत्य));
            सम्बन्धि.पङ्क्तिं_लिख(": ");
            सम्बन्धि.पङ्क्तिं_लिख(blockp->hierMkFilename(सत्य));
            सम्बन्धि.पङ्क्तिं_लिख(" ");
            प्रति (ध्रुव V3GraphEdge& edge : blockp->बहिर्गमनम्_धाराः()) {
                ध्रुव V3_Hier_खण्डम्* ध्रुव dependencyp = edge.शिखरम्()->as<V3_Hier_खण्डम्>();
                सम्बन्धि.पङ्क्तिं_लिख(dependencyp->hierLibFilename(सत्य));
                सम्बन्धि.पङ्क्तिं_लिख(" ");
            }
            सम्बन्धि.पङ्क्तिं_लिख("\n\t$(MAKE) -f " + blockp->hierMkFilename(असत्य) + " -C " + उपसर्गः);
            सम्बन्धि.पङ्क्तिं_लिख(" VM_PREFIX=" + उपसर्गः);
            सम्बन्धि.पङ्क्तिं_लिख("\n\n");
        }
        सम्बन्धि.पङ्क्तिं_लिख("endif  # Guard\n");
    }

सार्वजनिक:
    स्पष्ट EmitMkHierVerilation(ध्रुव V3HierGraph* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.उपसर्गः() + "_hier.mk"} {
        V3_बहिर्गमनम्_Mk_File सम्बन्धि{m_makefile};
        emit(सम्बन्धि);
    }
};

//######################################################################
// Gate class functions

शून्य V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    ध्रुव EmitMk emitter;
}

शून्य V3EmitMk::emitHierVerilation(ध्रुव V3HierGraph* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
