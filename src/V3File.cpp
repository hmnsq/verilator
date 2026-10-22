// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"

#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <cerrno>
#ಅನ್ತರ್ಭಾವ <cstdarg>
#ಅನ್ತರ್ಭಾವ <fcntl.h>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>

#ಅನ್ತರ್ಭಾವ <sys/stat.h>
#ಅನ್ತರ್ಭಾವ <sys/types.h>

// clang-format off
#ಆದರೆ defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#ಇಲ್ಲದಿದ್ದರೆ
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ INFILTER_PIPE
# include <sys/wait.h>
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr ಪೂರ್ಣಾಂಕ INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr ಪೂರ್ಣಾಂಕ INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_ಗಾತ್ರ_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

ವರ್ಗ V3FileDependImp final {
    // TYPES
    ವರ್ಗ DependFile final {
        // A single file
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_ಗುರಿ;  // True if write, else read
        ತಾರ್ಕಿಕ m_exists = ನಿಜ;  // File exists
        ಸ್ಥಿರಾಂಕ string m_filename;  // Filename
        ರಚನೆ stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    ಸಾರ್ವಜನಿಕ:
        DependFile(ಸ್ಥಿರಾಂಕ string& filename, ತಾರ್ಕಿಕ ಗುರಿ)
            : m_ಗುರಿ{ಗುರಿ}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = ಪೂರ್ವನಿಯೋಜಿತ;
        ಸ್ಥಿರಾಂಕ string& filename() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_filename; }
        ತಾರ್ಕಿಕ ಗುರಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಗುರಿ; }
        ತಾರ್ಕಿಕ exists() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_exists; }
        off_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stat.st_ಗಾತ್ರ; }
        ino_t ino() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stat.st_ino; }
        ಸಮಯ_t cstime() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stat.st_ctime; }  // Seconds
        ಸಮಯ_t cnstime() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        ಸಮಯ_t mstime() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stat.st_mtime; }  // Seconds
        ಸಮಯ_t mnstime() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_ಚಿಹ್ನೆ() {
            ಸ್ಥಿರ VHashSha512 s_ಖಾಲಿಗಳು_Hash;
            ಹಿಂತಿರುಗಿ m_hash.digestSymbol24() != s_ಖಾಲಿಗಳು_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        ಶೂನ್ಯ ತುಂಬು_Stats() {
            ಆದರೆ (!m_stat.st_mtime) {
                ಸ್ಥಿರಾಂಕ string fn = filename();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ err = stat(fn.c_str(), &m_stat);
                ಆದರೆ (err != 0) {
                    std::memset(&m_stat, 0, ಗಾತ್ರ(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = ಸುಳ್ಳು;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    ಆದರೆ (!ಗುರಿ() && ಗಾತ್ರ_2() <= FILE_HASH_ಗಾತ್ರ_MAX) m_hash.ಸೇರಿಸು_File(filename());
                }
            }
        }
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ DependFile& rhs) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::ಗಣ<string> m_filename_ಗಣ VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::ಗಣ<DependFile> m_filenameList;  // Files sourced/generated

    ಸ್ಥಿರ string stripQuotes(ಸ್ಥಿರಾಂಕ string& ಒಳ) {
        string pretty = ಒಳ;
        string::ಗಾತ್ರ_type pos;
        ಆಗ ((pos = pretty.ಕಂಡುಹಿಡಿ('\"')) != string::npos) pretty.replace(pos, 1, "_");
        ಆಗ ((pos = pretty.ಕಂಡುಹಿಡಿ('\n')) != string::npos) pretty.replace(pos, 1, "_");
        ಹಿಂತಿರುಗಿ pretty;
    }

ಸಾರ್ವಜನಿಕ:
    // ACCESSOR METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_Src_Depend(ಸ್ಥಿರಾಂಕ string& filename) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it_ಸಿಕ್ಕಿತು_Pair = m_filename_ಗಣ.ಸೇರಿಸು(filename);
        ಆದರೆ (it_ಸಿಕ್ಕಿತು_Pair.ಎರಡನೇ) {
            DependFile df{filename, ಸುಳ್ಳು};
            df.ತುಂಬು_Stats();  // Get size now, in case changes during the run
            m_filenameList.ಸೇರಿಸು(df);
        }
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Tgt_Depend(ಸ್ಥಿರಾಂಕ string& filename) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it_ಸಿಕ್ಕಿತು_Pair = m_filename_ಗಣ.ಸೇರಿಸು(filename);
        ಆದರೆ (it_ಸಿಕ್ಕಿತು_Pair.ಎರಡನೇ) m_filenameList.ಸೇರಿಸು(DependFile{filename, ನಿಜ});
    }
    ಶೂನ್ಯ writeDepend(ಸ್ಥಿರಾಂಕ string& filename);
    std::ಸದಿಶ<string> getAllDeps() ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ writeTimes(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& cmdline_ಒಳ);
    ತಾರ್ಕಿಕ checkTimes(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& cmdline_ಒಳ);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

ಶೂನ್ಯ V3FileDependImp::writeDepend(ಸ್ಥಿರಾಂಕ string& filename) {
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ಆದರೆ (ofp->fail()) v3fatal("Can't write file: " << filename);

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ DependFile& i : m_filenameList) {
        ಆದರೆ (i.ಗುರಿ()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ DependFile& i : m_filenameList) {
        ಆದರೆ (!i.ಗುರಿ()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    ಆದರೆ (v3Global.opt.makePhony()) {
        *ofp << '\n';
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ DependFile& i : m_filenameList) {
            ಆದರೆ (!i.ಗುರಿ()) *ofp << i.filename() << ":\n";
        }
    }
}

std::ಸದಿಶ<string> V3FileDependImp::getAllDeps() ಸ್ಥಿರಾಂಕ {
    std::ಸದಿಶ<string> r;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_filenameList) {
        ಆದರೆ (!itr.ಗುರಿ() && itr.exists()) r.ತಳ್ಳು_ಹಿಂಭಾಗ(itr.filename());
    }
    ಹಿಂತಿರುಗಿ r;
}

ಶೂನ್ಯ V3FileDependImp::writeTimes(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& cmdline_ಒಳ) {
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ಆದರೆ (ofp->fail()) v3fatal("Can't write file: " << filename);

    ಸ್ಥಿರಾಂಕ string cmdline = stripQuotes(cmdline_ಒಳ);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& it : m_filenameList) {
        DependFile* ಸ್ಥಿರಾಂಕ depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->ತುಂಬು_Stats();
        off_t show_ಗಾತ್ರ = depfilep->ಗಾತ್ರ_2();
        ino_t showIno = depfilep->ino();
        ಆದರೆ (depfilep->filename() == filename) {
            show_ಗಾತ್ರ = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->ಗುರಿ() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_ಗಾತ್ರ;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_ಚಿಹ್ನೆ() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

ತಾರ್ಕಿಕ V3FileDependImp::checkTimes(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& cmdline_ಒಳ) {
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    ಆದರೆ (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    {
        ಸ್ಥಿರಾಂಕ string ignore = V3Os::getline(*ifp);
        ಆದರೆ (ignore.ಖಾಲಿಗಳು()) { /*ಬಳಸಿದ*/
        }
    }
    {
        ಅಕ್ಷರ chkDir;
        *ifp >> chkDir;
        ಅಕ್ಷರ quote;
        *ifp >> quote;
        ಸ್ಥಿರಾಂಕ string chkCmdline = V3Os::getline(*ifp, '"');
        ಸ್ಥಿರಾಂಕ string cmdline = stripQuotes(cmdline_ಒಳ);
        ಆದರೆ (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
    }

    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").ಖಾಲಿಗಳು();

    ಆಗ (!ifp->eof()) {
        ಅಕ್ಷರ chkDir;
        *ifp >> chkDir;
        off_t chk_ಗಾತ್ರ;
        *ifp >> chk_ಗಾತ್ರ;
        ino_t chkIno;
        *ifp >> chkIno;
        ಆದರೆ (ifp->eof()) ನಿಲ್ಲಿಸು;  // Needed to read final whitespace before found eof
        ಸಮಯ_t chkCstime;
        *ifp >> chkCstime;
        ಸಮಯ_t chkCnstime;
        *ifp >> chkCnstime;
        ಸಮಯ_t chkMstime;
        *ifp >> chkMstime;
        ಸಮಯ_t chkMnstime;
        *ifp >> chkMnstime;
        ಅಕ್ಷರ quote;
        *ifp >> quote;
        ಸ್ಥಿರಾಂಕ string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ಸ್ಥಿರಾಂಕ string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ರಚನೆ stat curStat;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ err = stat(chkFilename.c_str(), &curStat);
        ಆದರೆ (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        ಆದರೆ (filename == chkFilename) ಮುಂದುವರಿ;

        UINFO(9, " got d=" << chkDir << " s=" << chk_ಗಾತ್ರ << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_ಗಾತ್ರ << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        ಆದರೆ (!(curStat.st_ಗಾತ್ರ >= chk_ಗಾತ್ರ && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_ಗಾತ್ರ << "=?" << chk_ಗಾತ್ರ << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            ಆದರೆ (skipHashing) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            ಆದರೆ (chkHash == "unhashed") ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

            VHashSha512 curHash;
            curHash.ಸೇರಿಸು_File(chkFilename);
            ಆದರೆ (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

//######################################################################
// V3File

ಶೂನ್ಯ V3File::ಕೂಡಿಸು_Src_Depend(ಸ್ಥಿರಾಂಕ string& filename) VL_MT_ಸುರಕ್ಷಿತ { dependImp.ಕೂಡಿಸು_Src_Depend(filename); }
ಶೂನ್ಯ V3File::ಕೂಡಿಸು_Tgt_Depend(ಸ್ಥಿರಾಂಕ string& filename) VL_MT_ಸುರಕ್ಷಿತ { dependImp.ಕೂಡಿಸು_Tgt_Depend(filename); }
ಶೂನ್ಯ V3File::writeDepend(ಸ್ಥಿರಾಂಕ string& filename) { dependImp.writeDepend(filename); }
std::ಸದಿಶ<string> V3File::getAllDeps() { ಹಿಂತಿರುಗಿ dependImp.getAllDeps(); }
ಶೂನ್ಯ V3File::writeTimes(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& cmdline_ಒಳ) {
    dependImp.writeTimes(filename, cmdline_ಒಳ);
}
ತಾರ್ಕಿಕ V3File::checkTimes(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& cmdline_ಒಳ) {
    ಹಿಂತಿರುಗಿ dependImp.checkTimes(filename, cmdline_ಒಳ);
}
ಶೂನ್ಯ V3File::createMakeDirFor(ಸ್ಥಿರಾಂಕ string& filename) {
    ಆದರೆ (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().ಉದ್ದಳತೆ() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
ಶೂನ್ಯ V3File::createMakeDir() {
    ಸ್ಥಿರ ತಾರ್ಕಿಕ s_created = ಸುಳ್ಳು;
    ಆದರೆ (!s_created) {
        s_created = ನಿಜ;
        V3Os::createDir(v3Global.opt.makeDir());
        ಆದರೆ (v3Global.opt.hier_ತುದಿ()) V3Os::createDir(v3Global.opt.hier_ತುದಿ_ದತ್ತಾಂಶ_Dir());
    }
}

//######################################################################
// VInFilterImp

ವರ್ಗ VInFilterImp final {
    ಬಳಸಿ StrList = VInFilter::StrList;

    std::map<ಸ್ಥಿರಾಂಕ std::string, std::string> m_contentsMap;  // Cache of file contents
    ತಾರ್ಕಿಕ m_readEof = ಸುಳ್ಳು;  // Received EOF on read
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#ಇಲ್ಲದಿದ್ದರೆ
    ಪೂರ್ಣಾಂಕ m_pid = 0;  // fork() process id - always zero as disabled
#ಆದರೆ_ಅಂತ್ಯ
    ತಾರ್ಕಿಕ m_pidExited = ಸುಳ್ಳು;  //  waitpid() reaped the filter process
    ಪೂರ್ಣಾಂಕ m_pid_ಸ್ಥಿತಿ = 0;  // Exit status of the filter process
    ಪೂರ್ಣಾಂಕ m_writeFd = 0;  // File descriptor TO filter
    ಪೂರ್ಣಾಂಕ m_readFd = 0;  // File descriptor FROM filter

ಖಾಸಗಿ:
    // METHODS

    ತಾರ್ಕಿಕ readContents(ಸ್ಥಿರಾಂಕ string& filename, StrList& outl) {
        ಆದರೆ (m_pid) {
            ಹಿಂತಿರುಗಿ readContentsFilter(filename, outl);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ readContentsFile(filename, outl);
        }
    }
    ತಾರ್ಕಿಕ readContentsFile(ಸ್ಥಿರಾಂಕ string& filename, StrList& outl) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ fd = open(filename.c_str(), O_RDONLY);
        ಆದರೆ (fd < 0) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        m_readEof = ಸುಳ್ಳು;
        read_ಖಂಡಗಳು(fd, -1, outl);
        close(fd);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    // cppcheck-suppress constParameterReference
    ತಾರ್ಕಿಕ readContentsFilter(ಸ್ಥಿರಾಂಕ string& filename, StrList& outl) {
        (ಶೂನ್ಯ)filename;  // Prevent unused variable warning
        (ಶೂನ್ಯ)outl;  // Prevent unused variable warning
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ಸ್ಥಿರಾಂಕ string ಸಾಲು = read_Filter_ಸಾಲು();
        ಆದರೆ (ಸಾಲು.ಕಂಡುಹಿಡಿ("Content-Length") != string::npos) {
            ಪೂರ್ಣಾಂಕ len = 0;
            ಅಕ್ಷರಸರಣಿ_ಓದು(ಸಾಲು.c_str(), "Content-Length: %d\n", &len);
            read_ಖಂಡಗಳು(m_readFd, len, outl);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (ಸಾಲು != "") v3error("--pipe-filter protocol error, unexpected: " << ಸಾಲು);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
#ಇಲ್ಲದಿದ್ದರೆ
        v3fatalSrc("--pipe-filter not implemented on this platform");
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
#ಆದರೆ_ಅಂತ್ಯ
    }

    // cppcheck-suppress functionConst
    ಶೂನ್ಯ checkFilter(ತಾರ್ಕಿಕ hang) {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ INFILTER_PIPE
        ಆದರೆ (!m_pidExited && waitpid(m_pid, &m_pid_ಸ್ಥಿತಿ, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_ಸ್ಥಿತಿ << " exit=" << WEXITSTATUS(m_pid_ಸ್ಥಿತಿ) << " err"
                         << std::strerror(errno));
            m_readEof = ನಿಜ;
            m_pidExited = ನಿಜ;
        }
#ಆದರೆ_ಅಂತ್ಯ
    }

    ಶೂನ್ಯ read_ಖಂಡಗಳು(ಪೂರ್ಣಾಂಕ fd, ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2, StrList& outl) {
        ಅಕ್ಷರ buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        ಆಗ (!m_readEof && (ಗಾತ್ರ_2 < 0 || ಗಾತ್ರ_2 > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            ಆದರೆ (ಗಾತ್ರ_2 > 0 && ಗಾತ್ರ_2 < todo) todo = ಗಾತ್ರ_2;
            errno = 0;
            ಸ್ಥಿರಾಂಕ ssize_t got = ಪಠ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ಆದರೆ (got > 0) {
                outl.emplace_ಹಿಂಭಾಗ(string(buf, got));
                sizegot += got;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (errno == EINTR || errno == EAGAIN
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ EWOULDBLOCK
                       || errno == EWOULDBLOCK
#ಆದರೆ_ಅಂತ್ಯ
            ) {
                checkFilter(ಸುಳ್ಳು);
                V3Os::u_sleep(1000);
                ಮುಂದುವರಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_readEof = ನಿಜ;
                ನಿಲ್ಲಿಸು;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_ಸಾಲು() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string ಸಾಲು;
        ಆಗ (!m_readEof) {
            StrList outl;
            read_ಖಂಡಗಳು(m_readFd, 1, outl);
            ಸ್ಥಿರಾಂಕ string onechar = listString(outl);
            ಸಾಲು += onechar;
            ಆದರೆ (onechar == "\n") {
                ಆದರೆ (ಸಾಲು == "\n") {
                    ಸಾಲು = "";
                    ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        UINFO(6, "filter-line-in: " << ಸಾಲು);
        ಹಿಂತಿರುಗಿ ಸಾಲು;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    ಶೂನ್ಯ writeFilter(ಸ್ಥಿರಾಂಕ string& ಹೊರಹೋಗು) {
        ಆದರೆ (debug() >= 6) {
            UINFO(6, "filter-out: " << ಹೊರಹೋಗು);
            ಆದರೆ (ಹೊರಹೋಗು[ಹೊರಹೋಗು.ಉದ್ದಳತೆ() - 1] != '\n') ಹೊರಗೆ << '\n';
        }
        ಆದರೆ (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = ನಿಜ;
            stop();
        }
        ಚಿಹ್ನೆರಹಿತ ಸ್ಥಾನಾಂತರ = 0;
        ಆಗ (!m_readEof && ಹೊರಹೋಗು.ಉದ್ದಳತೆ() > ಸ್ಥಾನಾಂತರ) {
            errno = 0;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ got = ಲಿಖ(m_writeFd, (ಹೊರಹೋಗು.c_str()) + ಸ್ಥಾನಾಂತರ, ಹೊರಹೋಗು.ಉದ್ದಳತೆ() - ಸ್ಥಾನಾಂತರ);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ಆದರೆ (got > 0) {
                ಸ್ಥಾನಾಂತರ += got;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (errno == EINTR || errno == EAGAIN
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ EWOULDBLOCK
                       || errno == EWOULDBLOCK
#ಆದರೆ_ಅಂತ್ಯ
            ) {
                checkFilter(ಸುಳ್ಳು);
                V3Os::u_sleep(1000);
                ಮುಂದುವರಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ನಿಲ್ಲಿಸು;
            }
        }
    }

    // Start the filter
    ಶೂನ್ಯ ಆರಂಭ(ಸ್ಥಿರಾಂಕ string& command) {
        ಆದರೆ (command == "") {
            m_pid = 0;  // Disabled
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆರಂಭ_Filter(command);
        }
    }
    ಶೂನ್ಯ ಆರಂಭ_Filter(ಸ್ಥಿರಾಂಕ string& command) {
        (ಶೂನ್ಯ)command;  // Prevent Unused variable warning
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ INFILTER_PIPE
        ಪೂರ್ಣಾಂಕ fd_stdin[2];  // Can't use std::array
        ಪೂರ್ಣಾಂಕ fd_stdout[2];  // Can't use std::array
        constexpr ಪೂರ್ಣಾಂಕ P_RD = 0;
        constexpr ಪೂರ್ಣಾಂಕ P_WR = 1;

        ಆದರೆ (VL_UNCOVERABLE(ಪ್ರಣಾಲ(fd_stdin) != 0 || ಪ್ರಣಾಲ(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ಆದರೆ (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ಸ್ಥಿರಾಂಕ pid_t pid = fork();
        ಆದರೆ (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ಆದರೆ (pid == 0) {  //ಮಗು
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<ಅಕ್ಷರ*>(ಶೂನ್ಯಸೂಚಕ));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } ಇಲ್ಲದಿದ್ದರೆ {  //ಪೋಷಕ
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = ಸುಳ್ಳು;
            m_pid_ಸ್ಥಿತಿ = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = ಸುಳ್ಳು;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            ಪೂರ್ಣಾಂಕ flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#ಇಲ್ಲದಿದ್ದರೆ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#ಆದರೆ_ಅಂತ್ಯ
    }

    ಶೂನ್ಯ stop() {
        ಆದರೆ (m_pid) stopFilter();
    }
    ಶೂನ್ಯ stopFilter() {
        UINFO(6, "Stopping filter process");
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ INFILTER_PIPE
        close(m_writeFd);
        checkFilter(ನಿಜ);
        ಆದರೆ (!WIFEXITED(m_pid_ಸ್ಥಿತಿ) || WEXITSTATUS(m_pid_ಸ್ಥಿತಿ) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#ಇಲ್ಲದಿದ್ದರೆ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#ಆದರೆ_ಅಂತ್ಯ
    }

ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ ವರ್ಗ VInFilter;
    // Read file contents and return it
    ತಾರ್ಕಿಕ readWholefile(ಸ್ಥಿರಾಂಕ string& filename, StrList& outl) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_contentsMap.ಕಂಡುಹಿಡಿ(filename);
        ಆದರೆ (it != m_contentsMap.ಕೊನೆ()) {
            outl.ತಳ್ಳು_ಹಿಂಭಾಗ(it->ಎರಡನೇ);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (!readContents(filename, outl)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (list_ಗಾತ್ರ(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಸ್ಥಿರ ಗಾತ್ರ_t list_ಗಾತ್ರ(ಸ್ಥಿರಾಂಕ StrList& sl) {
        ಗಾತ್ರ_t ಫಲಿತಾಂಶ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : sl) ಫಲಿತಾಂಶ += i.ಉದ್ದಳತೆ();
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
    ಸ್ಥಿರ string listString(ಸ್ಥಿರಾಂಕ StrList& sl) {
        string ಫಲಿತಾಂಶ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : sl) ಫಲಿತಾಂಶ += i;
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ VInFilterImp(ಸ್ಥಿರಾಂಕ string& command) { ಆರಂಭ(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ಸ್ಥಿರಾಂಕ string& command) { m_impp = ಹೊಸ VInFilterImp{command}; }
VInFilter::~VInFilter() {
    ಆದರೆ (m_impp) VL_DO_CLEAR(ಅಳಿಸು m_impp, m_impp = ಶೂನ್ಯಸೂಚಕ);
}

ತಾರ್ಕಿಕ VInFilter::readWholefile(ಸ್ಥಿರಾಂಕ string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    ಹಿಂತಿರುಗಿ m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_ಹೊರಹೋಗು_Formatter::V3_ಹೊರಹೋಗು_Formatter(V3_ಹೊರಹೋಗು_Formatter::Language lang)
    : m_lang{lang} {
    m_ಖಂಡ_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_ಅಗಲ = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> ಕೋಷ್ಟಕ;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i <= MAXSPACE; ++i) ಕೋಷ್ಟಕ[i] = std::string(static_cast<ಗಾತ್ರ_t>(i), ' ');
    ಹಿಂತಿರುಗಿ ಕೋಷ್ಟಕ;
}();

ಸ್ಥಿರಾಂಕ std::string& V3_ಹೊರಹೋಗು_Formatter::indentSpaces(ಪೂರ್ಣಾಂಕ ಅಂಕೆ) {
    // Indent the specified number of spaces.
    ಹಿಂತಿರುಗಿ s_indentSpaces[std::max(0, std::min(ಅಂಕೆ, MAXSPACE))];
}

ತಾರ್ಕಿಕ V3_ಹೊರಹೋಗು_Formatter::token_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cmp) {
    ಆಗ (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    ಆದರೆ (*cmp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (*cp && !std::ಖಾಲಿಯೇ(*cp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

ತಾರ್ಕಿಕ V3_ಹೊರಹೋಗು_Formatter::token_ಆರಂಭ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp) {
    ಹಿಂತಿರುಗಿ (token_ಹೊಂದಿಕೆ(cp, "begin") || token_ಹೊಂದಿಕೆ(cp, "case") || token_ಹೊಂದಿಕೆ(cp, "casex")
            || token_ಹೊಂದಿಕೆ(cp, "casez") || token_ಹೊಂದಿಕೆ(cp, "class") || token_ಹೊಂದಿಕೆ(cp, "function")
            || token_ಹೊಂದಿಕೆ(cp, "interface") || token_ಹೊಂದಿಕೆ(cp, "module") || token_ಹೊಂದಿಕೆ(cp, "package")
            || token_ಹೊಂದಿಕೆ(cp, "task"));
}

ತಾರ್ಕಿಕ V3_ಹೊರಹೋಗು_Formatter::token_ಕೊನೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp) {
    ಹಿಂತಿರುಗಿ (token_ಹೊಂದಿಕೆ(cp, "end") || token_ಹೊಂದಿಕೆ(cp, "endcase") || token_ಹೊಂದಿಕೆ(cp, "endclass")
            || token_ಹೊಂದಿಕೆ(cp, "endfunction") || token_ಹೊಂದಿಕೆ(cp, "endinterface")
            || token_ಹೊಂದಿಕೆ(cp, "endmodule") || token_ಹೊಂದಿಕೆ(cp, "endpackage")
            || token_ಹೊಂದಿಕೆ(cp, "endtask"));
}

ತಾರ್ಕಿಕ V3_ಹೊರಹೋಗು_Formatter::token_Not_ಆರಂಭ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp) {
    ಹಿಂತಿರುಗಿ (token_ಹೊಂದಿಕೆ(cp, "export") || token_ಹೊಂದಿಕೆ(cp, "import"));
}

ಪೂರ್ಣಾಂಕ V3_ಹೊರಹೋಗು_Formatter::ಕೊನೆ_ಮಟ್ಟಗಳು(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg) {
    ಪೂರ್ಣಾಂಕ ಮಟ್ಟಗಳು = m_indentLevel;
    {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = strg;
        ಆಗ (std::ಖಾಲಿಯೇ(*cp)) ++cp;
        ಆಯ್ಕೆ (*cp) {
        ಸ್ಥಿತಿ '\n':  // Newlines.. No need for whitespace before it
            ಹಿಂತಿರುಗಿ 0;
        ಸ್ಥಿತಿ '#':  // Preproc directive
            ಆದರೆ (m_lang == LA_C) ಹಿಂತಿರುಗಿ 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* mp = cp;
            ಗಾಗಿ (; std::ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(*mp); ++mp) {}
            ಆದರೆ (mp[0] == ':' && mp[1] != ':') ಹಿಂತಿರುಗಿ (ಮಟ್ಟಗಳು - m_ಖಂಡ_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = strg; *cp; ++cp) {
        ಆಯ್ಕೆ (*cp) {
        ಸ್ಥಿತಿ '}':
        ಸ್ಥಿತಿ ')': ಮಟ್ಟಗಳು -= m_ಖಂಡ_Indent; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '<':
            ಆದರೆ (m_lang == LA_XML) {
                ಆದರೆ (cp[1] == '/') ಮಟ್ಟಗಳು -= m_ಖಂಡ_Indent;
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 'e':
            ಆದರೆ (m_lang == LA_VERILOG && token_ಕೊನೆ(cp)) ಮಟ್ಟಗಳು -= m_ಖಂಡ_Indent;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '\t':
        ಸ್ಥಿತಿ ' ': ನಿಲ್ಲಿಸು;  // Continue
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ ಮಟ್ಟಗಳು;  // Letter
        }
    }
    ಹಿಂತಿರುಗಿ ಮಟ್ಟಗಳು;
}

ಶೂನ್ಯ V3_ಹೊರಹೋಗು_Formatter::putns(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg) {
    ಆದರೆ (!v3Global.opt.decoration()) {
        putsOutput(strg);
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ put_ಗಂಟು_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_ಮೂಲ_ಕೊನೆಯ_Filenameno != nodep->fileline()->filenameno()  //
            || m_ಮೂಲ_ಕೊನೆಯ_Lineno != nodep->fileline()->ಮೊದಲ_Lineno())  //
        && File_ಸಾಲು::built_ಒಳ_Filename() != nodep->fileline()->filename();

    ಆದರೆ (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_ಗಂಟು_Decoration)) {
        putsNoTracking(indentSpaces(ಕೊನೆ_ಮಟ್ಟಗಳು(strg)));
        m_prependIndent = ಸುಳ್ಳು;
    }

    ಆದರೆ (put_ಗಂಟು_Decoration) {
        ಸ್ಥಿರಾಂಕ File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        m_ಮೂಲ_ಕೊನೆಯ_Lineno = flp->ಮೊದಲ_Lineno();
        m_ಮೂಲ_ಕೊನೆಯ_Filenameno = flp->filenameno();
        ಸ್ಥಿರಾಂಕ std::string lineno = std::ಗಮ್ಯ_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_ಗಮ್ಯ_Hex(nodep) + "*/");
    }

    ತಾರ್ಕಿಕ notstart = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ wordstart = ನಿಜ;
    ತಾರ್ಕಿಕ equalsForBracket = ಸುಳ್ಳು;  // Looking for "= {"
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        ಆದರೆ (std::ಅಕ್ಷರವೇ(*cp)) {
            ಆದರೆ (wordstart && m_lang == LA_VERILOG && token_Not_ಆರಂಭ(cp)) notstart = ನಿಜ;
            ಆದರೆ (wordstart && m_lang == LA_VERILOG && !notstart && token_ಆರಂಭ(cp)) indentInc();
            ಆದರೆ (wordstart && m_lang == LA_VERILOG && token_ಕೊನೆ(cp)) indentDec();
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ was_ಒಳ_Backslash = m_ಒಳ_Backslash;
        m_ಒಳ_Backslash = ಸುಳ್ಳು;
        ಆಯ್ಕೆ (*cp) {
        ಸ್ಥಿತಿ '\n':
            ++m_lineno;
            wordstart = ನಿಜ;
            ಆದರೆ (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_prependIndent = ಸುಳ್ಳು;
                putsNoTracking(indentSpaces(ಕೊನೆ_ಮಟ್ಟಗಳು(cp + 1)));
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ ' ': wordstart = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '\t': wordstart = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '"':
            wordstart = ಸುಳ್ಳು;
            ಆದರೆ (!was_ಒಳ_Backslash) m_ಒಳ_String_Literal = !m_ಒಳ_String_Literal;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '\\':
            wordstart = ಸುಳ್ಳು;
            ಆದರೆ (!was_ಒಳ_Backslash) m_ಒಳ_Backslash = ನಿಜ;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '/':
            ಆದರೆ (m_lang == LA_C || m_lang == LA_VERILOG) {
                ಆದರೆ (cp > strg && cp[-1] == '/' && !m_ಒಳ_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    ಆಗ (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    ಆದರೆ (*cp) putcNoTracking(*cp);
                }
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '{':
            ಆದರೆ (!m_ಒಳ_String_Literal) {
                ಆದರೆ (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ತಳ್ಳು(m_indentLevel
                                    * m_ಖಂಡ_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '}':
            ಆದರೆ (!m_ಒಳ_String_Literal) {
                ಆದರೆ (m_bracketLevel > 0) {
                    m_parenVec.ತೆಗೆ_2();
                    --m_bracketLevel;
                }
                indentDec();
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '(':
            ಆದರೆ (!m_ಒಳ_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ತಳ್ಳು(m_ಸ್ತಂಭ);
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ ')':
            ಆದರೆ (!m_ಒಳ_String_Literal) {
                ಆದರೆ (!m_parenVec.ಖಾಲಿಗಳು()) m_parenVec.ತೆಗೆ_2();
                indentDec();
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '<':
            ಆದರೆ (m_lang == LA_XML) {
                ಆದರೆ (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } ಇಲ್ಲದಿದ್ದರೆ {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '>':
            ಆದರೆ (m_lang == LA_XML) {
                indentDec();
                ಆದರೆ (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: wordstart = ಸುಳ್ಳು; ನಿಲ್ಲಿಸು;
        }

        ಆಯ್ಕೆ (*cp) {
        ಸ್ಥಿತಿ '=': equalsForBracket = ನಿಜ; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ ' ': ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: equalsForBracket = ಸುಳ್ಳು; ನಿಲ್ಲಿಸು;
        }
    }
}

ಶೂನ್ಯ V3_ಹೊರಹೋಗು_Formatter::put_Break_ಅಭಿವ್ಯಕ್ತಿ() {
    ಆದರೆ (!m_parenVec.ಖಾಲಿಗಳು()) putBreak();
}

// Add a line break if too wide
ಶೂನ್ಯ V3_ಹೊರಹೋಗು_Formatter::putBreak() {
    ಆದರೆ (!v3Global.opt.decoration()) ಹಿಂತಿರುಗಿ;
    ಆದರೆ (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        ಆದರೆ (exceeded_ಅಗಲ()) {
            putcNoTracking('\n');
            ಆದರೆ (!m_parenVec.ಖಾಲಿಗಳು()) putsNoTracking(indentSpaces(m_parenVec.ತುದಿ()));
        }
    }
}

ಶೂನ್ಯ V3_ಹೊರಹೋಗು_Formatter::putsQuoted(ಸ್ಥಿರಾಂಕ string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ಸ್ಥಿರಾಂಕ string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    ಆದರೆ (strg.ಕಂಡುಹಿಡಿ('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
ಶೂನ್ಯ V3_ಹೊರಹೋಗು_Formatter::putsNoTracking(ಸ್ಥಿರಾಂಕ string& strg) {
    ಆದರೆ (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        ಹಿಂತಿರುಗಿ;
    }
    // Don't track {}'s, probably because it's a $display format string
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : strg) putcNoTracking(c);
}

ಶೂನ್ಯ V3_ಹೊರಹೋಗು_Formatter::putcNoTracking(ಅಕ್ಷರ chr) {
    ಆದರೆ (!v3Global.opt.decoration()) {
        putcOutput(chr);
        ಹಿಂತಿರುಗಿ;
    }
    ಆಯ್ಕೆ (chr) {
    ಸ್ಥಿತಿ '\n':
        ++m_lineno;
        m_ಸ್ತಂಭ = 0;
        m_nobreak = ನಿಜ;
        ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ '\t': m_ಸ್ತಂಭ = ((m_ಸ್ತಂಭ + 9) / 8) * 8; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ ' ':
    ಸ್ಥಿತಿ '(':
    ಸ್ಥಿತಿ '|':
    ಸ್ಥಿತಿ '&': ++m_ಸ್ತಂಭ; ನಿಲ್ಲಿಸು;
    ಪೂರ್ವನಿಯೋಜಿತ:
        ++m_ಸ್ತಂಭ;
        m_nobreak = ಸುಳ್ಳು;
        ನಿಲ್ಲಿಸು;
    }
    putcOutput(chr);
}

string V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(ಸ್ಥಿರಾಂಕ string& namein,
                                         V3_ಹೊರಹೋಗು_Formatter::Language lang) VL_ಶುದ್ಧ {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string ಹೊರಹೋಗು;
    ಆದರೆ (lang == LA_XML) {
        // Encode chars into XML string
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : namein) {
            ಆದರೆ (c == '"') {
                ಹೊರಹೋಗು += "&quot;"s;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (c == '\'') {
                ಹೊರಹೋಗು += "&apos;"s;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (c == '<') {
                ಹೊರಹೋಗು += "&lt;"s;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (c == '>') {
                ಹೊರಹೋಗು += "&gt;"s;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (c == '&') {
                ಹೊರಹೋಗು += "&amp;"s;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::isprint(c)) {
                ಹೊರಹೋಗು += c;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹೊರಹೋಗು += "&#"s + cvt_ಗಮ್ಯ_Str(static_cast<ಚಿಹ್ನೆರಹಿತ ಪೂರ್ಣಾಂಕ>(c & 0xff)) + ";";
            }
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Encode control chars into C style escapes
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : namein) {
            ಆದರೆ (c == '\\' || c == '"') {
                ಹೊರಹೋಗು += "\\"s + c;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (c == '\n') {
                ಹೊರಹೋಗು += "\\n";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (c == '\r') {
                ಹೊರಹೋಗು += "\\r";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (c == '\t') {
                ಹೊರಹೋಗು += "\\t";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::isprint(c)) {
                ಹೊರಹೋಗು += c;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // This will also cover \a etc
                ಸ್ಥಿರಾಂಕ string octal
                    = "\\"s + cvt_ಗಮ್ಯ_Str((c >> 6) & 3) + cvt_ಗಮ್ಯ_Str((c >> 3) & 7) + cvt_ಗಮ್ಯ_Str(c & 7);
                ಹೊರಹೋಗು += octal;
            }
        }
    }
    ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
}

//----------------------------------------------------------------------
// Simple wrappers

ಶೂನ್ಯ V3_ಹೊರಹೋಗು_Formatter::ಬರೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* fmt...) {
    constexpr ಗಾತ್ರ_t bufsize = 5000;
    ಅಕ್ಷರ sbuff[bufsize];
    va_list ap;
    va_ಆರಂಭ(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_ಕೊನೆ(ap);
    ಇದು->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_ಹೊರಹೋಗು_File::V3_ಹೊರಹೋಗು_File(ಸ್ಥಿರಾಂಕ string& filename, V3_ಹೊರಹೋಗು_Formatter::Language lang)
    : V3_ಹೊರಹೋಗು_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{ಹೊಸ std::array<ಅಕ್ಷರ, WRITE_ಬಫರ್_ಗಾತ್ರ_ಬೈಟುಗಳು>{}} {
    ಆದರೆ ((m_fp = V3File::new_fopen_w(filename)) == ಶೂನ್ಯಸೂಚಕ) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_ಹೊರಹೋಗು_File::~V3_ಹೊರಹೋಗು_File() {
    write_ಖಂಡ();

    ಆದರೆ (m_fp) fclose(m_fp);
    m_fp = ಶೂನ್ಯಸೂಚಕ;
}

ಶೂನ್ಯ V3_ಹೊರಹೋಗು_File::putsForceIncs() {
    ಸ್ಥಿರಾಂಕ VStringList& forceIncs = v3Global.opt.forceIncs();
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

ಶೂನ್ಯ V3_ಹೊರಹೋಗು_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = ನಿಜ;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    ಗಾಗಿ (ಅಕ್ಷರ& c : var) {
        ಆದರೆ (!std::ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_ಹೊರಹೋಗು_ಹರಿವು::V3_ಹೊರಹೋಗು_ಹರಿವು(std::ostream& ostream, V3_ಹೊರಹೋಗು_Formatter::Language lang)
    : V3_ಹೊರಹೋಗು_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

ವರ್ಗ VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ಸ್ಥಿರಾಂಕ std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_ಗಣ<std::string> m_new_Id_ಗಣ VL_GUARDED_BY(m_mutex);  // Which new names exist
ಸಂರಕ್ಷಿತ:
    // CONSTRUCTORS
    ಮಿತ್ರ ವರ್ಗ VIdProtect;
    ಸ್ಥಿರ VIdProtectImp& singleton() VL_MT_ಸುರಕ್ಷಿತ {
        ಸ್ಥಿರ VIdProtectImp s;
        ಹಿಂತಿರುಗಿ s;
    }

ಸಾರ್ವಜನಿಕ:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    string passthru(ಸ್ಥಿರಾಂಕ string& old) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
        ಆದರೆ (!v3Global.opt.protectIds()) ಹಿಂತಿರುಗಿ old;
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_nameMap.ಕಂಡುಹಿಡಿ(old);
        ಆದರೆ (it != m_nameMap.ಕೊನೆ()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->ಎರಡನೇ,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_nameMap.emplace(old, old);
            m_new_Id_ಗಣ.ಸೇರಿಸು(old);
        }
        ಹಿಂತಿರುಗಿ old;
    }
    string protectIf(ಸ್ಥಿರಾಂಕ string& old, ತಾರ್ಕಿಕ doIt) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
        ಆದರೆ (!v3Global.opt.protectIds() || old.ಖಾಲಿಗಳು() || !doIt) ಹಿಂತಿರುಗಿ old;
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_nameMap.emplace(old, "");
        ಆದರೆ (pair.ಎರಡನೇ) {
            string ಹೊರಹೋಗು;
            ಆದರೆ (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                ಹೊರಹೋಗು = "PS" + old;
            } ಇಲ್ಲದಿದ್ದರೆ {
                VHashSha512 digest{v3Global.opt.protect_ಕೀಲಿ_Defaulted()};
                digest.ಸೇರಿಸು(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                ಹೊರಹೋಗು = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                ಗಾಗಿ (ಗಾತ್ರ_t len = 6; len < ಹೊರಹೋಗು.ಗಾತ್ರ_2() - 3; len += 3) {
                    ಸ್ಥಿರಾಂಕ string tryout = ಹೊರಹೋಗು.substr(0, len);
                    ಆದರೆ (m_new_Id_ಗಣ.ಸೇರಿಸು(tryout).ಎರಡನೇ) {
                        ಹೊರಹೋಗು = tryout;
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
            pair.ಮೊದಲ->ಎರಡನೇ = ಹೊರಹೋಗು;
        }
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }
    string protect_ಪದಗಳು_If(ಸ್ಥಿರಾಂಕ string& old, ತಾರ್ಕಿಕ doIt) VL_MT_ಸುರಕ್ಷಿತ {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        ಆದರೆ (!(doIt && v3Global.opt.protectIds())) ಹಿಂತಿರುಗಿ old;
        string ಹೊರಹೋಗು;
        string::ಗಾತ್ರ_type ಆರಂಭ = 0;
        // space, ., ->
        ಆಗ (ನಿಜ) {
            // When C++11, use find_if and lambda
            string::ಗಾತ್ರ_type pos = string::npos;
            string separator;
            trySep(old, ಆರಂಭ, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, ಆರಂಭ, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, ಆರಂಭ, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, ಆರಂಭ, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, ಆರಂಭ, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, ಆರಂಭ, ")", pos /*ref*/, separator /*ref*/);
            ಆದರೆ (pos == string::npos) ನಿಲ್ಲಿಸು;
            ಹೊರಹೋಗು += protectIf(old.substr(ಆರಂಭ, pos - ಆರಂಭ), ನಿಜ) + separator;
            ಆರಂಭ = pos + separator.ಉದ್ದಳತೆ();
        }
        ಹೊರಹೋಗು += protectIf(old.substr(ಆರಂಭ), ನಿಜ);
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
    }
    ಶೂನ್ಯ writeMapFile(ಸ್ಥಿರಾಂಕ string& filename) ಸ್ಥಿರಾಂಕ {
        V3_ಹೊರಹೋಗು_Xml_File ದ{filename};
        ದ.puts_ಶೀರ್ಷಿಕೆ();
        ದ.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        ದ.puts("<verilator_id_map>\n");
        {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_nameMap) {
                ದ.puts("<map from=\"" + itr.ಎರಡನೇ + "\" to=\"" + itr.ಮೊದಲ + "\"/>\n");
            }
        }
        ದ.puts("</verilator_id_map>\n");
    }

ಖಾಸಗಿ:
    ಶೂನ್ಯ trySep(ಸ್ಥಿರಾಂಕ string& old, string::ಗಾತ್ರ_type ಆರಂಭ, ಸ್ಥಿರಾಂಕ string& trySep,
                string::ಗಾತ್ರ_type& posr, string& separatorr) VL_ಶುದ್ಧ {
        ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type trypos = old.ಕಂಡುಹಿಡಿ(trySep, ಆರಂಭ);
        ಆದರೆ (trypos != string::npos) {
            ಆದರೆ (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ಸ್ಥಿರಾಂಕ string& ಒಳ) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ v3Global.opt.protectIds() ? "" : ಒಳ;
}
string VIdProtect::protectIf(ಸ್ಥಿರಾಂಕ string& old, ತಾರ್ಕಿಕ doIt) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_ಪದಗಳು_If(ಸ್ಥಿರಾಂಕ string& old, ತಾರ್ಕಿಕ doIt) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ VIdProtectImp::singleton().protect_ಪದಗಳು_If(old, doIt);
}
ಶೂನ್ಯ VIdProtect::writeMapFile(ಸ್ಥಿರಾಂಕ string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
