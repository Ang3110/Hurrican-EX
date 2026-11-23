/**
 * This implements logging to a file and cout.
 *
 * (c) 2002 Jörg M. Winterstein
 * (c) 2017 Stefan Schmidt
 **/

#ifndef _LOGDATEI_HPP_
#define _LOGDATEI_HPP_

#include <fstream>
#include <sstream>
#include <string>

class Logdatei : public std::ostringstream {
  private:
    std::ofstream file;

  public:
    ~Logdatei() override;
    void OpenFile(const std::string &filename);

    void flush();

    // inherit all the output-operators for compatibility
    template <typename T>
    inline Logdatei &operator<<(const T &t) {
        (*reinterpret_cast<std::ostringstream *>(this)) << t;
        return *this;
    }

    // compatibility for manipulators
    typedef Logdatei &(*Logdatei_manip)(Logdatei &);
    inline Logdatei &operator<<(Logdatei_manip manip) { return manip(*this); }
};

// overload std::endl with a Logdatei-compatible version
namespace std {
inline Logdatei &endl(Logdatei &out) {
    out.put('\n');
    out.flush();
    return out;
}
}  // namespace std

extern Logdatei Protokoll;

#endif
