import pandas as pd
import os

from models import Bar

class DataRepository:
    _instance = None
    
    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(DataRepository, cls).__new__(cls)
            cls._instance.data = {} # {'symbol': {'1min': df, '4H': df, '1D': df}}
            cls._instance.is_realtime = False
        return cls._instance

    def load_data_from_csv(self, symbol, filepath, timeframe='1min'):
        """加载历史数据用于回测"""
        if not os.path.exists(filepath):
            print(f"File not found: {filepath}")
            return
            
        df = pd.read_csv(filepath, parse_dates=['time'])
        df.set_index('time', inplace=True)
        
        if symbol not in self.data:
            self.data[symbol] = {}
        
        self.data[symbol][timeframe] = df
        
        # 自动重采样生成 4H 和 1D 数据 (如果只加载了1min)
        if timeframe == '1min':
            self._resample_data(symbol)

    def _resample_data(self, symbol):
        """基于1min数据生成4H和1D数据"""
        df_1min = self.data[symbol]['1min']
        
        ohlc_dict = {
            'open': 'first', 'high': 'max', 'low': 'min', 'close': 'last', 'volume': 'sum'
        }
        
        # 4H
        df_4h = df_1min.resample('4H').agg(ohlc_dict).dropna()
        self.data[symbol]['4H'] = df_4h
        
        # 1D
        df_1d = df_1min.resample('1D').agg(ohlc_dict).dropna()
        self.data[symbol]['1D'] = df_1d

    def get_latest_bars(self, symbol, timeframe, n=100):
        """获取最近N根K线"""
        if symbol in self.data and timeframe in self.data[symbol]:
            return self.data[symbol][timeframe].iloc[-n:]
        return pd.DataFrame()

    def update_realtime_bar(self, symbol, bar: Bar):
        """实盘中更新最新Bar并写入CSV"""
        # 简化逻辑：追加到DataFrame并保存
        # 实际生产中应该使用数据库或更高效的追加方式
        pass