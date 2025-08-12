
# 山寨币
# 3百万USDT-2千万USDT
# 30分钟、1、2、4、小时
# 价格、合约持仓量、合约主动买卖量、资金费率、多空比、

# 15分钟. 20倍
# 1小时.  20倍
#   2小时.  10倍

# 2.打印价格、


import ccxt
import time
import pytz
import logging
from datetime import datetime, timedelta

import pandas as pd                 # 计算rsi
import threading
from collections import defaultdict


from feishu_bot import send_feishu_message
from feishu_bot import build_feishu_post_message  # 根据文件结构替换
import sys
# 设置编码为utf-8
sys.stdout.reconfigure(encoding='utf-8')

# 设置日志配置
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("output.log", encoding='utf-8'),
        logging.StreamHandler(sys.stdout)
    ]
)


# 飞书机器人
# 检查当前 K 线收盘价是否较前 4 条 K 线中任意一条上涨超过 7%  
webhook = 'https://open.feishu.cn/open-apis/bot/v2/hook/6f399df9-a303-42a6-84cd-ea23d3d7cadd'

# 飞书机器人2
# 所有持仓用户、大户持仓、多空比 大于 60%,大户持仓量多空比 大于 70% 
webhook2 = 'https://open.feishu.cn/open-apis/bot/v2/hook/b7815671-5ba7-45d8-9535-52707d6689a3'



import signal
stop_event = threading.Event()  # 控制线程退出

# 设置代理（如果您需要使用代理的话）
exchange = ccxt.binance({
    'apiKey': 'v1He9r2cZnGZskc2gHDvriXMrLqy17GoUb2ZhtgKzKlB44QzdeSYe4UDtQN6pxXe', 
    'secret': 'vLCdveVVGkJ4iVTgBTxr6Oqxx9fLsekxEI6IVAWXRi8jDf3u7LRJkbF3gsUJ4918',
    # 'proxies': {
    #   'http': '127.0.0.1:7890',
    #   'https': '127.0.0.1:7890',
    # },
    'timeout': 10000,
    'enableRateLimit': True,
    'options': {'defaultType': 'future'}
})
 
exchange.load_markets()

# 参数设置
min_24h_volume = 3000000  # 24 小时最小成交额 300 万 USDT
max_24h_volume = 60000000  # 24 小时最大成交额 6000 万 USDT
time_interval = 900  #900：15分钟 #3600：1小时 每小时获取一次数据
num_threads = 4  # 线程数

time_printf = 10800 # 打印过的币种在 3小时内 不再打印
tz = pytz.timezone('Asia/Shanghai') # 设置时区为上海，确保打印时间符合北京时间。

# 获取 24 小时成交额在 100万 USDT 至 6000 万 USDT 之间的币种
def get_symbols():
    symbols = set()
    
    for symbol in exchange.symbols:
        if '/USDT' not in symbol:  # 只关心USDT交易对
            continue

        try:
            ticker = exchange.fetch_ticker(symbol)
            # if min_24h_volume <= ticker['quoteVolume'] <= max_24h_volume:
            if min_24h_volume <= ticker['quoteVolume'] :
                base_currency, quote_currency = symbol.split('/')  # 提取币种的基础货币和报价货币
                #new_field = base_currency + quote_currency  # 创建新的字段 GLMUSDT
                new_field = base_currency + 'USDT'  # 创建新的字段 GLMUSDT
                symbols.add(new_field)
        except Exception:
            pass  # 忽略错误
            
    return symbols


# 将资金费率（如 0.00038246）格式化为百分比字符串，如 '0.0382%'
def format_funding_rate(rate: float, digits: int = 4) -> str:
    return f"{rate * 100:.{digits}f}%"


