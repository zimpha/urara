#pragma once

#include <cstdio>
#include <cstring>
#include <ctime>
#include <glob.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/file.h>

#include <list>
#include <string>
#include <chrono>
#include <thread>

namespace urara {

using system_clock = std::chrono::system_clock;

namespace os {
  inline std::string join(const std::string& dirname, const std::string& basename) {
    if (!dirname.size()) {
      return basename;
    }
    if (!basename.size()) {
      return dirname;
    }
    if (dirname.back() == '/') {
      if (basename[0] == '/') {
        return dirname + basename.substr(1);
      } else {
        return dirname + basename;
      }
    } else {
      if (basename[0] == '/') {
        return dirname + basename;
      } else {
        return dirname + '/' + basename;
      }
    }
  }

  inline std::string dirname(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
      return "";
    } else {
      return path.substr(0, pos);
    }
  }

  inline std::string basename(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
      return path;
    } else {
      return path.substr(pos + 1);
    }
  }

  inline std::string extname(const std::string& path) {
    std::string base = basename(path);
    size_t pos = base.find_last_of('.');
    if (pos == std::string::npos) {
      return "";
    } else {
      return base.substr(pos);
    }
  }

  inline bool exists(const std::string& path) {
    struct stat buf;
    return stat(path.c_str(), &buf) == 0;
  }

  inline bool is_dir(const std::string& path) {
    struct stat buf;
    if (stat(path.c_str(), &buf) == -1) {
      return false;
    } else {
      return S_ISDIR(buf.st_mode);
    }
  }

  inline bool is_file(const std::string& path) {
    struct stat buf;
    if (stat(path.c_str(), &buf) == -1) {
      return false;
    } else {
      return S_ISREG(buf.st_mode);
    }
  }

  inline bool is_symlink(const std::string& path) {
    struct stat buf;
    if (stat(path.c_str(), &buf) == -1) {
      return false;
    } else {
      return S_ISLNK(buf.st_mode);
    }
  }

  inline int mkdir(const std::string& path, const mode_t mode = 0755) {
    if (is_dir(path)) {
      return 0;
    }
    for (size_t i = 0; i < path.size(); ++i) {
      if (i > 0 && path[i] == '/') {
        ::mkdir(path.substr(0, i).c_str(), mode);
      }
    }
    return ::mkdir(path.c_str(), mode);
  }

  inline int move(const std::string& src, const std::string& dst, bool overwrite = false) {
    if (overwrite) {
      return std::rename(src.c_str(), dst.c_str());
    } else if (!exists(dst)) {
      std::rename(src.c_str(), dst.c_str());
    }
    return -1;
  }

  inline int remove(const std::string& path) {
    return std::remove(path.c_str());
  }

  inline int rm_rf(const std::string& path) {
    if (unlink(path.c_str()) == 0) return 0;
    if (rmdir(path.c_str()) == 0) return 0;
    struct dirent **namelist = 0;
    int nlist = ::scandir(path.c_str(), &namelist, 0, alphasort);
    for (int i = 0; i < nlist; ++i) {
      const char* name = namelist[i]->d_name;
      if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) rm_rf(join(path, name));
      free(namelist[i]);
    }
    if (namelist) {
      free(namelist);
    }
    if (rmdir(path.c_str()) == 0) {
      return 0;
    }
    return -1;
  }

  inline int rename(const std::string& src, const std::string& dst) {
    return move(src, dst);
  }

  inline int chmod(const std::string& path, const mode_t mode) {
    return ::chmod(path.c_str(), mode);
  }

  inline std::list<std::string> ls(const std::string& path) {
    std::list<std::string> res;
    struct dirent **namelist = 0;
    int nlist = ::scandir(path.c_str(), &namelist, 0, alphasort);
    for (int i = 0; i < nlist; ++i) {
      const char * name = namelist[i]->d_name;
      if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
        res.emplace_back(name);
      }
      free(namelist[i]);
    }
    if (namelist) {
      free(namelist);
    }
    return res;
  }

  inline std::list<std::string> glob(const std::string& pattern) {
    std::list<std::string> res;
    glob_t globbuf;
    globbuf.gl_offs = 0;
    glob(pattern.c_str(), GLOB_DOOFFS | GLOB_NOSORT | GLOB_BRACE, NULL, &globbuf);
    for (size_t i = 0; ; ++i) {
      if (!globbuf.gl_pathv[i]) break;
      res.emplace_back(globbuf.gl_pathv[i]);
    }
    return res;
  }

  inline std::string read(const std::string& path, size_t max_length = 1024) {
    FILE *fp = fopen(path.c_str(), "r");
    if (!fp) {
      return "";
    }
    std::unique_ptr<char[]> buffer(new char[max_length + 1]);
    int size = fread(buffer.get(), 1, max_length + 1, fp);
    fclose(fp);
    buffer.get()[max_length] = 0;
    if (size >= 0 && (size_t)size < max_length) {
      buffer.get()[size] = 0;
    }
    return std::string(buffer.get());
  }

  inline int write(const std::string& path, const std::string& data) {
    FILE* fp = fopen(path.c_str(), "w");
    if (!fp) {
      return -1;
    }
    size_t size = fwrite(data.c_str(), data.size(), 1, fp);
    fclose(fp);
    return size ? 0 : -2;
  }

  inline system_clock::time_point now() {
    return system_clock::now();
  }

  inline std::tm localtime(const std::time_t& time) {
    std::tm tm;
    localtime_r(&time, &tm);
    return tm;
  }

  inline std::tm localtime() {
    std::time_t now = time(nullptr);
    return localtime(now);
  }

  inline std::tm gmtime(const std::time_t &time) {
    std::tm tm;
    gmtime_r(&time, &tm);
    return tm;
  }

  inline std::tm gmtime() {
    std::time_t now = time(nullptr);
    return gmtime(now);
  }

  inline size_t thread_id() {
    std::thread::id id = std::this_thread::get_id();
    size_t* ptr = reinterpret_cast<size_t*>(&id);
    return *ptr;
  }

  inline int pid() {
    return static_cast<int>(::getpid());
  }

  class scoped_file_lock {
  public:
    scoped_file_lock(const std::string& path) {
      int fd = open(path.c_str(), O_RDONLY);
      if (fd < 0) {
        return;
      }
      if (flock(fd, LOCK_EX) == 0) {
        this->fd_ =fd;
      } else {
        close(fd);
        this->fd_ = -1;
      }
    }

    ~scoped_file_lock() {
      int fd = this->fd_;
      if (fd < 0) {
        return;
      }
      flock(fd, LOCK_UN);
      close(fd);
    }

  private:
    int fd_;
  };
}
}
