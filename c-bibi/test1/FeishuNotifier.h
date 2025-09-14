#ifndef FEISHU_NOTIFIER_H
#define FEISHU_NOTIFIER_H

#include <string>
#include <mutex>

class FeishuNotifier {
public:
    static FeishuNotifier& instance() {
        static FeishuNotifier inst;
        return inst;
    }

    void setWebhook(const std::string& webhook) {
        std::lock_guard<std::mutex> lk(mu_);
        webhook_ = webhook;
    }

    bool sendMessage(const std::string& text);

private:
    FeishuNotifier() = default;
    ~FeishuNotifier() = default;
    FeishuNotifier(const FeishuNotifier&) = delete;
    FeishuNotifier& operator=(const FeishuNotifier&) = delete;

    std::string webhook_;
    std::mutex mu_;
};

#endif