# 使用标准方法计算币种的1小时 RSI 值（默认14周期），数据从Binance获取。
def get_rsi_1h(symbol: str, period: int = 16) -> float:

    # 获取最近100条K线数据
    url = f"https://fapi.binance.com/fapi/v1/klines?symbol={symbol}&interval=1h&limit=100"
    kline_data = exchange.fetch(url)

    if not kline_data or len(kline_data) < period + 1:
        print(f"{symbol} 的K线数据不足，无法计算 RSI")
        return None

    # 提取收盘价，并按时间从旧到新排列
    close_prices = [float(item[4]) for item in kline_data]

    # 计算每个周期的涨跌
    deltas = [close_prices[i] - close_prices[i - 1] for i in range(1, len(close_prices))]

    # 初始14周期的 gain / loss
    gains = [max(delta, 0) for delta in deltas[:period]]
    losses = [abs(min(delta, 0)) for delta in deltas[:period]]

    avg_gain = sum(gains) / period
    avg_loss = sum(losses) / period

    # 继续平滑后续的gain/loss
    for delta in deltas[period:]:
        gain = max(delta, 0)
        loss = abs(min(delta, 0))
        avg_gain = (avg_gain * (period - 1) + gain) / period
        avg_loss = (avg_loss * (period - 1) + loss) / period

    # 防止除零
    if avg_loss == 0:
        return 100.0

    rs = avg_gain / avg_loss
    rsi = 100 - (100 / (1 + rs))

    return round(rsi, 2)


# 检查主动买入/卖出量是否激增
def check_aggressive_volume_spike(symbol, period, multiple, limit):
    # period = "15m" 
    taker_vol_url = f"https://fapi.binance.com/futures/data/takerlongshortRatio?symbol={symbol}&period={period}&limit={limit}"
    taker_vol_data = exchange.fetch(taker_vol_url)

    if not taker_vol_data or len(taker_vol_data) < limit:
        return None, None, None, None,None

    # 提取主动买入量和主动卖出量
    buy_vols = [float(item['buyVol']) for item in taker_vol_data]
    sell_vols = [float(item['sellVol']) for item in taker_vol_data]
    buy_sell_ratio_2 = [float(item['buySellRatio']) for item in taker_vol_data]

    # 最新值
    buy_latest = buy_vols[-1]
    sell_latest = sell_vols[-1]
    ratio_latest = buy_sell_ratio_2[-1]

    # 前 limit-1 次平均
    buy_avg_prev = sum(buy_vols[:-1]) / (limit - 1)
    sell_avg_prev = sum(sell_vols[:-1]) / (limit - 1)

    # 是否激增（15倍）
    # multiple = 15
    bool_buy_spike_2 = buy_latest > buy_avg_prev * multiple
    bool_sell_spike_2 = sell_latest > sell_avg_prev * multiple

    # 倍数
    buy_mult_2iple = buy_latest / buy_avg_prev if buy_avg_prev > 0 else float('inf')
    sell_mult_2iple = sell_latest / sell_avg_prev if sell_avg_prev > 0 else float('inf')
    
    # if buy_spike_2 or sell_spike_2:
    # print(f"⚠️ {symbol} 主动买/卖激增！买入 {buy_mult_2iple:.2f} 倍，卖出 {sell_mult_2iple:.2f} 倍,买入卖出比 {ratio_latest:.2f}")
            

    return bool_buy_spike_2, bool_sell_spike_2, buy_mult_2iple, sell_mult_2iple, ratio_latest

# 获取币种当前24小时内的成交额
def get_24h_quote_volume(symbol):
    ticker = exchange.fetch_ticker(symbol)
    # print(f"BTCUSDT 24小时成交额: {ticker['quoteVolume']} USDT")      
    return ticker['quoteVolume']
    
    
 
# 检查当前 K 线收盘价是否较前 4 条 K 线中任意一条上涨超过 7%
    # 参数:
    #     symbol_pair (str): 交易对，例如 "BTCUSDT"
    #     contract_type (str): 合约类型，"PERPETUAL"、"CURRENT_QUARTER" 或 "NEXT_QUARTER"
    #     interval (str): K 线时间间隔，例如 "15m"
    #     limit (int): 获取的 K 线数量，默认 5 条（当前 K 线 + 前 4 条）
