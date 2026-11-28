#include <gtest/gtest.h>

#include <vector>

#include "esphome/components/hdmi_cec/hdmi_cec.h"

namespace esphome::hdmi_cec::testing {

// The UART bytes worked out from the line itself: a CEC bit is five UART bits, low for three then
// high (a 0) or low for one then high (a 1); each UART byte is a low start bit, eight data bits
// sent least significant first, and a high stop bit
static std::vector<uint8_t> reference_body(const Frame &frame) {
  std::vector<bool> cec_bits;
  const auto bytes = frame.bytes();
  for (size_t i = 0; i < bytes.size(); i++) {
    for (int bit = (i == 0 ? 3 : 7); bit >= 0; bit--) {
      cec_bits.push_back((bytes[i] >> bit) & 1);
    }
    cec_bits.push_back(i == bytes.size() - 1);  // end of message
    cec_bits.push_back(true);                   // acknowledge, sent high
  }
  std::vector<bool> line;
  for (bool bit : cec_bits) {
    for (int t = 0; t < 5; t++) {
      line.push_back(t >= (bit ? 1 : 3));
    }
  }
  std::vector<uint8_t> uart;
  for (size_t start = 0; start < line.size(); start += 10) {
    EXPECT_FALSE(line[start]) << "UART start bit must be low";
    EXPECT_TRUE(line[start + 9]) << "UART stop bit must be high";
    uint8_t byte = 0;
    for (int bit = 0; bit < 8; bit++) {
      byte |= line[start + 1 + bit] << bit;
    }
    uart.push_back(byte);
  }
  return uart;
}

class EncodeUartBodyTest : public ::testing::TestWithParam<std::vector<uint8_t>> {};

TEST_P(EncodeUartBodyTest, MatchesTheLineWaveform) {
  const auto &payload = GetParam();
  const Frame frame(0x4, 0x0, payload);
  uint8_t out[MAX_UART_BODY_LENGTH];
  const size_t length = encode_uart_body(frame, out);
  EXPECT_EQ(std::vector<uint8_t>(out, out + length), reference_body(frame));
  if (payload.size() == Frame::MAX_PAYLOAD_LENGTH) {
    EXPECT_EQ(length, MAX_UART_BODY_LENGTH);
  }
}

INSTANTIATE_TEST_SUITE_P(Frames, EncodeUartBodyTest,
                         ::testing::Values(std::vector<uint8_t>{},            // header only
                                           std::vector<uint8_t>{0x9F},        // one byte
                                           std::vector<uint8_t>{0x00, 0xFF},  // all zeros and ones
                                           std::vector<uint8_t>(Frame::MAX_PAYLOAD_LENGTH, 0xA5)));

}  // namespace esphome::hdmi_cec::testing
