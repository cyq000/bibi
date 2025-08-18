
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
min_24h_volume = 100000000  # 24 小时最小成交额 100 百万 USDT
max_24h_volume = 60000000  # 24 小时最大成交额 6000 万 USDT
time_interval = 900  #900：15分钟 #3600：1小时 每小时获取一次数据
num_threads = 4  # 线程数

time_printf = 10800 # 打印过的币种在 3小时内 不再打印
tz = pytz.timezone('Asia/Shanghai') # 设置时区为上海，确保打印时间符合北京时间。

# 获取 24 小时成交额在 1亿 USDT以上的 币种
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
            if increase > 0.02:
                bool_increase = True
                # print(f"⚠️ {symbol_pair} 当前收盘价较第 {idx+1} 条前的收盘价上涨 {increase*100:.2f}%")
                break  # 一旦满足条件即可退出循环
        return bool_increase, increase
    except Exception as e:
        # print(f"获取 {symbol_pair} 的 K 线数据时发生错误: {e}")
        return bool_increase,increase


# 获取合约持仓量（Open Interest） 
    # 检查合约持仓量是否激增
    # symbol: 币种，如 'BTCUSDT'
    # period: 时间周期，'15m' 或 '1h'
    # limit: 获取的K线数量（默认5，前4+最新1根）
    # threshold: 激增阈值（默认 > 1.03 倍算激增）
def get_open_interest_increase(symbol, period="15m", limit=5, threshold=1.03):
    url = f"https://fapi.binance.com/futures/data/openInterestHist?symbol={symbol}&period={period}&limit={limit}"
    resp = exchange.fetch(url)

    if not resp or len(resp) < limit:
        return False, None, None, None

    volumes = [float(item['sumOpenInterest']) for item in resp]
    latest = volumes[-1]                                        # 最新持仓量
    avg_prev = sum(volumes[:-1]) / (limit - 1)                  # 前四次平均 limit /
    multiple = latest / avg_prev if avg_prev > 0 else 0         # 是否激增
    is_increase = multiple > threshold                          # 倍数

    return is_increase, latest, avg_prev, multiple


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
    open_interest_multiple_15m: float = None,
    open_interest_multiple_1h: float = None,
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
    if open_interest_multiple_15m is not None:
        lines.append(f"⚠️ 15分钟的持仓量大于前三次平均的**{open_interest_multiple_15m}**倍 ")
    if open_interest_multiple_1h is not None:
        lines.append(f"⚠️ 1小时的持仓量大于前三次平均的**{open_interest_multiple_1h}**倍 ")
    if buy_mult_15m is not None and sell_mult_15m is not None:  
        lines.append(f"⚠️ 15分钟的合约主动买/卖持仓量激增！买入 {buy_mult_15m:.2f} 倍，卖出 {sell_mult_15m:.2f} 倍")       
    if buy_mult_2h is not None and sell_mult_2h is not None:  
        lines.append(f"⚠️ 2小时的合约主动买/卖持仓量激增！买入 {buy_mult_2h:.2f} 倍，卖出 {sell_mult_2h:.2f} 倍")      
    return lines


