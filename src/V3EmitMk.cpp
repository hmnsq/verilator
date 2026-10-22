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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3EmitMk.h"

#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3HierBlock.h"
#అన్తర్భావ "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Groups adjacent files in a list, evenly distributing sum of scores
తరగతి Emit_సమూహం final {
బహిరంగ:
    నిర్మాణం FileOrConcatenatedFilesList final {
        స్థిరరాశి std::string m_filename;  // Filename or output group filename if grouping
        std::సదిశ<std::string> m_concatenatedFilenames;  // Grouped filenames if grouping

        తార్కిక isConcatenatingFile() స్థిరరాశి { తిరిగి !m_concatenatedFilenames.ఖాళీలు(); }
    };

    నిర్మాణం FilenameWithScore final {
        స్థిరరాశి std::string m_filename;  // Input filename
        స్థిరరాశి uint64_t m_score;  // Input file complexity
        స్థిరరాశి uint64_t m_కొలత_బైట్లు;  // Input file size
    };

అంతరంగ:
    // Data of a single work unit used in `singleConcatenatedFilesList()`.
    నిర్మాణం WorkList final {
        uint64_t m_మొత్తం_Score = 0;  // Sum of scores of included files
        std::సదిశ<FilenameWithScore> m_files;  // Included filenames
        పూర్ణాంకం m_buckets_అంకె
            = 0;  // Number of buckets assigned for this list. Used only in concatenable lists.
        తార్కిక m_isConcatenable = నిజం;  // Indicated whether files on this list can be concatenated.
        స్థిరరాశి పూర్ణాంకం m_dbgId;  // Work list ID for debugging.

        WorkList() = తొలగించు;
        స్పష్టమైన WorkList(పూర్ణాంకం id)
            : m_dbgId{id} {}
    };

    // MAIN PARAMETERS

    // Minimum number of input files required to perform concatenation.
    // Concatenation of a small number of files does not give any performance advantages.
    // The value has been chosen arbitrarily, most likely could be larger.
    స్థిర constexpr కొలత_t MIN_FILES_లెక్క = 16;

    // Concatenation of only a few files most likely does not increase performance.
    // The value has been chosen arbitrarily.
    స్థిర constexpr కొలత_t MIN_FILES_చొప్పున_పాత్ర = 4;

    // MEMBERS

    స్థిరరాశి std::సదిశ<FilenameWithScore>
        m_ఆదానం_Files;  // List of filenames from initial AstCFile list
    std::సదిశ<FileOrConcatenatedFilesList>
        m_outputFiles;  // Output list of files and group files
    స్థిరరాశి uint64_t m_మొత్తం_Score;  // Sum of file scores
    స్థిరరాశి std::string m_సమూహం_File_ఉపసర్గ;  // Prefix for output group filenames
    స్థిరరాశి uint64_t m_max_సమూహం_బైట్లు;  // Maximum sum of file sizes in an output group
    std::సదిశ<WorkList> m_workLists;  // Lists of small enough files
    std::unique_ptr<std::ofstream> m_logp;  // Dump file
    std::సదిశ<WorkList*> m_concatenable_Lists_By_Desc_కొలత;  // Lists sorted by size, descending
    పూర్ణాంకం m_సమూహం_అంకె = 0;  // Number of output groups created

    Emit_సమూహం(std::సదిశ<FilenameWithScore> ఆదానం_Files, uint64_t మొత్తం_Score,
              స్థిరరాశి std::string& సమూహం_File_ఉపసర్గ, uint64_t max_సమూహం_బైట్లు)
        : m_ఆదానం_Files{std::move(ఆదానం_Files)}
        , m_మొత్తం_Score{మొత్తం_Score}
        , m_సమూహం_File_ఉపసర్గ{సమూహం_File_ఉపసర్గ}
        , m_max_సమూహం_బైట్లు{max_సమూహం_బైట్లు} {}

    string సమూహం_Filename(పూర్ణాంకం id) స్థిరరాశి {
        తిరిగి v3Global.opt.ఉపసర్గ() + "_" + m_సమూహం_File_ఉపసర్గ + std::గమ్యం_string(id);
    }

    // Debug logging: prints scores histogram
    శూన్య dumpLogScoreHistogram(std::ostream& os) {
        constexpr పూర్ణాంకం MAX_BAR_నిడివి = 80;
        constexpr పూర్ణాంకం MAX_INTERVALS_అంకె = 60;

        // All scores arranged in ascending order.
        std::సదిశ<uint64_t> sortedScores;
        sortedScores.reserve(m_ఆదానం_Files.కొలత());
        std::transform(m_ఆదానం_Files.begin(), m_ఆదానం_Files.చివర(), std::వెనుక_inserter(sortedScores),
                       [](స్థిరరాశి FilenameWithScore& ఆదానం_File) { తిరిగి ఆదానం_File.m_score; });
        std::క్రమీకరించు(sortedScores.begin(), sortedScores.చివర());

        స్థిరరాశి int64_t శిఖరం_Score = sortedScores.వెనుక();
        os << "Top score: " << శిఖరం_Score << '\n';
        స్థిరరాశి పూర్ణాంకం max_Score_వెడల్పు = std::గమ్యం_string(శిఖరం_Score).నిడివి();

        స్థిరరాశి int64_t intervals_అంకె = std::min<int64_t>(శిఖరం_Score + 1, MAX_INTERVALS_అంకె);

        నిర్మాణం Interval final {
            uint64_t m_క్రింది_Bound = 0;  // Lowest score included in this histogram bucket
            పూర్ణాంకం m_కొలత = 0;  // Number of scores that fell into this bucket
        };

        std::సదిశ<Interval> intervals;
        intervals.resize(intervals_అంకె);

        intervals[0].m_క్రింది_Bound = 0;
        కొరకు (పూర్ణాంకం i = 1; i < intervals_అంకె; i++) {
            intervals[i].m_క్రింది_Bound = (శిఖరం_Score + 1) * i / intervals_అంకె + 1;
        }

        కొరకు (స్థిరరాశి uint64_t score : sortedScores) {
            స్థిరరాశి గుర్తులేని పూర్ణాంకం ivIdx = score * intervals_అంకె / (శిఖరం_Score + 1);
            ++intervals[ivIdx].m_కొలత;
        }

        పూర్ణాంకం శిఖరం_Interval_కొలత = 0;
        కొరకు (స్థిరరాశి Interval& iv : intervals)
            శిఖరం_Interval_కొలత = std::max(శిఖరం_Interval_కొలత, iv.m_కొలత);

        os << "Input files' scores histogram:\n";

        కొరకు (స్థిరరాశి Interval& iv : intervals) {
            స్థిరరాశి పూర్ణాంకం scaled_కొలత = iv.m_కొలత * (MAX_BAR_నిడివి + 1) / శిఖరం_Interval_కొలత;
            స్థిరరాశి std::string పంక్తి = " |" + std::string(scaled_కొలత, '#');

            os << std::setw(max_Score_వెడల్పు) << iv.m_క్రింది_Bound << పంక్తి << "  " << iv.m_కొలత << '\n';
        }
        os << std::setw(max_Score_వెడల్పు) << (శిఖరం_Score + 1) << '\n';
        os << '\n';
    }

    // PRIVATE METHODS

    // Debug logging: dumps Work Lists and their lists of files
    శూన్య dumpWorkLists(std::ostream& os) {
        os << "Initial Work Lists with their concatenation eligibility status:\n";
        కొరకు (స్థిరరాశి WorkList& list : m_workLists) {
            os << "+ [" << (list.m_isConcatenable ? 'x' : ' ') << "] Work List #" << list.m_dbgId
               << "  (num of files: " << list.m_files.కొలత()
               << "; total score: " << list.m_మొత్తం_Score << ")\n";
            ఒకవేళ (debug() >= 6 || list.m_files.కొలత() < 4) {
                // Log all files
                కొరకు (స్థిరరాశి FilenameWithScore& file : list.m_files)
                    os << "| + " << file.m_filename << "  (score: " << file.m_score << ")\n";
            } లేకపోతే {
                // Log only first and last file
                స్థిరరాశి FilenameWithScore& మొదటి = list.m_files.front();
                స్థిరరాశి FilenameWithScore& చివరి = list.m_files.వెనుక();
                os << "| + " << మొదటి.m_filename << "  (score: " << మొదటి.m_score << ")\n";
                os << "| | (... " << (list.m_files.కొలత() - 2) << " files ...)\n";
                os << "| + " << చివరి.m_filename << "  (score: " << చివరి.m_score << ")\n";
            }
        }
        os << '\n';
    }

    // Debug logging: dumps list of output files. List of grouped files is additionally printed
    // for each concatenating file.
    శూన్య dumpOutputList(std::ostream& os) స్థిరరాశి {
        os << "List of output files after execution of concatenation:\n";

        కొరకు (స్థిరరాశి FileOrConcatenatedFilesList& నమోదు_2 : m_outputFiles) {
            ఒకవేళ (నమోదు_2.isConcatenatingFile()) {
                os << "+ " << నమోదు_2.m_filename << "  (concatenating file)\n";
                కొరకు (స్థిరరాశి string& f : నమోదు_2.m_concatenatedFilenames) {
                    os << "| + " << f << '\n';
                }
            } లేకపోతే {
                os << "+ " << నమోదు_2.m_filename << '\n';
            }
        }
    }

    // Called when the concatenation is aborted, creates an identity mapping
    తార్కిక fallbackNoGrouping(కొలత_t ఆదానం_Files_లెక్క) {
        స్థిరరాశి పూర్ణాంకం మొత్తం_Buckets_అంకె = v3Global.opt.outputGroups();
        // Return early if there's nothing to do.
        తార్కిక groupingRedundant = అబద్ధం;
        ఒకవేళ (ఆదానం_Files_లెక్క < MIN_FILES_లెక్క
            && ఆదానం_Files_లెక్క <= static_cast<కొలత_t>(మొత్తం_Buckets_అంకె)) {
            UINFO(4, "File concatenation skipped: Too few files (" << m_ఆదానం_Files.కొలత() << " < "
                                                                   << MIN_FILES_లెక్క << ")");
            groupingRedundant = నిజం;
        }
        ఒకవేళ (ఆదానం_Files_లెక్క < (MIN_FILES_చొప్పున_పాత్ర * మొత్తం_Buckets_అంకె)) {
            UINFO(4, "File concatenation skipped: Too few files per bucket ("
                         << m_ఆదానం_Files.కొలత() << " < " << MIN_FILES_చొప్పున_పాత్ర << " - "
                         << మొత్తం_Buckets_అంకె << ")");
            groupingRedundant = నిజం;
        }
        ఒకవేళ (!groupingRedundant) తిరిగి అబద్ధం;

        m_outputFiles.reserve(m_ఆదానం_Files.కొలత());
        కొరకు (స్థిరరాశి FilenameWithScore& filename : m_ఆదానం_Files) {
            m_outputFiles.నెట్టు_వెనుక({std::move(filename.m_filename), {}});
        }
        తిరిగి నిజం;
    }

    శూన్య createWorkLists() {
        // Create initial Work Lists.

        స్థిరరాశి పూర్ణాంకం మొత్తం_Buckets_అంకె = v3Global.opt.outputGroups();
        // Input files with a score exceeding this value are excluded from concatenation.
        స్థిరరాశి uint64_t concatenableFileMaxScore = m_మొత్తం_Score / మొత్తం_Buckets_అంకె / 2;

        V3Stats::కూడు_Stat("Concatenation max score", concatenableFileMaxScore);
        పూర్ణాంకం తదుపరి_Work_List_Id = 0;

        ఒకవేళ (m_logp) *m_logp << "Input files with their concatenation eligibility status:\n";
        కొరకు (స్థిరరాశి FilenameWithScore& ఆదానం_File : m_ఆదానం_Files) {
            స్థిరరాశి తార్కిక fileIsConcatenable = (ఆదానం_File.m_score <= concatenableFileMaxScore);
            ఒకవేళ (m_logp)
                *m_logp << " + [" << (fileIsConcatenable ? 'x' : ' ') << "] "
                        << ఆదానం_File.m_filename << "  (score: " << ఆదానం_File.m_score << ")\n";
            V3Stats::కూడు_Stat_కూడిక(fileIsConcatenable ? "Concatenation total grouped score"
                                                   : "Concatenation total non-grouped score",
                                ఆదానం_File.m_score);
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            ఒకవేళ (m_workLists.ఖాళీలు() || m_workLists.వెనుక().m_isConcatenable != fileIsConcatenable) {
                m_workLists.emplace_వెనుక(WorkList{తదుపరి_Work_List_Id++});
                m_workLists.వెనుక().m_isConcatenable = fileIsConcatenable;
            }
            // Add inputFile to the last list
            WorkList& list = m_workLists.వెనుక();
            list.m_files.నెట్టు_వెనుక(
                {ఆదానం_File.m_filename, ఆదానం_File.m_score, ఆదానం_File.m_కొలత_బైట్లు});
            list.m_మొత్తం_Score += ఆదానం_File.m_score;
        }
        ఒకవేళ (m_logp) *m_logp << '\n';
    }

    శూన్య కేటాయించు_Buckets(uint64_t concatenable_Files_మొత్తం_Score) {
        // Assign buckets to lists

        స్థిరరాశి కొలత_t మొత్తం_Buckets_అంకె = v3Global.opt.outputGroups();

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        // Does not happen very often due to files being already filtered out by comparison of
        // their score to ConcatenableFilesMaxScore.
        ఒకవేళ (m_concatenable_Lists_By_Desc_కొలత.కొలత() > మొత్తం_Buckets_అంకె) {
            // Debugging: Log which work lists will be kept
            ఒకవేళ (m_logp) {
                *m_logp << "More Work Lists than buckets; "
                           "Work Lists with statuses indicating whether the list will be kept:\n";
                // Only lists that will be kept. List that will be removed are logged below.
                std::for_each(m_concatenable_Lists_By_Desc_కొలత.begin(),
                              m_concatenable_Lists_By_Desc_కొలత.begin() + మొత్తం_Buckets_అంకె,
                              [&](WorkList* listp) {
                                  *m_logp << "+ [x] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.కొలత()
                                          << "; total score: " << listp->m_మొత్తం_Score << ")\n";
                              });
            }
            // NOTE: Not just debug logging - notice `isConcatenable` assignment in the loop.
            std::for_each(m_concatenable_Lists_By_Desc_కొలత.begin() + మొత్తం_Buckets_అంకె,
                          m_concatenable_Lists_By_Desc_కొలత.చివర(), [&](WorkList* listp) {
                              listp->m_isConcatenable = అబద్ధం;

                              ఒకవేళ (m_logp)
                                  *m_logp << "+ [ ] Work List #" << listp->m_dbgId
                                          << "  (num of files: " << listp->m_files.కొలత()
                                          << "; total score: " << listp->m_మొత్తం_Score << ")\n";
                          });
            ఒకవేళ (m_logp) *m_logp << '\n';

            m_concatenable_Lists_By_Desc_కొలత.resize(మొత్తం_Buckets_అంకె);
            // Recalculate stats
            concatenable_Files_మొత్తం_Score = 0;
            కొరకు (స్థిరరాశి WorkList* స్థిరరాశి listp : m_concatenable_Lists_By_Desc_కొలత) {
                concatenable_Files_మొత్తం_Score += listp->m_మొత్తం_Score;
            }
        }

        స్థిరరాశి uint64_t ideal_పాత్ర_Score = concatenable_Files_మొత్తం_Score / మొత్తం_Buckets_అంకె;

        V3Stats::కూడు_Stat("Concatenation ideal bucket score", ideal_పాత్ర_Score);

        ఒకవేళ (m_logp) *m_logp << "Buckets assigned to Work Lists:\n";
        పూర్ణాంకం అందుబాటు_Buckets = v3Global.opt.outputGroups();
        కొరకు (WorkList* listp : m_concatenable_Lists_By_Desc_కొలత) {
            ఒకవేళ (అందుబాటు_Buckets > 0 && ideal_పాత్ర_Score > 0) {
                listp->m_buckets_అంకె = std::min(
                    అందుబాటు_Buckets, std::max<పూర్ణాంకం>(1, listp->m_మొత్తం_Score / ideal_పాత్ర_Score));
                అందుబాటు_Buckets -= listp->m_buckets_అంకె;
                ఒకవేళ (m_logp)
                    *m_logp << "+ [" << std::setw(2) << listp->m_buckets_అంకె << "] Work List #"
                            << listp->m_dbgId << '\n';
            } లేకపోతే {
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->m_isConcatenable = అబద్ధం;
                ఒకవేళ (m_logp)
                    *m_logp << "+ [ 0] Work List #" << std::ఎడమ << std::setw(4) << listp->m_dbgId
                            << std::కుడి << " (excluding from concatenation)\n";
            }
        }
        ఒకవేళ (m_logp) *m_logp << '\n';
    }

    శూన్య buildOutputList() {
        // Assign files to buckets and build final list of files

        // At this point the workLists contains concatenatable file lists separated by one or more
        // non-concatenable file lists. Each concatenatable list has N buckets (where N > 0)
        // assigned to it, which have to be filled with files from this list. Ideally, sum of file
        // scores in every bucket should be the same.
        కొరకు (WorkList& list : m_workLists) {
            ఒకవేళ (!list.m_isConcatenable) {
                కొరకు (స్థిరరాశి FilenameWithScore& file : list.m_files) {
                    m_outputFiles.నెట్టు_వెనుక({std::move(file.m_filename), {}});
                }
                కొనసాగు;
            }

            // Ideal bucket score limited to buckets and score of the current Work List.
            స్థిరరాశి uint64_t list_Ideal_పాత్ర_Score = list.m_మొత్తం_Score / list.m_buckets_అంకె;

            std::సదిశ<std::సదిశ<స్థిరరాశి FilenameWithScore*>> buckets;
            స్వయంచాలక fileIt = list.m_files.cbegin();
            కొరకు (పూర్ణాంకం i = 0; i < list.m_buckets_అంకె; ++i) {
                std::సదిశ<స్థిరరాశి FilenameWithScore*> పాత్ర;
                uint64_t పాత్ర_Score = 0;

                కొరకు (; fileIt != list.m_files.cend(); ++fileIt) {
                    స్థిరరాశి uint64_t diffNow
                        = std::నిరపేక్ష(static_cast<int64_t>(list_Ideal_పాత్ర_Score - పాత్ర_Score));
                    స్థిరరాశి uint64_t diffIfAdded = std::నిరపేక్ష(static_cast<int64_t>(
                        list_Ideal_పాత్ర_Score - పాత్ర_Score - fileIt->m_score));
                    ఒకవేళ (పాత్ర_Score == 0 || fileIt->m_score == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it.
                        పాత్ర_Score += fileIt->m_score;
                        పాత్ర.నెట్టు_వెనుక(&*fileIt);
                    } లేకపోతే {
                        // Best possible bucket score reached, process next bucket.
                        ఆపు;
                    }
                }

                // Most likely no bucket will be empty in normal situations. If it happen the
                // bucket will just be dropped, as it creates no output.
                buckets.నెట్టు_వెనుక(std::move(పాత్ర));
            }
            కొరకు (; fileIt != list.m_files.cend(); ++fileIt) {
                // The Work List is out of buckets, but some files were left.
                // Add them to the last bucket.
                buckets.వెనుక().నెట్టు_వెనుక(&*fileIt);
            }
            కొరకు (స్థిరరాశి std::సదిశ<స్థిరరాశి FilenameWithScore*>& పాత్ర : buckets) {
                కూడు_పాత్ర_Groups(పాత్ర);
            }
        }
    }

    శూన్య కూడు_పాత్ర_Groups(స్థిరరాశి std::సదిశ<స్థిరరాశి FilenameWithScore*>& పాత్ర) {
        // Concatenate the files of a bucket, keeping their order. A new group is started
        // whenever the next file would make the current one larger than m_maxGroupBytes. A
        // group with only one file is not concatenated.
        std::సదిశ<std::సదిశ<std::string>> groups;
        uint64_t సమూహం_బైట్లు = 0;
        కొరకు (స్థిరరాశి FilenameWithScore* స్థిరరాశి filep : పాత్ర) {
            ఒకవేళ (groups.ఖాళీలు() || సమూహం_బైట్లు + filep->m_కొలత_బైట్లు > m_max_సమూహం_బైట్లు) {
                groups.emplace_వెనుక();
                సమూహం_బైట్లు = 0;
            }
            సమూహం_బైట్లు += filep->m_కొలత_బైట్లు;
            groups.వెనుక().నెట్టు_వెనుక(filep->m_filename);
        }
        కొరకు (std::సదిశ<std::string>& సమూహం : groups) {
            ఒకవేళ (సమూహం.కొలత() == 1) {
                m_outputFiles.నెట్టు_వెనుక({std::move(సమూహం.front()), {}});
            } లేకపోతే {
                m_outputFiles.నెట్టు_వెనుక({సమూహం_Filename(m_సమూహం_అంకె++), std::move(సమూహం)});
            }
        }
    }

    శూన్య assertFilesSame() స్థిరరాశి {
        స్వయంచాలక ifIt = m_ఆదానం_Files.begin();
        స్వయంచాలక యొక్క_It = m_outputFiles.begin();
        అయితే (ifIt != m_ఆదానం_Files.చివర() && యొక్క_It != m_outputFiles.చివర()) {
            ఒకవేళ (యొక్క_It->isConcatenatingFile()) {
                కొరకు (స్థిరరాశి string& ocf : యొక్క_It->m_concatenatedFilenames) {
                    UASSERT(ifIt != m_ఆదానం_Files.చివర(),
                            "More output files than input files. First extra file: " << ocf);
                    UASSERT(ifIt->m_filename == ocf,
                            "Name mismatch: " << ifIt->m_filename << " != " << ocf);
                    ++ifIt;
                }
                ++యొక్క_It;
            } లేకపోతే {
                UASSERT(ifIt->m_filename == యొక్క_It->m_filename,
                        "Name mismatch: " << ifIt->m_filename << " != " << యొక్క_It->m_filename);
                ++ifIt;
                ++యొక్క_It;
            }
        }
        UASSERT(ifIt == m_ఆదానం_Files.చివర(),
                "More input files than input files. First extra file: " << ifIt->m_filename);
        UASSERT(యొక్క_It == m_outputFiles.చివర(),
                "More output files than input files. First extra file: " << యొక్క_It->m_filename);
    }

    శూన్య process() {
        UINFO(4, __FUNCTION__ << " group file prefix: " << m_సమూహం_File_ఉపసర్గ);
        UINFO(5, "Number of input files: " << m_ఆదానం_Files.కొలత());
        UINFO(5, "Total score: " << m_మొత్తం_Score);

        స్థిరరాశి పూర్ణాంకం మొత్తం_Buckets_అంకె = v3Global.opt.outputGroups();
        UINFO(5, "Number of buckets: " << మొత్తం_Buckets_అంకె);
        UASSERT(మొత్తం_Buckets_అంకె > 0, "More than 0 buckets required");

        ఒకవేళ (fallbackNoGrouping(m_ఆదానం_Files.కొలత())) తిరిగి;

        ఒకవేళ (debug() >= 6 || dumpLevel() >= 6) {
            స్థిరరాశి string filename = v3Global.debugFilename("outputgroup") + ".txt";
            UINFO(5, "Dumping " << filename);
            m_logp = std::unique_ptr<std::ofstream>{V3File::new_ofstream(filename)};
            ఒకవేళ (m_logp->fail()) v3fatal("Can't write file: " << filename);
        }

        ఒకవేళ (m_logp) dumpLogScoreHistogram(*m_logp);

        createWorkLists();

        // Collect stats and mark lists with only one file as non-concatenable
        కొలత_t concatenable_Files_లెక్క = 0;
        int64_t concatenable_Files_మొత్తం_Score = 0;

        కొరకు (WorkList& list : m_workLists) {
            ఒకవేళ (!list.m_isConcatenable) కొనసాగు;

            // "Concatenation" of a single file is pointless
            ఒకవేళ (list.m_files.కొలత() == 1) {
                list.m_isConcatenable = అబద్ధం;
                UINFO(5, "Excluding from concatenation: Work List contains only one file: "
                         "Work List #"
                             << list.m_dbgId);
                కొనసాగు;
            }

            concatenable_Files_లెక్క += list.m_files.కొలత();
            concatenable_Files_మొత్తం_Score += list.m_మొత్తం_Score;
            // This vector is sorted below
            m_concatenable_Lists_By_Desc_కొలత.నెట్టు_వెనుక(&list);
        }

        ఒకవేళ (m_logp) dumpWorkLists(*m_logp);

        // Check concatenation conditions again using more precise data
        ఒకవేళ (fallbackNoGrouping(concatenable_Files_లెక్క)) తిరిగి;

        std::క్రమీకరించు(m_concatenable_Lists_By_Desc_కొలత.begin(), m_concatenable_Lists_By_Desc_కొలత.చివర(),
                  [](స్థిరరాశి WorkList* ap, స్థిరరాశి WorkList* bp) {
                      // Sort in descending order by number of files
                      ఒకవేళ (ap->m_files.కొలత() != bp->m_files.కొలత())
                          తిరిగి (ap->m_files.కొలత() > bp->m_files.కొలత());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      తిరిగి bp->m_మొత్తం_Score > ap->m_మొత్తం_Score;
                  });

        కేటాయించు_Buckets(concatenable_Files_మొత్తం_Score);

        buildOutputList();

        ఒకవేళ (m_logp) dumpOutputList(*m_logp);
        assertFilesSame();
    }

బహిరంగ:
    స్థిర std::సదిశ<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::సదిశ<FilenameWithScore> ఆదానం_Files, uint64_t మొత్తం_Score,
                                స్థిరరాశి std::string& సమూహం_File_ఉపసర్గ, uint64_t max_సమూహం_బైట్లు) {
        Emit_సమూహం సమూహం{std::move(ఆదానం_Files), మొత్తం_Score, సమూహం_File_ఉపసర్గ, max_సమూహం_బైట్లు};
        సమూహం.process();
        తిరిగి సమూహం.m_outputFiles;
    }
};

