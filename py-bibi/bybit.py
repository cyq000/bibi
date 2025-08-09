import ccxt

# 实例化 Binance 和 Bybit 的 futures 合约交易所
binance = ccxt.binance({
    'options': {'defaultType': 'future'},
})
bybit = ccxt.bybit({
    'options': {'defaultType': 'future'},
})

# 加载市场数据
binance.load_markets()
bybit.load_markets()

# 获取 USDT 合约币种集合
binance_symbols = {symbol for symbol in binance.symbols if '/USDT' in symbol}
bybit_symbols = {symbol for symbol in bybit.symbols if '/USDT' in symbol}

# 对比：Bybit 有而 Binance 没有的
only_in_bybit = bybit_symbols - binance_symbols

# 输出结果
print("📌 以下是 Bybit 有但 Binance 没有的 USDT 合约币种：\n")
for symbol in sorted(only_in_bybit):
    print(symbol)
