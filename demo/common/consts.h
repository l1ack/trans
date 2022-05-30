#pragma once

#include <cstdlib>
#include <cstdint>

namespace xmit {
namespace demo {

enum StreamType {
  kCtrlStream = 1,
  kDataStream = 2,
};

enum ControlSignal {
  kCloseSignal = 1,
};

}  // namespace demo
}  // namespace xmit