// ######################################################################
//  Emit statements and expressions

తరగతి EmitMk final {
    ఉపయోగించి FileOrConcatenatedFilesList = Emit_సమూహం::FileOrConcatenatedFilesList;
    ఉపయోగించి FilenameWithScore = Emit_సమూహం::FilenameWithScore;

    // MEMBERS
    ద్విగుణ m_put_Class_లెక్క = 0;  // Number of classEntries printed

బహిరంగ:
    // METHODS

    స్థిర శూన్య emitConcatenatingFile(స్థిరరాశి FileOrConcatenatedFilesList& నమోదు_2) {
        UASSERT(నమోదు_2.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3_బహిర్గతం_CFile concatenatingFile{v3Global.opt.makeDir() + "/" + నమోదు_2.m_filename + ".cpp"};
        concatenatingFile.puts_శీర్షిక();
        కొరకు (స్థిరరాశి string& file : నమోదు_2.m_concatenatedFilenames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    స్థిర uint64_t file_కొలత(స్థిరరాశి AstCFile* cfilep) {
        // All C++ files are already written, so take their size from the file system
        std::ifstream ifs{cfilep->name(), std::ios::ద్విమాన | std::ios::ate};
        UASSERT_OBJ(ifs.is_open(), cfilep, "Cannot open generated file");
        తిరిగి static_cast<uint64_t>(ifs.tellg());
    }

    శూన్య put_Make_Class_నమోదు(V3_బహిర్గతం_Mk_File& యొక్క, స్థిరరాశి string& name) {
        యొక్క.puts("  " + V3Os::filenameNonDirExt(name) + " \\\n");
        ++m_put_Class_లెక్క;
    }

    శూన్య emitClassMake() {
        std::సదిశ<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::సదిశ<FileOrConcatenatedFilesList> vmClassesFastList;
        ఒకవేళ (v3Global.opt.outputGroups() > 0) {
            std::సదిశ<FilenameWithScore> slowFiles;
            std::సదిశ<FilenameWithScore> fastFiles;
            uint64_t slow_మొత్తం_Score = 0;
            uint64_t fast_మొత్తం_Score = 0;
            // Generated headers are part of every translation unit through the precompiled header
            uint64_t శీర్షిక_బైట్లు = 0;

            కొరకు (Ast_కణుపు_File* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), కణుపు_File)) {
                స్థిరరాశి AstCFile* స్థిరరాశి cfilep = VN_CAST(nodep, CFile);
                ఒకవేళ (cfilep && VString::చివర్లు_With(cfilep->name(), ".h")) {
                    శీర్షిక_బైట్లు += file_కొలత(cfilep);
                }
                ఒకవేళ (cfilep && cfilep->ఆధారం() && cfilep->support() == అబద్ధం) {
                    std::సదిశ<FilenameWithScore>& files = cfilep->slow() ? slowFiles : fastFiles;
                    uint64_t& మొత్తం_Score = cfilep->slow() ? slow_మొత్తం_Score : fast_మొత్తం_Score;

                    మొత్తం_Score += cfilep->complexityScore();
                    files.నెట్టు_వెనుక({V3Os::filenameNonDirExt(cfilep->name()),
                                     cfilep->complexityScore(), file_కొలత(cfilep)});
                }
            }
            // Zero size limit means unlimited
            uint64_t max_సమూహం_బైట్లు = std::numeric_limits<uint64_t>::max();
            ఒకవేళ (v3Global.opt.output_Groups_కొలత()) {
                స్థిరరాశి uint64_t max_Tu_బైట్లు = static_cast<uint64_t>(v3Global.opt.output_Groups_కొలత());
                max_సమూహం_బైట్లు = max_Tu_బైట్లు > శీర్షిక_బైట్లు ? max_Tu_బైట్లు - శీర్షిక_బైట్లు : 0;
            }

            vmClassesSlowList = Emit_సమూహం::singleConcatenatedFilesList(
                std::move(slowFiles), slow_మొత్తం_Score, "vm_classes_Slow_", max_సమూహం_బైట్లు);
            vmClassesFastList = Emit_సమూహం::singleConcatenatedFilesList(
                std::move(fastFiles), fast_మొత్తం_Score, "vm_classes_", max_సమూహం_బైట్లు);
        }

        // Generate the makefile
        V3_బహిర్గతం_Mk_File యొక్క{v3Global.opt.makeDir() + "/" + v3Global.opt.ఉపసర్గ() + "_classes.mk"};
        యొక్క.puts_శీర్షిక();
        యొక్క.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        యొక్క.puts("#\n");
        యొక్క.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        యొక్క.puts("# See " + v3Global.opt.ఉపసర్గ() + ".mk" + " for the caller.\n");

        యొక్క.puts("\n### Switches...\n");
        యొక్క.puts("# C11 constructs required?  0/1 (always on now)\n");
        యొక్క.put_సమితి("VM_C11", "1");
        యొక్క.puts("# Timing enabled?  0/1\n");
        యొక్క.put_సమితి("VM_TIMING", v3Global.ఉపయోగిస్తుంది_Timing() ? "1" : "0");
        యొక్క.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        యొక్క.put_సమితి("VM_COVERAGE", v3Global.opt.coverage() ? "1" : "0");
        యొక్క.puts("# Parallel builds?  0/1 (from --output-split)\n");
        యొక్క.put_సమితి("VM_PARALLEL_BUILDS", v3Global.useParallelBuild() ? "1" : "0");
        యొక్క.puts("# Tracing output mode?  0/1 (from --trace-fst/--trace-saif/--trace-vcd)\n");
        యొక్క.put_సమితి("VM_TRACE", v3Global.opt.trace() ? "1" : "0");
        యొక్క.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        యొక్క.put_సమితి("VM_TRACE_FST", v3Global.opt.traceEnabledFst() ? "1" : "0");
        యొక్క.puts("# Tracing output mode in SAIF format?  0/1 (from --trace-saif)\n");
        యొక్క.put_సమితి("VM_TRACE_SAIF", v3Global.opt.traceEnabledSaif() ? "1" : "0");
        యొక్క.puts("# Tracing output mode in VCD format?  0/1 (from --trace-vcd)\n");
        యొక్క.put_సమితి("VM_TRACE_VCD", v3Global.opt.traceEnabledVcd() ? "1" : "0");
        యొక్క.puts("# VPI enabled?  0/1 (from --vpi)\n");
        యొక్క.put_సమితి("VM_VPI", v3Global.opt.vpi() ? "1" : "0");
        // Link flags for runtime VPI library loading are emitted by emitOverallMake() after
        // verilated.mk is included (so $(CFG_LDFLAGS_DYNAMIC)/$(CFG_LDLIBS_DYNAMIC) are defined).

        యొక్క.puts("\n### Object file lists...\n");
        కొరకు (పూర్ణాంకం support = 0; support < 3; ++support) {
            కొరకు (స్థిరరాశి తార్కిక& slow : {అబద్ధం, నిజం}) {
                ఒకవేళ (support == 2) {
                    యొక్క.puts("# Global classes, need linked once per executable");
                } లేకపోతే ఒకవేళ (support) {
                    యొక్క.puts("# Generated support classes");
                } లేకపోతే {
                    యొక్క.puts("# Generated module classes");
                }
                ఒకవేళ (slow) {
                    యొక్క.puts(", non-fast-path, compile with low/medium optimization\n");
                } లేకపోతే {
                    యొక్క.puts(", fast-path, compile with highest optimization\n");
                }
                స్థిరరాశి string లక్ష్యం_Var = (support == 2   ? "VM_GLOBAL"s
                                          : support == 1 ? "VM_SUPPORT"s
                                                         : "VM_CLASSES"s)
                                         + (slow ? "_SLOW" : "_FAST");
                m_put_Class_లెక్క = 0;
                యొక్క.puts(లక్ష్యం_Var + " += \\\n");
                ఒకవేళ (support == 2 && v3Global.opt.hier_బిడ్డ()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } లేకపోతే ఒకవేళ (support == 2 && !slow) {
                    కొరకు (స్థిరరాశి string& cpp : v3Global.verilatedCppFiles())
                        put_Make_Class_నమోదు(యొక్క, cpp);
                } లేకపోతే ఒకవేళ (support == 2 && slow) {
                } లేకపోతే ఒకవేళ (support == 0 && v3Global.opt.outputGroups() > 0) {
                    స్థిరరాశి std::సదిశ<FileOrConcatenatedFilesList>& list
                        = slow ? vmClassesSlowList : vmClassesFastList;
                    కొరకు (స్థిరరాశి FileOrConcatenatedFilesList& నమోదు_2 : list) {
                        ఒకవేళ (నమోదు_2.isConcatenatingFile()) emitConcatenatingFile(నమోదు_2);
                        put_Make_Class_నమోదు(యొక్క, నమోదు_2.m_filename);
                    }
                } లేకపోతే {
                    కొరకు (Ast_కణుపు_File* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), కణుపు_File)) {
                        స్థిరరాశి AstCFile* స్థిరరాశి cfilep = VN_CAST(nodep, CFile);
                        ఒకవేళ (cfilep && cfilep->ఆధారం() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            put_Make_Class_నమోదు(యొక్క, cfilep->name());
                        }
                    }
                }
                యొక్క.puts("\n");
                V3Stats::కూడు_Stat("Makefile targets, " + లక్ష్యం_Var, m_put_Class_లెక్క);
            }
        }

        యొక్క.puts_శీర్షిక();
    }

    శూన్య emitOverallMake() {
        // Generate the makefile
        V3_బహిర్గతం_Mk_File యొక్క{v3Global.opt.makeDir() + "/" + v3Global.opt.ఉపసర్గ() + ".mk"};
        యొక్క.puts_శీర్షిక();
        యొక్క.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        యొక్క.puts("#\n");
        యొక్క.puts("# Execute this makefile from the object directory:\n");
        యొక్క.puts("#    make -f " + v3Global.opt.ఉపసర్గ() + ".mk" + "\n");
        యొక్క.puts("\n");

        ఒకవేళ (v3Global.opt.exe()) {
            యొక్క.puts("default: " + v3Global.opt.exeName() + "\n");
        } లేకపోతే ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            యొక్క.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } లేకపోతే {
            యొక్క.puts("default: lib" + v3Global.opt.ఉపసర్గ() + "\n");
        }
        యొక్క.puts("\n### Constants...\n");
        యొక్క.puts("# Perl executable (from $PERL, defaults to 'perl' if not set)\n");
        యొక్క.put_సమితి("PERL", V3_బహిర్గతం_Formatter::quoteNameControls(V3Options::getenvPERL()));
        యొక్క.puts("# Python3 executable (from $PYTHON3, defaults to 'python3' if not set)\n");
        యొక్క.put_సమితి("PYTHON3", V3_బహిర్గతం_Formatter::quoteNameControls(V3Options::getenvPYTHON3()));
        యొక్క.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        యొక్క.put_సమితి("VERILATOR_ROOT",
                  V3_బహిర్గతం_Formatter::quoteNameControls(V3Options::getenv_VERILATOR_మూలం()));
        యొక్క.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        యొక్క.put_సమితి_Q("SYSTEMC_INCLUDE", V3Options::getenvSYSTEMC_INCLUDE());
        యొక్క.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        యొక్క.put_సమితి_Q("SYSTEMC_LIBDIR", V3Options::getenvSYSTEMC_LIBDIR());

        // Only check it if we really need the value
        ఒకవేళ (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see https://systemc.org\n");
        }

        యొక్క.puts("\n### Switches...\n");
        యొక్క.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        యొక్క.put_సమితి("VM_PROFC", ((v3Global.opt.profC()) ? "1" : "0"));
        యొక్క.puts("# SystemC output mode?  0/1 (from --sc)\n");
        యొక్క.put_సమితి("VM_SC", ((v3Global.opt.systemC()) ? "1" : "0"));
        యొక్క.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        యొక్క.put_సమితి("VM_SP_OR_SC", "$(VM_SC)");
        యొక్క.puts("# Deprecated\n");
        యొక్క.put_సమితి("VM_PCLI", (v3Global.opt.systemC() ? "0" : "1"));
        యొక్క.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        యొక్క.put_సమితి("VM_SC_TARGET_ARCH", V3Options::getenvSYSTEMC_ARCH());

        యొక్క.puts("\n### Vars...\n");
        యొక్క.puts("# Module prefix (from --prefix)\n");
        యొక్క.put_సమితి("VM_MODPREFIX", v3Global.opt.mod_ఉపసర్గ());
        యొక్క.puts("# Design prefix (from --prefix)\n");
        యొక్క.put_సమితి("VM_PREFIX", v3Global.opt.ఉపసర్గ());
        యొక్క.puts("# Design header include, for '#include VM_PREFIX_INCLUDE' (from --prefix)\n");
        యొక్క.put_సమితి("VM_PREFIX_INCLUDE", "<" + v3Global.opt.ఉపసర్గ() + ".h>");
        ఒకవేళ (v3Global.dpi()) {
            యొక్క.puts("# DPI header include, for '#include VM_PREFIX_INCLUDE_DPI';"
                    " undefined if no DPI\n");
            యొక్క.put_సమితి("VM_PREFIX_INCLUDE_DPI", "<" + v3Global.opt.ఉపసర్గ() + "__Dpi.h>");
        }

        యొక్క.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        యొక్క.puts("VM_USER_CFLAGS = \\\n");
        స్థిరరాశి std::string solver = V3Options::getenvVERILATOR_SOLVER();
        ఒకవేళ (v3Global.useRandomizeMethods() && solver != "")
            యొక్క.puts("\t-DVM_SOLVER_DEFAULT='\"" + V3_బహిర్గతం_Formatter::quoteNameControls(solver)
                    + "\"' \\\n");
        ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) యొక్క.puts("\t-fPIC \\\n");
        స్థిరరాశి VStringList& cFlags = v3Global.opt.cFlags();
        కొరకు (స్థిరరాశి string& i : cFlags) యొక్క.puts("  " + i + " \\\n");
        యొక్క.puts("\n");

        యొక్క.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        యొక్క.puts("VM_USER_LDLIBS = \\\n");
        స్థిరరాశి VStringList& ldLibs = v3Global.opt.ldLibs();
        కొరకు (స్థిరరాశి string& i : ldLibs) యొక్క.puts("  " + i + " \\\n");
        యొక్క.puts("\n");

        యొక్క.puts("# User library files (from .a/.o/.so on Verilator command line)\n");
        యొక్క.puts("# These are also in VM_USER_LDLIBS, and are link dependencies\n");
        యొక్క.puts("VM_USER_LDLIBS_FILES = \\\n");
        కొరకు (స్థిరరాశి string& i : v3Global.opt.ldLibFiles()) యొక్క.puts("  " + i + " \\\n");
        యొక్క.puts("\n");

        VString_సమితి dirs;
        యొక్క.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        యొక్క.puts("VM_USER_CLASSES = \\\n");
        స్థిరరాశి VString_సమితి& cppFiles = v3Global.opt.cppFiles();
        కొరకు (స్థిరరాశి స్వయంచాలక& cppfile : cppFiles) {
            యొక్క.puts("  " + V3Os::filenameNonDirExt(cppfile) + " \\\n");
            స్థిరరాశి string dir
                = V3Os::filename_Relative_మార్గం(V3Os::filenameDir(cppfile), v3Global.opt.makeDir());
            dirs.చొప్పించు(dir);
        }
        dirs.చొప్పించు(V3Os::filename_Relative_మార్గం(".", v3Global.opt.makeDir()));

        యొక్క.puts("\n");

        యొక్క.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        యొక్క.puts("VM_USER_DIR = \\\n");
        కొరకు (స్థిరరాశి స్వయంచాలక& i : dirs) యొక్క.puts("  " + i + " \\\n");
        యొక్క.puts("\n");

        యొక్క.puts("### Default rules...\n");
        యొక్క.puts("# Include list of all generated classes\n");
        యొక్క.puts("include " + v3Global.opt.ఉపసర్గ() + "_classes.mk\n");
        ఒకవేళ (v3Global.opt.hier_శిఖరం()) {
            యొక్క.puts("# Include rules for hierarchical blocks\n");
            యొక్క.puts("include " + v3Global.opt.ఉపసర్గ() + "_hier.mk\n");
        }
        యొక్క.puts("# Include global rules\n");
        యొక్క.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        ఒకవేళ (v3Global.opt.exe()) {
            యొక్క.puts("\n### Executable rules... (from --exe)\n");
            యొక్క.puts("VPATH += $(VM_USER_DIR)\n");
            యొక్క.puts("\n");

            ఒకవేళ (v3Global.opt.vpi()) {
                // Runtime VPI library loading (+verilator+vpi+<lib>) needs the executable to
                // export its VPI symbols so the dlopen'd library can resolve them, plus the
                // dl library for dlopen/dlsym.  The exact flags are probed at configure time
                // (CFG_LDFLAGS_DYNAMIC / CFG_LDLIBS_DYNAMIC in verilated.mk).
                యొక్క.puts("# Runtime VPI library loading (+verilator+vpi+) link requirements\n");
                యొక్క.puts("LDFLAGS += $(CFG_LDFLAGS_DYNAMIC)\n");
                యొక్క.puts("LDLIBS += $(CFG_LDLIBS_DYNAMIC)\n");
                యొక్క.puts("\n");
            }
        }

        స్థిరరాశి string compilerIncludePch
            = v3Global.opt.compilerIncludes().ఖాళీలు() ? "" : "$(VK_PCH_H).fast.gch";
        స్థిరరాశి string compilerIncludeFlag
            = v3Global.opt.compilerIncludes().ఖాళీలు() ? "" : "$(VK_PCH_I_FAST)";
        కొరకు (స్థిరరాశి string& cppfile : cppFiles) {
            స్థిరరాశి string basename = V3Os::filenameNonDirExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            యొక్క.puts(basename + ".o: " + cppfile + " " + compilerIncludePch + "\n");

            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            యొక్క.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) "
                    + compilerIncludeFlag + " -c -o $@ $<\n");
        }

        ఒకవేళ (v3Global.opt.exe()) {
            యొక్క.puts("\n### Link rules... (from --exe)\n");
            // '{prefix}__ALL.a', contains all hierarchical libraries
            యొక్క.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a"
                      " $(VM_USER_LDLIBS_FILES)\n");
            // User library files are in LDLIBS, so do not also link them from $^
            యొక్క.puts("\t$(LINK) $(LDFLAGS) $(filter-out $(VM_USER_LDLIBS_FILES),$^) $(LOADLIBES) "
                    "$(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            యొక్క.puts("\n");
        } లేకపోతే ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            స్థిరరాశి string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            యొక్క.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            యొక్క.puts(v3Global.opt.libCreateName(అబద్ధం) + ": " + libCreateDeps + "\n");
            యొక్క.puts("\n");
            ఒకవేళ (v3Global.opt.hier_బిడ్డ()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                యొక్క.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(అబద్ధం)
                        + "\n");
            } లేకపోతే {
                యొక్క.puts(v3Global.opt.libCreateName(నిజం) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                యొక్క.puts("ifeq ($(shell uname -s),Darwin)\n");
                యొక్క.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared $(LDFLAGS) -flat_namespace -o $@ $^ $(LDLIBS) "
                        "$(LIBS)\n");
                యొక్క.puts("else\n");
                యొక్క.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared "
                        "$(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBS)\n");
                యొక్క.puts("endif\n");
                యొక్క.puts("\n");
                యొక్క.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(అబద్ధం)
                        + " " + v3Global.opt.libCreateName(నిజం) + "\n");
            }
        } లేకపోతే {
            స్థిరరాశి string libname = "lib" + v3Global.opt.ఉపసర్గ() + ".a";
            యొక్క.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            యొక్క.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            యొక్క.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            యొక్క.puts("lib" + v3Global.opt.ఉపసర్గ() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        యొక్క.puts_శీర్షిక();
    }

    స్పష్టమైన EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    మాయా ~EmitMk() = అప్రమేయ;
};

