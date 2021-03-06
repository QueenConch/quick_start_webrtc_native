/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains mock implementations of observers used in PeerConnection.
// TODO(steveanton): These aren't really mocks and should be renamed.

#ifndef PC_TEST_MOCKPEERCONNECTIONOBSERVERS_H_
#define PC_TEST_MOCKPEERCONNECTIONOBSERVERS_H_

#include <memory>
#include <string>

#include "api/datachannelinterface.h"
#include "pc/streamcollection.h"
#include "rtc_base/checks.h"

namespace webrtc {

class MockPeerConnectionObserver : public PeerConnectionObserver {
 public:
  MockPeerConnectionObserver() : remote_streams_(StreamCollection::Create()) {}
  virtual ~MockPeerConnectionObserver() {}
  void SetPeerConnectionInterface(PeerConnectionInterface* pc) {
    pc_ = pc;
    if (pc) {
      state_ = pc_->signaling_state();
    }
  }
  void OnSignalingChange(
      PeerConnectionInterface::SignalingState new_state) override {
    RTC_DCHECK(pc_->signaling_state() == new_state);
    state_ = new_state;
  }

  MediaStreamInterface* RemoteStream(const std::string& label) {
    return remote_streams_->find(label);
  }
  StreamCollectionInterface* remote_streams() const { return remote_streams_; }
  void OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream) override {
    last_added_stream_ = stream;
    remote_streams_->AddStream(stream);
  }
  void OnRemoveStream(
      rtc::scoped_refptr<MediaStreamInterface> stream) override {
    last_removed_stream_ = stream;
    remote_streams_->RemoveStream(stream);
  }
  void OnRenegotiationNeeded() override { renegotiation_needed_ = true; }
  void OnDataChannel(
      rtc::scoped_refptr<DataChannelInterface> data_channel) override {
    last_datachannel_ = data_channel;
  }

  void OnIceConnectionChange(
      PeerConnectionInterface::IceConnectionState new_state) override {
    RTC_DCHECK(pc_->ice_connection_state() == new_state);
    callback_triggered_ = true;
  }
  void OnIceGatheringChange(
      PeerConnectionInterface::IceGatheringState new_state) override {
    RTC_DCHECK(pc_->ice_gathering_state() == new_state);
    ice_complete_ = new_state == PeerConnectionInterface::kIceGatheringComplete;
    callback_triggered_ = true;
  }
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {
    RTC_DCHECK(PeerConnectionInterface::kIceGatheringNew !=
               pc_->ice_gathering_state());

    std::string sdp;
    bool success = candidate->ToString(&sdp);
    RTC_DCHECK(success);
    RTC_DCHECK(!sdp.empty());
    last_candidate_.reset(webrtc::CreateIceCandidate(
        candidate->sdp_mid(), candidate->sdp_mline_index(), sdp, NULL));
    RTC_DCHECK(last_candidate_);
    callback_triggered_ = true;
  }

  void OnIceCandidatesRemoved(
      const std::vector<cricket::Candidate>& candidates) override {
    callback_triggered_ = true;
  }

  void OnIceConnectionReceivingChange(bool receiving) override {
    callback_triggered_ = true;
  }

  void OnAddTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
          streams) override {
    RTC_DCHECK(receiver);
    num_added_tracks_++;
    last_added_track_label_ = receiver->id();
  }

  // Returns the label of the last added stream.
  // Empty string if no stream have been added.
  std::string GetLastAddedStreamLabel() {
    if (last_added_stream_.get())
      return last_added_stream_->label();
    return "";
  }
  std::string GetLastRemovedStreamLabel() {
    if (last_removed_stream_.get())
      return last_removed_stream_->label();
    return "";
  }

  rtc::scoped_refptr<PeerConnectionInterface> pc_;
  PeerConnectionInterface::SignalingState state_;
  std::unique_ptr<IceCandidateInterface> last_candidate_;
  rtc::scoped_refptr<DataChannelInterface> last_datachannel_;
  rtc::scoped_refptr<StreamCollection> remote_streams_;
  bool renegotiation_needed_ = false;
  bool ice_complete_ = false;
  bool callback_triggered_ = false;
  int num_added_tracks_ = 0;
  std::string last_added_track_label_;

 private:
  rtc::scoped_refptr<MediaStreamInterface> last_added_stream_;
  rtc::scoped_refptr<MediaStreamInterface> last_removed_stream_;
};

