#include <gtest/gtest.h>

#include <vector>

#include "esphome/components/hdmi_cec/hdmi_cec.h"

namespace esphome::hdmi_cec {
// The tables codegen emits for osd_name and an on_message automation claiming opcode 0x44
const char OSD_NAME_REPLY[] PROGMEM = "Gesphome";
const uint8_t CLAIMED_OPCODES[] PROGMEM = {0x44};
}  // namespace esphome::hdmi_cec

namespace esphome::hdmi_cec::testing {

static constexpr uint8_t TV = 0x0;
static constexpr uint8_t US = 0x4;  // playback device 1

class TestCEC : public HDMICEC {
 public:
  using HDMICEC::HDMICEC;
  void receive(uint8_t initiator, uint8_t destination, std::vector<uint8_t> payload) {
    const Frame frame(initiator, destination, payload);
    this->handle_received_message_(&frame);
  }
  /// The queued frames, as header byte then payload
  std::vector<std::vector<uint8_t>> sent() {
    std::vector<std::vector<uint8_t>> frames;
    while (const Frame *frame = this->send_queue_.front()) {
      frames.emplace_back(frame->bytes().begin(), frame->bytes().end());
      this->send_queue_.pop_front();
    }
    return frames;
  }
};

struct ReplyCase {
  const char *name;
  uint8_t initiator;
  uint8_t destination;
  std::vector<uint8_t> payload;
  bool monitor_mode;
  std::vector<std::vector<uint8_t>> replies;
};

class ReplyTest : public ::testing::TestWithParam<ReplyCase> {};

TEST_P(ReplyTest, RepliesAsCecExpects) {
  const auto &c = GetParam();
  TestCEC cec(nullptr, US, 4);
  cec.set_monitor_mode(c.monitor_mode);
  cec.receive(c.initiator, c.destination, c.payload);
  EXPECT_EQ(cec.sent(), c.replies);
}

INSTANTIATE_TEST_SUITE_P(
    Frames, ReplyTest,
    ::testing::Values(ReplyCase{"CecVersion", TV, US, {0x9F}, false, {{0x40, 0x9E, 0x05}}},
                      ReplyCase{"OsdName", TV, US, {0x46}, false, {{0x40, 0x47, 'e', 's', 'p', 'h', 'o', 'm', 'e'}}},
                      // Without a physical address there is nothing to report, so it is refused like any other request
                      ReplyCase{"PhysicalAddressUnset", TV, US, {0x83}, false, {{0x40, 0x00, 0x83, 0x00}}},
                      ReplyCase{"UnknownOpcode", TV, US, {0x8F}, false, {{0x40, 0x00, 0x8F, 0x00}}},
                      ReplyCase{"AbortIsRefused", TV, US, {0xFF}, false, {{0x40, 0x00, 0xFF, 0x04}}},
                      ReplyCase{"FeatureAbortNotAnswered", TV, US, {0x00, 0x8F, 0x00}, false, {}},
                      ReplyCase{"ClaimedOpcode", TV, US, {0x44, 0x41}, false, {}},
                      ReplyCase{"Broadcast", TV, Frame::BROADCAST_ADDRESS, {0x9F}, false, {}},
                      ReplyCase{"OtherDestination", TV, 0x5, {0x9F}, false, {}},
                      ReplyCase{"UnregisteredInitiator", Frame::BROADCAST_ADDRESS, US, {0x9F}, false, {}},
                      ReplyCase{"MonitorMode", TV, US, {0x9F}, true, {}}),
    [](const ::testing::TestParamInfo<ReplyCase> &info) { return info.param.name; });

// <Report Physical Address> is broadcast with the device type codegen worked out for our address
TEST(PhysicalAddress, ReportsTheDeviceType) {
  TestCEC cec(nullptr, 0x9, 1);  // recording device 3
  cec.set_physical_address(0x2100);
  cec.receive(TV, 0x9, {0x83});
  EXPECT_EQ(cec.sent(), (std::vector<std::vector<uint8_t>>{{0x9F, 0x84, 0x21, 0x00, 1}}));
}

}  // namespace esphome::hdmi_cec::testing
