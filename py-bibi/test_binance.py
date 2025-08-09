import ccxt
from datetime import datetime

# 初始化 Binance 交易所对象
exchange = ccxt.binance()

# 指定交易对和时间周期
symbol = 'BTC/USDT'
timeframe = '1h'  # 支持 1m, 5m, 15m, 1h, 1d 等
limit = 100       # 获取最近 100 根K线

# 获取K线数据
ohlcv = exchange.fetch_ohlcv(symbol, timeframe=timeframe, limit=limit)

# 打印标题
print(f"{'时间':<20} {'开盘':>10} {'最高':>10} {'最低':>10} {'收盘':>10} {'成交量':>10}")
print("-" * 70)

# 打印每根K线
for candle in ohlcv:
    ts, open_, high, low, close, volume = candle
    time_str = datetime.utcfromtimestamp(ts / 1000).strftime('%Y-%m-%d %H:%M')
    print(f"{time_str:<20} {open_:>10.2f} {high:>10.2f} {low:>10.2f} {close:>10.2f} {volume:>10.4f}")