class MockCreateSessionDescriptionObserver
    : public webrtc::CreateSessionDescriptionObserver {
 public:
  MockCreateSessionDescriptionObserver()
      : called_(false),
        result_(false) {}
  virtual ~MockCreateSessionDescriptionObserver() {}
  virtual void OnSuccess(SessionDescriptionInterface* desc) {
    called_ = true;
    result_ = true;
    desc_.reset(desc);
  }
  virtual void OnFailure(const std::string& error) {
    called_ = true;
    result_ = false;
  }
  bool called() const { return called_; }
  bool result() const { return result_; }
  std::unique_ptr<SessionDescriptionInterface> MoveDescription() {
    return std::move(desc_);
  }

 private:
  bool called_;
  bool result_;
  std::unique_ptr<SessionDescriptionInterface> desc_;
};

class MockSetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  MockSetSessionDescriptionObserver()
      : called_(false),
        result_(false) {}
  virtual ~MockSetSessionDescriptionObserver() {}
  virtual void OnSuccess() {
    called_ = true;
    result_ = true;
  }
  virtual void OnFailure(const std::string& error) {
    called_ = true;
    result_ = false;
  }
  bool called() const { return called_; }
  bool result() const { return result_; }

 private:
  bool called_;
  bool result_;
};

class MockDataChannelObserver : public webrtc::DataChannelObserver {
 public:
  explicit MockDataChannelObserver(webrtc::DataChannelInterface* channel)
      : channel_(channel) {
    channel_->RegisterObserver(this);
    state_ = channel_->state();
  }
  virtual ~MockDataChannelObserver() {
    channel_->UnregisterObserver();
  }

  void OnBufferedAmountChange(uint64_t previous_amount) override {}

  void OnStateChange() override { state_ = channel_->state(); }
  void OnMessage(const DataBuffer& buffer) override {
    messages_.push_back(
        std::string(buffer.data.data<char>(), buffer.data.size()));
  }

  bool IsOpen() const { return state_ == DataChannelInterface::kOpen; }
  std::vector<std::string> messages() const { return messages_; }
  std::string last_message() const {
    return messages_.empty() ? std::string() : messages_.back();
  }
  size_t received_message_count() const { return messages_.size(); }

 private:
  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  DataChannelInterface::DataState state_;
  std::vector<std::string> messages_;
};

class MockStatsObserver : public webrtc::StatsObserver {
 public:
  MockStatsObserver() : called_(false), stats_() {}
  virtual ~MockStatsObserver() {}

  virtual void OnComplete(const StatsReports& reports) {
    RTC_CHECK(!called_);
    called_ = true;
    stats_.Clear();
    stats_.number_of_reports = reports.size();
    for (const auto* r : reports) {
      if (r->type() == StatsReport::kStatsReportTypeSsrc) {
        stats_.timestamp = r->timestamp();
        GetIntValue(r, StatsReport::kStatsValueNameAudioOutputLevel,
            &stats_.audio_output_level);
        GetIntValue(r, StatsReport::kStatsValueNameAudioInputLevel,
            &stats_.audio_input_level);
        GetIntValue(r, StatsReport::kStatsValueNameBytesReceived,
            &stats_.bytes_received);
        GetIntValue(r, StatsReport::kStatsValueNameBytesSent,
            &stats_.bytes_sent);
        GetInt64Value(r, StatsReport::kStatsValueNameCaptureStartNtpTimeMs,
            &stats_.capture_start_ntp_time);
      } else if (r->type() == StatsReport::kStatsReportTypeBwe) {
        stats_.timestamp = r->timestamp();
        GetIntValue(r, StatsReport::kStatsValueNameAvailableReceiveBandwidth,
            &stats_.available_receive_bandwidth);
      } else if (r->type() == StatsReport::kStatsReportTypeComponent) {
        stats_.timestamp = r->timestamp();
        GetStringValue(r, StatsReport::kStatsValueNameDtlsCipher,
            &stats_.dtls_cipher);
        GetStringValue(r, StatsReport::kStatsValueNameSrtpCipher,
            &stats_.srtp_cipher);
      }
    }
  }

