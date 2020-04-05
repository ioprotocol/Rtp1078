//
// Created by 84515 on 2020/4/5.
//

#ifndef RTP1078_RTMP_CMD_CONNECT_H
#define RTP1078_RTMP_CMD_CONNECT_H
#include <cstdint>
#include <iostream>

class rtmp_cmd_connect {
public:
    friend std::ostream &operator<<(std::ostream &os, const rtmp_cmd_connect &connect) {
        os << "name_: " << connect.name_ << " transaction_iD_: " << connect.transaction_id_ << " app_: " << connect.app_
           << " flashver_: " << connect.flashver_ << " swf_url_: " << connect.swf_url_ << " tc_url_: "
           << connect.tc_url_ << " fpad_: " << connect.fpad_ << " audio_codecs_: " << connect.audio_codecs_
           << " vidio_codecs_: " << connect.vidio_codecs_ << " vidio_function_: " << connect.vidio_function_
           << " page_url_: " << connect.page_url_ << " capabilities: " << connect.capabilities;
        return os;
    }

    const std::string &getName() const {
        return name_;
    }

    void setName(const std::string &name) {
        name_ = name;
    }

    const uint64_t &getTransactionId() const {
        return transaction_id_;
    }

    void setTransactionId(const uint64_t &transactionId) {
        transaction_id_ = transactionId;
    }

    const std::string &getApp() const {
        return app_;
    }

    void setApp(const std::string &app) {
        app_ = app;
    }

    const std::string &getFlashver() const {
        return flashver_;
    }

    void setFlashver(const std::string &flashver) {
        flashver_ = flashver;
    }

    const std::string &getSwfUrl() const {
        return swf_url_;
    }

    void setSwfUrl(const std::string &swfUrl) {
        swf_url_ = swfUrl;
    }

    const std::string &getTcUrl() const {
        return tc_url_;
    }

    void setTcUrl(const std::string &tcUrl) {
        tc_url_ = tcUrl;
    }

    const uint8_t &getFpad() const {
        return fpad_;
    }

    void setFpad(const uint8_t &fpad) {
        fpad_ = fpad;
    }

    const uint64_t &getAudioCodecs() const {
        return audio_codecs_;
    }

    void setAudioCodecs(const uint64_t &audioCodecs) {
        audio_codecs_ = audioCodecs;
    }

    const uint64_t &getVidioCodecs() const {
        return vidio_codecs_;
    }

    void setVidioCodecs(const uint64_t &vidioCodecs) {
        vidio_codecs_ = vidioCodecs;
    }

    const uint64_t &getVidioFunction() const {
        return vidio_function_;
    }

    void setVidioFunction(const uint64_t &vidioFunction) {
        vidio_function_ = vidioFunction;
    }

    const std::string &getPageUrl() const {
        return page_url_;
    }

    void setPageUrl(const std::string &pageUrl) {
        page_url_ = pageUrl;
    }

    const uint64_t &getCapabilities() const {
        return capabilities;
    }

    void setCapabilities(const uint64_t &capabilities) {
        rtmp_cmd_connect::capabilities = capabilities;
    }

private:
    std::string name_;
    uint64_t    transaction_id_;
    std::string app_;
    std::string flashver_;
    std::string swf_url_;
    std::string tc_url_;
    uint8_t fpad_;
    uint64_t audio_codecs_;
    uint64_t vidio_codecs_;
    uint64_t vidio_function_;
    std::string page_url_;
    uint64_t capabilities;
};
#endif //RTP1078_RTMP_CMD_CONNECT_H
