
# Webhook 地址 
# https://open.feishu.cn/open-apis/bot/v2/hook/6f399df9-a303-42a6-84cd-ea23d3d7cadd


# feishu_bot.py

import requests
import json

def send_feishu_message(webhook_url: str, msg_type: str, content: dict):
    """
    发送消息到飞书机器人

    参数:
        webhook_url (str): 飞书机器人的 Webhook 地址
        msg_type (str): 消息类型，'text' 或 'post'
        content (dict): 消息内容结构（根据 msg_type 而不同）
    """
    headers = {
        "Content-Type": "application/json"
    }

    data = {
        "msg_type": msg_type,
        "content": content
    }

    try:
        response = requests.post(url=webhook_url, headers=headers, data=json.dumps(data))
        response.raise_for_status()
        print("✅ 消息发送成功")
    except requests.RequestException as e:
        print(f"❌ 发送失败：{e}")


def build_feishu_post_message(title: str, lines: list[str]) -> dict:
    """
    构建飞书结构化 post 类型消息内容

    参数:
        title (str): 消息标题
        lines (list[str]): 每一行内容（自动换行，支持 emoji 和格式）

    返回:
        dict: 可直接用于 send_feishu_message 的 content 字段
    """
    post_content = []

    for line in lines:
        post_content.append([
            {"tag": "text", "text": line}
        ])

    return {
        "post": {
            "zh_cn": {
                "title": title,
                "content": post_content
            }
        }
    }
    

# 在其他脚本中调用

# from feishu_bot import send_feishu_message
# webhook = 'https://open.feishu.cn/open-apis/bot/v2/hook/xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx'

# # 发送纯文本消息
# send_feishu_message(
#     webhook_url=webhook,
#     msg_type='text',
#     content={
#         "text": "🚨 BTCUSDT 成交量异常，请关注！"
#     }
# )

# # 或发送结构化 Markdown 消息
# send_feishu_message(
#     webhook_url=webhook,
#     msg_type='post',
#     content={
#         "post": {
#             "zh_cn": {
#                 "title": "行情预警",
#                 "content": [
#                     [
#                         {"tag": "text", "text": "🚨 币种 "},
#                         {"tag": "text", "text": "BTCUSDT", "text_type": "bold"},
#                         {"tag": "text", "text": " 成交量放大，请立即检查。"}
#                     ]
#                 ]
#             }
#         }
#     }
# )



# from feishu_bot import send_feishu_message
# from your_module import build_feishu_post_message  # 根据文件结构替换
# symbol = "BTCUSDT"
# title = f"{symbol} 多头占比预警"
# lines = [
#     f"📌 币种: **{symbol}**",
#     f"👥 所有用户多空人数比: **62.3%**",
#     f"💰 持仓大户多空人数比: **65.1%**",
#     f"📊 持仓大户持仓量多空比: **68.7%**",
#     "⚠️ 满足条件：1小时级别内三个多单指标均大于 60%，可能即将下跌"
# ]
# content = build_feishu_post_message(title, lines)
# send_feishu_message(
#     webhook_url="https://open.feishu.cn/open-apis/bot/v2/hook/xxxx-xxxx-xxxx",
#     msg_type="post",
#     content=content
# )