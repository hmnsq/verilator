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
constexpr পূর্ণাঙ্ক INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr পূর্ণাঙ্ক INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

বর্গ V3FileDependImp final {
    // TYPES
    বর্গ DependFile final {
        // A single file
        ধ্রুব তার্কিক m_target;  // True if write, else read
        তার্কিক m_exists = সত্য;  // File exists
        ধ্রুব string m_filename;  // Filename
        সংরচনা stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    সার্বজনিক:
        DependFile(ধ্রুব string& filename, তার্কিক target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = স্বভাব;
        ধ্রুব string& filename() ধ্রুব { প্রত্যাগম m_filename; }
        তার্কিক target() ধ্রুব { প্রত্যাগম m_target; }
        তার্কিক exists() ধ্রুব { প্রত্যাগম m_exists; }
        off_t size() ধ্রুব { প্রত্যাগম m_stat.st_size; }
        ino_t ino() ধ্রুব { প্রত্যাগম m_stat.st_ino; }
        time_t cstime() ধ্রুব { প্রত্যাগম m_stat.st_ctime; }  // Seconds
        time_t cnstime() ধ্রুব { প্রত্যাগম VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ধ্রুব { প্রত্যাগম m_stat.st_mtime; }  // Seconds
        time_t mnstime() ধ্রুব { প্রত্যাগম VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_প্রতীক() {
            স্থির VHashSha512 s_emptyHash;
            প্রত্যাগম m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        শূন্য loadStats() {
            যদি (!m_stat.st_mtime) {
                ধ্রুব string fn = filename();
                ধ্রুব পূর্ণাঙ্ক err = stat(fn.c_str(), &m_stat);
                যদি (err != 0) {
                    std::memset(&m_stat, 0, পরিমাণ(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = অসত্য;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } অন্যথা {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    যদি (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.ঢোকাও_File(filename());
                }
            }
        }
        তার্কিক operator<(ধ্রুব DependFile& rhs) ধ্রুব { প্রত্যাগম filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    স্থির string stripQuotes(ধ্রুব string& in) {
        string pretty = in;
        string::size_type pos;
        যাবত্ ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        যাবত্ ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        প্রত্যাগম pretty;
    }

সার্বজনিক:
    // ACCESSOR METHODS
    শূন্য addSrcDepend(ধ্রুব string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ধ্রুব V3LockGuard lock{m_mutex};
        ধ্রুব auto itFoundPair = m_filenameSet.ঢোকাও(filename);
        যদি (itFoundPair.second) {
            DependFile df{filename, অসত্য};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.ঢোকাও(df);
        }
    }
    শূন্য addTgtDepend(ধ্রুব string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ধ্রুব V3LockGuard lock{m_mutex};
        ধ্রুব auto itFoundPair = m_filenameSet.ঢোকাও(filename);
        যদি (itFoundPair.second) m_filenameList.ঢোকাও(DependFile{filename, সত্য});
    }
    শূন্য writeDepend(ধ্রুব string& filename);
    std::vector<string> getAllDeps() ধ্রুব;
    শূন্য writeTimes(ধ্রুব string& filename, ধ্রুব string& cmdlineIn);
    তার্কিক checkTimes(ধ্রুব string& filename, ধ্রুব string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

শূন্য V3FileDependImp::writeDepend(ধ্রুব string& filename) {
    ধ্রুব std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    যদি (ofp->fail()) v3fatal("Can't write file: " << filename);

    প্রতি (ধ্রুব DependFile& i : m_filenameList) {
        যদি (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    প্রতি (ধ্রুব DependFile& i : m_filenameList) {
        যদি (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    যদি (v3Global.opt.makePhony()) {
        *ofp << '\n';
        প্রতি (ধ্রুব DependFile& i : m_filenameList) {
            যদি (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() ধ্রুব {
    std::vector<string> r;
    প্রতি (ধ্রুব auto& itr : m_filenameList) {
        যদি (!itr.target() && itr.exists()) r.ঠেলো_back(itr.filename());
    }
    প্রত্যাগম r;
}

শূন্য V3FileDependImp::writeTimes(ধ্রুব string& filename, ধ্রুব string& cmdlineIn) {
    ধ্রুব std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    যদি (ofp->fail()) v3fatal("Can't write file: " << filename);

    ধ্রুব string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    প্রতি (auto& it : m_filenameList) {
        DependFile* ধ্রুব depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        যদি (depfilep->filename() == filename) {
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
        *ofp << " \"" << depfilep->hash_Digest_প্রতীক() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

তার্কিক V3FileDependImp::checkTimes(ধ্রুব string& filename, ধ্রুব string& cmdlineIn) {
    ধ্রুব std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    যদি (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        প্রত্যাগম অসত্য;
    }
    {
        ধ্রুব string ignore = V3Os::getline(*ifp);
        যদি (ignore.empty()) { /*used*/
        }
    }
    {
        বর্ণ chkDir;
        *ifp >> chkDir;
        বর্ণ quote;
        *ifp >> quote;
        ধ্রুব string chkCmdline = V3Os::getline(*ifp, '"');
        ধ্রুব string cmdline = stripQuotes(cmdlineIn);
        যদি (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            প্রত্যাগম অসত্য;
        }
    }

    ধ্রুব তার্কিক skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    যাবত্ (!ifp->eof()) {
        বর্ণ chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        যদি (ifp->eof()) ভঙ্গ;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        বর্ণ quote;
        *ifp >> quote;
        ধ্রুব string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ধ্রুব string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        সংরচনা stat curStat;
        ধ্রুব পূর্ণাঙ্ক err = stat(chkFilename.c_str(), &curStat);
        যদি (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            প্রত্যাগম অসত্য;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        যদি (filename == chkFilename) অনুবর্ত;

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
        যদি (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            যদি (skipHashing) প্রত্যাগম অসত্য;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            যদি (chkHash == "unhashed") প্রত্যাগম অসত্য;

            VHashSha512 curHash;
            curHash.ঢোকাও_File(chkFilename);
            যদি (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                প্রত্যাগম অসত্য;
            }
        }
    }
    প্রত্যাগম সত্য;
}

//######################################################################
// V3File

শূন্য V3File::addSrcDepend(ধ্রুব string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
শূন্য V3File::addTgtDepend(ধ্রুব string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
শূন্য V3File::writeDepend(ধ্রুব string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { প্রত্যাগম dependImp.getAllDeps(); }
শূন্য V3File::writeTimes(ধ্রুব string& filename, ধ্রুব string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
তার্কিক V3File::checkTimes(ধ্রুব string& filename, ধ্রুব string& cmdlineIn) {
    প্রত্যাগম dependImp.checkTimes(filename, cmdlineIn);
}
শূন্য V3File::createMakeDirFor(ধ্রুব string& filename) {
    যদি (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
শূন্য V3File::createMakeDir() {
    স্থির তার্কিক s_created = অসত্য;
    যদি (!s_created) {
        s_created = সত্য;
        V3Os::createDir(v3Global.opt.makeDir());
        যদি (v3Global.opt.hier_শীর্ষ()) V3Os::createDir(v3Global.opt.hier_শীর্ষ_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

বর্গ VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<ধ্রুব std::string, std::string> m_contentsMap;  // Cache of file contents
    তার্কিক m_readEof = অসত্য;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#অন্যথা
    পূর্ণাঙ্ক m_pid = 0;  // fork() process id - always zero as disabled
#endif
    তার্কিক m_pidExited = অসত্য;  //  waitpid() reaped the filter process
    পূর্ণাঙ্ক m_pidStatus = 0;  // Exit status of the filter process
    পূর্ণাঙ্ক m_writeFd = 0;  // File descriptor TO filter
    পূর্ণাঙ্ক m_readFd = 0;  // File descriptor FROM filter

গুপ্ত:
    // METHODS

    তার্কিক readContents(ধ্রুব string& filename, StrList& outl) {
        যদি (m_pid) {
            প্রত্যাগম readContentsFilter(filename, outl);
        } অন্যথা {
            প্রত্যাগম readContentsFile(filename, outl);
        }
    }
    তার্কিক readContentsFile(ধ্রুব string& filename, StrList& outl) {
        ধ্রুব পূর্ণাঙ্ক fd = open(filename.c_str(), O_RDONLY);
        যদি (fd < 0) প্রত্যাগম অসত্য;
        m_readEof = অসত্য;
        readBlocks(fd, -1, outl);
        close(fd);
        প্রত্যাগম সত্য;
    }
    // cppcheck-suppress constParameterReference
    তার্কিক readContentsFilter(ধ্রুব string& filename, StrList& outl) {
        (শূন্য)filename;  // Prevent unused variable warning
        (শূন্য)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ধ্রুব string সারি = read_Filter_সারি();
        যদি (সারি.find("Content-Length") != string::npos) {
            পূর্ণাঙ্ক len = 0;
            sscanf(সারি.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            প্রত্যাগম সত্য;
        } অন্যথা {
            যদি (সারি != "") v3error("--pipe-filter protocol error, unexpected: " << সারি);
            প্রত্যাগম অসত্য;
        }
#অন্যথা
        v3fatalSrc("--pipe-filter not implemented on this platform");
        প্রত্যাগম অসত্য;
#endif
    }

    // cppcheck-suppress functionConst
    শূন্য checkFilter(তার্কিক hang) {
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

    শূন্য readBlocks(পূর্ণাঙ্ক fd, পূর্ণাঙ্ক size, StrList& outl) {
        বর্ণ buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        যাবত্ (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            যদি (size > 0 && size < todo) todo = size;
            errno = 0;
            ধ্রুব ssize_t got = পঠ(fd, buf, todo);
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
                checkFilter(অসত্য);
                V3Os::u_sleep(1000);
                অনুবর্ত;
            } অন্যথা {
                m_readEof = সত্য;
                ভঙ্গ;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_সারি() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string সারি;
        যাবত্ (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            ধ্রুব string onechar = listString(outl);
            সারি += onechar;
            যদি (onechar == "\n") {
                যদি (সারি == "\n") {
                    সারি = "";
                    অনুবর্ত;
                } অন্যথা {
                    ভঙ্গ;
                }
            }
        }
        UINFO(6, "filter-line-in: " << সারি);
        প্রত্যাগম সারি;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    শূন্য writeFilter(ধ্রুব string& out) {
        যদি (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            যদি (out[out.length() - 1] != '\n') cout << '\n';
        }
        যদি (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = সত্য;
            stop();
        }
        unsigned offset = 0;
        যাবত্ (!m_readEof && out.length() > offset) {
            errno = 0;
            ধ্রুব পূর্ণাঙ্ক got = লিখ(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            যদি (got > 0) {
                offset += got;
            } অন্যথা যদি (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(অসত্য);
                V3Os::u_sleep(1000);
                অনুবর্ত;
            } অন্যথা {
                ভঙ্গ;
            }
        }
    }

    // Start the filter
    শূন্য start(ধ্রুব string& command) {
        যদি (command == "") {
            m_pid = 0;  // Disabled
        } অন্যথা {
            startFilter(command);
        }
    }
    শূন্য startFilter(ধ্রুব string& command) {
        (শূন্য)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        পূর্ণাঙ্ক fd_stdin[2];  // Can't use std::array
        পূর্ণাঙ্ক fd_stdout[2];  // Can't use std::array
        constexpr পূর্ণাঙ্ক P_RD = 0;
        constexpr পূর্ণাঙ্ক P_WR = 1;

        যদি (VL_UNCOVERABLE(প্রণাল(fd_stdin) != 0 || প্রণাল(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        যদি (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ধ্রুব pid_t pid = fork();
        যদি (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        যদি (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<বর্ণ*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } অন্যথা {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = অসত্য;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = অসত্য;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            পূর্ণাঙ্ক flags = fcntl(m_readFd, F_GETFL, 0);
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

রক্ষিত:
    friend বর্গ VInFilter;
    // Read file contents and return it
    তার্কিক readWholefile(ধ্রুব string& filename, StrList& outl) {
        ধ্রুব auto it = m_contentsMap.find(filename);
        যদি (it != m_contentsMap.end()) {
            outl.ঠেলো_back(it->second);
            প্রত্যাগম সত্য;
        }
        যদি (!readContents(filename, outl)) প্রত্যাগম অসত্য;
        যদি (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        প্রত্যাগম সত্য;
    }
    স্থির size_t listSize(ধ্রুব StrList& sl) {
        size_t result = 0;
        প্রতি (ধ্রুব string& i : sl) result += i.length();
        প্রত্যাগম result;
    }
    স্থির string listString(ধ্রুব StrList& sl) {
        string result;
        প্রতি (ধ্রুব string& i : sl) result += i;
        প্রত্যাগম result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(ধ্রুব string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ধ্রুব string& command) { m_impp = নূতন VInFilterImp{command}; }
VInFilter::~VInFilter() {
    যদি (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

তার্কিক VInFilter::readWholefile(ধ্রুব string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    প্রত্যাগম m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

স্থির constexpr পূর্ণাঙ্ক MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
স্থির ধ্রুব std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    প্রতি (পূর্ণাঙ্ক i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    প্রত্যাগম table;
}();

ধ্রুব std::string& V3OutFormatter::indentSpaces(পূর্ণাঙ্ক অঙ্ক) {
    // Indent the specified number of spaces.
    প্রত্যাগম s_indentSpaces[std::max(0, std::min(অঙ্ক, MAXSPACE))];
}

তার্কিক V3OutFormatter::tokenMatch(ধ্রুব বর্ণ* cp, ধ্রুব বর্ণ* cmp) {
    যাবত্ (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    যদি (*cmp) প্রত্যাগম অসত্য;
    যদি (*cp && !std::isspace(*cp)) প্রত্যাগম অসত্য;
    প্রত্যাগম সত্য;
}

তার্কিক V3OutFormatter::tokenStart(ধ্রুব বর্ণ* cp) {
    প্রত্যাগম (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

তার্কিক V3OutFormatter::tokenEnd(ধ্রুব বর্ণ* cp) {
    প্রত্যাগম (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

তার্কিক V3OutFormatter::tokenNotStart(ধ্রুব বর্ণ* cp) {
    প্রত্যাগম (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

পূর্ণাঙ্ক V3OutFormatter::endLevels(ধ্রুব বর্ণ* strg) {
    পূর্ণাঙ্ক levels = m_indentLevel;
    {
        ধ্রুব বর্ণ* cp = strg;
        যাবত্ (std::isspace(*cp)) ++cp;
        বিকল্প (*cp) {
        পক্ষ '\n':  // Newlines.. No need for whitespace before it
            প্রত্যাগম 0;
        পক্ষ '#':  // Preproc directive
            যদি (m_lang == LA_C) প্রত্যাগম 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ধ্রুব বর্ণ* mp = cp;
            প্রতি (; std::isalnum(*mp); ++mp) {}
            যদি (mp[0] == ':' && mp[1] != ':') প্রত্যাগম (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    প্রতি (ধ্রুব বর্ণ* cp = strg; *cp; ++cp) {
        বিকল্প (*cp) {
        পক্ষ '}':
        পক্ষ ')': levels -= m_blockIndent; ভঙ্গ;
        পক্ষ '<':
            যদি (m_lang == LA_XML) {
                যদি (cp[1] == '/') levels -= m_blockIndent;
            }
            ভঙ্গ;
        পক্ষ 'e':
            যদি (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            ভঙ্গ;
        পক্ষ '\t':
        পক্ষ ' ': ভঙ্গ;  // Continue
        স্বভাব: প্রত্যাগম levels;  // Letter
        }
    }
    প্রত্যাগম levels;
}

শূন্য V3OutFormatter::putns(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব বর্ণ* strg) {
    যদি (!v3Global.opt.decoration()) {
        putsOutput(strg);
        প্রত্যাগম;
    }

    ধ্রুব তার্কিক put_গ্রন্থি_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_সারি::builtInFilename() != nodep->fileline()->filename();

    যদি (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_গ্রন্থি_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = অসত্য;
    }

    যদি (put_গ্রন্থি_Decoration) {
        ধ্রুব File_সারি* ধ্রুব flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        ধ্রুব std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    তার্কিক notstart = অসত্য;
    তার্কিক wordstart = সত্য;
    তার্কিক equalsForBracket = অসত্য;  // Looking for "= {"
    প্রতি (ধ্রুব বর্ণ* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        যদি (std::isalpha(*cp)) {
            যদি (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = সত্য;
            যদি (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            যদি (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        ধ্রুব তার্কিক wasInBackslash = m_inBackslash;
        m_inBackslash = অসত্য;
        বিকল্প (*cp) {
        পক্ষ '\n':
            ++m_lineno;
            wordstart = সত্য;
            যদি (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = সত্য;
            } অন্যথা {
                m_prependIndent = অসত্য;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            ভঙ্গ;
        পক্ষ ' ': wordstart = সত্য; ভঙ্গ;
        পক্ষ '\t': wordstart = সত্য; ভঙ্গ;
        পক্ষ '"':
            wordstart = অসত্য;
            যদি (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            ভঙ্গ;
        পক্ষ '\\':
            wordstart = অসত্য;
            যদি (!wasInBackslash) m_inBackslash = সত্য;
            ভঙ্গ;
        পক্ষ '/':
            যদি (m_lang == LA_C || m_lang == LA_VERILOG) {
                যদি (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    যাবত্ (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    যদি (*cp) putcNoTracking(*cp);
                }
            }
            ভঙ্গ;
        পক্ষ '{':
            যদি (!m_inStringLiteral) {
                যদি (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ঠেলো(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            ভঙ্গ;
        পক্ষ '}':
            যদি (!m_inStringLiteral) {
                যদি (m_bracketLevel > 0) {
                    m_parenVec.বের_করো();
                    --m_bracketLevel;
                }
                indentDec();
            }
            ভঙ্গ;
        পক্ষ '(':
            যদি (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ঠেলো(m_column);
            }
            ভঙ্গ;
        পক্ষ ')':
            যদি (!m_inStringLiteral) {
                যদি (!m_parenVec.empty()) m_parenVec.বের_করো();
                indentDec();
            }
            ভঙ্গ;
        পক্ষ '<':
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
            ভঙ্গ;
        পক্ষ '>':
            যদি (m_lang == LA_XML) {
                indentDec();
                যদি (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            ভঙ্গ;
        স্বভাব: wordstart = অসত্য; ভঙ্গ;
        }

        বিকল্প (*cp) {
        পক্ষ '=': equalsForBracket = সত্য; ভঙ্গ;
        পক্ষ ' ': ভঙ্গ;
        স্বভাব: equalsForBracket = অসত্য; ভঙ্গ;
        }
    }
}

শূন্য V3OutFormatter::put_Break_রাশিমালা() {
    যদি (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
শূন্য V3OutFormatter::putBreak() {
    যদি (!v3Global.opt.decoration()) প্রত্যাগম;
    যদি (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        যদি (exceededWidth()) {
            putcNoTracking('\n');
            যদি (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.শীর্ষ()));
        }
    }
}

শূন্য V3OutFormatter::putsQuoted(ধ্রুব string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ধ্রুব string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    প্রতি (ধ্রুব বর্ণ c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    যদি (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
শূন্য V3OutFormatter::putsNoTracking(ধ্রুব string& strg) {
    যদি (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        প্রত্যাগম;
    }
    // Don't track {}'s, probably because it's a $display format string
    প্রতি (ধ্রুব বর্ণ c : strg) putcNoTracking(c);
}

শূন্য V3OutFormatter::putcNoTracking(বর্ণ chr) {
    যদি (!v3Global.opt.decoration()) {
        putcOutput(chr);
        প্রত্যাগম;
    }
    বিকল্প (chr) {
    পক্ষ '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = সত্য;
        ভঙ্গ;
    পক্ষ '\t': m_column = ((m_column + 9) / 8) * 8; ভঙ্গ;
    পক্ষ ' ':
    পক্ষ '(':
    পক্ষ '|':
    পক্ষ '&': ++m_column; ভঙ্গ;
    স্বভাব:
        ++m_column;
        m_nobreak = অসত্য;
        ভঙ্গ;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(ধ্রুব string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    যদি (lang == LA_XML) {
        // Encode chars into XML string
        প্রতি (ধ্রুব বর্ণ c : namein) {
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
                out += "&#"s + cvtToStr(static_cast<unsigned পূর্ণাঙ্ক>(c & 0xff)) + ";";
            }
        }
    } অন্যথা {
        // Encode control chars into C style escapes
        প্রতি (ধ্রুব বর্ণ c : namein) {
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
                ধ্রুব string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    প্রত্যাগম out;
}

//----------------------------------------------------------------------
// Simple wrappers

শূন্য V3OutFormatter::printf(ধ্রুব বর্ণ* fmt...) {
    constexpr size_t bufsize = 5000;
    বর্ণ sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    এতত্->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(ধ্রুব string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{নূতন std::array<বর্ণ, WRITE_BUFFER_SIZE_BYTES>{}} {
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
    ধ্রুব VStringList& forceIncs = v3Global.opt.forceIncs();
    প্রতি (ধ্রুব string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

শূন্য V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = সত্য;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    প্রতি (বর্ণ& c : var) {
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

বর্গ VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ধ্রুব std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
রক্ষিত:
    // CONSTRUCTORS
    friend বর্গ VIdProtect;
    স্থির VIdProtectImp& singleton() VL_MT_SAFE {
        স্থির VIdProtectImp s;
        প্রত্যাগম s;
    }

সার্বজনিক:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = স্বভাব;
    // METHODS
    string passthru(ধ্রুব string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        যদি (!v3Global.opt.protectIds()) প্রত্যাগম old;
        ধ্রুব V3LockGuard lock{m_mutex};
        ধ্রুব auto it = m_nameMap.find(old);
        যদি (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } অন্যথা {
            m_nameMap.emplace(old, old);
            m_newIdSet.ঢোকাও(old);
        }
        প্রত্যাগম old;
    }
    string protectIf(ধ্রুব string& old, তার্কিক doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        যদি (!v3Global.opt.protectIds() || old.empty() || !doIt) প্রত্যাগম old;
        ধ্রুব V3LockGuard lock{m_mutex};
        ধ্রুব auto pair = m_nameMap.emplace(old, "");
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
                প্রতি (size_t len = 6; len < out.size() - 3; len += 3) {
                    ধ্রুব string tryout = out.substr(0, len);
                    যদি (m_newIdSet.ঢোকাও(tryout).second) {
                        out = tryout;
                        ভঙ্গ;
                    }
                }
            }
            pair.first->second = out;
        }
        প্রত্যাগম pair.first->second;
    }
    string protect_শব্দগুলি_If(ধ্রুব string& old, তার্কিক doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        যদি (!(doIt && v3Global.opt.protectIds())) প্রত্যাগম old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        যাবত্ (সত্য) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            যদি (pos == string::npos) ভঙ্গ;
            out += protectIf(old.substr(start, pos - start), সত্য) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), সত্য);
        প্রত্যাগম out;
    }
    শূন্য writeMapFile(ধ্রুব string& filename) ধ্রুব {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            প্রতি (ধ্রুব auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

গুপ্ত:
    শূন্য trySep(ধ্রুব string& old, string::size_type start, ধ্রুব string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        ধ্রুব string::size_type trypos = old.find(trySep, start);
        যদি (trypos != string::npos) {
            যদি (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ধ্রুব string& in) VL_MT_SAFE {
    প্রত্যাগম v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(ধ্রুব string& old, তার্কিক doIt) VL_MT_SAFE {
    প্রত্যাগম VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_শব্দগুলি_If(ধ্রুব string& old, তার্কিক doIt) VL_MT_SAFE {
    প্রত্যাগম VIdProtectImp::singleton().protect_শব্দগুলি_If(old, doIt);
}
শূন্য VIdProtect::writeMapFile(ধ্রুব string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
