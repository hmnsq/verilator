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

#شامل "V3PchAstMT.h"

#شامل "V3File.h"

#شامل "V3Os.h"
#شامل "V3String.h"

#شامل <cerrno>
#شامل <cstdarg>
#شامل <fcntl.h>
#شامل <iomanip>
#شامل <map>
#شامل <memory>

#شامل <sys/stat.h>
#شامل <sys/types.h>

// clang-format off
#اگر defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#وگرنه
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#اگر defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr صحیح INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr صحیح INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

رده V3FileDependImp final {
    // TYPES
    رده DependFile final {
        // A single file
        ثابت منطقی m_target;  // True if write, else read
        منطقی m_exists = درست;  // File exists
        ثابت string m_filename;  // Filename
        ساختار stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    عمومی:
        DependFile(ثابت string& filename, منطقی target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = پیشفرض;
        ثابت string& filename() ثابت { بازگشت m_filename; }
        منطقی target() ثابت { بازگشت m_target; }
        منطقی exists() ثابت { بازگشت m_exists; }
        off_t size() ثابت { بازگشت m_stat.st_size; }
        ino_t ino() ثابت { بازگشت m_stat.st_ino; }
        time_t cstime() ثابت { بازگشت m_stat.st_ctime; }  // Seconds
        time_t cnstime() ثابت { بازگشت VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ثابت { بازگشت m_stat.st_mtime; }  // Seconds
        time_t mnstime() ثابت { بازگشت VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_نماد() {
            ایستا VHashSha512 s_emptyHash;
            بازگشت m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        تهی loadStats() {
            اگر (!m_stat.st_mtime) {
                ثابت string fn = filename();
                ثابت صحیح err = stat(fn.c_str(), &m_stat);
                اگر (err != 0) {
                    std::memset(&m_stat, 0, اندازه(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = نادرست;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } وگرنه {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    اگر (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.درج_File(filename());
                }
            }
        }
        منطقی operator<(ثابت DependFile& rhs) ثابت { بازگشت filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    ایستا string stripQuotes(ثابت string& in) {
        string pretty = in;
        string::size_type pos;
        وقتی ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        وقتی ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        بازگشت pretty;
    }

عمومی:
    // ACCESSOR METHODS
    تهی addSrcDepend(ثابت string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto itFoundPair = m_filenameSet.درج(filename);
        اگر (itFoundPair.second) {
            DependFile df{filename, نادرست};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.درج(df);
        }
    }
    تهی addTgtDepend(ثابت string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto itFoundPair = m_filenameSet.درج(filename);
        اگر (itFoundPair.second) m_filenameList.درج(DependFile{filename, درست});
    }
    تهی writeDepend(ثابت string& filename);
    std::vector<string> getAllDeps() ثابت;
    تهی writeTimes(ثابت string& filename, ثابت string& cmdlineIn);
    منطقی checkTimes(ثابت string& filename, ثابت string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

تهی V3FileDependImp::writeDepend(ثابت string& filename) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    اگر (ofp->fail()) v3fatal("Can't write file: " << filename);

    برای (ثابت DependFile& i : m_filenameList) {
        اگر (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    برای (ثابت DependFile& i : m_filenameList) {
        اگر (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    اگر (v3Global.opt.makePhony()) {
        *ofp << '\n';
        برای (ثابت DependFile& i : m_filenameList) {
            اگر (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() ثابت {
    std::vector<string> r;
    برای (ثابت auto& itr : m_filenameList) {
        اگر (!itr.target() && itr.exists()) r.افزودن_back(itr.filename());
    }
    بازگشت r;
}

تهی V3FileDependImp::writeTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    اگر (ofp->fail()) v3fatal("Can't write file: " << filename);

    ثابت string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    برای (auto& it : m_filenameList) {
        DependFile* ثابت depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        اگر (depfilep->filename() == filename) {
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
        *ofp << " \"" << depfilep->hash_Digest_نماد() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

منطقی V3FileDependImp::checkTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    اگر (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        بازگشت نادرست;
    }
    {
        ثابت string ignore = V3Os::getline(*ifp);
        اگر (ignore.empty()) { /*used*/
        }
    }
    {
        نویسه chkDir;
        *ifp >> chkDir;
        نویسه quote;
        *ifp >> quote;
        ثابت string chkCmdline = V3Os::getline(*ifp, '"');
        ثابت string cmdline = stripQuotes(cmdlineIn);
        اگر (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            بازگشت نادرست;
        }
    }

    ثابت منطقی skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    وقتی (!ifp->eof()) {
        نویسه chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        اگر (ifp->eof()) بشکن;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        نویسه quote;
        *ifp >> quote;
        ثابت string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ثابت string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ساختار stat curStat;
        ثابت صحیح err = stat(chkFilename.c_str(), &curStat);
        اگر (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            بازگشت نادرست;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        اگر (filename == chkFilename) ادامه;

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
        اگر (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            اگر (skipHashing) بازگشت نادرست;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            اگر (chkHash == "unhashed") بازگشت نادرست;

            VHashSha512 curHash;
            curHash.درج_File(chkFilename);
            اگر (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                بازگشت نادرست;
            }
        }
    }
    بازگشت درست;
}

//######################################################################
// V3File

تهی V3File::addSrcDepend(ثابت string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
تهی V3File::addTgtDepend(ثابت string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
تهی V3File::writeDepend(ثابت string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { بازگشت dependImp.getAllDeps(); }
تهی V3File::writeTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
منطقی V3File::checkTimes(ثابت string& filename, ثابت string& cmdlineIn) {
    بازگشت dependImp.checkTimes(filename, cmdlineIn);
}
تهی V3File::createMakeDirFor(ثابت string& filename) {
    اگر (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
تهی V3File::createMakeDir() {
    ایستا منطقی s_created = نادرست;
    اگر (!s_created) {
        s_created = درست;
        V3Os::createDir(v3Global.opt.makeDir());
        اگر (v3Global.opt.hier_بالاترین()) V3Os::createDir(v3Global.opt.hier_بالاترین_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

رده VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<ثابت std::string, std::string> m_contentsMap;  // Cache of file contents
    منطقی m_readEof = نادرست;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#وگرنه
    صحیح m_pid = 0;  // fork() process id - always zero as disabled
#endif
    منطقی m_pidExited = نادرست;  //  waitpid() reaped the filter process
    صحیح m_pidStatus = 0;  // Exit status of the filter process
    صحیح m_writeFd = 0;  // File descriptor TO filter
    صحیح m_readFd = 0;  // File descriptor FROM filter

خصوصی:
    // METHODS

    منطقی readContents(ثابت string& filename, StrList& outl) {
        اگر (m_pid) {
            بازگشت readContentsFilter(filename, outl);
        } وگرنه {
            بازگشت readContentsFile(filename, outl);
        }
    }
    منطقی readContentsFile(ثابت string& filename, StrList& outl) {
        ثابت صحیح fd = open(filename.c_str(), O_RDONLY);
        اگر (fd < 0) بازگشت نادرست;
        m_readEof = نادرست;
        readBlocks(fd, -1, outl);
        close(fd);
        بازگشت درست;
    }
    // cppcheck-suppress constParameterReference
    منطقی readContentsFilter(ثابت string& filename, StrList& outl) {
        (تهی)filename;  // Prevent unused variable warning
        (تهی)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ثابت string سطر = read_Filter_سطر();
        اگر (سطر.find("Content-Length") != string::npos) {
            صحیح len = 0;
            sscanf(سطر.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            بازگشت درست;
        } وگرنه {
            اگر (سطر != "") v3error("--pipe-filter protocol error, unexpected: " << سطر);
            بازگشت نادرست;
        }
#وگرنه
        v3fatalSrc("--pipe-filter not implemented on this platform");
        بازگشت نادرست;
#endif
    }

    // cppcheck-suppress functionConst
    تهی checkFilter(منطقی hang) {
#ifdef INFILTER_PIPE
        اگر (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = درست;
            m_pidExited = درست;
        }
#endif
    }

    تهی readBlocks(صحیح fd, صحیح size, StrList& outl) {
        نویسه buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        وقتی (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            اگر (size > 0 && size < todo) todo = size;
            errno = 0;
            ثابت ssize_t got = اقرأ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            اگر (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } وگرنه اگر (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(نادرست);
                V3Os::u_sleep(1000);
                ادامه;
            } وگرنه {
                m_readEof = درست;
                بشکن;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_سطر() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string سطر;
        وقتی (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            ثابت string onechar = listString(outl);
            سطر += onechar;
            اگر (onechar == "\n") {
                اگر (سطر == "\n") {
                    سطر = "";
                    ادامه;
                } وگرنه {
                    بشکن;
                }
            }
        }
        UINFO(6, "filter-line-in: " << سطر);
        بازگشت سطر;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    تهی writeFilter(ثابت string& out) {
        اگر (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            اگر (out[out.length() - 1] != '\n') cout << '\n';
        }
        اگر (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = درست;
            stop();
        }
        unsigned offset = 0;
        وقتی (!m_readEof && out.length() > offset) {
            errno = 0;
            ثابت صحیح got = اكتب(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            اگر (got > 0) {
                offset += got;
            } وگرنه اگر (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(نادرست);
                V3Os::u_sleep(1000);
                ادامه;
            } وگرنه {
                بشکن;
            }
        }
    }

    // Start the filter
    تهی start(ثابت string& command) {
        اگر (command == "") {
            m_pid = 0;  // Disabled
        } وگرنه {
            startFilter(command);
        }
    }
    تهی startFilter(ثابت string& command) {
        (تهی)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        صحیح fd_stdin[2];  // Can't use std::array
        صحیح fd_stdout[2];  // Can't use std::array
        constexpr صحیح P_RD = 0;
        constexpr صحیح P_WR = 1;

        اگر (VL_UNCOVERABLE(أنبوب(fd_stdin) != 0 || أنبوب(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        اگر (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ثابت pid_t pid = fork();
        اگر (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        اگر (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<نویسه*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } وگرنه {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = نادرست;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = نادرست;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            صحیح flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#وگرنه
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    تهی stop() {
        اگر (m_pid) stopFilter();
    }
    تهی stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(درست);
        اگر (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#وگرنه
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

محافظتشده:
    friend رده VInFilter;
    // Read file contents and return it
    منطقی readWholefile(ثابت string& filename, StrList& outl) {
        ثابت auto it = m_contentsMap.find(filename);
        اگر (it != m_contentsMap.end()) {
            outl.افزودن_back(it->second);
            بازگشت درست;
        }
        اگر (!readContents(filename, outl)) بازگشت نادرست;
        اگر (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        بازگشت درست;
    }
    ایستا size_t listSize(ثابت StrList& sl) {
        size_t result = 0;
        برای (ثابت string& i : sl) result += i.length();
        بازگشت result;
    }
    ایستا string listString(ثابت StrList& sl) {
        string result;
        برای (ثابت string& i : sl) result += i;
        بازگشت result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(ثابت string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ثابت string& command) { m_impp = جدید VInFilterImp{command}; }
VInFilter::~VInFilter() {
    اگر (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

منطقی VInFilter::readWholefile(ثابت string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    بازگشت m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ایستا constexpr صحیح MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ایستا ثابت std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    برای (صحیح i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    بازگشت table;
}();

ثابت std::string& V3OutFormatter::indentSpaces(صحیح رقم) {
    // Indent the specified number of spaces.
    بازگشت s_indentSpaces[std::max(0, std::min(رقم, MAXSPACE))];
}

منطقی V3OutFormatter::tokenMatch(ثابت نویسه* cp, ثابت نویسه* cmp) {
    وقتی (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    اگر (*cmp) بازگشت نادرست;
    اگر (*cp && !std::isspace(*cp)) بازگشت نادرست;
    بازگشت درست;
}

منطقی V3OutFormatter::tokenStart(ثابت نویسه* cp) {
    بازگشت (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

منطقی V3OutFormatter::tokenEnd(ثابت نویسه* cp) {
    بازگشت (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

منطقی V3OutFormatter::tokenNotStart(ثابت نویسه* cp) {
    بازگشت (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

صحیح V3OutFormatter::endLevels(ثابت نویسه* strg) {
    صحیح levels = m_indentLevel;
    {
        ثابت نویسه* cp = strg;
        وقتی (std::isspace(*cp)) ++cp;
        انتخاب (*cp) {
        حالت '\n':  // Newlines.. No need for whitespace before it
            بازگشت 0;
        حالت '#':  // Preproc directive
            اگر (m_lang == LA_C) بازگشت 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ثابت نویسه* mp = cp;
            برای (; std::isalnum(*mp); ++mp) {}
            اگر (mp[0] == ':' && mp[1] != ':') بازگشت (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    برای (ثابت نویسه* cp = strg; *cp; ++cp) {
        انتخاب (*cp) {
        حالت '}':
        حالت ')': levels -= m_blockIndent; بشکن;
        حالت '<':
            اگر (m_lang == LA_XML) {
                اگر (cp[1] == '/') levels -= m_blockIndent;
            }
            بشکن;
        حالت 'e':
            اگر (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            بشکن;
        حالت '\t':
        حالت ' ': بشکن;  // Continue
        پیشفرض: بازگشت levels;  // Letter
        }
    }
    بازگشت levels;
}

تهی V3OutFormatter::putns(ثابت Ast_گره* nodep, ثابت نویسه* strg) {
    اگر (!v3Global.opt.decoration()) {
        putsOutput(strg);
        بازگشت;
    }

    ثابت منطقی put_گره_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_سطر::builtInFilename() != nodep->fileline()->filename();

    اگر (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_گره_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = نادرست;
    }

    اگر (put_گره_Decoration) {
        ثابت File_سطر* ثابت flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        ثابت std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    منطقی notstart = نادرست;
    منطقی wordstart = درست;
    منطقی equalsForBracket = نادرست;  // Looking for "= {"
    برای (ثابت نویسه* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        اگر (std::isalpha(*cp)) {
            اگر (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = درست;
            اگر (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            اگر (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        ثابت منطقی wasInBackslash = m_inBackslash;
        m_inBackslash = نادرست;
        انتخاب (*cp) {
        حالت '\n':
            ++m_lineno;
            wordstart = درست;
            اگر (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = درست;
            } وگرنه {
                m_prependIndent = نادرست;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            بشکن;
        حالت ' ': wordstart = درست; بشکن;
        حالت '\t': wordstart = درست; بشکن;
        حالت '"':
            wordstart = نادرست;
            اگر (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            بشکن;
        حالت '\\':
            wordstart = نادرست;
            اگر (!wasInBackslash) m_inBackslash = درست;
            بشکن;
        حالت '/':
            اگر (m_lang == LA_C || m_lang == LA_VERILOG) {
                اگر (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    وقتی (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    اگر (*cp) putcNoTracking(*cp);
                }
            }
            بشکن;
        حالت '{':
            اگر (!m_inStringLiteral) {
                اگر (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.افزودن(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            بشکن;
        حالت '}':
            اگر (!m_inStringLiteral) {
                اگر (m_bracketLevel > 0) {
                    m_parenVec.برداشت();
                    --m_bracketLevel;
                }
                indentDec();
            }
            بشکن;
        حالت '(':
            اگر (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.افزودن(m_column);
            }
            بشکن;
        حالت ')':
            اگر (!m_inStringLiteral) {
                اگر (!m_parenVec.empty()) m_parenVec.برداشت();
                indentDec();
            }
            بشکن;
        حالت '<':
            اگر (m_lang == LA_XML) {
                اگر (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } وگرنه اگر (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } وگرنه {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            بشکن;
        حالت '>':
            اگر (m_lang == LA_XML) {
                indentDec();
                اگر (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            بشکن;
        پیشفرض: wordstart = نادرست; بشکن;
        }

        انتخاب (*cp) {
        حالت '=': equalsForBracket = درست; بشکن;
        حالت ' ': بشکن;
        پیشفرض: equalsForBracket = نادرست; بشکن;
        }
    }
}

تهی V3OutFormatter::put_Break_عبارت() {
    اگر (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
تهی V3OutFormatter::putBreak() {
    اگر (!v3Global.opt.decoration()) بازگشت;
    اگر (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        اگر (exceededWidth()) {
            putcNoTracking('\n');
            اگر (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.بالاترین()));
        }
    }
}

تهی V3OutFormatter::putsQuoted(ثابت string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ثابت string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    برای (ثابت نویسه c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    اگر (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
تهی V3OutFormatter::putsNoTracking(ثابت string& strg) {
    اگر (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        بازگشت;
    }
    // Don't track {}'s, probably because it's a $display format string
    برای (ثابت نویسه c : strg) putcNoTracking(c);
}

تهی V3OutFormatter::putcNoTracking(نویسه chr) {
    اگر (!v3Global.opt.decoration()) {
        putcOutput(chr);
        بازگشت;
    }
    انتخاب (chr) {
    حالت '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = درست;
        بشکن;
    حالت '\t': m_column = ((m_column + 9) / 8) * 8; بشکن;
    حالت ' ':
    حالت '(':
    حالت '|':
    حالت '&': ++m_column; بشکن;
    پیشفرض:
        ++m_column;
        m_nobreak = نادرست;
        بشکن;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(ثابت string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    اگر (lang == LA_XML) {
        // Encode chars into XML string
        برای (ثابت نویسه c : namein) {
            اگر (c == '"') {
                out += "&quot;"s;
            } وگرنه اگر (c == '\'') {
                out += "&apos;"s;
            } وگرنه اگر (c == '<') {
                out += "&lt;"s;
            } وگرنه اگر (c == '>') {
                out += "&gt;"s;
            } وگرنه اگر (c == '&') {
                out += "&amp;"s;
            } وگرنه اگر (std::isprint(c)) {
                out += c;
            } وگرنه {
                out += "&#"s + cvtToStr(static_cast<unsigned صحیح>(c & 0xff)) + ";";
            }
        }
    } وگرنه {
        // Encode control chars into C style escapes
        برای (ثابت نویسه c : namein) {
            اگر (c == '\\' || c == '"') {
                out += "\\"s + c;
            } وگرنه اگر (c == '\n') {
                out += "\\n";
            } وگرنه اگر (c == '\r') {
                out += "\\r";
            } وگرنه اگر (c == '\t') {
                out += "\\t";
            } وگرنه اگر (std::isprint(c)) {
                out += c;
            } وگرنه {
                // This will also cover \a etc
                ثابت string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    بازگشت out;
}

//----------------------------------------------------------------------
// Simple wrappers

تهی V3OutFormatter::printf(ثابت نویسه* fmt...) {
    constexpr size_t bufsize = 5000;
    نویسه sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    این->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(ثابت string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{جدید std::array<نویسه, WRITE_BUFFER_SIZE_BYTES>{}} {
    اگر ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    اگر (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

تهی V3OutFile::putsForceIncs() {
    ثابت VStringList& forceIncs = v3Global.opt.forceIncs();
    برای (ثابت string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

تهی V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = درست;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    برای (نویسه& c : var) {
        اگر (!std::isalnum(c)) c = '_';
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

رده VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ثابت std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
محافظتشده:
    // CONSTRUCTORS
    friend رده VIdProtect;
    ایستا VIdProtectImp& singleton() VL_MT_SAFE {
        ایستا VIdProtectImp s;
        بازگشت s;
    }

عمومی:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = پیشفرض;
    // METHODS
    string passthru(ثابت string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        اگر (!v3Global.opt.protectIds()) بازگشت old;
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto it = m_nameMap.find(old);
        اگر (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } وگرنه {
            m_nameMap.emplace(old, old);
            m_newIdSet.درج(old);
        }
        بازگشت old;
    }
    string protectIf(ثابت string& old, منطقی doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        اگر (!v3Global.opt.protectIds() || old.empty() || !doIt) بازگشت old;
        ثابت V3LockGuard lock{m_mutex};
        ثابت auto pair = m_nameMap.emplace(old, "");
        اگر (pair.second) {
            string out;
            اگر (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } وگرنه {
                VHashSha512 digest{v3Global.opt.protect_کلید_Defaulted()};
                digest.درج(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                برای (size_t len = 6; len < out.size() - 3; len += 3) {
                    ثابت string tryout = out.substr(0, len);
                    اگر (m_newIdSet.درج(tryout).second) {
                        out = tryout;
                        بشکن;
                    }
                }
            }
            pair.first->second = out;
        }
        بازگشت pair.first->second;
    }
    string protect_واژگان_If(ثابت string& old, منطقی doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        اگر (!(doIt && v3Global.opt.protectIds())) بازگشت old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        وقتی (درست) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            اگر (pos == string::npos) بشکن;
            out += protectIf(old.substr(start, pos - start), درست) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), درست);
        بازگشت out;
    }
    تهی writeMapFile(ثابت string& filename) ثابت {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            برای (ثابت auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

خصوصی:
    تهی trySep(ثابت string& old, string::size_type start, ثابت string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        ثابت string::size_type trypos = old.find(trySep, start);
        اگر (trypos != string::npos) {
            اگر (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ثابت string& in) VL_MT_SAFE {
    بازگشت v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(ثابت string& old, منطقی doIt) VL_MT_SAFE {
    بازگشت VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_واژگان_If(ثابت string& old, منطقی doIt) VL_MT_SAFE {
    بازگشت VIdProtectImp::singleton().protect_واژگان_If(old, doIt);
}
تهی VIdProtect::writeMapFile(ثابت string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
