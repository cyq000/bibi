#include "FeishuNotifier.h"
#include <curl/curl.h>
#include <iostream>

bool FeishuNotifier::sendMessage(const std::string& text) {
    std::lock_guard<std::mutex> lk(mu_);
    if (webhook_.empty()) {
        std::cerr << "[ERROR] Feishu webhook not set!" << std::endl;
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string payload = "{\"msg_type\":\"text\",\"content\":{\"text\":\"" + text + "\"}}";

    curl_easy_setopt(curl, CURLOPT_URL, webhook_.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,
                     curl_slist_append(NULL, "Content-Type: application/json"));

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[ERROR] curl_easy_perform() failed: "
                  << curl_easy_strerror(res) << std::endl;
        return false;
    }
    return true;
}
