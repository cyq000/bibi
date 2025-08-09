### 线程1
# 统一监控逻辑说明
# 对每个币种、每4小时做一次判断：

# 最近穿过布林上下轨的K线。



### 线程2
# 统一监控逻辑说明
# 对每个币种、每4小时做一次判断：

# 📉 底部突破（break_down）条件：
# 最近5根K线中上涨K线数量 ≥ 3（close > open）
# 第5根K线的收盘价 < 前15根K线收盘价最小值

# 📈 顶部突破（break_up）条件：
# 最近5根K线中下跌K线数量 ≥ 3（close < open）
# 第5根K线的收盘价 > 前15根K线收盘价最大值



import ccxt
import pandas as pd
import threading
import logging
import requests
import time
from datetime import datetime, timedelta

# ---------- 配置 ----------
#symbols = ['BTC/USDT', 'ETH/USDT', 'SOL/USDT','BNB/USDT','XRP/USDT','1000PEPE/USDT','DOGE/USDT','SUI/USDT','AAVE/USDT','BCH/USDT']  # 可添加更多币种
symbols = ['BTC/USDT', 'ETH/USDT', 'DOGE/USDT','BCH/USDT']  # 可添加更多币种
timeframe = '4h'
boll_period = 60
boll_std_dev = 2

# 设置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s %(levelname)s: %(message)s',
    handlers=[
        logging.FileHandler("monitor.log"),
        logging.StreamHandler()
    ]
)

# 初始化交易所（合约市场）
exchange = ccxt.binance({
    'options': {'defaultType': 'future'}
})

# 飞书机器人 Webhook
webhook = 'https://open.feishu.cn/open-apis/bot/v2/hook/b7815671-5ba7-45d8-9535-52707d6689a3'

def notify_feishu(message):
    try:
        headers = {'Content-Type': 'application/json'}
        json_data = {'msg_type': 'text', 'content': {'text': message}}
        response = requests.post(webhook, headers=headers, json=json_data)
        if response.status_code != 200:
            logging.warning(f"飞书通知失败: {response.text}")
    except Exception as e:
        logging.error(f"飞书请求异常: {e}")

def monitor_boll_loop():
    printed = set()
    while True:
        try:
            now = datetime.utcnow() + timedelta(hours=8)
            print(f"\n⏳【BOLL穿越监控】开始：{now.strftime('%Y-%m-%d %H:%M:%S')} (北京时间)")
            for symbol in symbols:
                try:
                    ohlcv = exchange.fetch_ohlcv(symbol, timeframe=timeframe, limit=1000)
                    df = pd.DataFrame(ohlcv, columns=['timestamp', 'open', 'high', 'low', 'close', 'volume'])
                    df['timestamp'] = pd.to_datetime(df['timestamp'], unit='ms') + timedelta(hours=8)
                    df.set_index('timestamp', inplace=True)
                    
                    # 计算布林带指标（BOLL）
                    df['ma'] = df['close'].rolling(window=boll_period).mean()
                    df['std'] = df['close'].rolling(window=boll_period).std()
                    df['upper'] = df['ma'] + boll_std_dev * df['std']
                    df['lower'] = df['ma'] - boll_std_dev * df['std']
                    
                    # 判断是否穿越布林带上下轨
                    df['break_upper'] = (df['high'] >= df['upper']) & (df['low'] <= df['upper'])
                    df['break_lower'] = (df['low'] <= df['lower']) & (df['high'] >= df['lower'])
                    
                    # 检查最近一根K线是否首次穿越
                    last_row = df.iloc[-1]
                    key = f"{symbol}_{last_row.name}"

                    if key not in printed:
                        if last_row['break_upper']:
                            direction = "🔺突破上轨"
                        elif last_row['break_lower']:
                            direction = "🔻跌破下轨"
                        else:
                            direction = None

                        if direction:
                            msg = (
                            f"📊BOLL穿越信号：{symbol}\n"
                            f"{direction}\n"
                            f"时间: {last_row.name}\n"
                            f"收盘: {last_row['close']:.2f}\n"
                            # f"上轨: {last_row['upper']:.2f}, 下轨: {last_row['lower']:.2f}\n"
                            # f"最高: {last_row['high']:.2f}, 最低: {last_row['low']:.2f}"
                            )
                            logging.info(msg)
                            notify_feishu(msg)
                            printed.add(key)

                except Exception as e:
                    logging.error(f"{symbol} BOLL监控异常: {e}")

            print("【BOLL穿越监控】本轮结束，等待4小时...\n")

        except Exception as e:
            logging.error(f"BOLL线程异常: {e}")

        time.sleep(4 * 60 * 60)

def monitor_breakout_loop():
    while True:
        try:
            now = datetime.utcnow() + timedelta(hours=8)
            print(f"\n⏳【顶部/底部突破监控】开始：{now.strftime('%Y-%m-%d %H:%M:%S')} (北京时间)")

            for symbol in symbols:
                try:
                    ohlcv = exchange.fetch_ohlcv(symbol, timeframe=timeframe, limit=1000)
                    df = pd.DataFrame(ohlcv, columns=['timestamp', 'open', 'high', 'low', 'close', 'volume'])
                    df['timestamp'] = pd.to_datetime(df['timestamp'], unit='ms') + timedelta(hours=8)
                    df.set_index('timestamp', inplace=True)

                    last_20 = df.iloc[-20:]
                    last_5 = last_20.iloc[-5:]
                    candle_5 = last_20.iloc[-5]
                    candle_5_close = candle_5['close']
                    candle_5_time = candle_5.name

                    up_count = (last_5['close'] > last_5['open']).sum()
                    down_count = (last_5['close'] < last_5['open']).sum()

                    previous_15 = last_20.iloc[:-5]
                    min_prev_close = previous_15['close'].min()
                    max_prev_close = previous_15['close'].max()

                    breakout_type = None
                    if up_count >= 3 and candle_5_close < min_prev_close:
                        breakout_type = '📉底部反突破'
                    elif down_count >= 3 and candle_5_close > max_prev_close:
                        breakout_type = '📈顶部反突破'

                    if breakout_type:
                        msg = (
                            f"{breakout_type} 信号：{symbol}\n"
                            f"最近5根K线内：上涨 {up_count} 根，下跌 {down_count} 根\n"
                            # f"第5根K线收盘价: {candle_5_close:.2f}\n"
                            # f"前15根收盘最低: {min_prev_close:.2f}，最高: {max_prev_close:.2f}\n"
                            f"K线时间: {candle_5_time}"
                        )
                        logging.info(msg)
                        notify_feishu(msg)

                except Exception as e:
                    logging.error(f"{symbol} 突破监控异常: {e}")

            print("【顶部/底部突破监控】本轮结束，等待4小时...\n")

        except Exception as e:
            logging.error(f"突破线程异常: {e}")

        time.sleep(4 * 60 * 60)

def start_monitoring():
    threading.Thread(target=monitor_boll_loop, daemon=True).start()
    #threading.Thread(target=monitor_breakout_loop, daemon=True).start()

if __name__ == '__main__':
    start_monitoring()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("🔚 手动中断，程序退出。")
