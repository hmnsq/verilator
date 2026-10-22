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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitMk.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HierBlock.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Groups adjacent files in a list, evenly distributing sum of scores
ਵਰਗ Emit_ਸਮੂਹ final {
ਜਨਤਕ:
    ਬਣਤਰ FileOrConcatenatedFilesList final {
        ਅਚਲ std::string m_filename;  // Filename or output group filename if grouping
        std::ਸਦਿਸ਼<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        ਤਾਰਕਿਕ isConcatenatingFile() ਅਚਲ { ਵਾਪਸ !m_concatenatedFilenames.ਖਾਲੀ_ਥਾਵਾਂ(); }
    };

    ਬਣਤਰ FilenameWithScore final {
        ਅਚਲ std::string m_filename;  // Input filename
        ਅਚਲ uint64_t m_score;  // Input file complexity
        ਅਚਲ uint64_t m_ਮਾਪ_ਬਾਈਟਾਂ;  // Input file size
    };

ਨਿਜੀ:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    ਬਣਤਰ WorkList final {
        uint64_t m_ਕੁੱਲ_Score = 0;  // Sum of scores of included files
        std::ਸਦਿਸ਼<FilenameWithScore> m_files;  // Included filenames
        ਪੂਰਨ m_buckets_ਅੰਕ
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        ਤਾਰਕਿਕ m_isConcatenable = ਸੱਚ;  // Indicated whether files on this list can be concatenated.
        ਅਚਲ ਪੂਰਨ m_dbgId;  // Work list ID for debugging.

        WorkList() = ਮਿਟਾਓ;
        ਸਪਸ਼ਟ WorkList(ਪੂਰਨ id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    ਸਥਿਰ constexpr ਮਾਪ_t MIN_FILES_ਗਿਣਤੀ = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    ਸਥਿਰ constexpr ਮਾਪ_t MIN_FILES_ਪ੍ਰਤੀ_ਬਾਲਟੀ = 4;

    // MEMBERS

    ਅਚਲ std::ਸਦਿਸ਼<FilenameWithScore>
        m_ਨਿਵੇਸ਼_Files;  // List of filenames from initial AstCFile list
    std::ਸਦਿਸ਼<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    ਅਚਲ uint64_t m_ਕੁੱਲ_Score;  // Sum of file scores
    ਅਚਲ std::string m_ਸਮੂਹ_File_ਅਗੇਤਰ;  // Prefix for output group filenames
    ਅਚਲ uint64_t m_max_ਸਮੂਹ_ਬਾਈਟਾਂ;  // Maximum sum of file sizes in an output group
    std::ਸਦਿਸ਼<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::ਸਦਿਸ਼<WorkList*> m_concatenable_Lists_By_Desc_ਮਾਪ;  // Lists sorted by size, descending
    ਪੂਰਨ m_ਸਮੂਹ_ਅੰਕ = 0;  // Number of output groups created

    Emit_ਸਮੂਹ(std::ਸਦਿਸ਼<FilenameWithScore> ਨਿਵੇਸ਼_Files, uint64_t ਕੁੱਲ_Score,
              ਅਚਲ std::string& ਸਮੂਹ_File_ਅਗੇਤਰ, uint64_t max_ਸਮੂਹ_ਬਾਈਟਾਂ)
        : m_ਨਿਵੇਸ਼_Files{std::move(ਨਿਵੇਸ਼_Files)}
        , m_ਕੁੱਲ_Score{ਕੁੱਲ_Score}
        , m_ਸਮੂਹ_File_ਅਗੇਤਰ{ਸਮੂਹ_File_ਅਗੇਤਰ}
        , m_max_ਸਮੂਹ_ਬਾਈਟਾਂ{max_ਸਮੂਹ_ਬਾਈਟਾਂ} {}

    string ਸਮੂਹ_Filename(ਪੂਰਨ id) ਅਚਲ {
        ਵਾਪਸ v3Global.opt.ਅਗੇਤਰ() + "_" + m_ਸਮੂਹ_File_ਅਗੇਤਰ + std::ਮੰਜ਼ਿਲ_string(id);
    }

    // Debug logging: prints scores histogram
    ਖਾਲੀ dumpLogScoreHistogram(std::ostream& os) {
        constexpr ਪੂਰਨ MAX_BAR_ਲੰਮਾਈ = 80;
        constexpr ਪੂਰਨ MAX_INTERVALS_ਅੰਕ = 60;

        // All scores arranged in ascending order.
        std::ਸਦਿਸ਼<uint64_t> sortedScores;
        sortedScores.reserve(m_ਨਿਵੇਸ਼_Files.ਮਾਪ());
        std::transform(m_ਨਿਵੇਸ਼_Files.begin(), m_ਨਿਵੇਸ਼_Files.ਸਿਰਾ(), std::ਪਿੱਠ_inserter(sortedScores),
                       [](ਅਚਲ FilenameWithScore& ਨਿਵੇਸ਼_File) { ਵਾਪਸ ਨਿਵੇਸ਼_File.m_score; });
        std::ਛਾਂਟੋ(sortedScores.begin(), sortedScores.ਸਿਰਾ());

        ਅਚਲ int64_t ਸਿਖਰ_Score = sortedScores.ਪਿੱਠ();
        os << "Top score: " << ਸਿਖਰ_Score << '\n';
        ਅਚਲ ਪੂਰਨ max_Score_ਚੌੜਾਈ = std::ਮੰਜ਼ਿਲ_string(ਸਿਖਰ_Score).ਲੰਮਾਈ();

        ਅਚਲ int64_t intervals_ਅੰਕ = std::min<int64_t>(ਸਿਖਰ_Score + 1, MAX_INTERVALS_ਅੰਕ);

        ਬਣਤਰ Interval final {
            uint64_t m_ਹੇਠਲਾ_Bound = 0;  // Lowest score included in this histogram bucket
            ਪੂਰਨ m_ਮਾਪ = 0;  // Number of scores that fell into this bucket
        };

        std::ਸਦਿਸ਼<Interval> intervals;
        intervals.resize(intervals_ਅੰਕ);

        intervals[0].m_ਹੇਠਲਾ_Bound = 0;
        ਲਈ (ਪੂਰਨ i = 1; i < intervals_ਅੰਕ; i++) {
            intervals[i].m_ਹੇਠਲਾ_Bound = (ਸਿਖਰ_Score + 1) * i / intervals_ਅੰਕ + 1;
        }

        ਲਈ (ਅਚਲ uint64_t score : sortedScores) {
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਪੂਰਨ ivIdx = score * intervals_ਅੰਕ / (ਸਿਖਰ_Score + 1);
            ++intervals[ivIdx].m_ਮਾਪ;
        }

        ਪੂਰਨ ਸਿਖਰ_Interval_ਮਾਪ = 0;
        ਲਈ (ਅਚਲ Interval& iv : intervals)
            ਸਿਖਰ_Interval_ਮਾਪ = std::max(ਸਿਖਰ_Interval_ਮਾਪ, iv.m_ਮਾਪ);

        os << "Input files' scores histogram:\n";

        ਲਈ (ਅਚਲ Interval& iv : intervals) {
            ਅਚਲ ਪੂਰਨ scaled_ਮਾਪ = iv.m_ਮਾਪ * (MAX_BAR_ਲੰਮਾਈ + 1) / ਸਿਖਰ_Interval_ਮਾਪ;
            ਅਚਲ std::string ਸਤਰ = " |" + std::string(scaled_ਮਾਪ, '#');

            os << std::setw(max_Score_ਚੌੜਾਈ) << iv.m_ਹੇਠਲਾ_Bound << ਸਤਰ << "  " << iv.m_ਮਾਪ << '\n';
        }
        os << std::setw(max_Score_ਚੌੜਾਈ) << (ਸਿਖਰ_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    ਖਾਲੀ dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        ਲਈ (ਅਚਲ WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.ਮਾਪ()
               << "; total score: " << list.m_ਕੁੱਲ_Score << ")\n";
            ਜੇ (debug() >= 6 || list.m_files.ਮਾਪ() < 4) {
                // Log all files
                ਲਈ (ਅਚਲ FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } ਨਹੀਂਤਾਂ {
                // Log only first and last file
                ਅਚਲ FilenameWithScore& ਪਹਿਲਾ = list.m_files.front();
                ਅਚਲ FilenameWithScore& ਆਖਰੀ = list.m_files.ਪਿੱਠ();
                os << "| + " << ਪਹਿਲਾ.m_filename << "  (score: " << ਪਹਿਲਾ.m_score << ")\n";
                os << "| | (... " << (list.m_files.ਮਾਪ() - 2) << " files ...)\n";
                os << "| + " << ਆਖਰੀ.m_filename << "  (score: " << ਆਖਰੀ.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    ਖਾਲੀ dumpOutputList(std::ostream& os) ਅਚਲ {
        os << "List of output files after execution of concatenation:\n";

        ਲਈ (ਅਚਲ FileOrConcatenatedFilesList& ਇੰਦਰਾਜ : m_outputFiles) {
            ਜੇ (ਇੰਦਰਾਜ.isConcatenatingFile()) {
                os << "+ " << ਇੰਦਰਾਜ.m_filename << "  (concatenating file)\n";
                ਲਈ (ਅਚਲ string& f : ਇੰਦਰਾਜ.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } ਨਹੀਂਤਾਂ {
                os << "+ " << ਇੰਦਰਾਜ.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    ਤਾਰਕਿਕ fallbackNoGrouping(ਮਾਪ_t ਨਿਵੇਸ਼_Files_ਗਿਣਤੀ) {
        ਅਚਲ ਪੂਰਨ ਕੁੱਲ_Buckets_ਅੰਕ = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        ਤਾਰਕਿਕ groupingRedundant = ਝੂਠ;
        ਜੇ (ਨਿਵੇਸ਼_Files_ਗਿਣਤੀ < MIN_FILES_ਗਿਣਤੀ
            && ਨਿਵੇਸ਼_Files_ਗਿਣਤੀ <= static_cast<ਮਾਪ_t>(ਕੁੱਲ_Buckets_ਅੰਕ)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_ਨਿਵੇਸ਼_Files.ਮਾਪ() << " < "
                                                                   << MIN_FILES_ਗਿਣਤੀ << ")");
            groupingRedundant = ਸੱਚ;
        }
        ਜੇ (ਨਿਵੇਸ਼_Files_ਗਿਣਤੀ < (MIN_FILES_ਪ੍ਰਤੀ_ਬਾਲਟੀ * ਕੁੱਲ_Buckets_ਅੰਕ)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_ਨਿਵੇਸ਼_Files.ਮਾਪ() << " < " << MIN_FILES_ਪ੍ਰਤੀ_ਬਾਲਟੀ << " - "
                         << ਕੁੱਲ_Buckets_ਅੰਕ << ")");
            groupingRedundant = ਸੱਚ;
        }
        ਜੇ (!groupingRedundant) ਵਾਪਸ ਝੂਠ;

        m_outputFiles.reserve(m_ਨਿਵੇਸ਼_Files.ਮਾਪ());
        ਲਈ (ਅਚਲ FilenameWithScore& filename : m_ਨਿਵੇਸ਼_Files) {
            m_outputFiles.ਧੱਕੋ_ਪਿੱਠ({std::move(filename.m_filename), {}});
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ createWorkLists() {
        // Create initial Work Lists.

        ਅਚਲ ਪੂਰਨ ਕੁੱਲ_Buckets_ਅੰਕ = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        ਅਚਲ uint64_t concatenableFileMaxScore = m_ਕੁੱਲ_Score / ਕੁੱਲ_Buckets_ਅੰਕ / 2;

        V3Stats::ਜੋੜੋ_Stat("Concatenation max score", concatenableFileMaxScore);
        ਪੂਰਨ ਅਗਲਾ_Work_List_Id = 0;

        ਜੇ (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        ਲਈ (ਅਚਲ FilenameWithScore& ਨਿਵੇਸ਼_File : m_ਨਿਵੇਸ਼_Files) {
            ਅਚਲ ਤਾਰਕਿਕ fileIsConcatenable = (ਨਿਵੇਸ਼_File.m_score <= concatenableFileMaxScore);
            ਜੇ (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << ਨਿਵੇਸ਼_File.m_filename << "  (score: " << ਨਿਵੇਸ਼_File.m_score << ")\n";
            V3Stats::ਜੋੜੋ_Stat_ਜੋੜ(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                ਨਿਵੇਸ਼_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            ਜੇ (m_workLists.ਖਾਲੀ_ਥਾਵਾਂ() || m_workLists.ਪਿੱਠ().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_ਪਿੱਠ(WorkList{ਅਗਲਾ_Work_List_Id++});
                m_workLists.ਪਿੱਠ().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.ਪਿੱਠ();
            list.m_files.ਧੱਕੋ_ਪਿੱਠ(
                {ਨਿਵੇਸ਼_File.m_filename, ਨਿਵੇਸ਼_File.m_score, ਨਿਵੇਸ਼_File.m_ਮਾਪ_ਬਾਈਟਾਂ});
            list.m_ਕੁੱਲ_Score += ਨਿਵੇਸ਼_File.m_score;
        }
        ਜੇ (m_logp) *m_logp << '\n';
    }

    ਖਾਲੀ ਸੌਂਪੋ_Buckets(uint64_t concatenable_Files_ਕੁੱਲ_Score) {
        // Assign buckets to lists

        ਅਚਲ ਮਾਪ_t ਕੁੱਲ_Buckets_ਅੰਕ = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        ਜੇ (m_concatenable_Lists_By_Desc_ਮਾਪ.ਮਾਪ() > ਕੁੱਲ_Buckets_ਅੰਕ) {
            // Debugging: Log which work lists will be kept
            ਜੇ (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_ਮਾਪ.begin(),
                              m_concatenable_Lists_By_Desc_ਮਾਪ.begin() + ਕੁੱਲ_Buckets_ਅੰਕ,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.ਮਾਪ()
                                          << "; total score: " << listp->m_ਕੁੱਲ_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_ਮਾਪ.begin() + ਕੁੱਲ_Buckets_ਅੰਕ,
                          m_concatenable_Lists_By_Desc_ਮਾਪ.ਸਿਰਾ(), [&](WorkList* listp) {
                              listp->m_isConcatenable = ਝੂਠ;

                              ਜੇ (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.ਮਾਪ()
                                          << "; total score: " << listp->m_ਕੁੱਲ_Score << ")\n";
                          });
            ਜੇ (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_ਮਾਪ.resize(ਕੁੱਲ_Buckets_ਅੰਕ);
            // Recalculate stats
            concatenable_Files_ਕੁੱਲ_Score = 0;
            ਲਈ (ਅਚਲ WorkList* ਅਚਲ listp : m_concatenable_Lists_By_Desc_ਮਾਪ) {
                concatenable_Files_ਕੁੱਲ_Score += listp->m_ਕੁੱਲ_Score;
            }
        }

        ਅਚਲ uint64_t ideal_ਬਾਲਟੀ_Score = concatenable_Files_ਕੁੱਲ_Score / ਕੁੱਲ_Buckets_ਅੰਕ;

        V3Stats::ਜੋੜੋ_Stat("Concatenation ideal bucket score", ideal_ਬਾਲਟੀ_Score);

        ਜੇ (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        ਪੂਰਨ ਉਪਲਬਧ_Buckets = v3Global.opt.outputGroups();
        ਲਈ (WorkList* listp : m_concatenable_Lists_By_Desc_ਮਾਪ) {
            ਜੇ (ਉਪਲਬਧ_Buckets > 0 && ideal_ਬਾਲਟੀ_Score > 0) {
                listp->m_buckets_ਅੰਕ = std::min(
                    ਉਪਲਬਧ_Buckets, std::max<ਪੂਰਨ>(1, listp->m_ਕੁੱਲ_Score / ideal_ਬਾਲਟੀ_Score));
                ਉਪਲਬਧ_Buckets -= listp->m_buckets_ਅੰਕ;
                ਜੇ (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_ਅੰਕ << "] Work List #"
                            << listp->m_dbgId << '\n';
            } ਨਹੀਂਤਾਂ {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = ਝੂਠ;
                ਜੇ (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::ਖੱਬੇ << std::setw(4) << listp->m_dbgId
                            << std::ਸੱਜੇ << " (excluding from concatenation)\n";
            }
        }
        ਜੇ (m_logp) *m_logp << '\n';
    }

    ਖਾਲੀ buildOutputList() {
        // Assign files to buckets and build final list of files

        // At this point the workLists contains concatenatable file lists separated by one or more
        // non-concatenable file lists. Each concatenatable list has N buckets (where N > 0)
        // assigned to it, which have to be filled with files from this list. Ideally, sum of file
        // scores in every bucket should be the same.
        ਲਈ (WorkList& list : m_workLists) {
            ਜੇ (!list.m_isConcatenable) {
                ਲਈ (ਅਚਲ FilenameWithScore& file : list.m_files) {
                    m_outputFiles.ਧੱਕੋ_ਪਿੱਠ({std::move(file.m_filename), {}});
                }
                ਜਾਰੀ;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            ਅਚਲ uint64_t list_Ideal_ਬਾਲਟੀ_Score = list.m_ਕੁੱਲ_Score / list.m_buckets_ਅੰਕ;

            std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<ਅਚਲ FilenameWithScore*>> buckets;
            ਸਵੈਚਾਲਿਤ fileIt = list.m_files.cbegin();
            ਲਈ (ਪੂਰਨ i = 0; i < list.m_buckets_ਅੰਕ; ++i) {
                std::ਸਦਿਸ਼<ਅਚਲ FilenameWithScore*> ਬਾਲਟੀ;
                uint64_t ਬਾਲਟੀ_Score = 0;

                ਲਈ (; fileIt != list.m_files.cend(); ++fileIt) {
                    ਅਚਲ uint64_t diffNow
                        = std::ਨਿਰਪੇਖ(static_cast<int64_t>(list_Ideal_ਬਾਲਟੀ_Score - ਬਾਲਟੀ_Score));
                    ਅਚਲ uint64_t diffIfAdded = std::ਨਿਰਪੇਖ(static_cast<int64_t>(
                        list_Ideal_ਬਾਲਟੀ_Score - ਬਾਲਟੀ_Score - fileIt->m_score));
                    ਜੇ (ਬਾਲਟੀ_Score == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        ਬਾਲਟੀ_Score += fileIt->m_score;
                        ਬਾਲਟੀ.ਧੱਕੋ_ਪਿੱਠ(&*fileIt);
                    } ਨਹੀਂਤਾਂ {
                        // Best possible bucket score reached, process next bucket.
                        ਰੋਕੋ;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.ਧੱਕੋ_ਪਿੱਠ(std::move(ਬਾਲਟੀ));
            }
            ਲਈ (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.ਪਿੱਠ().ਧੱਕੋ_ਪਿੱਠ(&*fileIt);
            }
            ਲਈ (ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ FilenameWithScore*>& ਬਾਲਟੀ : buckets) {
                ਜੋੜੋ_ਬਾਲਟੀ_Groups(ਬਾਲਟੀ);
            }
        }
    }

    ਖਾਲੀ ਜੋੜੋ_ਬਾਲਟੀ_Groups(ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ FilenameWithScore*>& ਬਾਲਟੀ) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<std::string>> groups;
        uint64_t ਸਮੂਹ_ਬਾਈਟਾਂ = 0;
        ਲਈ (ਅਚਲ FilenameWithScore* ਅਚਲ filep : ਬਾਲਟੀ) {
            ਜੇ (groups.ਖਾਲੀ_ਥਾਵਾਂ() || ਸਮੂਹ_ਬਾਈਟਾਂ + filep->m_ਮਾਪ_ਬਾਈਟਾਂ > m_max_ਸਮੂਹ_ਬਾਈਟਾਂ) {
                groups.emplace_ਪਿੱਠ();
                ਸਮੂਹ_ਬਾਈਟਾਂ = 0;
            }
            ਸਮੂਹ_ਬਾਈਟਾਂ += filep->m_ਮਾਪ_ਬਾਈਟਾਂ;
            groups.ਪਿੱਠ().ਧੱਕੋ_ਪਿੱਠ(filep->m_filename);
        }
        ਲਈ (std::ਸਦਿਸ਼<std::string>& ਸਮੂਹ : groups) {
            ਜੇ (ਸਮੂਹ.ਮਾਪ() == 1) {
                m_outputFiles.ਧੱਕੋ_ਪਿੱਠ({std::move(ਸਮੂਹ.front()), {}});
            } ਨਹੀਂਤਾਂ {
                m_outputFiles.ਧੱਕੋ_ਪਿੱਠ({ਸਮੂਹ_Filename(m_ਸਮੂਹ_ਅੰਕ++), std::move(ਸਮੂਹ)});
            }
        }
    }

    ਖਾਲੀ assertFilesSame() ਅਚਲ {
        ਸਵੈਚਾਲਿਤ ifIt = m_ਨਿਵੇਸ਼_Files.begin();
        ਸਵੈਚਾਲਿਤ ਦਾ_It = m_outputFiles.begin();
        ਜਦੋਂ (ifIt != m_ਨਿਵੇਸ਼_Files.ਸਿਰਾ() && ਦਾ_It != m_outputFiles.ਸਿਰਾ()) {
            ਜੇ (ਦਾ_It->isConcatenatingFile()) {
                ਲਈ (ਅਚਲ string& ocf : ਦਾ_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_ਨਿਵੇਸ਼_Files.ਸਿਰਾ(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++ਦਾ_It;
            } ਨਹੀਂਤਾਂ {
                UASSERT(ifIt->m_filename == ਦਾ_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << ਦਾ_It->m_filename);
                ++ifIt;
                ++ਦਾ_It;
            }
        }
        UASSERT(ifIt == m_ਨਿਵੇਸ਼_Files.ਸਿਰਾ(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(ਦਾ_It == m_outputFiles.ਸਿਰਾ(),
                "More output files than input files. First extra file: " << ਦਾ_It->m_filename);
    }

    ਖਾਲੀ process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_ਸਮੂਹ_File_ਅਗੇਤਰ);
        UINFO(5, "Number of input files: " << m_ਨਿਵੇਸ਼_Files.ਮਾਪ());
        UINFO(5, "Total score: " << m_ਕੁੱਲ_Score);

        ਅਚਲ ਪੂਰਨ ਕੁੱਲ_Buckets_ਅੰਕ = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << ਕੁੱਲ_Buckets_ਅੰਕ);
        UASSERT(ਕੁੱਲ_Buckets_ਅੰਕ > 0, "More than 0 buckets required");

        ਜੇ (fallbackNoGrouping(m_ਨਿਵੇਸ਼_Files.ਮਾਪ())) ਵਾਪਸ;

        ਜੇ (debug() >= 6 || dumpLevel() >= 6) {
            ਅਚਲ string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            ਜੇ (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        ਜੇ (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        ਮਾਪ_t concatenable_Files_ਗਿਣਤੀ = 0;
        int64_t concatenable_Files_ਕੁੱਲ_Score = 0;

        ਲਈ (WorkList& list : m_workLists) {
            ਜੇ (!list.m_isConcatenable) ਜਾਰੀ;

            // "Concatenation" of a single file is pointless
            ਜੇ (list.m_files.ਮਾਪ() == 1) {
                list.m_isConcatenable = ਝੂਠ;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                ਜਾਰੀ;
            }

            concatenable_Files_ਗਿਣਤੀ += list.m_files.ਮਾਪ();
            concatenable_Files_ਕੁੱਲ_Score += list.m_ਕੁੱਲ_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_ਮਾਪ.ਧੱਕੋ_ਪਿੱਠ(&list);
        }

        ਜੇ (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        ਜੇ (fallbackNoGrouping(concatenable_Files_ਗਿਣਤੀ)) ਵਾਪਸ;

        std::ਛਾਂਟੋ(m_concatenable_Lists_By_Desc_ਮਾਪ.begin(), m_concatenable_Lists_By_Desc_ਮਾਪ.ਸਿਰਾ(),
                  [](ਅਚਲ WorkList* ap, ਅਚਲ WorkList* bp) {
                      // Sort in descending order by number of files
                      ਜੇ (ap->m_files.ਮਾਪ() != bp->m_files.ਮਾਪ())
                          ਵਾਪਸ (ap->m_files.ਮਾਪ() > bp->m_files.ਮਾਪ());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      ਵਾਪਸ bp->m_ਕੁੱਲ_Score > ap->m_ਕੁੱਲ_Score;
                  });

        ਸੌਂਪੋ_Buckets(concatenable_Files_ਕੁੱਲ_Score);

        buildOutputList();

        ਜੇ (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

ਜਨਤਕ:
    ਸਥਿਰ std::ਸਦਿਸ਼<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::ਸਦਿਸ਼<FilenameWithScore> ਨਿਵੇਸ਼_Files, uint64_t ਕੁੱਲ_Score,
                                ਅਚਲ std::string& ਸਮੂਹ_File_ਅਗੇਤਰ, uint64_t max_ਸਮੂਹ_ਬਾਈਟਾਂ) {
        Emit_ਸਮੂਹ ਸਮੂਹ{std::move(ਨਿਵੇਸ਼_Files), ਕੁੱਲ_Score, ਸਮੂਹ_File_ਅਗੇਤਰ, max_ਸਮੂਹ_ਬਾਈਟਾਂ};
        ਸਮੂਹ.process();
        ਵਾਪਸ ਸਮੂਹ.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

ਵਰਗ EmitMk final {
    ਵਰਤੋਂ FileOrConcatenatedFilesList = Emit_ਸਮੂਹ::FileOrConcatenatedFilesList;
    ਵਰਤੋਂ FilenameWithScore = Emit_ਸਮੂਹ::FilenameWithScore;

    // MEMBERS
    ਦੁੱਗਣਾ m_put_Class_ਗਿਣਤੀ = 0;  // Number of classEntries printed

ਜਨਤਕ:
    // METHODS

    ਸਥਿਰ ਖਾਲੀ emitConcatenatingFile(ਅਚਲ FileOrConcatenatedFilesList& ਇੰਦਰਾਜ) {
        UASSERT(ਇੰਦਰਾਜ.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_ਬਾਹਰਲਾ_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + ਇੰਦਰਾਜ.m_filename + ".cpp"};
        concatenatingFile.puts_ਸਿਰਲੇਖ();
        ਲਈ (ਅਚਲ string& file : ਇੰਦਰਾਜ.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    ਸਥਿਰ uint64_t file_ਮਾਪ(ਅਚਲ AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::ਦੁਆਧਾਰੀ | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        ਵਾਪਸ static_cast<uint64_t>(ifs.tellg());
    }

    ਖਾਲੀ put_Make_Class_ਇੰਦਰਾਜ(V3_ਬਾਹਰਲਾ_Mk_File& ਦਾ, ਅਚਲ string& name) {
        ਦਾ.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_ਗਿਣਤੀ;
    }

    ਖਾਲੀ emitClassMake() {
        std::ਸਦਿਸ਼<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::ਸਦਿਸ਼<FileOrConcatenatedFilesList> vmClassesFastList;
        ਜੇ (v3Global.opt.outputGroups() > 0) {
            std::ਸਦਿਸ਼<FilenameWithScore> slowFiles;
            std::ਸਦਿਸ਼<FilenameWithScore> fastFiles;
            uint64_t slow_ਕੁੱਲ_Score = 0;
            uint64_t fast_ਕੁੱਲ_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t ਸਿਰਲੇਖ_ਬਾਈਟਾਂ = 0;

            ਲਈ (Ast_ਗੰਢ_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), ਗੰਢ_File)) {
                ਅਚਲ AstCFile* ਅਚਲ cfilep = VN_CAST(nodep, CFile);
                ਜੇ (cfilep && VString::ਸਿਰੇ_With(cfilep->name(), ".h")) {
                    ਸਿਰਲੇਖ_ਬਾਈਟਾਂ += file_ਮਾਪ(cfilep);
                }
                ਜੇ (cfilep && cfilep->ਸਰੋਤ() && cfilep->support() == ਝੂਠ) {
                    std::ਸਦਿਸ਼<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& ਕੁੱਲ_Score = cfilep->slow() ? slow_ਕੁੱਲ_Score : fast_ਕੁੱਲ_Score;

                    ਕੁੱਲ_Score += cfilep->complexityScore();
                    files.ਧੱਕੋ_ਪਿੱਠ({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_ਮਾਪ(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_ਸਮੂਹ_ਬਾਈਟਾਂ = std::numeric_limits<uint64_t>::max();
            ਜੇ (v3Global.opt.output_Groups_ਮਾਪ()) {
                ਅਚਲ uint64_t max_Tu_ਬਾਈਟਾਂ = static_cast<uint64_t>(v3Global.opt.output_Groups_ਮਾਪ());
                max_ਸਮੂਹ_ਬਾਈਟਾਂ = max_Tu_ਬਾਈਟਾਂ > ਸਿਰਲੇਖ_ਬਾਈਟਾਂ ? max_Tu_ਬਾਈਟਾਂ - ਸਿਰਲੇਖ_ਬਾਈਟਾਂ : 0;
            }

            vmClassesSlowList = Emit_ਸਮੂਹ::singleConcatenatedFilesList(
                std::move(slowFiles), slow_ਕੁੱਲ_Score, "vm_classes_Slow_", max_ਸਮੂਹ_ਬਾਈਟਾਂ);
            vmClassesFastList = Emit_ਸਮੂਹ::singleConcatenatedFilesList(
                std::move(fastFiles), fast_ਕੁੱਲ_Score, "vm_classes_", max_ਸਮੂਹ_ਬਾਈਟਾਂ);
        }

        // Generate the makefile
        V3_ਬਾਹਰਲਾ_Mk_File ਦਾ{v3Global.opt.makeDir() + "/" + v3Global.opt.ਅਗੇਤਰ() + "_classes.mk"};
        ਦਾ.puts_ਸਿਰਲੇਖ();
        ਦਾ.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        ਦਾ.puts("#\n");
        ਦਾ.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        ਦਾ.puts("# See " + v3Global.opt.ਅਗੇਤਰ() + ".mk" + " for the caller.\n");

        ਦਾ.puts("\n### Switches...\n");
        ਦਾ.puts("# C11 constructs required?  0/1 (always on now)\n");
        ਦਾ.put_ਸੈੱਟ("VM_C11", "1");
        ਦਾ.puts("# Timing enabled?  0/1\n");
        ਦਾ.put_ਸੈੱਟ("VM_TIMING", v3Global.ਵਰਤਦਾ_Timing() ? "1" : "0");
        ਦਾ.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        ਦਾ.put_ਸੈੱਟ("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        ਦਾ.puts("# Parallel builds?  0/1 (from --output-split)\n");
        ਦਾ.put_ਸੈੱਟ("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        ਦਾ.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        ਦਾ.put_ਸੈੱਟ("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        ਦਾ.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        ਦਾ.put_ਸੈੱਟ("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        ਦਾ.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        ਦਾ.put_ਸੈੱਟ("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        ਦਾ.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        ਦਾ.put_ਸੈੱਟ("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        ਦਾ.puts("# VPI enabled?  0/1 (from --vpi)\n");
        ਦਾ.put_ਸੈੱਟ("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        ਦਾ.puts("\n### Object file lists...\n");
        ਲਈ (ਪੂਰਨ support = 0; support < 3; ++support) {
            ਲਈ (ਅਚਲ ਤਾਰਕਿਕ& slow : {ਝੂਠ, ਸੱਚ}) {
                ਜੇ (support == 2) {
                    ਦਾ.puts("# Global classes, need linked once per executable");
                } ਨਹੀਂਤਾਂ ਜੇ (support) {
                    ਦਾ.puts("# Generated support classes");
                } ਨਹੀਂਤਾਂ {
                    ਦਾ.puts("# Generated module classes");
                }
                ਜੇ (slow) {
                    ਦਾ.puts(", non-fast-path, compile with low/medium optimization\n");
                } ਨਹੀਂਤਾਂ {
                    ਦਾ.puts(", fast-path, compile with highest optimization\n");
                }
                ਅਚਲ string ਨਿਸ਼ਾਨਾ_Var = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_ਗਿਣਤੀ = 0;
                ਦਾ.puts(ਨਿਸ਼ਾਨਾ_Var + " += \\\n");
                ਜੇ (support == 2 && v3Global.opt.hier_ਬੱਚਾ()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } ਨਹੀਂਤਾਂ ਜੇ (support == 2 && !slow) {
                    ਲਈ (ਅਚਲ string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_ਇੰਦਰਾਜ(ਦਾ, cpp);
                } ਨਹੀਂਤਾਂ ਜੇ (support == 2 && slow) {
                } ਨਹੀਂਤਾਂ ਜੇ (support == 0 && v3Global.opt.outputGroups() > 0) {
                    ਅਚਲ std::ਸਦਿਸ਼<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    ਲਈ (ਅਚਲ FileOrConcatenatedFilesList& ਇੰਦਰਾਜ : list) {
                        ਜੇ (ਇੰਦਰਾਜ.isConcatenatingFile()) emitConcatenatingFile(ਇੰਦਰਾਜ);
                        put_Make_Class_ਇੰਦਰਾਜ(ਦਾ, ਇੰਦਰਾਜ.m_filename);
                    }
                } ਨਹੀਂਤਾਂ {
                    ਲਈ (Ast_ਗੰਢ_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), ਗੰਢ_File)) {
                        ਅਚਲ AstCFile* ਅਚਲ cfilep = VN_CAST(nodep, CFile);
                        ਜੇ (cfilep && cfilep->ਸਰੋਤ() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_ਇੰਦਰਾਜ(ਦਾ, cfilep->name());
                        }
                    }
                }
                ਦਾ.puts("\n");
                V3Stats::ਜੋੜੋ_Stat("Makefile targets, " + ਨਿਸ਼ਾਨਾ_Var, m_put_Class_ਗਿਣਤੀ);
            }
        }

        ਦਾ.puts_ਸਿਰਲੇਖ();
    }

    ਖਾਲੀ emitOverallMake() {
        // Generate the makefile
        V3_ਬਾਹਰਲਾ_Mk_File ਦਾ{v3Global.opt.makeDir() + "/" + v3Global.opt.ਅਗੇਤਰ() + ".mk"};
        ਦਾ.puts_ਸਿਰਲੇਖ();
        ਦਾ.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        ਦਾ.puts("#\n");
        ਦਾ.puts("# Execute this makefile from the object directory:\n");
        ਦਾ.puts("#    make -f " + v3Global.opt.ਅਗੇਤਰ() + ".mk" + "\n");
        ਦਾ.puts("\n");

        ਜੇ (v3Global.opt.exe()) {
            ਦਾ.puts("default: " + v3Global.opt.exeName() + "\n");
        } ਨਹੀਂਤਾਂ ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਦਾ.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } ਨਹੀਂਤਾਂ {
            ਦਾ.puts("default: lib" + v3Global.opt.ਅਗੇਤਰ() + "\n");
        }
        ਦਾ.puts("\n### Constants...\n");
        ਦਾ.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        ਦਾ.put_ਸੈੱਟ("PERL", V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(V3Options::getenvPERL()));
        ਦਾ.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        ਦਾ.put_ਸੈੱਟ("PYTHON3", V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        ਦਾ.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        ਦਾ.put_ਸੈੱਟ("VERILATOR_ROOT",
                  V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_ਜੜ੍ਹ()));
        ਦਾ.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        ਦਾ.put_ਸੈੱਟ_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        ਦਾ.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        ਦਾ.put_ਸੈੱਟ_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        ਜੇ (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        ਦਾ.puts("\n### Switches...\n");
        ਦਾ.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        ਦਾ.put_ਸੈੱਟ("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        ਦਾ.puts("# SystemC output mode?  0/1 (from --sc)\n");
        ਦਾ.put_ਸੈੱਟ("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        ਦਾ.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        ਦਾ.put_ਸੈੱਟ("VM_SP_OR_SC", "$(VM_SC)");
        ਦਾ.puts("# Deprecated\n");
        ਦਾ.put_ਸੈੱਟ("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        ਦਾ.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        ਦਾ.put_ਸੈੱਟ("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        ਦਾ.puts("\n### Vars...\n");
        ਦਾ.puts("# Module prefix (from --prefix)\n");
        ਦਾ.put_ਸੈੱਟ("VM_MODPREFIX", v3Global.opt.mod_ਅਗੇਤਰ());
        ਦਾ.puts("# Design prefix (from --prefix)\n");
        ਦਾ.put_ਸੈੱਟ("VM_PREFIX", v3Global.opt.ਅਗੇਤਰ());
        ਦਾ.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        ਦਾ.put_ਸੈੱਟ("VM_PREFIX_INCLUDE", "<" + v3Global.opt.ਅਗੇਤਰ() + ".h>");
        ਜੇ (v3Global.dpi()) {
            ਦਾ.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            ਦਾ.put_ਸੈੱਟ("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.ਅਗੇਤਰ() + "__Dpi.h>");
        }

        ਦਾ.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        ਦਾ.puts("VM_USER_CFLAGS = \\\n");
        ਅਚਲ std::string solver = V3Options::getenvVERILATOR_SOLVER();
        ਜੇ (v3Global.useRandomizeMethods() && solver != "")
            ਦਾ.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) ਦਾ.puts("\t-fPIC \\\n");
        ਅਚਲ VStringList& cFlags = v3Global.opt.cFlags();
        ਲਈ (ਅਚਲ string& i : cFlags) ਦਾ.puts("  " + i + " \\\n");
        ਦਾ.puts("\n");

        ਦਾ.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        ਦਾ.puts("VM_USER_LDLIBS = \\\n");
        ਅਚਲ VStringList& ldLibs = v3Global.opt.ldLibs();
        ਲਈ (ਅਚਲ string& i : ldLibs) ਦਾ.puts("  " + i + " \\\n");
        ਦਾ.puts("\n");

        ਦਾ.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        ਦਾ.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        ਦਾ.puts("VM_USER_LDLIBS_FILES = \\\n");
        ਲਈ (ਅਚਲ string& i : v3Global.opt.ldLibFiles()) ਦਾ.puts("  " + i + " \\\n");
        ਦਾ.puts("\n");

        VString_ਸੈੱਟ dirs;
        ਦਾ.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        ਦਾ.puts("VM_USER_CLASSES = \\\n");
        ਅਚਲ VString_ਸੈੱਟ& cppFiles = v3Global.opt.cppFiles();
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& cppfile : cppFiles) {
            ਦਾ.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            ਅਚਲ string dir
                = V3Os::filename_Relative_ਰਸਤਾ(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.ਪਾਓ(dir);
        }
        dirs.ਪਾਓ(V3Os::filename_Relative_ਰਸਤਾ(".", v3Global.opt.makeDir()));

        ਦਾ.puts("\n");

        ਦਾ.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        ਦਾ.puts("VM_USER_DIR = \\\n");
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : dirs) ਦਾ.puts("  " + i + " \\\n");
        ਦਾ.puts("\n");

        ਦਾ.puts("### Default rules...\n");
        ਦਾ.puts("# Include list of all generated classes\n");
        ਦਾ.puts("include " + v3Global.opt.ਅਗੇਤਰ() + "_classes.mk\n");
        ਜੇ (v3Global.opt.hier_ਸਿਖਰ()) {
            ਦਾ.puts("# Include rules for hierarchical blocks\n");
            ਦਾ.puts("include " + v3Global.opt.ਅਗੇਤਰ() + "_hier.mk\n");
        }
        ਦਾ.puts("# Include global rules\n");
        ਦਾ.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        ਜੇ (v3Global.opt.exe()) {
            ਦਾ.puts("\n### Executable rules... (from --exe)\n");
            ਦਾ.puts("VPATH += $(VM_USER_DIR)\n");
            ਦਾ.puts("\n");

            ਜੇ (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                ਦਾ.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                ਦਾ.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                ਦਾ.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                ਦਾ.puts("\n");
            }
        }

        ਅਚਲ string compilerIncludePch
            = v3Global.opt.compilerIncludes().ਖਾਲੀ_ਥਾਵਾਂ() ? "" : "$(VK_PCH_H).fast.gch";
        ਅਚਲ string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().ਖਾਲੀ_ਥਾਵਾਂ() ? "" : "$(VK_PCH_I_FAST)";
        ਲਈ (ਅਚਲ string& cppfile : cppFiles) {
            ਅਚਲ string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            ਦਾ.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            ਦਾ.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        ਜੇ (v3Global.opt.exe()) {
            ਦਾ.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            ਦਾ.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            ਦਾ.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            ਦਾ.puts("\n");
        } ਨਹੀਂਤਾਂ ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            ਦਾ.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            ਦਾ.puts(v3Global.opt.libCreateName(ਝੂਠ) + ": " + libCreateDeps + "\n");
            ਦਾ.puts("\n");
            ਜੇ (v3Global.opt.hier_ਬੱਚਾ()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                ਦਾ.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(ਝੂਠ)
                        + "\n");
            } ਨਹੀਂਤਾਂ {
                ਦਾ.puts(v3Global.opt.libCreateName(ਸੱਚ) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                ਦਾ.puts("ifeq ($(shell uname -s),Darwin)\n");
                ਦਾ.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                ਦਾ.puts("else\n");
                ਦਾ.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                ਦਾ.puts("endif\n");
                ਦਾ.puts("\n");
                ਦਾ.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(ਝੂਠ)
                        + " " + v3Global.opt.libCreateName(ਸੱਚ) + "\n");
            }
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ string libname = "lib" + v3Global.opt.ਅਗੇਤਰ() + ".a";
            ਦਾ.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            ਦਾ.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            ਦਾ.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            ਦਾ.puts("lib" + v3Global.opt.ਅਗੇਤਰ() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        ਦਾ.puts_ਸਿਰਲੇਖ();
    }

    ਸਪਸ਼ਟ EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    ਆਭਾਸੀ ~EmitMk() = ਮੂਲ;
};

//######################################################################

ਵਰਗ EmitMkHierVerilation final {
    ਅਚਲ V3_Hier_ਗ੍ਰਾਫ* ਅਚਲ m_graphp;
    ਅਚਲ string m_makefile;  // path of this makefile
    ਖਾਲੀ emitCommonOpts(V3_ਬਾਹਰਲਾ_Mk_File& ਦਾ) ਅਚਲ {
        ਦਾ.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        ਦਾ.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        ਦਾ.puts("# Common options for hierarchical blocks\n");
        ਅਚਲ string fullpath_bin = V3Os::filename_Real_ਰਸਤਾ(v3Global.opt.buildDepBin());
        ਅਚਲ string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        ਦਾ.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        ਦਾ.puts("VM_HIER_INPUT_FILES := \\\n");
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : v3Global.opt.vFiles())
            ਦਾ.puts("  " + V3Os::filename_Real_ਰਸਤਾ(i.filename()) + " \\\n");
        ਦਾ.puts("\n");
        ਦਾ.puts("VM_HIER_VERILOG_LIBS := \\\n");
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : v3Global.opt.libraryFiles()) {
            ਦਾ.puts("  " + V3Os::filename_Real_ਰਸਤਾ(i.filename()) + " \\\n");
        }
        ਦਾ.puts("\n");
    }
    ਖਾਲੀ emitLaunchVerilator(V3_ਬਾਹਰਲਾ_Mk_File& ਦਾ, ਅਚਲ string& ਦਲੀਲਾਂ_File) ਅਚਲ {
        ਦਾ.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        ਦਾ.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + ਦਲੀਲਾਂ_File + "\"\n");
    }
    ਖਾਲੀ emit(V3_ਬਾਹਰਲਾ_Mk_File& ਦਾ) ਅਚਲ {
        ਦਾ.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        ਦਾ.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        ਦਾ.puts("#\n");
        ਦਾ.puts("# The main makefile " + v3Global.opt.ਅਗੇਤਰ() + ".mk calls this makefile\n");
        ਦਾ.puts("\n");
        ਦਾ.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        ਦਾ.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        ਦਾ.puts(".SUFFIXES:\n");
        ਦਾ.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        ਦਾ.puts("# Libraries of hierarchical blocks\n");
        ਦਾ.puts("VM_HIER_LIBS := \\\n");
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_graphp->vertices()) {
            ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ blockp = vtx.as<V3_Hier_ਖੰਡ>();
            ਦਾ.puts("  " + blockp->hierLibFilename(ਸੱਚ) + " \\\n");
        }
        ਦਾ.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        ਦਾ.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.ਅਗੇਤਰ() + ".mk\n");
        ਦਾ.puts("\t$(MAKE) -f " + v3Global.opt.ਅਗੇਤਰ() + ".mk\n");
        ਦਾ.puts("hier_verilation: " + v3Global.opt.ਅਗੇਤਰ() + ".mk\n");
        emitCommonOpts(ਦਾ);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        ਦਾ.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        ਦਾ.puts("hier_launch_verilator:\n");
        ਦਾ.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            ਅਚਲ string ਦਲੀਲਾਂ_File = v3Global.hierGraphp()->ਸਿਖਰ_Command_ਦਲੀਲਾਂ_Filename(ਝੂਠ);
            ਦਾ.puts("\n# Verilate the top module\n");
            ਦਾ.puts(v3Global.opt.ਅਗੇਤਰ()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            ਦਾ.puts(V3Os::filenameNonDir(ਦਲੀਲਾਂ_File) + " ");
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_graphp->vertices()) {
                ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ blockp = vtx.as<V3_Hier_ਖੰਡ>();
                ਦਾ.puts(blockp->hierWrapperFilename(ਸੱਚ) + " ");
            }
            ਦਾ.puts("\n");
            emitLaunchVerilator(ਦਾ, ਦਲੀਲਾਂ_File);
        }

        // Rules to process hierarchical blocks
        ਦਾ.puts("\n# Verilate hierarchical blocks\n");
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_graphp->vertices()) {
            ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ blockp = vtx.as<V3_Hier_ਖੰਡ>();
            ਅਚਲ string ਅਗੇਤਰ = blockp->hier_ਅਗੇਤਰ();
            ਅਚਲ string ਦਲੀਲਾਂ_Filename = blockp->command_ਦਲੀਲਾਂ_Filename(ਝੂਠ);
            ਦਾ.puts(blockp->hierGeneratedFilenames(ਸੱਚ));
            ਦਾ.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            ਦਾ.puts(V3Os::filenameNonDir(ਦਲੀਲਾਂ_Filename) + " ");
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : blockp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ dependencyp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<V3_Hier_ਖੰਡ>();
                ਦਾ.puts(dependencyp->hierWrapperFilename(ਸੱਚ) + " ");
            }
            ਦਾ.puts("\n");
            emitLaunchVerilator(ਦਾ, ਦਲੀਲਾਂ_Filename);

            // Rule to build lib*.a
            ਦਾ.puts(blockp->hierLibFilename(ਸੱਚ));
            ਦਾ.puts(": ");
            ਦਾ.puts(blockp->hierMkFilename(ਸੱਚ));
            ਦਾ.puts(" ");
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : blockp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਅਚਲ V3_Hier_ਖੰਡ* ਅਚਲ dependencyp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<V3_Hier_ਖੰਡ>();
                ਦਾ.puts(dependencyp->hierLibFilename(ਸੱਚ));
                ਦਾ.puts(" ");
            }
            ਦਾ.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(ਝੂਠ) + " -C " + ਅਗੇਤਰ);
            ਦਾ.puts(" VM_PREFIX=" + ਅਗੇਤਰ);
            ਦਾ.puts("\n\n");
        }
        ਦਾ.puts("endif  # Guard\n");
    }

ਜਨਤਕ:
    ਸਪਸ਼ਟ EmitMkHierVerilation(ਅਚਲ V3_Hier_ਗ੍ਰਾਫ* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.ਅਗੇਤਰ() + "_hier.mk"} {
        V3_ਬਾਹਰਲਾ_Mk_File ਦਾ{m_makefile};
        emit(ਦਾ);
    }
};

//######################################################################
// Gate class functions

ਖਾਲੀ V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    ਅਚਲ EmitMk emitter;
}

ਖਾਲੀ V3EmitMk::emitHierVerilation(ਅਚਲ V3_Hier_ਗ੍ਰਾਫ* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
