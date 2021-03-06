#ifndef DIAG_H
#define DIAG_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>

namespace mxrlang {

enum class DiagID {
#define DIAG(ID, Level, Msg) ID,
#include "Diag.def"
};

class Diag {
  // Returns the error text based on the error ID.
  static const char *getDiagText(DiagID diagType);
  // Returns the error kind based on the error ID.
  static llvm::SourceMgr::DiagKind getDiagKind(DiagID diagID);

  llvm::SourceMgr &srcMgr;

  // Total number of seen errors.
  uint32_t numErrs;

public:
  Diag(llvm::SourceMgr &srcMgr) : srcMgr(srcMgr), numErrs(0) {}

  // Report an error. Provide the LoC where it happened, its ID, and additional
  // textual parameters where needed.
  template <typename... Args>
  void report(llvm::SMLoc loc, DiagID diagID, Args &&...args) {
    auto msg =
        llvm::formatv(getDiagText(diagID), std::forward<Args>(args)...).str();
    auto kind = getDiagKind(diagID);
    srcMgr.PrintMessage(loc, kind, msg);
    numErrs += (kind == llvm::SourceMgr::DK_Error);
  }

  uint32_t getNumErrs() { return numErrs; }
};

} // namespace mxrlang

#endif // DIAG_H