def check_price_spike(symbol_pair, contract_type, interval, limit):

    url = f"https://fapi.binance.com/fapi/v1/continuousKlines?pair={symbol_pair}&contractType={contract_type}&interval={interval}&limit={limit}"
    
    
    bool_increase = False
    increase = 0
        
    try:
        kline_data = exchange.fetch(url)
        if not kline_data or len(kline_data) < limit:
            print(f"{symbol_pair} 获取 K 线数据不足 {limit} 条，跳过")
            return bool_increase,increase

        # 提取收盘价
        close_prices = [float(k[4]) for k in kline_data]
        latest_close = close_prices[-1]
        previous_closes = close_prices[:-1]

        
        # 判断是否上涨超过 7%
        for idx, prev_close in enumerate(previous_closes):
            if prev_close == 0:
                continue  # 避免除以零
            increase = (latest_close - prev_close) / prev_close
            if increase > 0.07:
                bool_increase = True
                # print(f"⚠️ {symbol_pair} 当前收盘价较第 {idx+1} 条前的收盘价上涨 {increase*100:.2f}%")
                break  # 一旦满足条件即可退出循环
        return bool_increase, increase
    except Exception as e:
        # print(f"获取 {symbol_pair} 的 K 线数据时发生错误: {e}")
        return bool_increase,increase

# 封装输出到飞书的打印内容
def build_symbol_metrics_lines(
    symbol: str = "",
    quoteVolume: float = None,
    premium_index_mark_price: float = None,
    premium_index_last_funding_rate: float = None,
    global_long_account: float = None,
    top_position_long_account: float = None,
    top_account_long_account: float = None,
    increase: float = None,
    increase_text: str = "5分钟K线当前收盘价较前4条收盘价最高值涨幅",
    open_interest_multiple: float = None,
    buy_mult_15m: float = None,
    sell_mult_15m: float = None,
    buy_mult_2h: float = None,
    sell_mult_2h: float = None,
) -> list[str]:
    lines = []
    

    lines.append(f"📌 当前时间: {datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')}")
    if symbol:
        lines.append(f"📌 币种: **{symbol}**")
    if quoteVolume:
        lines.append(f"📌 24小时成交额: **{quoteVolume}** 万USDT")
    if premium_index_mark_price is not None:
        lines.append(f"💰 标记价格: **{premium_index_mark_price}**")
    if premium_index_last_funding_rate is not None:
        lines.append(f"💰 资金费率: **{premium_index_last_funding_rate}**")
    # if global_long_account is not None:
    #     lines.append(f"👥 所有用户多空人数比: **{global_long_account}**")
    # if top_position_long_account is not None:
    #     lines.append(f"📊 持仓大户持仓量多空比: **{top_position_long_account}**")
    # if top_account_long_account is not None:
    #     lines.append(f"💰 持仓大户多空人数比: **{top_account_long_account}**")
    # if increase is not None:
    #     lines.append(f"📈 当前价格：**{increase * 100:.2f}%**")
        # lines.append(f"📈 {increase_text}: **{increase * 100:.2f}%**")
    if open_interest_multiple is not None:
        lines.append(f"⚠️ 15分钟的持仓量大于前三次平均的**{open_interest_multiple}**倍 ")
    if buy_mult_15m is not None and sell_mult_15m is not None:  
        lines.append(f"⚠️ 15分钟的合约主动买/卖持仓量激增！买入 {buy_mult_15m:.2f} 倍，卖出 {sell_mult_15m:.2f} 倍")       
    if buy_mult_2h is not None and sell_mult_2h is not None:  
        lines.append(f"⚠️ 2小时的合约主动买/卖持仓量激增！买入 {buy_mult_2h:.2f} 倍，卖出 {sell_mult_2h:.2f} 倍")      
    return lines


# 参考文档 https://developers.binance.com/docs/zh-CN/derivatives/usds-margined-futures/market-data/rest-api/Top-Trader-Long-Short-Ratio

# L.S Ratio - 多空人数持仓比
# 所有持仓用户的净持仓多头和空头账户数占比，一个账户记一次。
# GET /futures/data/globalLongShortAccountRatio

