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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cerrno>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdarg>
#ਅਨ੍ਤਰ੍ਭਾਵ <fcntl.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>

#ਅਨ੍ਤਰ੍ਭਾਵ <sys/stat.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/types.h>

// clang-format off
#ਜੇ defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#ਜੇ_ਅੰਤ

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#ਨਹੀਂਤਾਂ
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#ਜੇ_ਅੰਤ

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ INFILTER_PIPE
# include <sys/wait.h>
#ਜੇ_ਅੰਤ

#ਜੇ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#ਜੇ_ਅੰਤ
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr ਪੂਰਨ INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr ਪੂਰਨ INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_ਮਾਪ_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

ਵਰਗ V3FileDependImp final {
    // TYPES
    ਵਰਗ DependFile final {
        // A single file
        ਅਚਲ ਤਾਰਕਿਕ m_ਨਿਸ਼ਾਨਾ;  // True if write, else read
        ਤਾਰਕਿਕ m_exists = ਸੱਚ;  // File exists
        ਅਚਲ string m_filename;  // Filename
        ਬਣਤਰ stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    ਜਨਤਕ:
        DependFile(ਅਚਲ string& filename, ਤਾਰਕਿਕ ਨਿਸ਼ਾਨਾ)
            : m_ਨਿਸ਼ਾਨਾ{ਨਿਸ਼ਾਨਾ}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = ਮੂਲ;
        ਅਚਲ string& filename() ਅਚਲ { ਵਾਪਸ m_filename; }
        ਤਾਰਕਿਕ ਨਿਸ਼ਾਨਾ() ਅਚਲ { ਵਾਪਸ m_ਨਿਸ਼ਾਨਾ; }
        ਤਾਰਕਿਕ exists() ਅਚਲ { ਵਾਪਸ m_exists; }
        off_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_stat.st_ਮਾਪ; }
        ino_t ino() ਅਚਲ { ਵਾਪਸ m_stat.st_ino; }
        ਸਮਾਂ_t cstime() ਅਚਲ { ਵਾਪਸ m_stat.st_ctime; }  // Seconds
        ਸਮਾਂ_t cnstime() ਅਚਲ { ਵਾਪਸ VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        ਸਮਾਂ_t mstime() ਅਚਲ { ਵਾਪਸ m_stat.st_mtime; }  // Seconds
        ਸਮਾਂ_t mnstime() ਅਚਲ { ਵਾਪਸ VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_ਚਿੰਨ੍ਹ() {
            ਸਥਿਰ VHashSha512 s_ਖਾਲੀ_ਥਾਵਾਂ_Hash;
            ਵਾਪਸ m_hash.digestSymbol24() != s_ਖਾਲੀ_ਥਾਵਾਂ_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        ਖਾਲੀ ਲੱਦੋ_Stats() {
            ਜੇ (!m_stat.st_mtime) {
                ਅਚਲ string fn = filename();
                ਅਚਲ ਪੂਰਨ err = stat(fn.c_str(), &m_stat);
                ਜੇ (err != 0) {
                    std::memset(&m_stat, 0, ਆਕਾਰ(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = ਝੂਠ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } ਨਹੀਂਤਾਂ {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    ਜੇ (!ਨਿਸ਼ਾਨਾ() && ਮਾਪ() <= FILE_HASH_ਮਾਪ_MAX) m_hash.ਪਾਓ_File(filename());
                }
            }
        }
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ<(ਅਚਲ DependFile& rhs) ਅਚਲ { ਵਾਪਸ filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::ਸੈੱਟ<string> m_filename_ਸੈੱਟ VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::ਸੈੱਟ<DependFile> m_filenameList;  // Files sourced/generated

    ਸਥਿਰ string stripQuotes(ਅਚਲ string& ਵਿੱਚ_2) {
        string pretty = ਵਿੱਚ_2;
        string::ਮਾਪ_type pos;
        ਜਦੋਂ ((pos = pretty.ਲੱਭੋ('\"')) != string::npos) pretty.replace(pos, 1, "_");
        ਜਦੋਂ ((pos = pretty.ਲੱਭੋ('\n')) != string::npos) pretty.replace(pos, 1, "_");
        ਵਾਪਸ pretty;
    }

ਜਨਤਕ:
    // ACCESSOR METHODS
    ਖਾਲੀ ਜੋੜੋ_Src_Depend(ਅਚਲ string& filename) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        ਅਚਲ ਸਵੈਚਾਲਿਤ it_ਮਿਲਿਆ_Pair = m_filename_ਸੈੱਟ.ਪਾਓ(filename);
        ਜੇ (it_ਮਿਲਿਆ_Pair.ਦੂਜਾ) {
            DependFile df{filename, ਝੂਠ};
            df.ਲੱਦੋ_Stats();  // Get size now, in case changes during the run
            m_filenameList.ਪਾਓ(df);
        }
    }
    ਖਾਲੀ ਜੋੜੋ_Tgt_Depend(ਅਚਲ string& filename) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        ਅਚਲ ਸਵੈਚਾਲਿਤ it_ਮਿਲਿਆ_Pair = m_filename_ਸੈੱਟ.ਪਾਓ(filename);
        ਜੇ (it_ਮਿਲਿਆ_Pair.ਦੂਜਾ) m_filenameList.ਪਾਓ(DependFile{filename, ਸੱਚ});
    }
    ਖਾਲੀ writeDepend(ਅਚਲ string& filename);
    std::ਸਦਿਸ਼<string> getAllDeps() ਅਚਲ;
    ਖਾਲੀ writeTimes(ਅਚਲ string& filename, ਅਚਲ string& cmdline_ਵਿੱਚ);
    ਤਾਰਕਿਕ checkTimes(ਅਚਲ string& filename, ਅਚਲ string& cmdline_ਵਿੱਚ);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

ਖਾਲੀ V3FileDependImp::writeDepend(ਅਚਲ string& filename) {
    ਅਚਲ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ਜੇ (ofp->fail()) v3fatal("Can't write file: " << filename);

    ਲਈ (ਅਚਲ DependFile& i : m_filenameList) {
        ਜੇ (i.ਨਿਸ਼ਾਨਾ()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    ਲਈ (ਅਚਲ DependFile& i : m_filenameList) {
        ਜੇ (!i.ਨਿਸ਼ਾਨਾ()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    ਜੇ (v3Global.opt.makePhony()) {
        *ofp << '\n';
        ਲਈ (ਅਚਲ DependFile& i : m_filenameList) {
            ਜੇ (!i.ਨਿਸ਼ਾਨਾ()) *ofp << i.filename() << ":\n";
        }
    }
}

std::ਸਦਿਸ਼<string> V3FileDependImp::getAllDeps() ਅਚਲ {
    std::ਸਦਿਸ਼<string> r;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_filenameList) {
        ਜੇ (!itr.ਨਿਸ਼ਾਨਾ() && itr.exists()) r.ਧੱਕੋ_ਪਿੱਠ(itr.filename());
    }
    ਵਾਪਸ r;
}

ਖਾਲੀ V3FileDependImp::writeTimes(ਅਚਲ string& filename, ਅਚਲ string& cmdline_ਵਿੱਚ) {
    ਅਚਲ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ਜੇ (ofp->fail()) v3fatal("Can't write file: " << filename);

    ਅਚਲ string cmdline = stripQuotes(cmdline_ਵਿੱਚ);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    ਲਈ (ਸਵੈਚਾਲਿਤ& it : m_filenameList) {
        DependFile* ਅਚਲ depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->ਲੱਦੋ_Stats();
        off_t show_ਮਾਪ = depfilep->ਮਾਪ();
        ino_t showIno = depfilep->ino();
        ਜੇ (depfilep->filename() == filename) {
            show_ਮਾਪ = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->ਨਿਸ਼ਾਨਾ() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_ਮਾਪ;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_ਚਿੰਨ੍ਹ() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

ਤਾਰਕਿਕ V3FileDependImp::checkTimes(ਅਚਲ string& filename, ਅਚਲ string& cmdline_ਵਿੱਚ) {
    ਅਚਲ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    ਜੇ (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        ਵਾਪਸ ਝੂਠ;
    }
    {
        ਅਚਲ string ignore = V3Os::getline(*ifp);
        ਜੇ (ignore.ਖਾਲੀ_ਥਾਵਾਂ()) { /*ਵਰਤਿਆ*/
        }
    }
    {
        ਅੱਖਰ chkDir;
        *ifp >> chkDir;
        ਅੱਖਰ quote;
        *ifp >> quote;
        ਅਚਲ string chkCmdline = V3Os::getline(*ifp, '"');
        ਅਚਲ string cmdline = stripQuotes(cmdline_ਵਿੱਚ);
        ਜੇ (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            ਵਾਪਸ ਝੂਠ;
        }
    }

    ਅਚਲ ਤਾਰਕਿਕ skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").ਖਾਲੀ_ਥਾਵਾਂ();

    ਜਦੋਂ (!ifp->eof()) {
        ਅੱਖਰ chkDir;
        *ifp >> chkDir;
        off_t chk_ਮਾਪ;
        *ifp >> chk_ਮਾਪ;
        ino_t chkIno;
        *ifp >> chkIno;
        ਜੇ (ifp->eof()) ਰੋਕੋ;  // Needed to read final whitespace before found eof
        ਸਮਾਂ_t chkCstime;
        *ifp >> chkCstime;
        ਸਮਾਂ_t chkCnstime;
        *ifp >> chkCnstime;
        ਸਮਾਂ_t chkMstime;
        *ifp >> chkMstime;
        ਸਮਾਂ_t chkMnstime;
        *ifp >> chkMnstime;
        ਅੱਖਰ quote;
        *ifp >> quote;
        ਅਚਲ string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ਅਚਲ string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ਬਣਤਰ stat curStat;
        ਅਚਲ ਪੂਰਨ err = stat(chkFilename.c_str(), &curStat);
        ਜੇ (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            ਵਾਪਸ ਝੂਠ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        ਜੇ (filename == chkFilename) ਜਾਰੀ;

        UINFO(9, " got d=" << chkDir << " s=" << chk_ਮਾਪ << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_ਮਾਪ << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        ਜੇ (!(curStat.st_ਮਾਪ >= chk_ਮਾਪ && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_ਮਾਪ << "=?" << chk_ਮਾਪ << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            ਜੇ (skipHashing) ਵਾਪਸ ਝੂਠ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            ਜੇ (chkHash == "unhashed") ਵਾਪਸ ਝੂਠ;

            VHashSha512 curHash;
            curHash.ਪਾਓ_File(chkFilename);
            ਜੇ (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                ਵਾਪਸ ਝੂਠ;
            }
        }
    }
    ਵਾਪਸ ਸੱਚ;
}

//######################################################################
// V3File

ਖਾਲੀ V3File::ਜੋੜੋ_Src_Depend(ਅਚਲ string& filename) VL_MT_ਸੁਰੱਖਿਅਤ { dependImp.ਜੋੜੋ_Src_Depend(filename); }
ਖਾਲੀ V3File::ਜੋੜੋ_Tgt_Depend(ਅਚਲ string& filename) VL_MT_ਸੁਰੱਖਿਅਤ { dependImp.ਜੋੜੋ_Tgt_Depend(filename); }
ਖਾਲੀ V3File::writeDepend(ਅਚਲ string& filename) { dependImp.writeDepend(filename); }
std::ਸਦਿਸ਼<string> V3File::getAllDeps() { ਵਾਪਸ dependImp.getAllDeps(); }
ਖਾਲੀ V3File::writeTimes(ਅਚਲ string& filename, ਅਚਲ string& cmdline_ਵਿੱਚ) {
    dependImp.writeTimes(filename, cmdline_ਵਿੱਚ);
}
ਤਾਰਕਿਕ V3File::checkTimes(ਅਚਲ string& filename, ਅਚਲ string& cmdline_ਵਿੱਚ) {
    ਵਾਪਸ dependImp.checkTimes(filename, cmdline_ਵਿੱਚ);
}
ਖਾਲੀ V3File::createMakeDirFor(ਅਚਲ string& filename) {
    ਜੇ (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().ਲੰਮਾਈ() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
ਖਾਲੀ V3File::createMakeDir() {
    ਸਥਿਰ ਤਾਰਕਿਕ s_created = ਝੂਠ;
    ਜੇ (!s_created) {
        s_created = ਸੱਚ;
        V3Os::createDir(v3Global.opt.makeDir());
        ਜੇ (v3Global.opt.hier_ਸਿਖਰ()) V3Os::createDir(v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir());
    }
}

//######################################################################
// VInFilterImp

ਵਰਗ VInFilterImp final {
    ਵਰਤੋਂ StrList = VInFilter::StrList;

    std::map<ਅਚਲ std::string, std::string> m_contentsMap;  // Cache of file contents
    ਤਾਰਕਿਕ m_readEof = ਝੂਠ;  // Received EOF on read
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#ਨਹੀਂਤਾਂ
    ਪੂਰਨ m_pid = 0;  // fork() process id - always zero as disabled
#ਜੇ_ਅੰਤ
    ਤਾਰਕਿਕ m_pidExited = ਝੂਠ;  //  waitpid() reaped the filter process
    ਪੂਰਨ m_pid_ਸਥਿਤੀ = 0;  // Exit status of the filter process
    ਪੂਰਨ m_writeFd = 0;  // File descriptor TO filter
    ਪੂਰਨ m_readFd = 0;  // File descriptor FROM filter

ਨਿਜੀ:
    // METHODS

    ਤਾਰਕਿਕ readContents(ਅਚਲ string& filename, StrList& outl) {
        ਜੇ (m_pid) {
            ਵਾਪਸ readContentsFilter(filename, outl);
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ readContentsFile(filename, outl);
        }
    }
    ਤਾਰਕਿਕ readContentsFile(ਅਚਲ string& filename, StrList& outl) {
        ਅਚਲ ਪੂਰਨ fd = open(filename.c_str(), O_RDONLY);
        ਜੇ (fd < 0) ਵਾਪਸ ਝੂਠ;
        m_readEof = ਝੂਠ;
        read_ਖੰਡਾਂ(fd, -1, outl);
        close(fd);
        ਵਾਪਸ ਸੱਚ;
    }
    // cppcheck-suppress constParameterReference
    ਤਾਰਕਿਕ readContentsFilter(ਅਚਲ string& filename, StrList& outl) {
        (ਖਾਲੀ)filename;  // Prevent unused variable warning
        (ਖਾਲੀ)outl;  // Prevent unused variable warning
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ਅਚਲ string ਸਤਰ = read_Filter_ਸਤਰ();
        ਜੇ (ਸਤਰ.ਲੱਭੋ("Content-Length") != string::npos) {
            ਪੂਰਨ len = 0;
            ਲੜੀ_ਤੋਂ_ਪੜ੍ਹੋ(ਸਤਰ.c_str(), "Content-Length: %d\n", &len);
            read_ਖੰਡਾਂ(m_readFd, len, outl);
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ {
            ਜੇ (ਸਤਰ != "") v3error("--pipe-filter protocol error, unexpected: " << ਸਤਰ);
            ਵਾਪਸ ਝੂਠ;
        }
#ਨਹੀਂਤਾਂ
        v3fatalSrc("--pipe-filter not implemented on this platform");
        ਵਾਪਸ ਝੂਠ;
#ਜੇ_ਅੰਤ
    }

    // cppcheck-suppress functionConst
    ਖਾਲੀ checkFilter(ਤਾਰਕਿਕ hang) {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ INFILTER_PIPE
        ਜੇ (!m_pidExited && waitpid(m_pid, &m_pid_ਸਥਿਤੀ, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_ਸਥਿਤੀ << " exit=" << WEXITSTATUS(m_pid_ਸਥਿਤੀ) << " err"
                         << std::strerror(errno));
            m_readEof = ਸੱਚ;
            m_pidExited = ਸੱਚ;
        }
#ਜੇ_ਅੰਤ
    }

    ਖਾਲੀ read_ਖੰਡਾਂ(ਪੂਰਨ fd, ਪੂਰਨ ਮਾਪ, StrList& outl) {
        ਅੱਖਰ buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        ਜਦੋਂ (!m_readEof && (ਮਾਪ < 0 || ਮਾਪ > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            ਜੇ (ਮਾਪ > 0 && ਮਾਪ < todo) todo = ਮਾਪ;
            errno = 0;
            ਅਚਲ ssize_t got = ਪਠ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ਜੇ (got > 0) {
                outl.emplace_ਪਿੱਠ(string(buf, got));
                sizegot += got;
            } ਨਹੀਂਤਾਂ ਜੇ (errno == EINTR || errno == EAGAIN
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ EWOULDBLOCK
                       || errno == EWOULDBLOCK
#ਜੇ_ਅੰਤ
            ) {
                checkFilter(ਝੂਠ);
                V3Os::u_sleep(1000);
                ਜਾਰੀ;
            } ਨਹੀਂਤਾਂ {
                m_readEof = ਸੱਚ;
                ਰੋਕੋ;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_ਸਤਰ() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string ਸਤਰ;
        ਜਦੋਂ (!m_readEof) {
            StrList outl;
            read_ਖੰਡਾਂ(m_readFd, 1, outl);
            ਅਚਲ string onechar = listString(outl);
            ਸਤਰ += onechar;
            ਜੇ (onechar == "\n") {
                ਜੇ (ਸਤਰ == "\n") {
                    ਸਤਰ = "";
                    ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ {
                    ਰੋਕੋ;
                }
            }
        }
        UINFO(6, "filter-line-in: " << ਸਤਰ);
        ਵਾਪਸ ਸਤਰ;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    ਖਾਲੀ writeFilter(ਅਚਲ string& ਬਾਹਰਲਾ) {
        ਜੇ (debug() >= 6) {
            UINFO(6, "filter-out: " << ਬਾਹਰਲਾ);
            ਜੇ (ਬਾਹਰਲਾ[ਬਾਹਰਲਾ.ਲੰਮਾਈ() - 1] != '\n') ਬਾਹਰ << '\n';
        }
        ਜੇ (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = ਸੱਚ;
            stop();
        }
        ਬਿਨਾਨਿਸ਼ਾਨੀ ਵਿੱਥ = 0;
        ਜਦੋਂ (!m_readEof && ਬਾਹਰਲਾ.ਲੰਮਾਈ() > ਵਿੱਥ) {
            errno = 0;
            ਅਚਲ ਪੂਰਨ got = ਲਿਖ(m_writeFd, (ਬਾਹਰਲਾ.c_str()) + ਵਿੱਥ, ਬਾਹਰਲਾ.ਲੰਮਾਈ() - ਵਿੱਥ);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ਜੇ (got > 0) {
                ਵਿੱਥ += got;
            } ਨਹੀਂਤਾਂ ਜੇ (errno == EINTR || errno == EAGAIN
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ EWOULDBLOCK
                       || errno == EWOULDBLOCK
#ਜੇ_ਅੰਤ
            ) {
                checkFilter(ਝੂਠ);
                V3Os::u_sleep(1000);
                ਜਾਰੀ;
            } ਨਹੀਂਤਾਂ {
                ਰੋਕੋ;
            }
        }
    }

    // Start the filter
    ਖਾਲੀ ਸ਼ੁਰੂ(ਅਚਲ string& command) {
        ਜੇ (command == "") {
            m_pid = 0;  // Disabled
        } ਨਹੀਂਤਾਂ {
            ਸ਼ੁਰੂ_Filter(command);
        }
    }
    ਖਾਲੀ ਸ਼ੁਰੂ_Filter(ਅਚਲ string& command) {
        (ਖਾਲੀ)command;  // Prevent Unused variable warning
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ INFILTER_PIPE
        ਪੂਰਨ fd_stdin[2];  // Can't use std::array
        ਪੂਰਨ fd_stdout[2];  // Can't use std::array
        constexpr ਪੂਰਨ P_RD = 0;
        constexpr ਪੂਰਨ P_WR = 1;

        ਜੇ (VL_UNCOVERABLE(ਪ੍ਰਣਾਲ(fd_stdin) != 0 || ਪ੍ਰਣਾਲ(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ਜੇ (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ਅਚਲ pid_t pid = fork();
        ਜੇ (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ਜੇ (pid == 0) {  //ਬੱਚਾ
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<ਅੱਖਰ*>(ਖਾਲੀ_ਸੂਚਕ));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } ਨਹੀਂਤਾਂ {  //ਮਾਪਾ
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = ਝੂਠ;
            m_pid_ਸਥਿਤੀ = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = ਝੂਠ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            ਪੂਰਨ flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#ਨਹੀਂਤਾਂ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#ਜੇ_ਅੰਤ
    }

    ਖਾਲੀ stop() {
        ਜੇ (m_pid) stopFilter();
    }
    ਖਾਲੀ stopFilter() {
        UINFO(6, "Stopping filter process");
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ INFILTER_PIPE
        close(m_writeFd);
        checkFilter(ਸੱਚ);
        ਜੇ (!WIFEXITED(m_pid_ਸਥਿਤੀ) || WEXITSTATUS(m_pid_ਸਥਿਤੀ) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#ਨਹੀਂਤਾਂ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#ਜੇ_ਅੰਤ
    }

ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ ਵਰਗ VInFilter;
    // Read file contents and return it
    ਤਾਰਕਿਕ readWholefile(ਅਚਲ string& filename, StrList& outl) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_contentsMap.ਲੱਭੋ(filename);
        ਜੇ (it != m_contentsMap.ਸਿਰਾ()) {
            outl.ਧੱਕੋ_ਪਿੱਠ(it->ਦੂਜਾ);
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (!readContents(filename, outl)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (list_ਮਾਪ(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਸਥਿਰ ਮਾਪ_t list_ਮਾਪ(ਅਚਲ StrList& sl) {
        ਮਾਪ_t ਨਤੀਜਾ = 0;
        ਲਈ (ਅਚਲ string& i : sl) ਨਤੀਜਾ += i.ਲੰਮਾਈ();
        ਵਾਪਸ ਨਤੀਜਾ;
    }
    ਸਥਿਰ string listString(ਅਚਲ StrList& sl) {
        string ਨਤੀਜਾ;
        ਲਈ (ਅਚਲ string& i : sl) ਨਤੀਜਾ += i;
        ਵਾਪਸ ਨਤੀਜਾ;
    }
    // CONSTRUCTORS
    ਸਪਸ਼ਟ VInFilterImp(ਅਚਲ string& command) { ਸ਼ੁਰੂ(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ਅਚਲ string& command) { m_impp = ਨਵਾਂ VInFilterImp{command}; }
VInFilter::~VInFilter() {
    ਜੇ (m_impp) VL_DO_CLEAR(ਮਿਟਾਓ m_impp, m_impp = ਖਾਲੀ_ਸੂਚਕ);
}

ਤਾਰਕਿਕ VInFilter::readWholefile(ਅਚਲ string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    ਵਾਪਸ m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_ਬਾਹਰਲਾ_Formatter::V3_ਬਾਹਰਲਾ_Formatter(V3_ਬਾਹਰਲਾ_Formatter::Language lang)
    : m_lang{lang} {
    m_ਖੰਡ_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_ਚੌੜਾਈ = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ਸਥਿਰ constexpr ਪੂਰਨ MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ਸਥਿਰ ਅਚਲ std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> ਸਾਰਣੀ;
    ਲਈ (ਪੂਰਨ i = 0; i <= MAXSPACE; ++i) ਸਾਰਣੀ[i] = std::string(static_cast<ਮਾਪ_t>(i), ' ');
    ਵਾਪਸ ਸਾਰਣੀ;
}();

ਅਚਲ std::string& V3_ਬਾਹਰਲਾ_Formatter::indentSpaces(ਪੂਰਨ ਅੰਕ) {
    // Indent the specified number of spaces.
    ਵਾਪਸ s_indentSpaces[std::max(0, std::min(ਅੰਕ, MAXSPACE))];
}

ਤਾਰਕਿਕ V3_ਬਾਹਰਲਾ_Formatter::token_ਮੇਲ(ਅਚਲ ਅੱਖਰ* cp, ਅਚਲ ਅੱਖਰ* cmp) {
    ਜਦੋਂ (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    ਜੇ (*cmp) ਵਾਪਸ ਝੂਠ;
    ਜੇ (*cp && !std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp)) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ ਸੱਚ;
}

ਤਾਰਕਿਕ V3_ਬਾਹਰਲਾ_Formatter::token_ਸ਼ੁਰੂ(ਅਚਲ ਅੱਖਰ* cp) {
    ਵਾਪਸ (token_ਮੇਲ(cp, "begin") || token_ਮੇਲ(cp, "case") || token_ਮੇਲ(cp, "casex")
            || token_ਮੇਲ(cp, "casez") || token_ਮੇਲ(cp, "class") || token_ਮੇਲ(cp, "function")
            || token_ਮੇਲ(cp, "interface") || token_ਮੇਲ(cp, "module") || token_ਮੇਲ(cp, "package")
            || token_ਮੇਲ(cp, "task"));
}

ਤਾਰਕਿਕ V3_ਬਾਹਰਲਾ_Formatter::token_ਸਿਰਾ(ਅਚਲ ਅੱਖਰ* cp) {
    ਵਾਪਸ (token_ਮੇਲ(cp, "end") || token_ਮੇਲ(cp, "endcase") || token_ਮੇਲ(cp, "endclass")
            || token_ਮੇਲ(cp, "endfunction") || token_ਮੇਲ(cp, "endinterface")
            || token_ਮੇਲ(cp, "endmodule") || token_ਮੇਲ(cp, "endpackage")
            || token_ਮੇਲ(cp, "endtask"));
}

ਤਾਰਕਿਕ V3_ਬਾਹਰਲਾ_Formatter::token_Not_ਸ਼ੁਰੂ(ਅਚਲ ਅੱਖਰ* cp) {
    ਵਾਪਸ (token_ਮੇਲ(cp, "export") || token_ਮੇਲ(cp, "import"));
}

ਪੂਰਨ V3_ਬਾਹਰਲਾ_Formatter::ਸਿਰਾ_ਪੱਧਰ(ਅਚਲ ਅੱਖਰ* strg) {
    ਪੂਰਨ ਪੱਧਰ = m_indentLevel;
    {
        ਅਚਲ ਅੱਖਰ* cp = strg;
        ਜਦੋਂ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp)) ++cp;
        ਚੋਣ (*cp) {
        ਹਾਲਤ '\n':  // Newlines.. No need for whitespace before it
            ਵਾਪਸ 0;
        ਹਾਲਤ '#':  // Preproc directive
            ਜੇ (m_lang == LA_C) ਵਾਪਸ 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ਅਚਲ ਅੱਖਰ* mp = cp;
            ਲਈ (; std::ਅੱਖਰ_ਜਾਂ_ਅੰਕ(*mp); ++mp) {}
            ਜੇ (mp[0] == ':' && mp[1] != ':') ਵਾਪਸ (ਪੱਧਰ - m_ਖੰਡ_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    ਲਈ (ਅਚਲ ਅੱਖਰ* cp = strg; *cp; ++cp) {
        ਚੋਣ (*cp) {
        ਹਾਲਤ '}':
        ਹਾਲਤ ')': ਪੱਧਰ -= m_ਖੰਡ_Indent; ਰੋਕੋ;
        ਹਾਲਤ '<':
            ਜੇ (m_lang == LA_XML) {
                ਜੇ (cp[1] == '/') ਪੱਧਰ -= m_ਖੰਡ_Indent;
            }
            ਰੋਕੋ;
        ਹਾਲਤ 'e':
            ਜੇ (m_lang == LA_VERILOG && token_ਸਿਰਾ(cp)) ਪੱਧਰ -= m_ਖੰਡ_Indent;
            ਰੋਕੋ;
        ਹਾਲਤ '\t':
        ਹਾਲਤ ' ': ਰੋਕੋ;  // Continue
        ਮੂਲ: ਵਾਪਸ ਪੱਧਰ;  // Letter
        }
    }
    ਵਾਪਸ ਪੱਧਰ;
}

ਖਾਲੀ V3_ਬਾਹਰਲਾ_Formatter::putns(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ ਅੱਖਰ* strg) {
    ਜੇ (!v3Global.opt.decoration()) {
        putsOutput(strg);
        ਵਾਪਸ;
    }

    ਅਚਲ ਤਾਰਕਿਕ put_ਗੰਢ_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_ਸਰੋਤ_ਆਖਰੀ_Filenameno != nodep->fileline()->filenameno()  //
            || m_ਸਰੋਤ_ਆਖਰੀ_Lineno != nodep->fileline()->ਪਹਿਲਾ_Lineno())  //
        && File_ਸਤਰ::built_ਵਿੱਚ_Filename() != nodep->fileline()->filename();

    ਜੇ (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_ਗੰਢ_Decoration)) {
        putsNoTracking(indentSpaces(ਸਿਰਾ_ਪੱਧਰ(strg)));
        m_prependIndent = ਝੂਠ;
    }

    ਜੇ (put_ਗੰਢ_Decoration) {
        ਅਚਲ File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        m_ਸਰੋਤ_ਆਖਰੀ_Lineno = flp->ਪਹਿਲਾ_Lineno();
        m_ਸਰੋਤ_ਆਖਰੀ_Filenameno = flp->filenameno();
        ਅਚਲ std::string lineno = std::ਮੰਜ਼ਿਲ_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_ਮੰਜ਼ਿਲ_Hex(nodep) + "*/");
    }

    ਤਾਰਕਿਕ notstart = ਝੂਠ;
    ਤਾਰਕਿਕ wordstart = ਸੱਚ;
    ਤਾਰਕਿਕ equalsForBracket = ਝੂਠ;  // Looking for "= {"
    ਲਈ (ਅਚਲ ਅੱਖਰ* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        ਜੇ (std::ਅੱਖਰ_ਹੈ(*cp)) {
            ਜੇ (wordstart && m_lang == LA_VERILOG && token_Not_ਸ਼ੁਰੂ(cp)) notstart = ਸੱਚ;
            ਜੇ (wordstart && m_lang == LA_VERILOG && !notstart && token_ਸ਼ੁਰੂ(cp)) indentInc();
            ਜੇ (wordstart && m_lang == LA_VERILOG && token_ਸਿਰਾ(cp)) indentDec();
        }
        ਅਚਲ ਤਾਰਕਿਕ was_ਵਿੱਚ_Backslash = m_ਵਿੱਚ_Backslash;
        m_ਵਿੱਚ_Backslash = ਝੂਠ;
        ਚੋਣ (*cp) {
        ਹਾਲਤ '\n':
            ++m_lineno;
            wordstart = ਸੱਚ;
            ਜੇ (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = ਸੱਚ;
            } ਨਹੀਂਤਾਂ {
                m_prependIndent = ਝੂਠ;
                putsNoTracking(indentSpaces(ਸਿਰਾ_ਪੱਧਰ(cp + 1)));
            }
            ਰੋਕੋ;
        ਹਾਲਤ ' ': wordstart = ਸੱਚ; ਰੋਕੋ;
        ਹਾਲਤ '\t': wordstart = ਸੱਚ; ਰੋਕੋ;
        ਹਾਲਤ '"':
            wordstart = ਝੂਠ;
            ਜੇ (!was_ਵਿੱਚ_Backslash) m_ਵਿੱਚ_String_Literal = !m_ਵਿੱਚ_String_Literal;
            ਰੋਕੋ;
        ਹਾਲਤ '\\':
            wordstart = ਝੂਠ;
            ਜੇ (!was_ਵਿੱਚ_Backslash) m_ਵਿੱਚ_Backslash = ਸੱਚ;
            ਰੋਕੋ;
        ਹਾਲਤ '/':
            ਜੇ (m_lang == LA_C || m_lang == LA_VERILOG) {
                ਜੇ (cp > strg && cp[-1] == '/' && !m_ਵਿੱਚ_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    ਜਦੋਂ (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    ਜੇ (*cp) putcNoTracking(*cp);
                }
            }
            ਰੋਕੋ;
        ਹਾਲਤ '{':
            ਜੇ (!m_ਵਿੱਚ_String_Literal) {
                ਜੇ (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ਧੱਕੋ(m_indentLevel
                                    * m_ਖੰਡ_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            ਰੋਕੋ;
        ਹਾਲਤ '}':
            ਜੇ (!m_ਵਿੱਚ_String_Literal) {
                ਜੇ (m_bracketLevel > 0) {
                    m_parenVec.ਕੱਢੋ();
                    --m_bracketLevel;
                }
                indentDec();
            }
            ਰੋਕੋ;
        ਹਾਲਤ '(':
            ਜੇ (!m_ਵਿੱਚ_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ਧੱਕੋ(m_ਕਾਲਮ);
            }
            ਰੋਕੋ;
        ਹਾਲਤ ')':
            ਜੇ (!m_ਵਿੱਚ_String_Literal) {
                ਜੇ (!m_parenVec.ਖਾਲੀ_ਥਾਵਾਂ()) m_parenVec.ਕੱਢੋ();
                indentDec();
            }
            ਰੋਕੋ;
        ਹਾਲਤ '<':
            ਜੇ (m_lang == LA_XML) {
                ਜੇ (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } ਨਹੀਂਤਾਂ ਜੇ (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } ਨਹੀਂਤਾਂ {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            ਰੋਕੋ;
        ਹਾਲਤ '>':
            ਜੇ (m_lang == LA_XML) {
                indentDec();
                ਜੇ (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            ਰੋਕੋ;
        ਮੂਲ: wordstart = ਝੂਠ; ਰੋਕੋ;
        }

        ਚੋਣ (*cp) {
        ਹਾਲਤ '=': equalsForBracket = ਸੱਚ; ਰੋਕੋ;
        ਹਾਲਤ ' ': ਰੋਕੋ;
        ਮੂਲ: equalsForBracket = ਝੂਠ; ਰੋਕੋ;
        }
    }
}

ਖਾਲੀ V3_ਬਾਹਰਲਾ_Formatter::put_Break_ਵਿਅੰਜਕ() {
    ਜੇ (!m_parenVec.ਖਾਲੀ_ਥਾਵਾਂ()) putBreak();
}

// Add a line break if too wide
ਖਾਲੀ V3_ਬਾਹਰਲਾ_Formatter::putBreak() {
    ਜੇ (!v3Global.opt.decoration()) ਵਾਪਸ;
    ਜੇ (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        ਜੇ (exceeded_ਚੌੜਾਈ()) {
            putcNoTracking('\n');
            ਜੇ (!m_parenVec.ਖਾਲੀ_ਥਾਵਾਂ()) putsNoTracking(indentSpaces(m_parenVec.ਸਿਖਰ()));
        }
    }
}

ਖਾਲੀ V3_ਬਾਹਰਲਾ_Formatter::putsQuoted(ਅਚਲ string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ਅਚਲ string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    ਲਈ (ਅਚਲ ਅੱਖਰ c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    ਜੇ (strg.ਲੱਭੋ('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
ਖਾਲੀ V3_ਬਾਹਰਲਾ_Formatter::putsNoTracking(ਅਚਲ string& strg) {
    ਜੇ (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        ਵਾਪਸ;
    }
    // Don't track {}'s, probably because it's a $display format string
    ਲਈ (ਅਚਲ ਅੱਖਰ c : strg) putcNoTracking(c);
}

ਖਾਲੀ V3_ਬਾਹਰਲਾ_Formatter::putcNoTracking(ਅੱਖਰ chr) {
    ਜੇ (!v3Global.opt.decoration()) {
        putcOutput(chr);
        ਵਾਪਸ;
    }
    ਚੋਣ (chr) {
    ਹਾਲਤ '\n':
        ++m_lineno;
        m_ਕਾਲਮ = 0;
        m_nobreak = ਸੱਚ;
        ਰੋਕੋ;
    ਹਾਲਤ '\t': m_ਕਾਲਮ = ((m_ਕਾਲਮ + 9) / 8) * 8; ਰੋਕੋ;
    ਹਾਲਤ ' ':
    ਹਾਲਤ '(':
    ਹਾਲਤ '|':
    ਹਾਲਤ '&': ++m_ਕਾਲਮ; ਰੋਕੋ;
    ਮੂਲ:
        ++m_ਕਾਲਮ;
        m_nobreak = ਝੂਠ;
        ਰੋਕੋ;
    }
    putcOutput(chr);
}

string V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(ਅਚਲ string& namein,
                                         V3_ਬਾਹਰਲਾ_Formatter::Language lang) VL_ਸ਼ੁੱਧ {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string ਬਾਹਰਲਾ;
    ਜੇ (lang == LA_XML) {
        // Encode chars into XML string
        ਲਈ (ਅਚਲ ਅੱਖਰ c : namein) {
            ਜੇ (c == '"') {
                ਬਾਹਰਲਾ += "&quot;"s;
            } ਨਹੀਂਤਾਂ ਜੇ (c == '\'') {
                ਬਾਹਰਲਾ += "&apos;"s;
            } ਨਹੀਂਤਾਂ ਜੇ (c == '<') {
                ਬਾਹਰਲਾ += "&lt;"s;
            } ਨਹੀਂਤਾਂ ਜੇ (c == '>') {
                ਬਾਹਰਲਾ += "&gt;"s;
            } ਨਹੀਂਤਾਂ ਜੇ (c == '&') {
                ਬਾਹਰਲਾ += "&amp;"s;
            } ਨਹੀਂਤਾਂ ਜੇ (std::isprint(c)) {
                ਬਾਹਰਲਾ += c;
            } ਨਹੀਂਤਾਂ {
                ਬਾਹਰਲਾ += "&#"s + cvt_ਮੰਜ਼ਿਲ_Str(static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ ਪੂਰਨ>(c & 0xff)) + ";";
            }
        }
    } ਨਹੀਂਤਾਂ {
        // Encode control chars into C style escapes
        ਲਈ (ਅਚਲ ਅੱਖਰ c : namein) {
            ਜੇ (c == '\\' || c == '"') {
                ਬਾਹਰਲਾ += "\\"s + c;
            } ਨਹੀਂਤਾਂ ਜੇ (c == '\n') {
                ਬਾਹਰਲਾ += "\\n";
            } ਨਹੀਂਤਾਂ ਜੇ (c == '\r') {
                ਬਾਹਰਲਾ += "\\r";
            } ਨਹੀਂਤਾਂ ਜੇ (c == '\t') {
                ਬਾਹਰਲਾ += "\\t";
            } ਨਹੀਂਤਾਂ ਜੇ (std::isprint(c)) {
                ਬਾਹਰਲਾ += c;
            } ਨਹੀਂਤਾਂ {
                // This will also cover \a etc
                ਅਚਲ string octal
                    = "\\"s + cvt_ਮੰਜ਼ਿਲ_Str((c >> 6) & 3) + cvt_ਮੰਜ਼ਿਲ_Str((c >> 3) & 7) + cvt_ਮੰਜ਼ਿਲ_Str(c & 7);
                ਬਾਹਰਲਾ += octal;
            }
        }
    }
    ਵਾਪਸ ਬਾਹਰਲਾ;
}

//----------------------------------------------------------------------
// Simple wrappers

ਖਾਲੀ V3_ਬਾਹਰਲਾ_Formatter::ਲਿਖੋ(ਅਚਲ ਅੱਖਰ* fmt...) {
    constexpr ਮਾਪ_t bufsize = 5000;
    ਅੱਖਰ sbuff[bufsize];
    va_list ap;
    va_ਸ਼ੁਰੂ(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_ਸਿਰਾ(ap);
    ਇਹ->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_ਬਾਹਰਲਾ_File::V3_ਬਾਹਰਲਾ_File(ਅਚਲ string& filename, V3_ਬਾਹਰਲਾ_Formatter::Language lang)
    : V3_ਬਾਹਰਲਾ_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{ਨਵਾਂ std::array<ਅੱਖਰ, WRITE_ਬਫਰ_ਮਾਪ_ਬਾਈਟਾਂ>{}} {
    ਜੇ ((m_fp = V3File::new_fopen_w(filename)) == ਖਾਲੀ_ਸੂਚਕ) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_ਬਾਹਰਲਾ_File::~V3_ਬਾਹਰਲਾ_File() {
    write_ਖੰਡ();

    ਜੇ (m_fp) fclose(m_fp);
    m_fp = ਖਾਲੀ_ਸੂਚਕ;
}

ਖਾਲੀ V3_ਬਾਹਰਲਾ_File::putsForceIncs() {
    ਅਚਲ VStringList& forceIncs = v3Global.opt.forceIncs();
    ਲਈ (ਅਚਲ string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

ਖਾਲੀ V3_ਬਾਹਰਲਾ_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = ਸੱਚ;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    ਲਈ (ਅੱਖਰ& c : var) {
        ਜੇ (!std::ਅੱਖਰ_ਜਾਂ_ਅੰਕ(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_ਬਾਹਰਲਾ_ਧਾਰਾ::V3_ਬਾਹਰਲਾ_ਧਾਰਾ(std::ostream& ostream, V3_ਬਾਹਰਲਾ_Formatter::Language lang)
    : V3_ਬਾਹਰਲਾ_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

ਵਰਗ VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ਅਚਲ std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_ਸੈੱਟ<std::string> m_new_Id_ਸੈੱਟ VL_GUARDED_BY(m_mutex);  // Which new names exist
ਸੁਰੱਖਿਅਤ:
    // CONSTRUCTORS
    ਮਿੱਤਰ ਵਰਗ VIdProtect;
    ਸਥਿਰ VIdProtectImp& singleton() VL_MT_ਸੁਰੱਖਿਅਤ {
        ਸਥਿਰ VIdProtectImp s;
        ਵਾਪਸ s;
    }

ਜਨਤਕ:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = ਮੂਲ;
    // METHODS
    string passthru(ਅਚਲ string& old) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
        ਜੇ (!v3Global.opt.protectIds()) ਵਾਪਸ old;
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_nameMap.ਲੱਭੋ(old);
        ਜੇ (it != m_nameMap.ਸਿਰਾ()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->ਦੂਜਾ,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } ਨਹੀਂਤਾਂ {
            m_nameMap.emplace(old, old);
            m_new_Id_ਸੈੱਟ.ਪਾਓ(old);
        }
        ਵਾਪਸ old;
    }
    string protectIf(ਅਚਲ string& old, ਤਾਰਕਿਕ doIt) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
        ਜੇ (!v3Global.opt.protectIds() || old.ਖਾਲੀ_ਥਾਵਾਂ() || !doIt) ਵਾਪਸ old;
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_nameMap.emplace(old, "");
        ਜੇ (pair.ਦੂਜਾ) {
            string ਬਾਹਰਲਾ;
            ਜੇ (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                ਬਾਹਰਲਾ = "PS" + old;
            } ਨਹੀਂਤਾਂ {
                VHashSha512 digest{v3Global.opt.protect_ਕੁੰਜੀ_Defaulted()};
                digest.ਪਾਓ(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                ਬਾਹਰਲਾ = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                ਲਈ (ਮਾਪ_t len = 6; len < ਬਾਹਰਲਾ.ਮਾਪ() - 3; len += 3) {
                    ਅਚਲ string tryout = ਬਾਹਰਲਾ.substr(0, len);
                    ਜੇ (m_new_Id_ਸੈੱਟ.ਪਾਓ(tryout).ਦੂਜਾ) {
                        ਬਾਹਰਲਾ = tryout;
                        ਰੋਕੋ;
                    }
                }
            }
            pair.ਪਹਿਲਾ->ਦੂਜਾ = ਬਾਹਰਲਾ;
        }
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }
    string protect_ਸ਼ਬਦ_If(ਅਚਲ string& old, ਤਾਰਕਿਕ doIt) VL_MT_ਸੁਰੱਖਿਅਤ {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        ਜੇ (!(doIt && v3Global.opt.protectIds())) ਵਾਪਸ old;
        string ਬਾਹਰਲਾ;
        string::ਮਾਪ_type ਸ਼ੁਰੂ = 0;
        // space, ., ->
        ਜਦੋਂ (ਸੱਚ) {
            // When C++11, use find_if and lambda
            string::ਮਾਪ_type pos = string::npos;
            string separator;
            trySep(old, ਸ਼ੁਰੂ, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, ਸ਼ੁਰੂ, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, ਸ਼ੁਰੂ, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, ਸ਼ੁਰੂ, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, ਸ਼ੁਰੂ, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, ਸ਼ੁਰੂ, ")", pos /*ref*/, separator /*ref*/);
            ਜੇ (pos == string::npos) ਰੋਕੋ;
            ਬਾਹਰਲਾ += protectIf(old.substr(ਸ਼ੁਰੂ, pos - ਸ਼ੁਰੂ), ਸੱਚ) + separator;
            ਸ਼ੁਰੂ = pos + separator.ਲੰਮਾਈ();
        }
        ਬਾਹਰਲਾ += protectIf(old.substr(ਸ਼ੁਰੂ), ਸੱਚ);
        ਵਾਪਸ ਬਾਹਰਲਾ;
    }
    ਖਾਲੀ writeMapFile(ਅਚਲ string& filename) ਅਚਲ {
        V3_ਬਾਹਰਲਾ_Xml_File ਦਾ{filename};
        ਦਾ.puts_ਸਿਰਲੇਖ();
        ਦਾ.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        ਦਾ.puts("<verilator_id_map>\n");
        {
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_nameMap) {
                ਦਾ.puts("<map from=\"" + itr.ਦੂਜਾ + "\" to=\"" + itr.ਪਹਿਲਾ + "\"/>\n");
            }
        }
        ਦਾ.puts("</verilator_id_map>\n");
    }

ਨਿਜੀ:
    ਖਾਲੀ trySep(ਅਚਲ string& old, string::ਮਾਪ_type ਸ਼ੁਰੂ, ਅਚਲ string& trySep,
                string::ਮਾਪ_type& posr, string& separatorr) VL_ਸ਼ੁੱਧ {
        ਅਚਲ string::ਮਾਪ_type trypos = old.ਲੱਭੋ(trySep, ਸ਼ੁਰੂ);
        ਜੇ (trypos != string::npos) {
            ਜੇ (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ਅਚਲ string& ਵਿੱਚ_2) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ v3Global.opt.protectIds() ? "" : ਵਿੱਚ_2;
}
string VIdProtect::protectIf(ਅਚਲ string& old, ਤਾਰਕਿਕ doIt) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_ਸ਼ਬਦ_If(ਅਚਲ string& old, ਤਾਰਕਿਕ doIt) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ VIdProtectImp::singleton().protect_ਸ਼ਬਦ_If(old, doIt);
}
ਖਾਲੀ VIdProtect::writeMapFile(ਅਚਲ string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
