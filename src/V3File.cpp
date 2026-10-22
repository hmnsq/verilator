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
#ಯದಿ defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#ಅನ್ಯಥಾ
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr ಪೂರ್ಣಾಙ್ಕ INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr ಪೂರ್ಣಾಙ್ಕ INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

ವರ್ಗ V3FileDependImp final {
    // TYPES
    ವರ್ಗ DependFile final {
        // A single file
        ಧ್ರುವ ತಾರ್ಕಿಕ m_target;  // True if write, else read
        ತಾರ್ಕಿಕ m_exists = ಸತ್ಯ;  // File exists
        ಧ್ರುವ string m_filename;  // Filename
        ಸಂರಚನಾ stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    ಸಾರ್ವಜನಿಕ:
        DependFile(ಧ್ರುವ string& filename, ತಾರ್ಕಿಕ target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = ಸ್ವಭಾವ;
        ಧ್ರುವ string& filename() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_filename; }
        ತಾರ್ಕಿಕ target() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_target; }
        ತಾರ್ಕಿಕ exists() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_exists; }
        off_t size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stat.st_size; }
        ino_t ino() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stat.st_ino; }
        time_t cstime() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stat.st_ctime; }  // Seconds
        time_t cnstime() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stat.st_mtime; }  // Seconds
        time_t mnstime() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_ಚಿಹ್ನೆ() {
            ಸ್ಥಿರ VHashSha512 s_emptyHash;
            ಪ್ರತ್ಯಾಗಮ m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        ಶೂನ್ಯ loadStats() {
            ಯದಿ (!m_stat.st_mtime) {
                ಧ್ರುವ string fn = filename();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ err = stat(fn.c_str(), &m_stat);
                ಯದಿ (err != 0) {
                    std::memset(&m_stat, 0, ಪರಿಮಾಣ(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = ಅಸತ್ಯ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } ಅನ್ಯಥಾ {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    ಯದಿ (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.ಸೇರಿಸು_File(filename());
                }
            }
        }
        ತಾರ್ಕಿಕ operator<(ಧ್ರುವ DependFile& rhs) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    ಸ್ಥಿರ string stripQuotes(ಧ್ರುವ string& in) {
        string pretty = in;
        string::size_type pos;
        ಯಾವತ್ ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        ಯಾವತ್ ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        ಪ್ರತ್ಯಾಗಮ pretty;
    }

ಸಾರ್ವಜನಿಕ:
    // ACCESSOR METHODS
    ಶೂನ್ಯ addSrcDepend(ಧ್ರುವ string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        ಧ್ರುವ auto itFoundPair = m_filenameSet.ಸೇರಿಸು(filename);
        ಯದಿ (itFoundPair.second) {
            DependFile df{filename, ಅಸತ್ಯ};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.ಸೇರಿಸು(df);
        }
    }
    ಶೂನ್ಯ addTgtDepend(ಧ್ರುವ string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        ಧ್ರುವ auto itFoundPair = m_filenameSet.ಸೇರಿಸು(filename);
        ಯದಿ (itFoundPair.second) m_filenameList.ಸೇರಿಸು(DependFile{filename, ಸತ್ಯ});
    }
    ಶೂನ್ಯ writeDepend(ಧ್ರುವ string& filename);
    std::vector<string> getAllDeps() ಧ್ರುವ;
    ಶೂನ್ಯ writeTimes(ಧ್ರುವ string& filename, ಧ್ರುವ string& cmdlineIn);
    ತಾರ್ಕಿಕ checkTimes(ಧ್ರುವ string& filename, ಧ್ರುವ string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

ಶೂನ್ಯ V3FileDependImp::writeDepend(ಧ್ರುವ string& filename) {
    ಧ್ರುವ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ಯದಿ (ofp->fail()) v3fatal("Can't write file: " << filename);

    ಪ್ರತಿ (ಧ್ರುವ DependFile& i : m_filenameList) {
        ಯದಿ (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    ಪ್ರತಿ (ಧ್ರುವ DependFile& i : m_filenameList) {
        ಯದಿ (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    ಯದಿ (v3Global.opt.makePhony()) {
        *ofp << '\n';
        ಪ್ರತಿ (ಧ್ರುವ DependFile& i : m_filenameList) {
            ಯದಿ (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() ಧ್ರುವ {
    std::vector<string> r;
    ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_filenameList) {
        ಯದಿ (!itr.target() && itr.exists()) r.ತಳ್ಳು_back(itr.filename());
    }
    ಪ್ರತ್ಯಾಗಮ r;
}

ಶೂನ್ಯ V3FileDependImp::writeTimes(ಧ್ರುವ string& filename, ಧ್ರುವ string& cmdlineIn) {
    ಧ್ರುವ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    ಯದಿ (ofp->fail()) v3fatal("Can't write file: " << filename);

    ಧ್ರುವ string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    ಪ್ರತಿ (auto& it : m_filenameList) {
        DependFile* ಧ್ರುವ depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        ಯದಿ (depfilep->filename() == filename) {
            showSize = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->target() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << showSize;
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

ತಾರ್ಕಿಕ V3FileDependImp::checkTimes(ಧ್ರುವ string& filename, ಧ್ರುವ string& cmdlineIn) {
    ಧ್ರುವ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    ಯದಿ (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    {
        ಧ್ರುವ string ignore = V3Os::getline(*ifp);
        ಯದಿ (ignore.empty()) { /*used*/
        }
    }
    {
        ವರ್ಣ chkDir;
        *ifp >> chkDir;
        ವರ್ಣ quote;
        *ifp >> quote;
        ಧ್ರುವ string chkCmdline = V3Os::getline(*ifp, '"');
        ಧ್ರುವ string cmdline = stripQuotes(cmdlineIn);
        ಯದಿ (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
    }

    ಧ್ರುವ ತಾರ್ಕಿಕ skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    ಯಾವತ್ (!ifp->eof()) {
        ವರ್ಣ chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        ಯದಿ (ifp->eof()) ಭಙ್ಗ;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        ವರ್ಣ quote;
        *ifp >> quote;
        ಧ್ರುವ string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ಧ್ರುವ string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ಸಂರಚನಾ stat curStat;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ err = stat(chkFilename.c_str(), &curStat);
        ಯದಿ (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        ಯದಿ (filename == chkFilename) ಅನುವರ್ತ;

        UINFO(9, " got d=" << chkDir << " s=" << chkSize << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_size << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        ಯದಿ (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            ಯದಿ (skipHashing) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            ಯದಿ (chkHash == "unhashed") ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

            VHashSha512 curHash;
            curHash.ಸೇರಿಸು_File(chkFilename);
            ಯದಿ (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

//######################################################################
// V3File

ಶೂನ್ಯ V3File::addSrcDepend(ಧ್ರುವ string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
ಶೂನ್ಯ V3File::addTgtDepend(ಧ್ರುವ string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
ಶೂನ್ಯ V3File::writeDepend(ಧ್ರುವ string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { ಪ್ರತ್ಯಾಗಮ dependImp.getAllDeps(); }
ಶೂನ್ಯ V3File::writeTimes(ಧ್ರುವ string& filename, ಧ್ರುವ string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
ತಾರ್ಕಿಕ V3File::checkTimes(ಧ್ರುವ string& filename, ಧ್ರುವ string& cmdlineIn) {
    ಪ್ರತ್ಯಾಗಮ dependImp.checkTimes(filename, cmdlineIn);
}
ಶೂನ್ಯ V3File::createMakeDirFor(ಧ್ರುವ string& filename) {
    ಯದಿ (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
ಶೂನ್ಯ V3File::createMakeDir() {
    ಸ್ಥಿರ ತಾರ್ಕಿಕ s_created = ಅಸತ್ಯ;
    ಯದಿ (!s_created) {
        s_created = ಸತ್ಯ;
        V3Os::createDir(v3Global.opt.makeDir());
        ಯದಿ (v3Global.opt.hier_ತುದಿ()) V3Os::createDir(v3Global.opt.hier_ತುದಿ_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

ವರ್ಗ VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<ಧ್ರುವ std::string, std::string> m_contentsMap;  // Cache of file contents
    ತಾರ್ಕಿಕ m_readEof = ಅಸತ್ಯ;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#ಅನ್ಯಥಾ
    ಪೂರ್ಣಾಙ್ಕ m_pid = 0;  // fork() process id - always zero as disabled
#endif
    ತಾರ್ಕಿಕ m_pidExited = ಅಸತ್ಯ;  //  waitpid() reaped the filter process
    ಪೂರ್ಣಾಙ್ಕ m_pidStatus = 0;  // Exit status of the filter process
    ಪೂರ್ಣಾಙ್ಕ m_writeFd = 0;  // File descriptor TO filter
    ಪೂರ್ಣಾಙ್ಕ m_readFd = 0;  // File descriptor FROM filter

ಗುಪ್ತ:
    // METHODS

    ತಾರ್ಕಿಕ readContents(ಧ್ರುವ string& filename, StrList& outl) {
        ಯದಿ (m_pid) {
            ಪ್ರತ್ಯಾಗಮ readContentsFilter(filename, outl);
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ readContentsFile(filename, outl);
        }
    }
    ತಾರ್ಕಿಕ readContentsFile(ಧ್ರುವ string& filename, StrList& outl) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ fd = open(filename.c_str(), O_RDONLY);
        ಯದಿ (fd < 0) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        m_readEof = ಅಸತ್ಯ;
        readBlocks(fd, -1, outl);
        close(fd);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    // cppcheck-suppress constParameterReference
    ತಾರ್ಕಿಕ readContentsFilter(ಧ್ರುವ string& filename, StrList& outl) {
        (ಶೂನ್ಯ)filename;  // Prevent unused variable warning
        (ಶೂನ್ಯ)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ಧ್ರುವ string ಸಾಲು = read_Filter_ಸಾಲು();
        ಯದಿ (ಸಾಲು.find("Content-Length") != string::npos) {
            ಪೂರ್ಣಾಙ್ಕ len = 0;
            sscanf(ಸಾಲು.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            ಯದಿ (ಸಾಲು != "") v3error("--pipe-filter protocol error, unexpected: " << ಸಾಲು);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
#ಅನ್ಯಥಾ
        v3fatalSrc("--pipe-filter not implemented on this platform");
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
#endif
    }

    // cppcheck-suppress functionConst
    ಶೂನ್ಯ checkFilter(ತಾರ್ಕಿಕ hang) {
#ifdef INFILTER_PIPE
        ಯದಿ (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = ಸತ್ಯ;
            m_pidExited = ಸತ್ಯ;
        }
#endif
    }

    ಶೂನ್ಯ readBlocks(ಪೂರ್ಣಾಙ್ಕ fd, ಪೂರ್ಣಾಙ್ಕ size, StrList& outl) {
        ವರ್ಣ buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        ಯಾವತ್ (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            ಯದಿ (size > 0 && size < todo) todo = size;
            errno = 0;
            ಧ್ರುವ ssize_t got = ಪಠ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ಯದಿ (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } ಅನ್ಯಥಾ ಯದಿ (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(ಅಸತ್ಯ);
                V3Os::u_sleep(1000);
                ಅನುವರ್ತ;
            } ಅನ್ಯಥಾ {
                m_readEof = ಸತ್ಯ;
                ಭಙ್ಗ;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_ಸಾಲು() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string ಸಾಲು;
        ಯಾವತ್ (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            ಧ್ರುವ string onechar = listString(outl);
            ಸಾಲು += onechar;
            ಯದಿ (onechar == "\n") {
                ಯದಿ (ಸಾಲು == "\n") {
                    ಸಾಲು = "";
                    ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ {
                    ಭಙ್ಗ;
                }
            }
        }
        UINFO(6, "filter-line-in: " << ಸಾಲು);
        ಪ್ರತ್ಯಾಗಮ ಸಾಲು;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    ಶೂನ್ಯ writeFilter(ಧ್ರುವ string& out) {
        ಯದಿ (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            ಯದಿ (out[out.length() - 1] != '\n') cout << '\n';
        }
        ಯದಿ (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = ಸತ್ಯ;
            stop();
        }
        unsigned offset = 0;
        ಯಾವತ್ (!m_readEof && out.length() > offset) {
            errno = 0;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ got = ಲಿಖ(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            ಯದಿ (got > 0) {
                offset += got;
            } ಅನ್ಯಥಾ ಯದಿ (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(ಅಸತ್ಯ);
                V3Os::u_sleep(1000);
                ಅನುವರ್ತ;
            } ಅನ್ಯಥಾ {
                ಭಙ್ಗ;
            }
        }
    }

    // Start the filter
    ಶೂನ್ಯ start(ಧ್ರುವ string& command) {
        ಯದಿ (command == "") {
            m_pid = 0;  // Disabled
        } ಅನ್ಯಥಾ {
            startFilter(command);
        }
    }
    ಶೂನ್ಯ startFilter(ಧ್ರುವ string& command) {
        (ಶೂನ್ಯ)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        ಪೂರ್ಣಾಙ್ಕ fd_stdin[2];  // Can't use std::array
        ಪೂರ್ಣಾಙ್ಕ fd_stdout[2];  // Can't use std::array
        constexpr ಪೂರ್ಣಾಙ್ಕ P_RD = 0;
        constexpr ಪೂರ್ಣಾಙ್ಕ P_WR = 1;

        ಯದಿ (VL_UNCOVERABLE(ಪ್ರಣಾಲ(fd_stdin) != 0 || ಪ್ರಣಾಲ(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ಯದಿ (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ಧ್ರುವ pid_t pid = fork();
        ಯದಿ (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        ಯದಿ (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<ವರ್ಣ*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } ಅನ್ಯಥಾ {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = ಅಸತ್ಯ;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = ಅಸತ್ಯ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            ಪೂರ್ಣಾಙ್ಕ flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#ಅನ್ಯಥಾ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    ಶೂನ್ಯ stop() {
        ಯದಿ (m_pid) stopFilter();
    }
    ಶೂನ್ಯ stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(ಸತ್ಯ);
        ಯದಿ (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#ಅನ್ಯಥಾ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

ರಕ್ಷಿತ:
    friend ವರ್ಗ VInFilter;
    // Read file contents and return it
    ತಾರ್ಕಿಕ readWholefile(ಧ್ರುವ string& filename, StrList& outl) {
        ಧ್ರುವ auto it = m_contentsMap.find(filename);
        ಯದಿ (it != m_contentsMap.end()) {
            outl.ತಳ್ಳು_back(it->second);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (!readContents(filename, outl)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಸ್ಥಿರ size_t listSize(ಧ್ರುವ StrList& sl) {
        size_t result = 0;
        ಪ್ರತಿ (ಧ್ರುವ string& i : sl) result += i.length();
        ಪ್ರತ್ಯಾಗಮ result;
    }
    ಸ್ಥಿರ string listString(ಧ್ರುವ StrList& sl) {
        string result;
        ಪ್ರತಿ (ಧ್ರುವ string& i : sl) result += i;
        ಪ್ರತ್ಯಾಗಮ result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(ಧ್ರುವ string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ಧ್ರುವ string& command) { m_impp = ನೂತನ VInFilterImp{command}; }
VInFilter::~VInFilter() {
    ಯದಿ (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

ತಾರ್ಕಿಕ VInFilter::readWholefile(ಧ್ರುವ string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    ಪ್ರತ್ಯಾಗಮ m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ಸ್ಥಿರ ಧ್ರುವ std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    ಪ್ರತ್ಯಾಗಮ table;
}();

ಧ್ರುವ std::string& V3OutFormatter::indentSpaces(ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ) {
    // Indent the specified number of spaces.
    ಪ್ರತ್ಯಾಗಮ s_indentSpaces[std::max(0, std::min(ಅಂಕೆ, MAXSPACE))];
}

ತಾರ್ಕಿಕ V3OutFormatter::tokenMatch(ಧ್ರುವ ವರ್ಣ* cp, ಧ್ರುವ ವರ್ಣ* cmp) {
    ಯಾವತ್ (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    ಯದಿ (*cmp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (*cp && !std::isspace(*cp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

ತಾರ್ಕಿಕ V3OutFormatter::tokenStart(ಧ್ರುವ ವರ್ಣ* cp) {
    ಪ್ರತ್ಯಾಗಮ (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

ತಾರ್ಕಿಕ V3OutFormatter::tokenEnd(ಧ್ರುವ ವರ್ಣ* cp) {
    ಪ್ರತ್ಯಾಗಮ (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

ತಾರ್ಕಿಕ V3OutFormatter::tokenNotStart(ಧ್ರುವ ವರ್ಣ* cp) {
    ಪ್ರತ್ಯಾಗಮ (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

ಪೂರ್ಣಾಙ್ಕ V3OutFormatter::endLevels(ಧ್ರುವ ವರ್ಣ* strg) {
    ಪೂರ್ಣಾಙ್ಕ levels = m_indentLevel;
    {
        ಧ್ರುವ ವರ್ಣ* cp = strg;
        ಯಾವತ್ (std::isspace(*cp)) ++cp;
        ವಿಕಲ್ಪ (*cp) {
        ಪಕ್ಷ '\n':  // Newlines.. No need for whitespace before it
            ಪ್ರತ್ಯಾಗಮ 0;
        ಪಕ್ಷ '#':  // Preproc directive
            ಯದಿ (m_lang == LA_C) ಪ್ರತ್ಯಾಗಮ 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ಧ್ರುವ ವರ್ಣ* mp = cp;
            ಪ್ರತಿ (; std::isalnum(*mp); ++mp) {}
            ಯದಿ (mp[0] == ':' && mp[1] != ':') ಪ್ರತ್ಯಾಗಮ (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = strg; *cp; ++cp) {
        ವಿಕಲ್ಪ (*cp) {
        ಪಕ್ಷ '}':
        ಪಕ್ಷ ')': levels -= m_blockIndent; ಭಙ್ಗ;
        ಪಕ್ಷ '<':
            ಯದಿ (m_lang == LA_XML) {
                ಯದಿ (cp[1] == '/') levels -= m_blockIndent;
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ 'e':
            ಯದಿ (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            ಭಙ್ಗ;
        ಪಕ್ಷ '\t':
        ಪಕ್ಷ ' ': ಭಙ್ಗ;  // Continue
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ levels;  // Letter
        }
    }
    ಪ್ರತ್ಯಾಗಮ levels;
}

ಶೂನ್ಯ V3OutFormatter::putns(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ ವರ್ಣ* strg) {
    ಯದಿ (!v3Global.opt.decoration()) {
        putsOutput(strg);
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಧ್ರುವ ತಾರ್ಕಿಕ put_ಗಂಟು_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_ಸಾಲು::builtInFilename() != nodep->fileline()->filename();

    ಯದಿ (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_ಗಂಟು_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = ಅಸತ್ಯ;
    }

    ಯದಿ (put_ಗಂಟು_Decoration) {
        ಧ್ರುವ File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        ಧ್ರುವ std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    ತಾರ್ಕಿಕ notstart = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ wordstart = ಸತ್ಯ;
    ತಾರ್ಕಿಕ equalsForBracket = ಅಸತ್ಯ;  // Looking for "= {"
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        ಯದಿ (std::isalpha(*cp)) {
            ಯದಿ (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = ಸತ್ಯ;
            ಯದಿ (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            ಯದಿ (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ wasInBackslash = m_inBackslash;
        m_inBackslash = ಅಸತ್ಯ;
        ವಿಕಲ್ಪ (*cp) {
        ಪಕ್ಷ '\n':
            ++m_lineno;
            wordstart = ಸತ್ಯ;
            ಯದಿ (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = ಸತ್ಯ;
            } ಅನ್ಯಥಾ {
                m_prependIndent = ಅಸತ್ಯ;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ ' ': wordstart = ಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ '\t': wordstart = ಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ '"':
            wordstart = ಅಸತ್ಯ;
            ಯದಿ (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            ಭಙ್ಗ;
        ಪಕ್ಷ '\\':
            wordstart = ಅಸತ್ಯ;
            ಯದಿ (!wasInBackslash) m_inBackslash = ಸತ್ಯ;
            ಭಙ್ಗ;
        ಪಕ್ಷ '/':
            ಯದಿ (m_lang == LA_C || m_lang == LA_VERILOG) {
                ಯದಿ (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    ಯಾವತ್ (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    ಯದಿ (*cp) putcNoTracking(*cp);
                }
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ '{':
            ಯದಿ (!m_inStringLiteral) {
                ಯದಿ (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ತಳ್ಳು(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ '}':
            ಯದಿ (!m_inStringLiteral) {
                ಯದಿ (m_bracketLevel > 0) {
                    m_parenVec.ತೆಗೆ();
                    --m_bracketLevel;
                }
                indentDec();
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ '(':
            ಯದಿ (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ತಳ್ಳು(m_column);
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ ')':
            ಯದಿ (!m_inStringLiteral) {
                ಯದಿ (!m_parenVec.empty()) m_parenVec.ತೆಗೆ();
                indentDec();
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ '<':
            ಯದಿ (m_lang == LA_XML) {
                ಯದಿ (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } ಅನ್ಯಥಾ ಯದಿ (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } ಅನ್ಯಥಾ {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ '>':
            ಯದಿ (m_lang == LA_XML) {
                indentDec();
                ಯದಿ (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            ಭಙ್ಗ;
        ಸ್ವಭಾವ: wordstart = ಅಸತ್ಯ; ಭಙ್ಗ;
        }

        ವಿಕಲ್ಪ (*cp) {
        ಪಕ್ಷ '=': equalsForBracket = ಸತ್ಯ; ಭಙ್ಗ;
        ಪಕ್ಷ ' ': ಭಙ್ಗ;
        ಸ್ವಭಾವ: equalsForBracket = ಅಸತ್ಯ; ಭಙ್ಗ;
        }
    }
}

ಶೂನ್ಯ V3OutFormatter::put_Break_ಅಭಿವ್ಯಕ್ತಿ() {
    ಯದಿ (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
ಶೂನ್ಯ V3OutFormatter::putBreak() {
    ಯದಿ (!v3Global.opt.decoration()) ಪ್ರತ್ಯಾಗಮ;
    ಯದಿ (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        ಯದಿ (exceededWidth()) {
            putcNoTracking('\n');
            ಯದಿ (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.ತುದಿ()));
        }
    }
}

ಶೂನ್ಯ V3OutFormatter::putsQuoted(ಧ್ರುವ string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ಧ್ರುವ string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    ಯದಿ (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
ಶೂನ್ಯ V3OutFormatter::putsNoTracking(ಧ್ರುವ string& strg) {
    ಯದಿ (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        ಪ್ರತ್ಯಾಗಮ;
    }
    // Don't track {}'s, probably because it's a $display format string
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : strg) putcNoTracking(c);
}

ಶೂನ್ಯ V3OutFormatter::putcNoTracking(ವರ್ಣ chr) {
    ಯದಿ (!v3Global.opt.decoration()) {
        putcOutput(chr);
        ಪ್ರತ್ಯಾಗಮ;
    }
    ವಿಕಲ್ಪ (chr) {
    ಪಕ್ಷ '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = ಸತ್ಯ;
        ಭಙ್ಗ;
    ಪಕ್ಷ '\t': m_column = ((m_column + 9) / 8) * 8; ಭಙ್ಗ;
    ಪಕ್ಷ ' ':
    ಪಕ್ಷ '(':
    ಪಕ್ಷ '|':
    ಪಕ್ಷ '&': ++m_column; ಭಙ್ಗ;
    ಸ್ವಭಾವ:
        ++m_column;
        m_nobreak = ಅಸತ್ಯ;
        ಭಙ್ಗ;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(ಧ್ರುವ string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    ಯದಿ (lang == LA_XML) {
        // Encode chars into XML string
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : namein) {
            ಯದಿ (c == '"') {
                out += "&quot;"s;
            } ಅನ್ಯಥಾ ಯದಿ (c == '\'') {
                out += "&apos;"s;
            } ಅನ್ಯಥಾ ಯದಿ (c == '<') {
                out += "&lt;"s;
            } ಅನ್ಯಥಾ ಯದಿ (c == '>') {
                out += "&gt;"s;
            } ಅನ್ಯಥಾ ಯದಿ (c == '&') {
                out += "&amp;"s;
            } ಅನ್ಯಥಾ ಯದಿ (std::isprint(c)) {
                out += c;
            } ಅನ್ಯಥಾ {
                out += "&#"s + cvtToStr(static_cast<unsigned ಪೂರ್ಣಾಙ್ಕ>(c & 0xff)) + ";";
            }
        }
    } ಅನ್ಯಥಾ {
        // Encode control chars into C style escapes
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : namein) {
            ಯದಿ (c == '\\' || c == '"') {
                out += "\\"s + c;
            } ಅನ್ಯಥಾ ಯದಿ (c == '\n') {
                out += "\\n";
            } ಅನ್ಯಥಾ ಯದಿ (c == '\r') {
                out += "\\r";
            } ಅನ್ಯಥಾ ಯದಿ (c == '\t') {
                out += "\\t";
            } ಅನ್ಯಥಾ ಯದಿ (std::isprint(c)) {
                out += c;
            } ಅನ್ಯಥಾ {
                // This will also cover \a etc
                ಧ್ರುವ string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ out;
}

//----------------------------------------------------------------------
// Simple wrappers

ಶೂನ್ಯ V3OutFormatter::printf(ಧ್ರುವ ವರ್ಣ* fmt...) {
    constexpr size_t bufsize = 5000;
    ವರ್ಣ sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    ಏತತ್->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(ಧ್ರುವ string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{ನೂತನ std::array<ವರ್ಣ, WRITE_BUFFER_SIZE_BYTES>{}} {
    ಯದಿ ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    ಯದಿ (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

ಶೂನ್ಯ V3OutFile::putsForceIncs() {
    ಧ್ರುವ VStringList& forceIncs = v3Global.opt.forceIncs();
    ಪ್ರತಿ (ಧ್ರುವ string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

ಶೂನ್ಯ V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = ಸತ್ಯ;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    ಪ್ರತಿ (ವರ್ಣ& c : var) {
        ಯದಿ (!std::isalnum(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3OutStream::V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

ವರ್ಗ VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ಧ್ರುವ std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
ರಕ್ಷಿತ:
    // CONSTRUCTORS
    friend ವರ್ಗ VIdProtect;
    ಸ್ಥಿರ VIdProtectImp& singleton() VL_MT_SAFE {
        ಸ್ಥಿರ VIdProtectImp s;
        ಪ್ರತ್ಯಾಗಮ s;
    }

ಸಾರ್ವಜನಿಕ:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = ಸ್ವಭಾವ;
    // METHODS
    string passthru(ಧ್ರುವ string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ಯದಿ (!v3Global.opt.protectIds()) ಪ್ರತ್ಯಾಗಮ old;
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        ಧ್ರುವ auto it = m_nameMap.find(old);
        ಯದಿ (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } ಅನ್ಯಥಾ {
            m_nameMap.emplace(old, old);
            m_newIdSet.ಸೇರಿಸು(old);
        }
        ಪ್ರತ್ಯಾಗಮ old;
    }
    string protectIf(ಧ್ರುವ string& old, ತಾರ್ಕಿಕ doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ಯದಿ (!v3Global.opt.protectIds() || old.empty() || !doIt) ಪ್ರತ್ಯಾಗಮ old;
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        ಧ್ರುವ auto pair = m_nameMap.emplace(old, "");
        ಯದಿ (pair.second) {
            string out;
            ಯದಿ (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } ಅನ್ಯಥಾ {
                VHashSha512 digest{v3Global.opt.protect_ಕೀಲಿ_Defaulted()};
                digest.ಸೇರಿಸು(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                ಪ್ರತಿ (size_t len = 6; len < out.size() - 3; len += 3) {
                    ಧ್ರುವ string tryout = out.substr(0, len);
                    ಯದಿ (m_newIdSet.ಸೇರಿಸು(tryout).second) {
                        out = tryout;
                        ಭಙ್ಗ;
                    }
                }
            }
            pair.first->second = out;
        }
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }
    string protect_ಪದಗಳು_If(ಧ್ರುವ string& old, ತಾರ್ಕಿಕ doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        ಯದಿ (!(doIt && v3Global.opt.protectIds())) ಪ್ರತ್ಯಾಗಮ old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        ಯಾವತ್ (ಸತ್ಯ) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            ಯದಿ (pos == string::npos) ಭಙ್ಗ;
            out += protectIf(old.substr(start, pos - start), ಸತ್ಯ) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ out;
    }
    ಶೂನ್ಯ writeMapFile(ಧ್ರುವ string& filename) ಧ್ರುವ {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

ಗುಪ್ತ:
    ಶೂನ್ಯ trySep(ಧ್ರುವ string& old, string::size_type start, ಧ್ರುವ string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        ಧ್ರುವ string::size_type trypos = old.find(trySep, start);
        ಯದಿ (trypos != string::npos) {
            ಯದಿ (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ಧ್ರುವ string& in) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(ಧ್ರುವ string& old, ತಾರ್ಕಿಕ doIt) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_ಪದಗಳು_If(ಧ್ರುವ string& old, ತಾರ್ಕಿಕ doIt) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ VIdProtectImp::singleton().protect_ಪದಗಳು_If(old, doIt);
}
ಶೂನ್ಯ VIdProtect::writeMapFile(ಧ್ರುವ string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