  bool called() const { return called_; }
  size_t number_of_reports() const { return stats_.number_of_reports; }
  double timestamp() const { return stats_.timestamp; }

  int AudioOutputLevel() const {
    RTC_CHECK(called_);
    return stats_.audio_output_level;
  }

  int AudioInputLevel() const {
    RTC_CHECK(called_);
    return stats_.audio_input_level;
  }

  int BytesReceived() const {
    RTC_CHECK(called_);
    return stats_.bytes_received;
  }

  int BytesSent() const {
    RTC_CHECK(called_);
    return stats_.bytes_sent;
  }

  int64_t CaptureStartNtpTime() const {
    RTC_CHECK(called_);
    return stats_.capture_start_ntp_time;
  }

  int AvailableReceiveBandwidth() const {
    RTC_CHECK(called_);
    return stats_.available_receive_bandwidth;
  }

  std::string DtlsCipher() const {
    RTC_CHECK(called_);
    return stats_.dtls_cipher;
  }

  std::string SrtpCipher() const {
    RTC_CHECK(called_);
    return stats_.srtp_cipher;
  }

 private:
  bool GetIntValue(const StatsReport* report,
                   StatsReport::StatsValueName name,
                   int* value) {
    const StatsReport::Value* v = report->FindValue(name);
    if (v) {
      // TODO(tommi): We should really just be using an int here :-/
      *value = rtc::FromString<int>(v->ToString());
    }
    return v != nullptr;
  }

  bool GetInt64Value(const StatsReport* report,
                   StatsReport::StatsValueName name,
                   int64_t* value) {
    const StatsReport::Value* v = report->FindValue(name);
    if (v) {
      // TODO(tommi): We should really just be using an int here :-/
      *value = rtc::FromString<int64_t>(v->ToString());
    }
    return v != nullptr;
  }

  bool GetStringValue(const StatsReport* report,
                      StatsReport::StatsValueName name,
                      std::string* value) {
    const StatsReport::Value* v = report->FindValue(name);
    if (v)
      *value = v->ToString();
    return v != nullptr;
  }

  bool called_;
  struct {
    void Clear() {
      number_of_reports = 0;
      timestamp = 0;
      audio_output_level = 0;
      audio_input_level = 0;
      bytes_received = 0;
      bytes_sent = 0;
      capture_start_ntp_time = 0;
      available_receive_bandwidth = 0;
      dtls_cipher.clear();
      srtp_cipher.clear();
    }

    size_t number_of_reports;
    double timestamp;
    int audio_output_level;
    int audio_input_level;
    int bytes_received;
    int bytes_sent;
    int64_t capture_start_ntp_time;
    int available_receive_bandwidth;
    std::string dtls_cipher;
    std::string srtp_cipher;
  } stats_;
};

// Helper class that just stores the report from the callback.
class MockRTCStatsCollectorCallback : public webrtc::RTCStatsCollectorCallback {
 public:
  rtc::scoped_refptr<const RTCStatsReport> report() { return report_; }

  bool called() const { return called_; }

 protected:
  void OnStatsDelivered(
      const rtc::scoped_refptr<const RTCStatsReport>& report) override {
    report_ = report;
    called_ = true;
  }

 private:
  bool called_ = false;
  rtc::scoped_refptr<const RTCStatsReport> report_;
};

}  // namespace webrtc

#endif  // PC_TEST_MOCKPEERCONNECTIONOBSERVERS_H_