# 参考文档 https://developers.binance.com/docs/zh-CN/derivatives/usds-margined-futures/market-data/rest-api/Top-Trader-Long-Short-Ratio
# 获取币种的多空比数据
def get_long_short_ratios(symbol):
    try:
        # 最新标记价格和资金费率
        premium_index = f"https://fapi.binance.com/fapi/v1/premiumIndex?symbol={symbol}"
        premium_index_data = exchange.fetch(premium_index)
        if not premium_index_data:
            return False
        premium_index_latest = premium_index_data                                           # 返回的是一个 对象
        premium_index_mark_price = float(premium_index_latest['markPrice'])                 # 标记价格
        premium_index_last_funding_rate = float(premium_index_latest['lastFundingRate'])    # 最近更新的资金费率
        
        
        # https://fapi.binance.com/futures/data/openInterestHist?symbol=GALAUSDT&period=1h&limit=4
        # https://fapi.binance.com/futures/data/topLongShortPositionRatio?symbol={symbol}&period=1h&limit=4
        # 获取最近持仓量数据，并判断是否激增（本次大于前三次平均的1.03倍）
        # 获取 15m 和 1h 的合约持仓量情况
        inc_15m, latest_15m, avg_15m, mult_15m = get_open_interest_increase(symbol, "15m")
        inc_1h, latest_1h, avg_1h, mult_1h = get_open_interest_increase(symbol, "1h")
        
        ret = False
        # if inc_15m:
        #     # print(f"⚠️ {symbol} 15m 持仓量激增 {mult_15m:.2f} 倍 (最新={latest_15m:.2f}, 平均={avg_15m:.2f})")
        #     ret = True
        # if inc_1h:
        #     # print(f"⚠️ {symbol} 1h 持仓量激增 {mult_1h:.2f} 倍 (最新={latest_1h:.2f}, 平均={avg_1h:.2f})")
        #     ret = True
        
        # 检查主动买入/卖出量是否激增
        period_time_15m = '15m'
        period_time_2h = '2h'
        multiple_2h = 10
        multiple_15m = 15
        limit = 4
        buy_spike_2h, sell_spike_2h, buy_mult_2h, sell_mult_2h, buy_sell_ratio_2h = check_aggressive_volume_spike(symbol, period_time_2h,multiple_2h, limit)
        buy_spike_15m, sell_spike_15m, buy_mult_15m, sell_mult_15m, buy_sell_ratio_15m = check_aggressive_volume_spike(symbol, period_time_15m,multiple_15m, limit)

        # 检查当前 K 线收盘价是否较前 4 条 K 线中任意一条上涨超过 7%
        bool_increase, increase = check_price_spike(symbol,"PERPETUAL", "15m", 5)


        title = f"{symbol} 数据异常预警"
        
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

        
        # 最近持仓量数据，并判断是否激增
        if inc_15m or inc_1h:
            ret = True
  
        # 检查主动买入/卖出量是否激增
        if buy_spike_2h or sell_spike_2h :
            ret = True
            
        if buy_spike_15m or sell_spike_15m : 
            ret = True
        
        # 检查当前 K 线收盘价是否较前 4 条 K 线中任意一条上涨超过 3%    
        if bool_increase :
            quoteVolume = get_24h_quote_volume(symbol)
            quoteVolume = quoteVolume / 10000 
            # ret = True
        
        if ret:
            lines = build_symbol_metrics_lines(
                symbol = symbol,
                quoteVolume = quoteVolume,
                premium_index_mark_price = premium_index_mark_price,
                premium_index_last_funding_rate = premium_index_last_funding_rate,
                increase = increase,
                open_interest_multiple_15m = mult_15m if inc_15m is not None else None,
                open_interest_multiple_1h = mult_1h if inc_1h is not None else None,    
                buy_mult_2h = buy_mult_2h,
                sell_mult_2h = sell_mult_2h,
                buy_mult_15m = buy_mult_15m,
                sell_mult_15m = sell_mult_15m,
                )
            content = build_feishu_post_message(title, lines)
            send_feishu_message(
                webhook_url=webhook2,
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
    logging.info(f"符合条件的高流动性币种（100百万 美元 < 24 小时成交额 ）：{len(symbols)} 个")
    
    print_title = f"<1号监控机>"
    print_time = time_interval/3600
    print_min_24h_volume = min_24h_volume/1000000
    print_info = [
        f"📌 **<1号监控机>**",
        f"📌 **监控范围:**  币安所有合约币种",
        f"📌 **监控条件:**",
        f"📌 1.币种24小时成交额 >: **{print_min_24h_volume}.百万** USDT,{len(symbols)} 个",
        f"📌 2.每 **{print_time}** 小时循环获取数据.",
        f"📌 4.符合条件打印过的币种,3小时内不再打印,避免刷屏."
    ]
    content = build_feishu_post_message(print_title, print_info)
    send_feishu_message(
        webhook_url=webhook2,
        msg_type="post",
        content = content
        )
    logging.info(content)
        
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
        
    # 主线程循环等待中断
    while not stop_event.is_set():
        time.sleep(1)


if __name__ == "__main__":
    main()