//######################################################################

తరగతి EmitMkHierVerilation final {
    స్థిరరాశి V3_Hier_లేఖాచిత్రం* స్థిరరాశి m_graphp;
    స్థిరరాశి string m_makefile;  // path of this makefile
    శూన్య emitCommonOpts(V3_బహిర్గతం_Mk_File& యొక్క) స్థిరరాశి {
        యొక్క.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        యొక్క.puts("VM_HIER_RUN_DIR := " + V3Os::cwd() + "\n");
        యొక్క.puts("# Common options for hierarchical blocks\n");
        స్థిరరాశి string fullpath_bin = V3Os::filename_Real_మార్గం(v3Global.opt.buildDepBin());
        స్థిరరాశి string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        యొక్క.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        యొక్క.puts("VM_HIER_INPUT_FILES := \\\n");
        కొరకు (స్థిరరాశి స్వయంచాలక& i : v3Global.opt.vFiles())
            యొక్క.puts("  " + V3Os::filename_Real_మార్గం(i.filename()) + " \\\n");
        యొక్క.puts("\n");
        యొక్క.puts("VM_HIER_VERILOG_LIBS := \\\n");
        కొరకు (స్థిరరాశి స్వయంచాలక& i : v3Global.opt.libraryFiles()) {
            యొక్క.puts("  " + V3Os::filename_Real_మార్గం(i.filename()) + " \\\n");
        }
        యొక్క.puts("\n");
    }
    శూన్య emitLaunchVerilator(V3_బహిర్గతం_Mk_File& యొక్క, స్థిరరాశి string& వాదనలు_File) స్థిరరాశి {
        యొక్క.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        యొక్క.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + వాదనలు_File + "\"\n");
    }
    శూన్య emit(V3_బహిర్గతం_Mk_File& యొక్క) స్థిరరాశి {
        యొక్క.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        యొక్క.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        యొక్క.puts("#\n");
        యొక్క.puts("# The main makefile " + v3Global.opt.ఉపసర్గ() + ".mk calls this makefile\n");
        యొక్క.puts("\n");
        యొక్క.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        యొక్క.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        యొక్క.puts(".SUFFIXES:\n");
        యొక్క.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        యొక్క.puts("# Libraries of hierarchical blocks\n");
        యొక్క.puts("VM_HIER_LIBS := \\\n");
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : m_graphp->vertices()) {
            స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి blockp = vtx.as<V3_Hier_ఖండం>();
            యొక్క.puts("  " + blockp->hierLibFilename(నిజం) + " \\\n");
        }
        యొక్క.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        యొక్క.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.ఉపసర్గ() + ".mk\n");
        యొక్క.puts("\t$(MAKE) -f " + v3Global.opt.ఉపసర్గ() + ".mk\n");
        యొక్క.puts("hier_verilation: " + v3Global.opt.ఉపసర్గ() + ".mk\n");
        emitCommonOpts(యొక్క);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        యొక్క.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        యొక్క.puts("hier_launch_verilator:\n");
        యొక్క.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            స్థిరరాశి string వాదనలు_File = v3Global.hierGraphp()->శిఖరం_Command_వాదనలు_Filename(అబద్ధం);
            యొక్క.puts("\n# Verilate the top module\n");
            యొక్క.puts(v3Global.opt.ఉపసర్గ()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            యొక్క.puts(V3Os::filenameNonDir(వాదనలు_File) + " ");
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : m_graphp->vertices()) {
                స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి blockp = vtx.as<V3_Hier_ఖండం>();
                యొక్క.puts(blockp->hierWrapperFilename(నిజం) + " ");
            }
            యొక్క.puts("\n");
            emitLaunchVerilator(యొక్క, వాదనలు_File);
        }

        // Rules to process hierarchical blocks
        యొక్క.puts("\n# Verilate hierarchical blocks\n");
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : m_graphp->vertices()) {
            స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి blockp = vtx.as<V3_Hier_ఖండం>();
            స్థిరరాశి string ఉపసర్గ = blockp->hier_ఉపసర్గ();
            స్థిరరాశి string వాదనలు_Filename = blockp->command_వాదనలు_Filename(అబద్ధం);
            యొక్క.puts(blockp->hierGeneratedFilenames(నిజం));
            యొక్క.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            యొక్క.puts(V3Os::filenameNonDir(వాదనలు_Filename) + " ");
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : blockp->బహిర్గతం_అంచులు()) {
                స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి dependencyp = అంచు.శిఖరం()->as<V3_Hier_ఖండం>();
                యొక్క.puts(dependencyp->hierWrapperFilename(నిజం) + " ");
            }
            యొక్క.puts("\n");
            emitLaunchVerilator(యొక్క, వాదనలు_Filename);

            // Rule to build lib*.a
            యొక్క.puts(blockp->hierLibFilename(నిజం));
            యొక్క.puts(": ");
            యొక్క.puts(blockp->hierMkFilename(నిజం));
            యొక్క.puts(" ");
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : blockp->బహిర్గతం_అంచులు()) {
                స్థిరరాశి V3_Hier_ఖండం* స్థిరరాశి dependencyp = అంచు.శిఖరం()->as<V3_Hier_ఖండం>();
                యొక్క.puts(dependencyp->hierLibFilename(నిజం));
                యొక్క.puts(" ");
            }
            యొక్క.puts("\n\t$(MAKE) -f " + blockp->hierMkFilename(అబద్ధం) + " -C " + ఉపసర్గ);
            యొక్క.puts(" VM_PREFIX=" + ఉపసర్గ);
            యొక్క.puts("\n\n");
        }
        యొక్క.puts("endif  # Guard\n");
    }

బహిరంగ:
    స్పష్టమైన EmitMkHierVerilation(స్థిరరాశి V3_Hier_లేఖాచిత్రం* graphp)
        : m_graphp{graphp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.ఉపసర్గ() + "_hier.mk"} {
        V3_బహిర్గతం_Mk_File యొక్క{m_makefile};
        emit(యొక్క);
    }
};

//######################################################################
// Gate class functions

శూన్య V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ":");
    స్థిరరాశి EmitMk emitter;
}

శూన్య V3EmitMk::emitHierVerilation(స్థిరరాశి V3_Hier_లేఖాచిత్రం* graphp) {
    UINFO(2, __FUNCTION__ << ":");
    EmitMkHierVerilation{graphp};
}
