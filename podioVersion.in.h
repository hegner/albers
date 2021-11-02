#ifndef podioVersion_h
#define podioVersion_h

#include <cstdint>
#include <tuple>
#include <ostream>
#if __cplusplus >= 202002L
#include <compare>
#endif

// Some preprocessor constants and macros for the use cases where they might be
// necessary

/// Define a version to be used in podio.
#define PODIO_VERSION(major, minor, patch) (((unsigned long)(major) << 32) | ((unsigned long)(minor) << 16) | ((unsigned long)(patch)))
/// Get the major version from a preprocessor defined version
#define PODIO_MAJOR_VERSION(v) (((v) & (-1UL >> 16)) >> 32)
/// Get the minor version from a preprocessor defined version
#define PODIO_MINOR_VERSION(v) (((v) & (-1UL >> 32)) >> 16)
/// Get the patch version from a preprocessor defined version
#define PODIO_PATCH_VERSION(v) ((v) & (-1UL >> 48))

// Some helper constants that are populated by the cmake configure step
#define podio_VERSION @podio_VERSION@
#define podio_VERSION_MAJOR @podio_VERSION_MAJOR@
#define podio_VERSION_MINOR @podio_VERSION_MINOR@
#define podio_VERSION_PATCH @podio_VERSION_PATCH@

/// The encoded version with which podio has been built
#define PODIO_BUILD_VERSION PODIO_VERSION(podio_VERSION_MAJOR, podio_VERSION_MINOR, podio_VERSION_PATCH)

namespace podio::version {

  struct Version {
    uint16_t major{0};
    uint16_t minor{0};
    uint16_t patch{0};

#if __cplusplus >= 202002L
    auto operator<=>(const Version&) const = default;
#else
// No spaceship yet in c++17
#define DEFINE_COMP_OPERATOR(OP)                                                   \
    constexpr bool operator OP(const Version& o) const noexcept {                  \
      return std::tie(major, minor, patch) OP std::tie(o.major, o.minor, o.patch); \
    }

    DEFINE_COMP_OPERATOR(<)
    DEFINE_COMP_OPERATOR(<=)
    DEFINE_COMP_OPERATOR(>)
    DEFINE_COMP_OPERATOR(>=)
    DEFINE_COMP_OPERATOR(==)
    DEFINE_COMP_OPERATOR(!=)

#undef DEFINE_COMP_OPERATOR
#endif

    friend std::ostream& operator<<(std::ostream&, const Version& v);
  };

  inline std::ostream& operator<<(std::ostream& os, const Version& v) {
    return os << v.major << "." << v.minor << "." << v.patch;
  }

  /**
   * The current build version
   */
  static constexpr Version build_version{podio_VERSION_MAJOR, podio_VERSION_MINOR, podio_VERSION_PATCH};

  /**
   * Decode a version from a 64 bit unsigned
   */
  static constexpr Version decode_version(unsigned long version) noexcept {
    return Version{
      (uint16_t) PODIO_MAJOR_VERSION(version),
      (uint16_t) PODIO_MINOR_VERSION(version),
      (uint16_t) PODIO_PATCH_VERSION(version)
    };
  }


  enum class Compatibility {
    AnyNewer,  ///< A version is equal or higher than another version
    SameMajor, ///< Two versions have the same major version
    SameMinor, ///< Two versions have the same major and minor version
    Exact      ///< Two versions are exactly the same
  };


  /**
   * Check if Version va is compatible with Version vb under a given
   * compatibility strategy (defaults AnyNewer).
   */
  inline constexpr bool compatible(Version va, Version vb, Compatibility compat=Compatibility::AnyNewer) noexcept {
    switch (compat) {
      case Compatibility::Exact:
        return va == vb;
      case Compatibility::AnyNewer:
        return va >= vb;
      case Compatibility::SameMajor:
        return va.major == vb.major;
      case Compatibility::SameMinor:
        return va.major == vb.major && va.minor == vb.minor;
    }
  }

  /**
   * Check if the version is compatible with the current build_version
   */
  inline constexpr bool compatible(Version v, Compatibility compat=Compatibility::AnyNewer) noexcept {
    return compatible(v, build_version, compat);
  }
   
}


#endif