# L.S Posit. - 持仓量多空比
# 大户的多头和空头总持仓量占比，大户指保证金余额排名前 20% 的用户
# GET /futures/data/topLongShortPositionRatio

# L.S Acco. - 账户数多空比
# 持仓大户的净持仓多头和空头账户数占比，大户指保证金余额排名前 20% 的用户。
# GET /futures/data/topLongShortAccountRatio


# 获取币种的多空比数据
def get_long_short_ratios(symbol):
    try:
        period_time = '2h'
        # # 获取多空持仓人数比（L.S Ratio）（所有持仓者）
        # # https://fapi.binance.com/futures/data/globalLongShortAccountRatio?symbol=BTCUSDT&period=1h&limit=1
        # global_account_ratio_endpoint = f"https://fapi.binance.com/futures/data/globalLongShortAccountRatio?symbol={symbol}&period={period_time}&limit=1"
        # global_account_ratio_data = exchange.fetch(global_account_ratio_endpoint)
        # if not global_account_ratio_data:
        #     return False
        # global_latest_account_ratio = global_account_ratio_data[-1]
        # global_long_short_ratio = float(global_latest_account_ratio['longShortRatio'])
        # global_long_account = float(global_latest_account_ratio['longAccount'])
        # global_short_account = float(global_latest_account_ratio['shortAccount'])
        # global_timestamp = int(global_latest_account_ratio['timestamp'])
        # global_timestamp = timestamp_to_beijing_time(global_timestamp)  # 北京时间

        # # 获取账户数多空比（L.S Acco） （持仓大户）
        # # https://fapi.binance.com/futures/data/topLongShortAccountRatio?symbol=BTCUSDT&period=1h&limit=1
        # top_account_ratio_endpoint = f"https://fapi.binance.com/futures/data/topLongShortAccountRatio?symbol={symbol}&period={period_time}&limit=1"
        # top_account_ratio_data = exchange.fetch(top_account_ratio_endpoint)
        # if not top_account_ratio_data:
        #     return False
        # top_account_latest_account_ratio = top_account_ratio_data[-1]       # 返回的是一个 列表 → 正确用 [-1] 取最后一个
        # top_account_long_short_ratio = float(top_account_latest_account_ratio['longShortRatio'])
        # top_account_long_account = float(top_account_latest_account_ratio['longAccount'])
        # top_account_short_account = float(top_account_latest_account_ratio['shortAccount'])
        # top_account_timestamp = float(top_account_latest_account_ratio['timestamp'])

        # # 获取持仓量多空比（L.S Posit.）（持仓大户）
        # top_position_ratio_endpoint = f"https://fapi.binance.com/futures/data/topLongShortPositionRatio?symbol={symbol}&period={period_time}&limit=1"
        # top_position_ratio_data = exchange.fetch(top_position_ratio_endpoint)
        # if not top_position_ratio_data:
        #     return False
        # top_position_latest_account_ratio = top_position_ratio_data[-1]
        # top_position_long_short_ratio = float(top_position_latest_account_ratio['longShortRatio'])
        # top_position_long_account = float(top_position_latest_account_ratio['longAccount'])
        # top_position_short_account = float(top_position_latest_account_ratio['shortAccount'])
        # top_position_timestamp = float(top_position_latest_account_ratio['timestamp'])


        # logging.info(f"1 symbol:{symbol}")     

        # 最新标记价格和资金费率
        premium_index = f"https://fapi.binance.com/fapi/v1/premiumIndex?symbol={symbol}"
        premium_index_data = exchange.fetch(premium_index)
        if not premium_index_data:
            return False
        premium_index_latest = premium_index_data                                           # 返回的是一个 对象
        premium_index_mark_price = float(premium_index_latest['markPrice'])                 # 标记价格
        premium_index_last_funding_rate = float(premium_index_latest['lastFundingRate'])    # 最近更新的资金费率
        premium_index_last_funding_rate = format_funding_rate(premium_index_last_funding_rate)
        
        
        # https://fapi.binance.com/futures/data/openInterestHist?symbol=GALAUSDT&period=1h&limit=4
        # https://fapi.binance.com/futures/data/topLongShortPositionRatio?symbol={symbol}&period=1h&limit=4
        # 获取最近持仓量数据，并判断是否激增（本次大于前三次平均的1.2倍）
        open_interest_url = f"https://fapi.binance.com/futures/data/openInterestHist?symbol={symbol}&period={period_time}&limit=5"
        open_interest_data = exchange.fetch(open_interest_url)
        if not open_interest_data or len(open_interest_data) < 5:
            return False
        # 解析持仓量（sumOpenInterest）
        open_interest_volumes = [float(item['sumOpenInterest']) for item in open_interest_data]
        open_interest_latest = open_interest_volumes[-1]                  # 最新持仓量
        open_interest_avg_prev4 = sum(open_interest_volumes[:-1]) / 4     # 前四次平均
        open_interest_increase = open_interest_latest > open_interest_avg_prev4 * 1.4  # 是否激增
        open_interest_multiple = open_interest_latest / open_interest_avg_prev4 # 倍数
        
        
        # 获取RSI值
        # rsi_value = get_rsi_1h(symbol)
        # print(f"币种：{symbol}, 当前 RSI 值: {rsi_value}")
        
        # 检查主动买入/卖出量是否激增
        period_time_15m = '15m'
        period_time_2h = '2h'
        multiple_2h = 10
        multiple_15m = 15
        limit = 4
        buy_spike_2h, sell_spike_2h, buy_mult_2h, sell_mult_2h, buy_sell_ratio_2h = check_aggressive_volume_spike(symbol, period_time_2h,multiple_2h, limit)
        # if buy_spike_2 or sell_spike_2:
        #     print(f"⚠️ {symbol} 主动买/卖激增！买入 {buy_mult_2:.2f} 倍，卖出 {sell_mult_2:.2f} 倍,买入卖出比 {buy_sell_ratio_2:.2f}")
        buy_spike_15m, sell_spike_15m, buy_mult_15m, sell_mult_15m, buy_sell_ratio_15m = check_aggressive_volume_spike(symbol, period_time_15m,multiple_15m, limit)


        # 检查当前 K 线收盘价是否较前 4 条 K 线中任意一条上涨超过 7%
        bool_increase, increase = check_price_spike(symbol,"PERPETUAL", "15m", 5)


        title = f"{symbol} 数据异常预警"
        # lines = [
        #     f"📌 币种: **{symbol}**",
        #     f"💰 标记价格: **{premium_index_mark_price}**",
        #     f"💰 资金费率: **{premium_index_last_funding_rate}**",
        #     f"👥 所有用户多空人数比: **{global_long_account}**",
        #     f"📊 持仓大户持仓量多空比: **{top_position_long_account}**",
        #     f"💰 持仓大户多空人数比: **{top_account_long_account}**",
        #     f"⚠️ 所有持仓用户、大户持仓、大户持仓量、三个指标 多单 都大于 60%  如果价格已经上涨并遇到阻力,可能要跌 "
        # ]
    
        # lines2 = [
        #     f"📌 币种: **{symbol}**",
        #     f"💰 标记价格: **{premium_index_mark_price}**",
        #     f"💰 资金费率: **{premium_index_last_funding_rate}**",
        #     f"👥 所有用户多空人数比: **{global_long_account}**",
        #     f"📊 持仓大户持仓量多空比: **{top_position_long_account}**",
        #     f"💰 持仓大户多空人数比: **{top_account_long_account}**",
        #     f"⚠️ 所有持仓用户、大户持仓、多空比 大于 65%,大户持仓量多空比 小于 50% "
        # ]
    
        # lines3 = [
        #     f"📌 币种: **{symbol}**",
        #     f"💰 标记价格: **{premium_index_mark_price}**",
        #     f"💰 资金费率: **{premium_index_last_funding_rate}**",
        #     f"👥 所有用户多空人数比: **{global_long_account}**",
        #     f"📊 持仓大户持仓量多空比: **{top_position_long_account}**",
        #     f"💰 持仓大户多空人数比: **{top_account_long_account}**",
        #     f"⚠️ 所有持仓用户、大户持仓、多空比 小于 45%,大户持仓量多空比 小于 55% "
        # ]
        # lines4 = [
        #     f"📌 币种: **{symbol}**",
        #     f"💰 标记价格: **{premium_index_mark_price}**",
        #     f"💰 资金费率: **{premium_index_last_funding_rate}**",
        #     f"👥 所有用户多空人数比: **{global_long_account}**",
        #     f"📊 持仓大户持仓量多空比: **{top_position_long_account}**",
        #     f"💰 持仓大户多空人数比: **{top_account_long_account}**",
        #     f"⚠️ 所有持仓用户、大户持仓、多空比 大于 60%,大户持仓量多空比 大于 70% 如果价格长时间在低点,可能要涨.在高位则要跌 ",
        #     f"💰 RSI: **{rsi_value}**"
        # ]


        # lines5 = [
        #     f"📌 币种: **{symbol}**",
        #     f"💰 标记价格: **{premium_index_mark_price}**",
        #     f"💰 资金费率: **{premium_index_last_funding_rate}**",
        #     f"👥 所有用户多空人数比: **{global_long_account}**",
        #     f"📊 持仓大户持仓量多空比: **{top_position_long_account}**",
        #     f"💰 持仓大户多空人数比: **{top_account_long_account}**",
        #     f"💰 本次合约持仓量: **{open_interest_multiple}**",
        #     f"💰 前三次平均持仓量: **{open_interest_avg_prev3}**",
        #     f"⚠️ 获取最近持仓量数据,本次持仓量大于前三次平均的**{open_interest_multiple}** 倍 ",
        #     f"💰 RSI: **{rsi_value}**"
        # ]
        
        # lines6 = [
        #     f"📌 币种: **{symbol}**",
        #     f"💰 标记价格: **{premium_index_mark_price}**",
        #     f"💰 资金费率: **{premium_index_last_funding_rate}**",
        #     f"👥 所有用户多空人数比: **{global_long_account}**",
        #     f"📊 持仓大户持仓量多空比: **{top_position_long_account}**",
        #     f"💰 持仓大户多空人数比: **{top_account_long_account}**",
        #     f"💰 本次合约持仓量: **{open_interest_multiple}**",
        #     f"💰 前三次平均持仓量: **{open_interest_avg_prev3}**",
        #     f"⚠️ 获取最近持仓量数据,本次持仓量大于前三次平均的**{open_interest_multiple}**倍 ",
        #     f"💰 RSI: **{rsi_value}**"
        # ]
        
        # # 检查主动买入/卖出量是否激增
        # lines7 = [
        #     f"📌 币种: **{symbol}**",
        #     f"💰 标记价格: **{premium_index_mark_price}**",
        #     f"💰 资金费率: **{premium_index_last_funding_rate}**",
        #     f"👥 所有用户多空人数比: **{global_long_account}**",
        #     f"📊 持仓大户持仓量多空比: **{top_position_long_account}**",
        #     f"💰 持仓大户多空人数比: **{top_account_long_account}**",
        #     # f"💰 本次合约持仓量: **{open_interest_multiple}**",
        #     # f"💰 前三次平均持仓量: **{open_interest_avg_prev3}**",
        #     f"⚠️ 获取最近持仓量数据,本次持仓量大于前三次平均的**{open_interest_multiple}**倍 ",
        #     f"💰 RSI: **{rsi_value}**",
        #     f"⚠️ 15分钟的的k线.合约持仓量.主动买/卖激增！买入 {buy_mult_2:.2f} 倍，卖出 {sell_mult_2:.2f} 倍,买入卖出比 {buy_sell_ratio_2:.2f}"
        # ]
        
        # lines8 = [
        #     f"📌 币种: **{symbol}**",
        #     f"💰 标记价格: **{premium_index_mark_price}**",
        #     f"💰 资金费率: **{premium_index_last_funding_rate}**",
        #     f"👥 所有用户多空人数比: **{global_long_account}**",
        #     f"📊 持仓大户持仓量多空比: **{top_position_long_account}**",
        #     f"💰 持仓大户多空人数比: **{top_account_long_account}**",
        #     f"💰 15分钟的的k线.当前收盘价较前4条k线收盘价最高价涨幅: **{increase*100:.2f}**%"
        # ]

        ret = False
        # # 所有持仓用户、大户持仓、多空比 大于 60%,大户持仓量多空比 大于 70% 
        # if global_long_account > 0.60 and top_account_long_account > 0.60 and top_position_long_account > 0.70   :
        #     content = build_feishu_post_message(title, lines4)
        #     send_feishu_message(
        #         webhook_url=webhook2,
        #         msg_type="post",
        #         content = content
        #         )
        #     logging.info(content)# print(f"{symbol} 符合条件3: prinf_info:{prinf_info3}")
        #     ret = True

        # 最近持仓量数据，并判断是否激增
        if open_interest_increase  :
            # content = build_feishu_post_message(title, lines5)
            # send_feishu_message(
            #     webhook_url=webhook,
            #     msg_type="post",
            #     content = content
            #     )
            ret = True
            # print(f"{symbol} 符合条件3: prinf_info:{prinf_info3}")   
             
        # 获取RSI值
        # if (rsi_value >= 70.0 and rsi_value < 100 ) or rsi_value <= 26.0  :
        #     content = build_feishu_post_message(title, lines6)
        #     send_feishu_message(
        #         webhook_url=webhook,
        #         msg_type="post",
        #         content = content
        #         )
            # print(f"{symbol} 符合条件3: prinf_info:{prinf_info3}")
        
        # 检查主动买入/卖出量是否激增
        if buy_spike_2h or sell_spike_2h :
            # content = build_feishu_post_message(title, lines7)
            # send_feishu_message(
            #     webhook_url=webhook,
            #     msg_type="post",
            #     content = content
            #     )
            # logging.info(content) # print(f"{symbol} 符合条件3: prinf_info:{prinf_info3}")    
            ret = True
        if buy_spike_15m or sell_spike_15m :
            # content = build_feishu_post_message(title, lines7)
            # send_feishu_message(
            #     webhook_url=webhook,
            #     msg_type="post",
            #     content = content
            #     )
            # logging.info(content) # print(f"{symbol} 符合条件3: prinf_info:{prinf_info3}")    
            ret = True
        
        # 检查当前 K 线收盘价是否较前 4 条 K 线中任意一条上涨超过 9%    
        if bool_increase :
            quoteVolume = get_24h_quote_volume(symbol)
            quoteVolume = quoteVolume / 10000 
            ret = True
        
        if ret:
            lines = build_symbol_metrics_lines(
                symbol = symbol,
                quoteVolume = quoteVolume,
                premium_index_mark_price = premium_index_mark_price,
                premium_index_last_funding_rate = premium_index_last_funding_rate,
                increase = increase,
                open_interest_multiple = open_interest_multiple,
                buy_mult_2h = buy_mult_2h,
                sell_mult_2h = sell_mult_2h,
                buy_mult_15m = buy_mult_15m,
                sell_mult_15m = sell_mult_15m,
                )
            content = build_feishu_post_message(title, lines)
            send_feishu_message(
                webhook_url=webhook,
                msg_type="post",
                content = content
            )
            logging.info(content) # print(f"{symbol} 符合条件3: prinf_info:{prinf_info3}")        
        
        if ret:
            print(f"symbol:{symbol},ret:{ret}")
            return True
        
        return False

    except Exception as e:
        print(f"获取 {symbol} 的多空比数据时出错: {e}")
        return False

