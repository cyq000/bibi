#ifndef FEISHU_NOTIFIER_H
#define FEISHU_NOTIFIER_H

/*
* FeishuNotifier类
* 提供飞书消息通知功能
*/

#include <string>
#include <mutex>

class FeishuNotifier {
public:
    static FeishuNotifier& instance() {
        static FeishuNotifier inst;
        return inst;
    }

    /*
    * 设置飞书 Webhook（只需要一次）
    */
    void setWebhook(const std::string& webhook) {
        std::lock_guard<std::mutex> lk(mu_);
        webhook_ = webhook;
    }

    /*
    * 发送文本消息到飞书群
    */
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
