import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

from data_repo import DataRepository
from models import Bar, Trade, Side
from analytics import CostFunction

# 尝试导入talib，没有则使用简单的pandas计算
try:
    import talib
except ImportError:
    talib = None

def calc_rsi(series, period):
    if talib:
        return talib.RSI(series.values, timeperiod=period)
    else:
        delta = series.diff()
        gain = (delta.where(delta > 0, 0)).rolling(window=period).mean()
        loss = (-delta.where(delta < 0, 0)).rolling(window=period).mean()
        rs = gain / loss
        return 100 - (100 / (1 + rs))

class Strategy:
    def __init__(self, symbol, params):
        self.symbol = symbol
        self.params = params
        self.repo = DataRepository() # 获取单例
        self.trades = []
        self.cost_function = CostFunction()
        self.active_trades = []

    def on_bar(self, bar: Bar):
        """每根K线触发一次逻辑"""
        pass

    def run_simulation(self, start_time, end_time):
        print(f"Starting Backtest for {self.symbol}...")
        df = self.repo.data[self.symbol]['1min']
        
        # 截取时间段
        mask = (df.index >= start_time) & (df.index <= end_time)
        sim_data = df.loc[mask]
        
        # 遍历回测 (模拟逐根K线推进)
        # 这是一个简化的Event Loop。为了速度，指标计算通常向量化，但为了模拟实盘逻辑，我们循环。
        
        # 预计算指标以加速循环 (混合模式)
        full_history = self.repo.data[self.symbol]['1min'] # 获取全量数据计算指标
        
        # 这里需要子类实现具体的指标计算
        indicators = self.calculate_indicators(full_history)
        
        for time, row in sim_data.iterrows():
            # 构建当前时刻的Bar对象
            current_bar = Bar(
                symbol=self.symbol, time=time,
                open=row['open'], high=row['high'], low=row['low'], close=row['close'],
                volume=row['volume'], period='1min'
            )
            
            # 获取当前时刻的指标值
            current_indicators = {k: v.loc[time] if time in v.index else None for k, v in indicators.items()}
            
            self.on_bar(current_bar, current_indicators)
        
        # 回测结束，强制平仓所有单子以便统计
        last_price = sim_data.iloc[-1]['close']
        last_time = sim_data.index[-1]
        for t in self.active_trades:
            t.exit_price = last_price
            t.exit_time = last_time
            t.status = "CLOSED"
        
        # 计算统计
        self.cost_function.calc_cost_function(self.trades)
        print("Backtest Finished.")

    def calculate_indicators(self, df):
        return {}

    def open_position(self, side, price, time, sl_pips=0, tp_pips=0):
        # 简单的Pip转换，假设是EURUSD类型 (1 pip = 0.0001)
        pip_val = 0.0001 
        sl = price - (sl_pips * pip_val) if side == Side.LONG else price + (sl_pips * pip_val)
        tp = price + (tp_pips * pip_val) if side == Side.LONG else price - (tp_pips * pip_val)
        
        trade = Trade(self.symbol, side, time, price, size=10000, sl=sl, tp=tp)
        self.active_trades.append(trade)
        self.trades.append(trade) # 记录到总历史

    def check_exit(self, bar):
        # 检查止盈止损
        for t in self.active_trades[:]:
            if t.status == "CLOSED": continue
            
            # 模拟撮合：检查High和Low是否触及SL/TP
            if t.side == Side.LONG:
                if bar.low <= t.sl and t.sl > 0:
                    t.exit_price = t.sl
                    t.exit_time = bar.time
                    t.status = "CLOSED"
                    self.active_trades.remove(t)
                elif bar.high >= t.tp and t.tp > 0:
                    t.exit_price = t.tp
                    t.exit_time = bar.time
                    t.status = "CLOSED"
                    self.active_trades.remove(t)
            else: # SHORT
                if bar.high >= t.sl and t.sl > 0:
                    t.exit_price = t.sl
                    t.exit_time = bar.time
                    t.status = "CLOSED"
                    self.active_trades.remove(t)
                elif bar.low <= t.tp and t.tp > 0:
                    t.exit_price = t.tp
                    t.exit_time = bar.time
                    t.status = "CLOSED"
                    self.active_trades.remove(t)


class RSIReversionStrategy(Strategy):
    def calculate_indicators(self, df):
        # 计算RSI
        rsi_series = calc_rsi(df['close'], self.params['rsi_period'])
        return {'rsi': rsi_series}

    def on_bar(self, bar, indicators):
        # 1. 检查现有订单的出场
        self.check_exit(bar)
        
        # 2. 策略逻辑
        rsi_val = indicators.get('rsi')
        
        if rsi_val is None or np.isnan(rsi_val):
            return

        # 反转策略：RSI > 70 超买做空， RSI < 30 超卖做多
        threshold_up = self.params['param1'] # e.g., 70
        threshold_down = self.params['param2'] # e.g., 30
        
        # 简单的单向持仓逻辑
        if len(self.active_trades) == 0:
            if rsi_val > threshold_up:
                self.open_position(Side.SHORT, bar.close, bar.time, sl_pips=50, tp_pips=100)
            elif rsi_val < threshold_down:
                self.open_position(Side.LONG, bar.close, bar.time, sl_pips=50, tp_pips=100)