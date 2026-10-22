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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3File.h"

#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3String.h"

#অন্তর্ভাব <cerrno>
#অন্তর্ভাব <cstdarg>
#অন্তর্ভাব <fcntl.h>
#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <map>
#অন্তর্ভাব <memory>

#অন্তর্ভাব <sys/stat.h>
#অন্তর্ভাব <sys/types.h>

// clang-format off
#যদি defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#অন্যথা
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#যদি defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr পূর্ণসংখ্যা INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr পূর্ণসংখ্যা INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_আকার_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

শ্রেণী V3FileDependImp final {
    // TYPES
    শ্রেণী DependFile final {
        // A single file
        ধ্রুবক যৌক্তিক m_target;  // True if write, else read
        যৌক্তিক m_exists = সত্য;  // File exists
        ধ্রুবক string m_filename;  // Filename
        কাঠামো stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    সর্বজনীন:
        DependFile(ধ্রুবক string& filename, যৌক্তিক target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = পূর্বনির্ধারিত;
        ধ্রুবক string& filename() ধ্রুবক { ফেরত m_filename; }
        যৌক্তিক target() ধ্রুবক { ফেরত m_target; }
        যৌক্তিক exists() ধ্রুবক { ফেরত m_exists; }
        off_t আকার_2() ধ্রুবক { ফেরত m_stat.st_আকার; }
        ino_t ino() ধ্রুবক { ফেরত m_stat.st_ino; }
        time_t cstime() ধ্রুবক { ফেরত m_stat.st_ctime; }  // Seconds
        time_t cnstime() ধ্রুবক { ফেরত VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ধ্রুবক { ফেরত m_stat.st_mtime; }  // Seconds
        time_t mnstime() ধ্রুবক { ফেরত VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_প্রতীক() {
            স্থির VHashSha512 s_emptyHash;
            ফেরত m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        শূন্য loadStats() {
            যদি (!m_stat.st_mtime) {
                ধ্রুবক string fn = filename();
                ধ্রুবক পূর্ণসংখ্যা err = stat(fn.c_str(), &m_stat);
                যদি (err != 0) {
                    std::memset(&m_stat, 0, আকার(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = মিথ্যা;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } অন্যথা {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    যদি (!target() && আকার_2() <= FILE_HASH_আকার_MAX) m_hash.ঢোকাও_File(filename());
                }
            }
        }
        যৌক্তিক operator<(ধ্রুবক DependFile& rhs) ধ্রুবক { ফেরত filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    স্থির string stripQuotes(ধ্রুবক string& in) {
        string pretty = in;
        string::আকার_type pos;
        যখন ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        যখন ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        ফেরত pretty;
    }

সর্বজনীন:
    // ACCESSOR METHODS
    শূন্য addSrcDepend(ধ্রুবক string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ধ্রুবক V3LockGuard lock{m_mutex};
        ধ্রুবক auto itFoundPair = m_filenameSet.ঢোকাও(filename);
        যদি (itFoundPair.second) {
            DependFile df{filename, মিথ্যা};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.ঢোকাও(df);
        }
    }
    শূন্য addTgtDepend(ধ্রুবক string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ধ্রুবক V3LockGuard lock{m_mutex};
        ধ্রুবক auto itFoundPair = m_filenameSet.ঢোকাও(filename);
        যদি (itFoundPair.second) m_filenameList.ঢোকাও(DependFile{filename, সত্য});
    }
    শূন্য writeDepend(ধ্রুবক string& filename);
    std::vector<string> getAllDeps() ধ্রুবক;
    শূন্য writeTimes(ধ্রুবক string& filename, ধ্রুবক string& cmdlineIn);
    যৌক্তিক checkTimes(ধ্রুবক string& filename, ধ্রুবক string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

শূন্য V3FileDependImp::writeDepend(ধ্রুবক string& filename) {
    ধ্রুবক std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    যদি (ofp->fail()) v3fatal("Can't write file: " << filename);

    জন্য (ধ্রুবক DependFile& i : m_filenameList) {
        যদি (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    জন্য (ধ্রুবক DependFile& i : m_filenameList) {
        যদি (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    যদি (v3Global.opt.makePhony()) {
        *ofp << '\n';
        জন্য (ধ্রুবক DependFile& i : m_filenameList) {
            যদি (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() ধ্রুবক {
    std::vector<string> r;
    জন্য (ধ্রুবক auto& itr : m_filenameList) {
        যদি (!itr.target() && itr.exists()) r.ঠেলো_back(itr.filename());
    }
    ফেরত r;
}

শূন্য V3FileDependImp::writeTimes(ধ্রুবক string& filename, ধ্রুবক string& cmdlineIn) {
    ধ্রুবক std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    যদি (ofp->fail()) v3fatal("Can't write file: " << filename);

    ধ্রুবক string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    জন্য (auto& it : m_filenameList) {
        DependFile* ধ্রুবক depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t show_আকার = depfilep->আকার_2();
        ino_t showIno = depfilep->ino();
        যদি (depfilep->filename() == filename) {
            show_আকার = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->target() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_আকার;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_প্রতীক() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

যৌক্তিক V3FileDependImp::checkTimes(ধ্রুবক string& filename, ধ্রুবক string& cmdlineIn) {
    ধ্রুবক std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    যদি (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        ফেরত মিথ্যা;
    }
    {
        ধ্রুবক string ignore = V3Os::getline(*ifp);
        যদি (ignore.empty()) { /*used*/
        }
    }
    {
        অক্ষর chkDir;
        *ifp >> chkDir;
        অক্ষর quote;
        *ifp >> quote;
        ধ্রুবক string chkCmdline = V3Os::getline(*ifp, '"');
        ধ্রুবক string cmdline = stripQuotes(cmdlineIn);
        যদি (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            ফেরত মিথ্যা;
        }
    }

    ধ্রুবক যৌক্তিক skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    যখন (!ifp->eof()) {
        অক্ষর chkDir;
        *ifp >> chkDir;
        off_t chk_আকার;
        *ifp >> chk_আকার;
        ino_t chkIno;
        *ifp >> chkIno;
        যদি (ifp->eof()) বিরতি;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        অক্ষর quote;
        *ifp >> quote;
        ধ্রুবক string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ধ্রুবক string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        কাঠামো stat curStat;
        ধ্রুবক পূর্ণসংখ্যা err = stat(chkFilename.c_str(), &curStat);
        যদি (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            ফেরত মিথ্যা;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        যদি (filename == chkFilename) চালু;

        UINFO(9, " got d=" << chkDir << " s=" << chk_আকার << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_আকার << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        যদি (!(curStat.st_আকার >= chk_আকার && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_আকার << "=?" << chk_আকার << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            যদি (skipHashing) ফেরত মিথ্যা;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            যদি (chkHash == "unhashed") ফেরত মিথ্যা;

            VHashSha512 curHash;
            curHash.ঢোকাও_File(chkFilename);
            যদি (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                ফেরত মিথ্যা;
            }
        }
    }
    ফেরত সত্য;
}

//######################################################################
// V3File

শূন্য V3File::addSrcDepend(ধ্রুবক string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
শূন্য V3File::addTgtDepend(ধ্রুবক string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
শূন্য V3File::writeDepend(ধ্রুবক string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { ফেরত dependImp.getAllDeps(); }
শূন্য V3File::writeTimes(ধ্রুবক string& filename, ধ্রুবক string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
যৌক্তিক V3File::checkTimes(ধ্রুবক string& filename, ধ্রুবক string& cmdlineIn) {
    ফেরত dependImp.checkTimes(filename, cmdlineIn);
}
শূন্য V3File::createMakeDirFor(ধ্রুবক string& filename) {
    যদি (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().দৈর্ঘ্য() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
শূন্য V3File::createMakeDir() {
    স্থির যৌক্তিক s_created = মিথ্যা;
    যদি (!s_created) {
        s_created = সত্য;
        V3Os::createDir(v3Global.opt.makeDir());
        যদি (v3Global.opt.hier_শীর্ষ()) V3Os::createDir(v3Global.opt.hier_শীর্ষ_তথ্য_Dir());
    }
}

//######################################################################
// VInFilterImp

শ্রেণী VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<ধ্রুবক std::string, std::string> m_contentsMap;  // Cache of file contents
    যৌক্তিক m_readEof = মিথ্যা;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#অন্যথা
    পূর্ণসংখ্যা m_pid = 0;  // fork() process id - always zero as disabled
#endif
    যৌক্তিক m_pidExited = মিথ্যা;  //  waitpid() reaped the filter process
    পূর্ণসংখ্যা m_pidStatus = 0;  // Exit status of the filter process
    পূর্ণসংখ্যা m_writeFd = 0;  // File descriptor TO filter
    পূর্ণসংখ্যা m_readFd = 0;  // File descriptor FROM filter

ব্যক্তিগত:
    // METHODS

    যৌক্তিক readContents(ধ্রুবক string& filename, StrList& outl) {
        যদি (m_pid) {
            ফেরত readContentsFilter(filename, outl);
        } অন্যথা {
            ফেরত readContentsFile(filename, outl);
        }
    }
    যৌক্তিক readContentsFile(ধ্রুবক string& filename, StrList& outl) {
        ধ্রুবক পূর্ণসংখ্যা fd = open(filename.c_str(), O_RDONLY);
        যদি (fd < 0) ফেরত মিথ্যা;
        m_readEof = মিথ্যা;
        readBlocks(fd, -1, outl);
        close(fd);
        ফেরত সত্য;
    }
    // cppcheck-suppress constParameterReference
    যৌক্তিক readContentsFilter(ধ্রুবক string& filename, StrList& outl) {
        (শূন্য)filename;  // Prevent unused variable warning
        (শূন্য)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ধ্রুবক string সারি = read_Filter_সারি();
        যদি (সারি.find("Content-Length") != string::npos) {
            পূর্ণসংখ্যা len = 0;
            sscanf(সারি.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            ফেরত সত্য;
        } অন্যথা {
            যদি (সারি != "") v3error("--pipe-filter protocol error, unexpected: " << সারি);
            ফেরত মিথ্যা;
        }
#অন্যথা
        v3fatalSrc("--pipe-filter not implemented on this platform");
        ফেরত মিথ্যা;
#endif
    }

    // cppcheck-suppress functionConst
    শূন্য checkFilter(যৌক্তিক hang) {
#ifdef INFILTER_PIPE
        যদি (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = সত্য;
            m_pidExited = সত্য;
        }
#endif
    }

    শূন্য readBlocks(পূর্ণসংখ্যা fd, পূর্ণসংখ্যা আকার_2, StrList& outl) {
        অক্ষর buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        যখন (!m_readEof && (আকার_2 < 0 || আকার_2 > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            যদি (আকার_2 > 0 && আকার_2 < todo) todo = আকার_2;
            errno = 0;
            ধ্রুবক ssize_t got = পঠ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            যদি (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } অন্যথা যদি (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(মিথ্যা);
                V3Os::u_sleep(1000);
                চালু;
            } অন্যথা {
                m_readEof = সত্য;
                বিরতি;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_সারি() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string সারি;
        যখন (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            ধ্রুবক string onechar = listString(outl);
            সারি += onechar;
            যদি (onechar == "\n") {
                যদি (সারি == "\n") {
                    সারি = "";
                    চালু;
                } অন্যথা {
                    বিরতি;
                }
            }
        }
        UINFO(6, "filter-line-in: " << সারি);
        ফেরত সারি;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    শূন্য writeFilter(ধ্রুবক string& out) {
        যদি (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            যদি (out[out.দৈর্ঘ্য() - 1] != '\n') cout << '\n';
        }
        যদি (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = সত্য;
            stop();
        }
        unsigned offset = 0;
        যখন (!m_readEof && out.দৈর্ঘ্য() > offset) {
            errno = 0;
            ধ্রুবক পূর্ণসংখ্যা got = লিখ(m_writeFd, (out.c_str()) + offset, out.দৈর্ঘ্য() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            যদি (got > 0) {
                offset += got;
            } অন্যথা যদি (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(মিথ্যা);
                V3Os::u_sleep(1000);
                চালু;
            } অন্যথা {
                বিরতি;
            }
        }
    }

    // Start the filter
    শূন্য start(ধ্রুবক string& command) {
        যদি (command == "") {
            m_pid = 0;  // Disabled
        } অন্যথা {
            startFilter(command);
        }
    }
    শূন্য startFilter(ধ্রুবক string& command) {
        (শূন্য)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        পূর্ণসংখ্যা fd_stdin[2];  // Can't use std::array
        পূর্ণসংখ্যা fd_stdout[2];  // Can't use std::array
        constexpr পূর্ণসংখ্যা P_RD = 0;
        constexpr পূর্ণসংখ্যা P_WR = 1;

        যদি (VL_UNCOVERABLE(প্রণাল(fd_stdin) != 0 || প্রণাল(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        যদি (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ধ্রুবক pid_t pid = fork();
        যদি (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        যদি (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<অক্ষর*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } অন্যথা {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = মিথ্যা;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = মিথ্যা;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            পূর্ণসংখ্যা flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#অন্যথা
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    শূন্য stop() {
        যদি (m_pid) stopFilter();
    }
    শূন্য stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(সত্য);
        যদি (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#অন্যথা
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

সুরক্ষিত:
    friend শ্রেণী VInFilter;
    // Read file contents and return it
    যৌক্তিক readWholefile(ধ্রুবক string& filename, StrList& outl) {
        ধ্রুবক auto it = m_contentsMap.find(filename);
        যদি (it != m_contentsMap.end()) {
            outl.ঠেলো_back(it->second);
            ফেরত সত্য;
        }
        যদি (!readContents(filename, outl)) ফেরত মিথ্যা;
        যদি (list_আকার(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        ফেরত সত্য;
    }
    স্থির আকার_t list_আকার(ধ্রুবক StrList& sl) {
        আকার_t ফলাফল = 0;
        জন্য (ধ্রুবক string& i : sl) ফলাফল += i.দৈর্ঘ্য();
        ফেরত ফলাফল;
    }
    স্থির string listString(ধ্রুবক StrList& sl) {
        string ফলাফল;
        জন্য (ধ্রুবক string& i : sl) ফলাফল += i;
        ফেরত ফলাফল;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(ধ্রুবক string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ধ্রুবক string& command) { m_impp = নতুন VInFilterImp{command}; }
VInFilter::~VInFilter() {
    যদি (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

যৌক্তিক VInFilter::readWholefile(ধ্রুবক string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    ফেরত m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

স্থির constexpr পূর্ণসংখ্যা MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
স্থির ধ্রুবক std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    জন্য (পূর্ণসংখ্যা i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<আকার_t>(i), ' ');
    ফেরত table;
}();

ধ্রুবক std::string& V3OutFormatter::indentSpaces(পূর্ণসংখ্যা অঙ্ক) {
    // Indent the specified number of spaces.
    ফেরত s_indentSpaces[std::max(0, std::min(অঙ্ক, MAXSPACE))];
}

যৌক্তিক V3OutFormatter::tokenMatch(ধ্রুবক অক্ষর* cp, ধ্রুবক অক্ষর* cmp) {
    যখন (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    যদি (*cmp) ফেরত মিথ্যা;
    যদি (*cp && !std::isspace(*cp)) ফেরত মিথ্যা;
    ফেরত সত্য;
}

যৌক্তিক V3OutFormatter::tokenStart(ধ্রুবক অক্ষর* cp) {
    ফেরত (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

যৌক্তিক V3OutFormatter::tokenEnd(ধ্রুবক অক্ষর* cp) {
    ফেরত (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

যৌক্তিক V3OutFormatter::tokenNotStart(ধ্রুবক অক্ষর* cp) {
    ফেরত (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

পূর্ণসংখ্যা V3OutFormatter::endLevels(ধ্রুবক অক্ষর* strg) {
    পূর্ণসংখ্যা levels = m_indentLevel;
    {
        ধ্রুবক অক্ষর* cp = strg;
        যখন (std::isspace(*cp)) ++cp;
        নির্বাচন (*cp) {
        ক্ষেত্র '\n':  // Newlines.. No need for whitespace before it
            ফেরত 0;
        ক্ষেত্র '#':  // Preproc directive
            যদি (m_lang == LA_C) ফেরত 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ধ্রুবক অক্ষর* mp = cp;
            জন্য (; std::isalnum(*mp); ++mp) {}
            যদি (mp[0] == ':' && mp[1] != ':') ফেরত (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    জন্য (ধ্রুবক অক্ষর* cp = strg; *cp; ++cp) {
        নির্বাচন (*cp) {
        ক্ষেত্র '}':
        ক্ষেত্র ')': levels -= m_blockIndent; বিরতি;
        ক্ষেত্র '<':
            যদি (m_lang == LA_XML) {
                যদি (cp[1] == '/') levels -= m_blockIndent;
            }
            বিরতি;
        ক্ষেত্র 'e':
            যদি (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            বিরতি;
        ক্ষেত্র '\t':
        ক্ষেত্র ' ': বিরতি;  // Continue
        পূর্বনির্ধারিত: ফেরত levels;  // Letter
        }
    }
    ফেরত levels;
}

শূন্য V3OutFormatter::putns(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক অক্ষর* strg) {
    যদি (!v3Global.opt.decoration()) {
        putsOutput(strg);
        ফেরত;
    }

    ধ্রুবক যৌক্তিক put_গ্রন্থি_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_সারি::builtInFilename() != nodep->fileline()->filename();

    যদি (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_গ্রন্থি_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = মিথ্যা;
    }

    যদি (put_গ্রন্থি_Decoration) {
        ধ্রুবক File_সারি* ধ্রুবক flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        ধ্রুবক std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    যৌক্তিক notstart = মিথ্যা;
    যৌক্তিক wordstart = সত্য;
    যৌক্তিক equalsForBracket = মিথ্যা;  // Looking for "= {"
    জন্য (ধ্রুবক অক্ষর* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        যদি (std::isalpha(*cp)) {
            যদি (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = সত্য;
            যদি (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            যদি (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        ধ্রুবক যৌক্তিক wasInBackslash = m_inBackslash;
        m_inBackslash = মিথ্যা;
        নির্বাচন (*cp) {
        ক্ষেত্র '\n':
            ++m_lineno;
            wordstart = সত্য;
            যদি (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = সত্য;
            } অন্যথা {
                m_prependIndent = মিথ্যা;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            বিরতি;
        ক্ষেত্র ' ': wordstart = সত্য; বিরতি;
        ক্ষেত্র '\t': wordstart = সত্য; বিরতি;
        ক্ষেত্র '"':
            wordstart = মিথ্যা;
            যদি (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            বিরতি;
        ক্ষেত্র '\\':
            wordstart = মিথ্যা;
            যদি (!wasInBackslash) m_inBackslash = সত্য;
            বিরতি;
        ক্ষেত্র '/':
            যদি (m_lang == LA_C || m_lang == LA_VERILOG) {
                যদি (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    যখন (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    যদি (*cp) putcNoTracking(*cp);
                }
            }
            বিরতি;
        ক্ষেত্র '{':
            যদি (!m_inStringLiteral) {
                যদি (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ঠেলো(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            বিরতি;
        ক্ষেত্র '}':
            যদি (!m_inStringLiteral) {
                যদি (m_bracketLevel > 0) {
                    m_parenVec.বের_করো();
                    --m_bracketLevel;
                }
                indentDec();
            }
            বিরতি;
        ক্ষেত্র '(':
            যদি (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ঠেলো(m_column);
            }
            বিরতি;
        ক্ষেত্র ')':
            যদি (!m_inStringLiteral) {
                যদি (!m_parenVec.empty()) m_parenVec.বের_করো();
                indentDec();
            }
            বিরতি;
        ক্ষেত্র '<':
            যদি (m_lang == LA_XML) {
                যদি (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } অন্যথা যদি (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } অন্যথা {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            বিরতি;
        ক্ষেত্র '>':
            যদি (m_lang == LA_XML) {
                indentDec();
                যদি (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            বিরতি;
        পূর্বনির্ধারিত: wordstart = মিথ্যা; বিরতি;
        }

        নির্বাচন (*cp) {
        ক্ষেত্র '=': equalsForBracket = সত্য; বিরতি;
        ক্ষেত্র ' ': বিরতি;
        পূর্বনির্ধারিত: equalsForBracket = মিথ্যা; বিরতি;
        }
    }
}

শূন্য V3OutFormatter::put_Break_রাশিমালা() {
    যদি (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
শূন্য V3OutFormatter::putBreak() {
    যদি (!v3Global.opt.decoration()) ফেরত;
    যদি (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        যদি (exceededWidth()) {
            putcNoTracking('\n');
            যদি (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.শীর্ষ()));
        }
    }
}

শূন্য V3OutFormatter::putsQuoted(ধ্রুবক string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ধ্রুবক string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    জন্য (ধ্রুবক অক্ষর c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    যদি (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
শূন্য V3OutFormatter::putsNoTracking(ধ্রুবক string& strg) {
    যদি (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        ফেরত;
    }
    // Don't track {}'s, probably because it's a $display format string
    জন্য (ধ্রুবক অক্ষর c : strg) putcNoTracking(c);
}

শূন্য V3OutFormatter::putcNoTracking(অক্ষর chr) {
    যদি (!v3Global.opt.decoration()) {
        putcOutput(chr);
        ফেরত;
    }
    নির্বাচন (chr) {
    ক্ষেত্র '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = সত্য;
        বিরতি;
    ক্ষেত্র '\t': m_column = ((m_column + 9) / 8) * 8; বিরতি;
    ক্ষেত্র ' ':
    ক্ষেত্র '(':
    ক্ষেত্র '|':
    ক্ষেত্র '&': ++m_column; বিরতি;
    পূর্বনির্ধারিত:
        ++m_column;
        m_nobreak = মিথ্যা;
        বিরতি;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(ধ্রুবক string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    যদি (lang == LA_XML) {
        // Encode chars into XML string
        জন্য (ধ্রুবক অক্ষর c : namein) {
            যদি (c == '"') {
                out += "&quot;"s;
            } অন্যথা যদি (c == '\'') {
                out += "&apos;"s;
            } অন্যথা যদি (c == '<') {
                out += "&lt;"s;
            } অন্যথা যদি (c == '>') {
                out += "&gt;"s;
            } অন্যথা যদি (c == '&') {
                out += "&amp;"s;
            } অন্যথা যদি (std::isprint(c)) {
                out += c;
            } অন্যথা {
                out += "&#"s + cvtToStr(static_cast<unsigned পূর্ণসংখ্যা>(c & 0xff)) + ";";
            }
        }
    } অন্যথা {
        // Encode control chars into C style escapes
        জন্য (ধ্রুবক অক্ষর c : namein) {
            যদি (c == '\\' || c == '"') {
                out += "\\"s + c;
            } অন্যথা যদি (c == '\n') {
                out += "\\n";
            } অন্যথা যদি (c == '\r') {
                out += "\\r";
            } অন্যথা যদি (c == '\t') {
                out += "\\t";
            } অন্যথা যদি (std::isprint(c)) {
                out += c;
            } অন্যথা {
                // This will also cover \a etc
                ধ্রুবক string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    ফেরত out;
}

//----------------------------------------------------------------------
// Simple wrappers

শূন্য V3OutFormatter::printf(ধ্রুবক অক্ষর* fmt...) {
    constexpr আকার_t bufsize = 5000;
    অক্ষর sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    এই->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(ধ্রুবক string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{নতুন std::array<অক্ষর, WRITE_বাফার_আকার_BYTES>{}} {
    যদি ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    যদি (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

শূন্য V3OutFile::putsForceIncs() {
    ধ্রুবক VStringList& forceIncs = v3Global.opt.forceIncs();
    জন্য (ধ্রুবক string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

শূন্য V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = সত্য;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    জন্য (অক্ষর& c : var) {
        যদি (!std::isalnum(c)) c = '_';
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

শ্রেণী VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ধ্রুবক std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
সুরক্ষিত:
    // CONSTRUCTORS
    friend শ্রেণী VIdProtect;
    স্থির VIdProtectImp& singleton() VL_MT_SAFE {
        স্থির VIdProtectImp s;
        ফেরত s;
    }

সর্বজনীন:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = পূর্বনির্ধারিত;
    // METHODS
    string passthru(ধ্রুবক string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        যদি (!v3Global.opt.protectIds()) ফেরত old;
        ধ্রুবক V3LockGuard lock{m_mutex};
        ধ্রুবক auto it = m_nameMap.find(old);
        যদি (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } অন্যথা {
            m_nameMap.emplace(old, old);
            m_newIdSet.ঢোকাও(old);
        }
        ফেরত old;
    }
    string protectIf(ধ্রুবক string& old, যৌক্তিক doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        যদি (!v3Global.opt.protectIds() || old.empty() || !doIt) ফেরত old;
        ধ্রুবক V3LockGuard lock{m_mutex};
        ধ্রুবক auto pair = m_nameMap.emplace(old, "");
        যদি (pair.second) {
            string out;
            যদি (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } অন্যথা {
                VHashSha512 digest{v3Global.opt.protect_চাবি_Defaulted()};
                digest.ঢোকাও(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                জন্য (আকার_t len = 6; len < out.আকার_2() - 3; len += 3) {
                    ধ্রুবক string tryout = out.substr(0, len);
                    যদি (m_newIdSet.ঢোকাও(tryout).second) {
                        out = tryout;
                        বিরতি;
                    }
                }
            }
            pair.first->second = out;
        }
        ফেরত pair.first->second;
    }
    string protect_শব্দগুলি_If(ধ্রুবক string& old, যৌক্তিক doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        যদি (!(doIt && v3Global.opt.protectIds())) ফেরত old;
        string out;
        string::আকার_type start = 0;
        // space, ., ->
        যখন (সত্য) {
            // When C++11, use find_if and lambda
            string::আকার_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            যদি (pos == string::npos) বিরতি;
            out += protectIf(old.substr(start, pos - start), সত্য) + separator;
            start = pos + separator.দৈর্ঘ্য();
        }
        out += protectIf(old.substr(start), সত্য);
        ফেরত out;
    }
    শূন্য writeMapFile(ধ্রুবক string& filename) ধ্রুবক {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            জন্য (ধ্রুবক auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

ব্যক্তিগত:
    শূন্য trySep(ধ্রুবক string& old, string::আকার_type start, ধ্রুবক string& trySep,
                string::আকার_type& posr, string& separatorr) VL_PURE {
        ধ্রুবক string::আকার_type trypos = old.find(trySep, start);
        যদি (trypos != string::npos) {
            যদি (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ধ্রুবক string& in) VL_MT_SAFE {
    ফেরত v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(ধ্রুবক string& old, যৌক্তিক doIt) VL_MT_SAFE {
    ফেরত VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_শব্দগুলি_If(ধ্রুবক string& old, যৌক্তিক doIt) VL_MT_SAFE {
    ফেরত VIdProtectImp::singleton().protect_শব্দগুলি_If(old, doIt);
}
শূন্য VIdProtect::writeMapFile(ধ্রুবক string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
