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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3EmitMk.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3HierBlock.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Groups adjacent files in a list, evenly distributing sum of scores
ವರ್ಗ Emit_ಗುಂಪು final {
ಸಾರ್ವಜನಿಕ:
    ರಚನೆ FileOrConcatenatedFilesList final {
        ಸ್ಥಿರಾಂಕ std::string m_filename;  // Filename or output group filename if grouping
        std::ಸದಿಶ<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        ತಾರ್ಕಿಕ isConcatenatingFile() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_concatenatedFilenames.ಖಾಲಿಗಳು(); }
    };

    ರಚನೆ FilenameWithScore final {
        ಸ್ಥಿರಾಂಕ std::string m_filename;  // Input filename
        ಸ್ಥಿರಾಂಕ uint64_t m_score;  // Input file complexity
        ಸ್ಥಿರಾಂಕ uint64_t m_ಗಾತ್ರ_ಬೈಟುಗಳು;  // Input file size
    };

ಖಾಸಗಿ:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    ರಚನೆ WorkList final {
        uint64_t m_ಒಟ್ಟು_Score = 0;  // Sum of scores of included files
        std::ಸದಿಶ<FilenameWithScore> m_files;  // Included filenames
        ಪೂರ್ಣಾಂಕ m_buckets_ಅಂಕೆ
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        ತಾರ್ಕಿಕ m_isConcatenable = ನಿಜ;  // Indicated whether files on this list can be concatenated.
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_dbgId;  // Work list ID for debugging.

        WorkList() = ಅಳಿಸು;
        ಸ್ಪಷ್ಟ WorkList(ಪೂರ್ಣಾಂಕ id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    ಸ್ಥಿರ constexpr ಗಾತ್ರ_t MIN_FILES_ಎಣಿಕೆ = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    ಸ್ಥಿರ constexpr ಗಾತ್ರ_t MIN_FILES_ಪ್ರತಿ_ಪಾತ್ರೆ = 4;

    // MEMBERS

    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FilenameWithScore>
        m_ಒಳಹರಿವು_Files;  // List of filenames from initial AstCFile list
    std::ಸದಿಶ<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ಸ್ಥಿರಾಂಕ uint64_t m_ಒಟ್ಟು_Score;  // Sum of file scores
    ಸ್ಥಿರಾಂಕ std::string m_ಗುಂಪು_File_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Prefix for output group filenames
    ಸ್ಥಿರಾಂಕ uint64_t m_max_ಗುಂಪು_ಬೈಟುಗಳು;  // Maximum sum of file sizes in an output group
    std::ಸದಿಶ<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::ಸದಿಶ<WorkList*> m_concatenable_Lists_By_Desc_ಗಾತ್ರ;  // Lists sorted by size, descending
    ಪೂರ್ಣಾಂಕ m_ಗುಂಪು_ಅಂಕೆ = 0;  // Number of output groups created

    Emit_ಗುಂಪು(std::ಸದಿಶ<FilenameWithScore> ಒಳಹರಿವು_Files, uint64_t ಒಟ್ಟು_Score,
              ಸ್ಥಿರಾಂಕ std::string& ಗುಂಪು_File_ಪೂರ್ವಪ್ರತ್ಯಯ, uint64_t max_ಗುಂಪು_ಬೈಟುಗಳು)
        : m_ಒಳಹರಿವು_Files{std::move(ಒಳಹರಿವು_Files)}
        , m_ಒಟ್ಟು_Score{ಒಟ್ಟು_Score}
        , m_ಗುಂಪು_File_ಪೂರ್ವಪ್ರತ್ಯಯ{ಗುಂಪು_File_ಪೂರ್ವಪ್ರತ್ಯಯ}
        , m_max_ಗುಂಪು_ಬೈಟುಗಳು{max_ಗುಂಪು_ಬೈಟುಗಳು} {}

    string ಗುಂಪು_Filename(ಪೂರ್ಣಾಂಕ id) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "_" + m_ಗುಂಪು_File_ಪೂರ್ವಪ್ರತ್ಯಯ + std::ಗಮ್ಯ_string(id);
    }

    // Debug logging: prints scores histogram
    ಶೂನ್ಯ dumpLogScoreHistogram(std::ostream& os) {
        constexpr ಪೂರ್ಣಾಂಕ MAX_BAR_ಉದ್ದಳತೆ = 80;
        constexpr ಪೂರ್ಣಾಂಕ MAX_INTERVALS_ಅಂಕೆ = 60;

        // All scores arranged in ascending order.
        std::ಸದಿಶ<uint64_t> sortedScores;
        sortedScores.reserve(m_ಒಳಹರಿವು_Files.ಗಾತ್ರ_2());
        std::transform(m_ಒಳಹರಿವು_Files.begin(), m_ಒಳಹರಿವು_Files.ಕೊನೆ(), std::ಹಿಂಭಾಗ_inserter(sortedScores),
                       [](ಸ್ಥಿರಾಂಕ FilenameWithScore& ಒಳಹರಿವು_File) { ಹಿಂತಿರುಗಿ ಒಳಹರಿವು_File.m_score; });
        std::ವಿಂಗಡಿಸು(sortedScores.begin(), sortedScores.ಕೊನೆ());

        ಸ್ಥಿರಾಂಕ int64_t ತುದಿ_Score = sortedScores.ಹಿಂಭಾಗ();
        os << "Top score: " << ತುದಿ_Score << '\n';
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ max_Score_ಅಗಲ = std::ಗಮ್ಯ_string(ತುದಿ_Score).ಉದ್ದಳತೆ();

        ಸ್ಥಿರಾಂಕ int64_t intervals_ಅಂಕೆ = std::min<int64_t>(ತುದಿ_Score + 1, MAX_INTERVALS_ಅಂಕೆ);

        ರಚನೆ Interval final {
            uint64_t m_ಅಧೋ_Bound = 0;  // Lowest score included in this histogram bucket
            ಪೂರ್ಣಾಂಕ m_ಗಾತ್ರ = 0;  // Number of scores that fell into this bucket
        };

        std::ಸದಿಶ<Interval> intervals;
        intervals.resize(intervals_ಅಂಕೆ);

        intervals[0].m_ಅಧೋ_Bound = 0;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i < intervals_ಅಂಕೆ; i++) {
            intervals[i].m_ಅಧೋ_Bound = (ತುದಿ_Score + 1) * i / intervals_ಅಂಕೆ + 1;
        }

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint64_t score : sortedScores) {
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಪೂರ್ಣಾಂಕ ivIdx = score * intervals_ಅಂಕೆ / (ತುದಿ_Score + 1);
            ++intervals[ivIdx].m_ಗಾತ್ರ;
        }

        ಪೂರ್ಣಾಂಕ ತುದಿ_Interval_ಗಾತ್ರ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Interval& iv : intervals)
            ತುದಿ_Interval_ಗಾತ್ರ = std::max(ತುದಿ_Interval_ಗಾತ್ರ, iv.m_ಗಾತ್ರ);

        os << "Input files' scores histogram:\n";

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Interval& iv : intervals) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ scaled_ಗಾತ್ರ = iv.m_ಗಾತ್ರ * (MAX_BAR_ಉದ್ದಳತೆ + 1) / ತುದಿ_Interval_ಗಾತ್ರ;
            ಸ್ಥಿರಾಂಕ std::string ಸಾಲು = " |" + std::string(scaled_ಗಾತ್ರ, '#');

            os << std::setw(max_Score_ಅಗಲ) << iv.m_ಅಧೋ_Bound << ಸಾಲು << "  " << iv.m_ಗಾತ್ರ << '\n';
        }
        os << std::setw(max_Score_ಅಗಲ) << (ತುದಿ_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    ಶೂನ್ಯ dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.ಗಾತ್ರ_2()
               << "; total score: " << list.m_ಒಟ್ಟು_Score << ")\n";
            ಆದರೆ (debug() >= 6 || list.m_files.ಗಾತ್ರ_2() < 4) {
                // Log all files
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Log only first and last file
                ಸ್ಥಿರಾಂಕ FilenameWithScore& ಮೊದಲ = list.m_files.front();
                ಸ್ಥಿರಾಂಕ FilenameWithScore& ಕೊನೆಯ = list.m_files.ಹಿಂಭಾಗ();
                os << "| + " << ಮೊದಲ.m_filename << "  (score: " << ಮೊದಲ.m_score << ")\n";
                os << "| | (... " << (list.m_files.ಗಾತ್ರ_2() - 2) << " files ...)\n";
                os << "| + " << ಕೊನೆಯ.m_filename << "  (score: " << ಕೊನೆಯ.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    ಶೂನ್ಯ dumpOutputList(std::ostream& os) ಸ್ಥಿರಾಂಕ {
        os << "List of output files after execution of concatenation:\n";

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FileOrConcatenatedFilesList& ನಮೂದು : m_outputFiles) {
            ಆದರೆ (ನಮೂದು.isConcatenatingFile()) {
                os << "+ " << ನಮೂದು.m_filename << "  (concatenating file)\n";
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& f : ನಮೂದು.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                os << "+ " << ನಮೂದು.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    ತಾರ್ಕಿಕ fallbackNoGrouping(ಗಾತ್ರ_t ಒಳಹರಿವು_Files_ಎಣಿಕೆ) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಒಟ್ಟು_Buckets_ಅಂಕೆ = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        ತಾರ್ಕಿಕ groupingRedundant = ಸುಳ್ಳು;
        ಆದರೆ (ಒಳಹರಿವು_Files_ಎಣಿಕೆ < MIN_FILES_ಎಣಿಕೆ
            && ಒಳಹರಿವು_Files_ಎಣಿಕೆ <= static_cast<ಗಾತ್ರ_t>(ಒಟ್ಟು_Buckets_ಅಂಕೆ)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_ಒಳಹರಿವು_Files.ಗಾತ್ರ_2() << " < "
                                                                   << MIN_FILES_ಎಣಿಕೆ << ")");
            groupingRedundant = ನಿಜ;
        }
        ಆದರೆ (ಒಳಹರಿವು_Files_ಎಣಿಕೆ < (MIN_FILES_ಪ್ರತಿ_ಪಾತ್ರೆ * ಒಟ್ಟು_Buckets_ಅಂಕೆ)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_ಒಳಹರಿವು_Files.ಗಾತ್ರ_2() << " < " << MIN_FILES_ಪ್ರತಿ_ಪಾತ್ರೆ << " - "
                         << ಒಟ್ಟು_Buckets_ಅಂಕೆ << ")");
            groupingRedundant = ನಿಜ;
        }
        ಆದರೆ (!groupingRedundant) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        m_outputFiles.reserve(m_ಒಳಹರಿವು_Files.ಗಾತ್ರ_2());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FilenameWithScore& filename : m_ಒಳಹರಿವು_Files) {
            m_outputFiles.ತಳ್ಳು_ಹಿಂಭಾಗ({std::move(filename.m_filename), {}});
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ createWorkLists() {
        // Create initial Work Lists.

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಒಟ್ಟು_Buckets_ಅಂಕೆ = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ಸ್ಥಿರಾಂಕ uint64_t concatenableFileMaxScore = m_ಒಟ್ಟು_Score / ಒಟ್ಟು_Buckets_ಅಂಕೆ / 2;

        V3Stats::ಕೂಡಿಸು_Stat("Concatenation max score", concatenableFileMaxScore);
        ಪೂರ್ಣಾಂಕ ಮುಂದಿನ_Work_List_Id = 0;

        ಆದರೆ (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FilenameWithScore& ಒಳಹರಿವು_File : m_ಒಳಹರಿವು_Files) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ fileIsConcatenable = (ಒಳಹರಿವು_File.m_score <= concatenableFileMaxScore);
            ಆದರೆ (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << ಒಳಹರಿವು_File.m_filename << "  (score: " << ಒಳಹರಿವು_File.m_score << ")\n";
            V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                ಒಳಹರಿವು_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            ಆದರೆ (m_workLists.ಖಾಲಿಗಳು() || m_workLists.ಹಿಂಭಾಗ().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_ಹಿಂಭಾಗ(WorkList{ಮುಂದಿನ_Work_List_Id++});
                m_workLists.ಹಿಂಭಾಗ().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.ಹಿಂಭಾಗ();
            list.m_files.ತಳ್ಳು_ಹಿಂಭಾಗ(
                {ಒಳಹರಿವು_File.m_filename, ಒಳಹರಿವು_File.m_score, ಒಳಹರಿವು_File.m_ಗಾತ್ರ_ಬೈಟುಗಳು});
            list.m_ಒಟ್ಟು_Score += ಒಳಹರಿವು_File.m_score;
        }
        ಆದರೆ (m_logp) *m_logp << '\n';
    }

    ಶೂನ್ಯ ನಿಯೋಜಿಸು_Buckets(uint64_t concatenable_Files_ಒಟ್ಟು_Score) {
        // Assign buckets to lists

        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಒಟ್ಟು_Buckets_ಅಂಕೆ = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        ಆದರೆ (m_concatenable_Lists_By_Desc_ಗಾತ್ರ.ಗಾತ್ರ_2() > ಒಟ್ಟು_Buckets_ಅಂಕೆ) {
            // Debugging: Log which work lists will be kept
            ಆದರೆ (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_ಗಾತ್ರ.begin(),
                              m_concatenable_Lists_By_Desc_ಗಾತ್ರ.begin() + ಒಟ್ಟು_Buckets_ಅಂಕೆ,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.ಗಾತ್ರ_2()
                                          << "; total score: " << listp->m_ಒಟ್ಟು_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_ಗಾತ್ರ.begin() + ಒಟ್ಟು_Buckets_ಅಂಕೆ,
                          m_concatenable_Lists_By_Desc_ಗಾತ್ರ.ಕೊನೆ(), [&](WorkList* listp) {
                              listp->m_isConcatenable = ಸುಳ್ಳು;

                              ಆದರೆ (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.ಗಾತ್ರ_2()
                                          << "; total score: " << listp->m_ಒಟ್ಟು_Score << ")\n";
                          });
            ಆದರೆ (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_ಗಾತ್ರ.resize(ಒಟ್ಟು_Buckets_ಅಂಕೆ);
            // Recalculate stats
            concatenable_Files_ಒಟ್ಟು_Score = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ WorkList* ಸ್ಥಿರಾಂಕ listp : m_concatenable_Lists_By_Desc_ಗಾತ್ರ) {
                concatenable_Files_ಒಟ್ಟು_Score += listp->m_ಒಟ್ಟು_Score;
            }
        }

        ಸ್ಥಿರಾಂಕ uint64_t ideal_ಪಾತ್ರೆ_Score = concatenable_Files_ಒಟ್ಟು_Score / ಒಟ್ಟು_Buckets_ಅಂಕೆ;

        V3Stats::ಕೂಡಿಸು_Stat("Concatenation ideal bucket score", ideal_ಪಾತ್ರೆ_Score);

        ಆದರೆ (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        ಪೂರ್ಣಾಂಕ ಲಭ್ಯ_Buckets = v3Global.opt.outputGroups();
        ಗಾಗಿ (WorkList* listp : m_concatenable_Lists_By_Desc_ಗಾತ್ರ) {
            ಆದರೆ (ಲಭ್ಯ_Buckets > 0 && ideal_ಪಾತ್ರೆ_Score > 0) {
                listp->m_buckets_ಅಂಕೆ = std::min(
                    ಲಭ್ಯ_Buckets, std::max<ಪೂರ್ಣಾಂಕ>(1, listp->m_ಒಟ್ಟು_Score / ideal_ಪಾತ್ರೆ_Score));
                ಲಭ್ಯ_Buckets -= listp->m_buckets_ಅಂಕೆ;
                ಆದರೆ (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_ಅಂಕೆ << "] Work List #"
                            << listp->m_dbgId << '\n';
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = ಸುಳ್ಳು;
                ಆದರೆ (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::ಎಡ << std::setw(4) << listp->m_dbgId
                            << std::ಬಲ << " (excluding from concatenation)\n";
            }
        }
        ಆದರೆ (m_logp) *m_logp << '\n';
    }

    ಶೂನ್ಯ buildOutputList() {
        // Assign files to buckets and build final list of files

        // At this point the workLists contains concatenatable file lists separated by one or more
        // non-concatenable file lists. Each concatenatable list has N buckets (where N > 0)
        // assigned to it, which have to be filled with files from this list. Ideally, sum of file
        // scores in every bucket should be the same.
        ಗಾಗಿ (WorkList& list : m_workLists) {
            ಆದರೆ (!list.m_isConcatenable) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FilenameWithScore& file : list.m_files) {
                    m_outputFiles.ತಳ್ಳು_ಹಿಂಭಾಗ({std::move(file.m_filename), {}});
                }
                ಮುಂದುವರಿ;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ಸ್ಥಿರಾಂಕ uint64_t list_Ideal_ಪಾತ್ರೆ_Score = list.m_ಒಟ್ಟು_Score / list.m_buckets_ಅಂಕೆ;

            std::ಸದಿಶ<std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ FilenameWithScore*>> buckets;
            ಸ್ವಯಂಚಾಲಿತ fileIt = list.m_files.cbegin();
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < list.m_buckets_ಅಂಕೆ; ++i) {
                std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ FilenameWithScore*> ಪಾತ್ರೆ;
                uint64_t ಪಾತ್ರೆ_Score = 0;

                ಗಾಗಿ (; fileIt != list.m_files.cend(); ++fileIt) {
                    ಸ್ಥಿರಾಂಕ uint64_t diffNow
                        = std::ನಿರಪೇಕ್ಷ(static_cast<int64_t>(list_Ideal_ಪಾತ್ರೆ_Score - ಪಾತ್ರೆ_Score));
                    ಸ್ಥಿರಾಂಕ uint64_t diffIfAdded = std::ನಿರಪೇಕ್ಷ(static_cast<int64_t>(
                        list_Ideal_ಪಾತ್ರೆ_Score - ಪಾತ್ರೆ_Score - fileIt->m_score));
                    ಆದರೆ (ಪಾತ್ರೆ_Score == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        ಪಾತ್ರೆ_Score += fileIt->m_score;
                        ಪಾತ್ರೆ.ತಳ್ಳು_ಹಿಂಭಾಗ(&*fileIt);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // Best possible bucket score reached, process next bucket.
                        ನಿಲ್ಲಿಸು;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.ತಳ್ಳು_ಹಿಂಭಾಗ(std::move(ಪಾತ್ರೆ));
            }
            ಗಾಗಿ (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.ಹಿಂಭಾಗ().ತಳ್ಳು_ಹಿಂಭಾಗ(&*fileIt);
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ FilenameWithScore*>& ಪಾತ್ರೆ : buckets) {
                ಕೂಡಿಸು_ಪಾತ್ರೆ_Groups(ಪಾತ್ರೆ);
            }
        }
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಪಾತ್ರೆ_Groups(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ FilenameWithScore*>& ಪಾತ್ರೆ) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::ಸದಿಶ<std::ಸದಿಶ<std::string>> groups;
        uint64_t ಗುಂಪು_ಬೈಟುಗಳು = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FilenameWithScore* ಸ್ಥಿರಾಂಕ filep : ಪಾತ್ರೆ) {
            ಆದರೆ (groups.ಖಾಲಿಗಳು() || ಗುಂಪು_ಬೈಟುಗಳು + filep->m_ಗಾತ್ರ_ಬೈಟುಗಳು > m_max_ಗುಂಪು_ಬೈಟುಗಳು) {
                groups.emplace_ಹಿಂಭಾಗ();
                ಗುಂಪು_ಬೈಟುಗಳು = 0;
            }
            ಗುಂಪು_ಬೈಟುಗಳು += filep->m_ಗಾತ್ರ_ಬೈಟುಗಳು;
            groups.ಹಿಂಭಾಗ().ತಳ್ಳು_ಹಿಂಭಾಗ(filep->m_filename);
        }
        ಗಾಗಿ (std::ಸದಿಶ<std::string>& ಗುಂಪು : groups) {
            ಆದರೆ (ಗುಂಪು.ಗಾತ್ರ_2() == 1) {
                m_outputFiles.ತಳ್ಳು_ಹಿಂಭಾಗ({std::move(ಗುಂಪು.front()), {}});
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_outputFiles.ತಳ್ಳು_ಹಿಂಭಾಗ({ಗುಂಪು_Filename(m_ಗುಂಪು_ಅಂಕೆ++), std::move(ಗುಂಪು)});
            }
        }
    }

    ಶೂನ್ಯ assertFilesSame() ಸ್ಥಿರಾಂಕ {
        ಸ್ವಯಂಚಾಲಿತ ifIt = m_ಒಳಹರಿವು_Files.begin();
        ಸ್ವಯಂಚಾಲಿತ ದ_It = m_outputFiles.begin();
        ಆಗ (ifIt != m_ಒಳಹರಿವು_Files.ಕೊನೆ() && ದ_It != m_outputFiles.ಕೊನೆ()) {
            ಆದರೆ (ದ_It->isConcatenatingFile()) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& ocf : ದ_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_ಒಳಹರಿವು_Files.ಕೊನೆ(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++ದ_It;
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT(ifIt->m_filename == ದ_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << ದ_It->m_filename);
                ++ifIt;
                ++ದ_It;
            }
        }
        UASSERT(ifIt == m_ಒಳಹರಿವು_Files.ಕೊನೆ(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(ದ_It == m_outputFiles.ಕೊನೆ(),
                "More output files than input files. First extra file: " << ದ_It->m_filename);
    }

    ಶೂನ್ಯ process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_ಗುಂಪು_File_ಪೂರ್ವಪ್ರತ್ಯಯ);
        UINFO(5, "Number of input files: " << m_ಒಳಹರಿವು_Files.ಗಾತ್ರ_2());
        UINFO(5, "Total score: " << m_ಒಟ್ಟು_Score);

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಒಟ್ಟು_Buckets_ಅಂಕೆ = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << ಒಟ್ಟು_Buckets_ಅಂಕೆ);
        UASSERT(ಒಟ್ಟು_Buckets_ಅಂಕೆ > 0, "More than 0 buckets required");

        ಆದರೆ (fallbackNoGrouping(m_ಒಳಹರಿವು_Files.ಗಾತ್ರ_2())) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (debug() >= 6 || dumpLevel() >= 6) {
            ಸ್ಥಿರಾಂಕ string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            ಆದರೆ (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        ಆದರೆ (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        ಗಾತ್ರ_t concatenable_Files_ಎಣಿಕೆ = 0;
        int64_t concatenable_Files_ಒಟ್ಟು_Score = 0;

        ಗಾಗಿ (WorkList& list : m_workLists) {
            ಆದರೆ (!list.m_isConcatenable) ಮುಂದುವರಿ;

            // "Concatenation" of a single file is pointless
            ಆದರೆ (list.m_files.ಗಾತ್ರ_2() == 1) {
                list.m_isConcatenable = ಸುಳ್ಳು;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                ಮುಂದುವರಿ;
            }

            concatenable_Files_ಎಣಿಕೆ += list.m_files.ಗಾತ್ರ_2();
            concatenable_Files_ಒಟ್ಟು_Score += list.m_ಒಟ್ಟು_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_ಗಾತ್ರ.ತಳ್ಳು_ಹಿಂಭಾಗ(&list);
        }

        ಆದರೆ (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        ಆದರೆ (fallbackNoGrouping(concatenable_Files_ಎಣಿಕೆ)) ಹಿಂತಿರುಗಿ;

        std::ವಿಂಗಡಿಸು(m_concatenable_Lists_By_Desc_ಗಾತ್ರ.begin(), m_concatenable_Lists_By_Desc_ಗಾತ್ರ.ಕೊನೆ(),
                  [](ಸ್ಥಿರಾಂಕ WorkList* ap, ಸ್ಥಿರಾಂಕ WorkList* bp) {
                      // Sort in descending order by number of files
                      ಆದರೆ (ap->m_files.ಗಾತ್ರ_2() != bp->m_files.ಗಾತ್ರ_2())
                          ಹಿಂತಿರುಗಿ (ap->m_files.ಗಾತ್ರ_2() > bp->m_files.ಗಾತ್ರ_2());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      ಹಿಂತಿರುಗಿ bp->m_ಒಟ್ಟು_Score > ap->m_ಒಟ್ಟು_Score;
                  });

        ನಿಯೋಜಿಸು_Buckets(concatenable_Files_ಒಟ್ಟು_Score);

        buildOutputList();

        ಆದರೆ (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::ಸದಿಶ<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::ಸದಿಶ<FilenameWithScore> ಒಳಹರಿವು_Files, uint64_t ಒಟ್ಟು_Score,
                                ಸ್ಥಿರಾಂಕ std::string& ಗುಂಪು_File_ಪೂರ್ವಪ್ರತ್ಯಯ, uint64_t max_ಗುಂಪು_ಬೈಟುಗಳು) {
        Emit_ಗುಂಪು ಗುಂಪು{std::move(ಒಳಹರಿವು_Files), ಒಟ್ಟು_Score, ಗುಂಪು_File_ಪೂರ್ವಪ್ರತ್ಯಯ, max_ಗುಂಪು_ಬೈಟುಗಳು};
        ಗುಂಪು.process();
        ಹಿಂತಿರುಗಿ ಗುಂಪು.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

ವರ್ಗ EmitMk final {
    ಬಳಸಿ FileOrConcatenatedFilesList = Emit_ಗುಂಪು::FileOrConcatenatedFilesList;
    ಬಳಸಿ FilenameWithScore = Emit_ಗುಂಪು::FilenameWithScore;

    // MEMBERS
    ದ್ವಿಗುಣ m_put_Class_ಎಣಿಕೆ = 0;  // Number of classEntries printed

ಸಾರ್ವಜನಿಕ:
    // METHODS

    ಸ್ಥಿರ ಶೂನ್ಯ emitConcatenatingFile(ಸ್ಥಿರಾಂಕ FileOrConcatenatedFilesList& ನಮೂದು) {
        UASSERT(ನಮೂದು.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_ಹೊರಹೋಗು_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + ನಮೂದು.m_filename + ".cpp"};
        concatenatingFile.puts_ಶೀರ್ಷಿಕೆ();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& file : ನಮೂದು.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    ಸ್ಥಿರ uint64_t file_ಗಾತ್ರ(ಸ್ಥಿರಾಂಕ AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::ದ್ವಿಮಾನ | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        ಹಿಂತಿರುಗಿ static_cast<uint64_t>(ifs.tellg());
    }

    ಶೂನ್ಯ put_Make_Class_ನಮೂದು(V3_ಹೊರಹೋಗು_Mk_File& ದ, ಸ್ಥಿರಾಂಕ string& name) {
        ದ.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_ಎಣಿಕೆ;
    }

    ಶೂನ್ಯ emitClassMake() {
        std::ಸದಿಶ<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::ಸದಿಶ<FileOrConcatenatedFilesList> vmClassesFastList;
        ಆದರೆ (v3Global.opt.outputGroups() > 0) {
            std::ಸದಿಶ<FilenameWithScore> slowFiles;
            std::ಸದಿಶ<FilenameWithScore> fastFiles;
            uint64_t slow_ಒಟ್ಟು_Score = 0;
            uint64_t fast_ಒಟ್ಟು_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t ಶೀರ್ಷಿಕೆ_ಬೈಟುಗಳು = 0;

            ಗಾಗಿ (Ast_ಗಂಟು_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), ಗಂಟು_File)) {
                ಸ್ಥಿರಾಂಕ AstCFile* ಸ್ಥಿರಾಂಕ cfilep = VN_CAST(nodep, CFile);
                ಆದರೆ (cfilep && VString::ತುದಿಗಳು_With(cfilep->name(), ".h")) {
                    ಶೀರ್ಷಿಕೆ_ಬೈಟುಗಳು += file_ಗಾತ್ರ(cfilep);
                }
                ಆದರೆ (cfilep && cfilep->ಮೂಲ() && cfilep->support() == ಸುಳ್ಳು) {
                    std::ಸದಿಶ<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& ಒಟ್ಟು_Score = cfilep->slow() ? slow_ಒಟ್ಟು_Score : fast_ಒಟ್ಟು_Score;

                    ಒಟ್ಟು_Score += cfilep->complexityScore();
                    files.ತಳ್ಳು_ಹಿಂಭಾಗ({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_ಗಾತ್ರ(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_ಗುಂಪು_ಬೈಟುಗಳು = std::numeric_limits<uint64_t>::max();
            ಆದರೆ (v3Global.opt.output_Groups_ಗಾತ್ರ()) {
                ಸ್ಥಿರಾಂಕ uint64_t max_Tu_ಬೈಟುಗಳು = static_cast<uint64_t>(v3Global.opt.output_Groups_ಗಾತ್ರ());
                max_ಗುಂಪು_ಬೈಟುಗಳು = max_Tu_ಬೈಟುಗಳು > ಶೀರ್ಷಿಕೆ_ಬೈಟುಗಳು ? max_Tu_ಬೈಟುಗಳು - ಶೀರ್ಷಿಕೆ_ಬೈಟುಗಳು : 0;
            }

            vmClassesSlowList = Emit_ಗುಂಪು::singleConcatenatedFilesList(
                std::move(slowFiles), slow_ಒಟ್ಟು_Score, "vm_classes_Slow_", max_ಗುಂಪು_ಬೈಟುಗಳು);
            vmClassesFastList = Emit_ಗುಂಪು::singleConcatenatedFilesList(
                std::move(fastFiles), fast_ಒಟ್ಟು_Score, "vm_classes_", max_ಗುಂಪು_ಬೈಟುಗಳು);
        }

        // Generate the makefile
        V3_ಹೊರಹೋಗು_Mk_File ದ{v3Global.opt.makeDir() + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "_classes.mk"};
        ದ.puts_ಶೀರ್ಷಿಕೆ();
        ದ.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        ದ.puts("#\n");
        ದ.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        ದ.puts("# See " + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".mk" + " for the caller.\n");

        ದ.puts("\n### Switches...\n");
        ದ.puts("# C11 constructs required?  0/1 (always on now)\n");
        ದ.put_ಗಣ("VM_C11", "1");
        ದ.puts("# Timing enabled?  0/1\n");
        ದ.put_ಗಣ("VM_TIMING", v3Global.ಬಳಸುತ್ತದೆ_Timing() ? "1" : "0");
        ದ.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        ದ.put_ಗಣ("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        ದ.puts("# Parallel builds?  0/1 (from --output-split)\n");
        ದ.put_ಗಣ("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        ದ.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        ದ.put_ಗಣ("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        ದ.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        ದ.put_ಗಣ("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        ದ.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        ದ.put_ಗಣ("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        ದ.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        ದ.put_ಗಣ("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        ದ.puts("# VPI enabled?  0/1 (from --vpi)\n");
        ದ.put_ಗಣ("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        ದ.puts("\n### Object file lists...\n");
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ support = 0; support < 3; ++support) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ& slow : {ಸುಳ್ಳು, ನಿಜ}) {
                ಆದರೆ (support == 2) {
                    ದ.puts("# Global classes, need linked once per executable");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (support) {
                    ದ.puts("# Generated support classes");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ದ.puts("# Generated module classes");
                }
                ಆದರೆ (slow) {
                    ದ.puts(", non-fast-path, compile with low/medium optimization\n");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ದ.puts(", fast-path, compile with highest optimization\n");
                }
                ಸ್ಥಿರಾಂಕ string ಗುರಿ_Var = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_ಎಣಿಕೆ = 0;
                ದ.puts(ಗುರಿ_Var + " += \\\n");
                ಆದರೆ (support == 2 && v3Global.opt.hier_ಮಗು()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (support == 2 && !slow) {
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_ನಮೂದು(ದ, cpp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (support == 2 && slow) {
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FileOrConcatenatedFilesList& ನಮೂದು : list) {
                        ಆದರೆ (ನಮೂದು.isConcatenatingFile()) emitConcatenatingFile(ನಮೂದು);
                        put_Make_Class_ನಮೂದು(ದ, ನಮೂದು.m_filename);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಗಾಗಿ (Ast_ಗಂಟು_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), ಗಂಟು_File)) {
                        ಸ್ಥಿರಾಂಕ AstCFile* ಸ್ಥಿರಾಂಕ cfilep = VN_CAST(nodep, CFile);
                        ಆದರೆ (cfilep && cfilep->ಮೂಲ() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_ನಮೂದು(ದ, cfilep->name());
                        }
                    }
                }
                ದ.puts("\n");
                V3Stats::ಕೂಡಿಸು_Stat("Makefile targets, " + ಗುರಿ_Var, m_put_Class_ಎಣಿಕೆ);
            }
        }

        ದ.puts_ಶೀರ್ಷಿಕೆ();
    }

    ಶೂನ್ಯ emitOverallMake() {
        // Generate the makefile
        V3_ಹೊರಹೋಗು_Mk_File ದ{v3Global.opt.makeDir() + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".mk"};
        ದ.puts_ಶೀರ್ಷಿಕೆ();
        ದ.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        ದ.puts("#\n");
        ದ.puts("# Execute this makefile from the object directory:\n");
        ದ.puts("#    make -f " + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".mk" + "\n");
        ದ.puts("\n");

        ಆದರೆ (v3Global.opt.exe()) {
            ದ.puts("default: " + v3Global.opt.exeName() + "\n");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            ದ.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ದ.puts("default: lib" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "\n");
        }
        ದ.puts("\n### Constants...\n");
        ದ.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        ದ.put_ಗಣ("PERL", V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(V3Options::getenvPERL()));
        ದ.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        ದ.put_ಗಣ("PYTHON3", V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        ದ.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        ದ.put_ಗಣ("VERILATOR_ROOT",
                  V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_ಬೇರು()));
        ದ.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        ದ.put_ಗಣ_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        ದ.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        ದ.put_ಗಣ_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        ಆದರೆ (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        ದ.puts("\n### Switches...\n");
        ದ.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        ದ.put_ಗಣ("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        ದ.puts("# SystemC output mode?  0/1 (from --sc)\n");
        ದ.put_ಗಣ("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        ದ.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        ದ.put_ಗಣ("VM_SP_OR_SC", "$(VM_SC)");
        ದ.puts("# Deprecated\n");
        ದ.put_ಗಣ("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        ದ.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        ದ.put_ಗಣ("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        ದ.puts("\n### Vars...\n");
        ದ.puts("# Module prefix (from --prefix)\n");
        ದ.put_ಗಣ("VM_MODPREFIX", v3Global.opt.mod_ಪೂರ್ವಪ್ರತ್ಯಯ());
        ದ.puts("# Design prefix (from --prefix)\n");
        ದ.put_ಗಣ("VM_PREFIX", v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ());
        ದ.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        ದ.put_ಗಣ("VM_PREFIX_INCLUDE", "<" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".h>");
        ಆದರೆ (v3Global.dpi()) {
            ದ.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            ದ.put_ಗಣ("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "__Dpi.h>");
        }

        ದ.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        ದ.puts("VM_USER_CFLAGS = \\\n");
        ಸ್ಥಿರಾಂಕ std::string solver = V3Options::getenvVERILATOR_SOLVER();
        ಆದರೆ (v3Global.useRandomizeMethods() && solver != "")
            ದ.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) ದ.puts("\t-fPIC \\\n");
        ಸ್ಥಿರಾಂಕ VStringList& cFlags = v3Global.opt.cFlags();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : cFlags) ದ.puts("  " + i + " \\\n");
        ದ.puts("\n");

        ದ.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        ದ.puts("VM_USER_LDLIBS = \\\n");
        ಸ್ಥಿರಾಂಕ VStringList& ldLibs = v3Global.opt.ldLibs();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : ldLibs) ದ.puts("  " + i + " \\\n");
        ದ.puts("\n");

        ದ.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        ದ.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        ದ.puts("VM_USER_LDLIBS_FILES = \\\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : v3Global.opt.ldLibFiles()) ದ.puts("  " + i + " \\\n");
        ದ.puts("\n");

        VString_ಗಣ dirs;
        ದ.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        ದ.puts("VM_USER_CLASSES = \\\n");
        ಸ್ಥಿರಾಂಕ VString_ಗಣ& cppFiles = v3Global.opt.cppFiles();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& cppfile : cppFiles) {
            ದ.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ಸ್ಥಿರಾಂಕ string dir
                = V3Os::filename_Relative_ಮಾರ್ಗ(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.ಸೇರಿಸು(dir);
        }
        dirs.ಸೇರಿಸು(V3Os::filename_Relative_ಮಾರ್ಗ(".", v3Global.opt.makeDir()));

        ದ.puts("\n");

        ದ.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        ದ.puts("VM_USER_DIR = \\\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : dirs) ದ.puts("  " + i + " \\\n");
        ದ.puts("\n");

        ದ.puts("### Default rules...\n");
        ದ.puts("# Include list of all generated classes\n");
        ದ.puts("include " + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "_classes.mk\n");
        ಆದರೆ (v3Global.opt.hier_ತುದಿ()) {
            ದ.puts("# Include rules for hierarchical blocks\n");
            ದ.puts("include " + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "_hier.mk\n");
        }
        ದ.puts("# Include global rules\n");
        ದ.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        ಆದರೆ (v3Global.opt.exe()) {
            ದ.puts("\n### Executable rules... (from --exe)\n");
            ದ.puts("VPATH += $(VM_USER_DIR)\n");
            ದ.puts("\n");

            ಆದರೆ (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                ದ.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                ದ.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                ದ.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                ದ.puts("\n");
            }
        }

        ಸ್ಥಿರಾಂಕ string compilerIncludePch
            = v3Global.opt.compilerIncludes().ಖಾಲಿಗಳು() ? "" : "$(VK_PCH_H).fast.gch";
        ಸ್ಥಿರಾಂಕ string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().ಖಾಲಿಗಳು() ? "" : "$(VK_PCH_I_FAST)";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& cppfile : cppFiles) {
            ಸ್ಥಿರಾಂಕ string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            ದ.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            ದ.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        ಆದರೆ (v3Global.opt.exe()) {
            ದ.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            ದ.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            ದ.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            ದ.puts("\n");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            ದ.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            ದ.puts(v3Global.opt.libCreateName(ಸುಳ್ಳು) + ": " + libCreateDeps + "\n");
            ದ.puts("\n");
            ಆದರೆ (v3Global.opt.hier_ಮಗು()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                ದ.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(ಸುಳ್ಳು)
                        + "\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ದ.puts(v3Global.opt.libCreateName(ನಿಜ) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                ದ.puts("ifeq ($(shell uname -s),Darwin)\n");
                ದ.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                ದ.puts("else\n");
                ದ.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                ದ.puts("endif\n");
                ದ.puts("\n");
                ದ.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(ಸುಳ್ಳು)
                        + " " + v3Global.opt.libCreateName(ನಿಜ) + "\n");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ string libname = "lib" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".a";
            ದ.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            ದ.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            ದ.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            ದ.puts("lib" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        ದ.puts_ಶೀರ್ಷಿಕೆ();
    }

    ಸ್ಪಷ್ಟ EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    ವಾಸ್ತವಿಕ ~EmitMk() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################

ವರ್ಗ EmitMkHierVerilation final {
    ಸ್ಥಿರಾಂಕ V3_Hier_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_graphp;
    ಸ್ಥಿರಾಂಕ string m_makefile;  // path of this makefile
    ಶೂನ್ಯ emitCommonOpts(V3_ಹೊರಹೋಗು_Mk_File& ದ) ಸ್ಥಿರಾಂಕ {
        ದ.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        ದ.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        ದ.puts("# Common options for hierarchical blocks\n");
        ಸ್ಥಿರಾಂಕ string fullpath_bin = V3Os::filename_Real_ಮಾರ್ಗ(v3Global.opt.buildDepBin());
        ಸ್ಥಿರಾಂಕ string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        ದ.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        ದ.puts("VM_HIER_INPUT_FILES := \\\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : v3Global.opt.vFiles())
            ದ.puts("  " + V3Os::filename_Real_ಮಾರ್ಗ(i.filename()) + " \\\n");
        ದ.puts("\n");
        ದ.puts("VM_HIER_VERILOG_LIBS := \\\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : v3Global.opt.libraryFiles()) {
            ದ.puts("  " + V3Os::filename_Real_ಮಾರ್ಗ(i.filename()) + " \\\n");
        }
        ದ.puts("\n");
    }
    ಶೂನ್ಯ emitLaunchVerilator(V3_ಹೊರಹೋಗು_Mk_File& ದ, ಸ್ಥಿರಾಂಕ string& ವಾದಗಳು_File) ಸ್ಥಿರಾಂಕ {
        ದ.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        ದ.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + ವಾದಗಳು_File + "\"\n");
    }
    ಶೂನ್ಯ emit(V3_ಹೊರಹೋಗು_Mk_File& ದ) ಸ್ಥಿರಾಂಕ {
        ದ.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        ದ.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        ದ.puts("#\n");
        ದ.puts("# The main makefile " + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".mk calls this makefile\n");
        ದ.puts("\n");
        ದ.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        ದ.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        ದ.puts(".SUFFIXES:\n");
        ದ.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        ದ.puts("# Libraries of hierarchical blocks\n");
        ದ.puts("VM_HIER_LIBS := \\\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_graphp->vertices()) {
            ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = vtx.as<V3_Hier_ಖಂಡ>();
            ದ.puts("  " + blockp->hierLibFilename(ನಿಜ) + " \\\n");
        }
        ದ.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        ದ.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".mk\n");
        ದ.puts("\t$(MAKE) -f " + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".mk\n");
        ದ.puts("hier_verilation: " + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + ".mk\n");
        emitCommonOpts(ದ);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        ದ.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        ದ.puts("hier_launch_verilator:\n");
        ದ.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ಸ್ಥಿರಾಂಕ string ವಾದಗಳು_File = v3Global.hierGraphp()->ತುದಿ_Command_ವಾದಗಳು_Filename(ಸುಳ್ಳು);
            ದ.puts("\n# Verilate the top module\n");
            ದ.puts(v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            ದ.puts(V3Os::filenameNonDir(ವಾದಗಳು_File) + " ");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_graphp->vertices()) {
                ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = vtx.as<V3_Hier_ಖಂಡ>();
                ದ.puts(blockp->hierWrapperFilename(ನಿಜ) + " ");
            }
            ದ.puts("\n");
            emitLaunchVerilator(ದ, ವಾದಗಳು_File);
        }

        // Rules to process hierarchical blocks
        ದ.puts("\n# Verilate hierarchical blocks\n");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_graphp->vertices()) {
            ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = vtx.as<V3_Hier_ಖಂಡ>();
            ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ = blockp->hier_ಪೂರ್ವಪ್ರತ್ಯಯ();
            ಸ್ಥಿರಾಂಕ string ವಾದಗಳು_Filename = blockp->command_ವಾದಗಳು_Filename(ಸುಳ್ಳು);
            ದ.puts(blockp->hierGeneratedFilenames(ನಿಜ));
            ದ.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            ದ.puts(V3Os::filenameNonDir(ವಾದಗಳು_Filename) + " ");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : blockp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ dependencyp = ಅಂಚು.ತುದಿ()->as<V3_Hier_ಖಂಡ>();
                ದ.puts(dependencyp->hierWrapperFilename(ನಿಜ) + " ");
            }
            ದ.puts("\n");
            emitLaunchVerilator(ದ, ವಾದಗಳು_Filename);

            // Rule to build lib*.a
            ದ.puts(blockp->hierLibFilename(ನಿಜ));
            ದ.puts(": ");
            ದ.puts(blockp->hierMkFilename(ನಿಜ));
            ದ.puts(" ");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : blockp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ* ಸ್ಥಿರಾಂಕ dependencyp = ಅಂಚು.ತುದಿ()->as<V3_Hier_ಖಂಡ>();
                ದ.puts(dependencyp->hierLibFilename(ನಿಜ));
                ದ.puts(" ");
            }
            ದ.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(ಸುಳ್ಳು) + " -C " + ಪೂರ್ವಪ್ರತ್ಯಯ);
            ದ.puts(" VM_PREFIX=" + ಪೂರ್ವಪ್ರತ್ಯಯ);
            ದ.puts("\n\n");
        }
        ದ.puts("endif  # Guard\n");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ EmitMkHierVerilation(ಸ್ಥಿರಾಂಕ V3_Hier_ನಕ್ಷೆ* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "_hier.mk"} {
        V3_ಹೊರಹೋಗು_Mk_File ದ{m_makefile};
        emit(ದ);
    }
};

//######################################################################
// Gate class functions

ಶೂನ್ಯ V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    ಸ್ಥಿರಾಂಕ EmitMk emitter;
}

ಶೂನ್ಯ V3EmitMk::emitHierVerilation(ಸ್ಥಿರಾಂಕ V3_Hier_ನಕ್ಷೆ* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