# 将毫秒级时间戳转换为秒级时间戳
def timestamp_to_beijing_time(timestamp):
    timestamp_seconds = timestamp / 1000
    # 将时间戳转换为UTC时间的datetime对象
    utc_time = datetime.utcfromtimestamp(timestamp_seconds)
    # 北京时间比UTC时间提前8小时
    beijing_time = utc_time + timedelta(hours=8)
    return beijing_time.strftime('%Y-%m-%d %H:%M:%S')


# 按比例分配币种到多个数组
def divide_symbols(symbols, num_parts):
    symbols = list(symbols)
    chunk_size = len(symbols) // num_parts
    return [
        symbols[i * chunk_size: (i + 1) * chunk_size] if i < num_parts - 1 else symbols[i * chunk_size:]
        for i in range(num_parts)
    ]

# 每个线程运行的函数
# symbols_subset: 这个线程负责的币种子集（比如总共200个币种，每个线程处理50个）。
#  printed_symbols: 一个字典，用于记录这个线程中每个币种上一次被打印的小时，避免重复打印。
def worker(symbols_subset, printed_symbols):
    tz = pytz.timezone('Asia/Shanghai') # 设置时区为上海，确保打印时间符合北京时间。
    while True:
        print(f"\n线程 {threading.current_thread().name} 开始查询，当前时间: {datetime.now(tz).strftime('%Y-%m-%d %H:%M:%S')}")
        # current_hour = datetime.now(tz).hour
        current_time = time.time()
        for symbol in symbols_subset:                   # 遍历这个线程负责的所有币种。
            last_printed = printed_symbols.get(symbol)
            if last_printed is not None and (current_time - last_printed) < time_printf:      # 1800 30分钟 # 3小时 = 10800秒
                continue  # 已打印过则跳过
            if get_long_short_ratios(symbol):           # 判断是否符合打印条件
                printed_symbols[symbol] = current_time  # 如果返回 True，则说明这个币种需要打印，随后将当前时间记录下来，以便后续跳过
        # time.sleep(time_interval)
        stop_event.wait(time_interval)  # 替代 time.sleep，可被唤醒中断

# 捕获 Ctrl+C 信号
def signal_handler(sig, frame):
    print("\n[退出] 收到 Ctrl+C 信号，正在停止所有线程...")
    stop_event.set()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)


# 主函数
def main():
    # 获取符合条件的币种
    symbols = get_symbols()
    # print(f"符合条件的高流动性币种（200 万美元 < 24 小时成交额 < 8000 万美元）：{len(symbols)} 个")
    logging.info(f"符合条件的高流动性币种（300 万美元 < 24 小时成交额 < 6000 万美元）：{len(symbols)} 个")
    
    print_title = f"<1号监控机>"
    print_time = time_interval/3600
    print_min_24h_volume = min_24h_volume/10000
    print_info = [
        f"📌 **<1号监控机>**",
        f"📌 **监控范围:**  币安所有合约币种",
        f"📌 **监控条件:**",
        f"📌 1.币种24小时成交额 >: **{print_min_24h_volume}.万** USDT,{len(symbols)} 个",
        f"📌 2.每 **{print_time}** 小时循环获取数据.",
        f"📌 3.检查 **15分钟级别** K线收盘价是否较前 4 条 K 线中任意一条上涨超过 **7%**.",
        f"📌 4.符合条件打印过的币种,3小时内不再打印,避免刷屏."
    ]
    content = build_feishu_post_message(print_title, print_info)
    send_feishu_message(
        webhook_url=webhook,
        msg_type="post",
        content = content
        )
    logging.info(content) # print(f"{symbol} 符合条件3: prinf_info:{prinf_info3}")    
        
    divided_symbols = divide_symbols(symbols, num_threads)
    printed_symbols_list = [defaultdict(float) for _ in range(num_threads)]
    
    threads = []
    for i in range(num_threads):
        t = threading.Thread(
            target=worker,
            args=(divided_symbols[i], printed_symbols_list[i]),
            name=f"Worker-{i+1}"
        )
        t.daemon = True  # 后台线程
        t.start()
        threads.append(t)

    # for t in threads:
    #     t.join()
        
        
    # 主线程循环等待中断
    while not stop_event.is_set():
        time.sleep(1)


if __name__ == "__main__":
    main()
