from dataclasses import dataclass
from datetime import datetime
from enum import Enum

class Side(Enum):
    LONG = 1
    SHORT = -1

@dataclass
@dataclass
class Bar:
    symbol: str      # 交易品种符号
    time: datetime   # 时间戳
    open: float      # 开盘价
    high: float      # 最高价
    low: float       # 最低价
    close: float     # 收盘价
    volume: float    # 成交量
    period: str      # 时间周期，例如 '1min', '4H', '1D'

@dataclass
class Trade:
    symbol: str          # 交易品种符号
    side: Side           # 交易方向，LONG 或 SHORT
    entry_time: datetime # 开仓时间
    entry_price: float   # 开仓价格
    size: float          # 仓位大小
    sl: float = 0.0      # 止损价格
    tp: float = 0.0      # 止盈价格
    
    # 出场信息
    exit_time: datetime = None   # 平仓时间
    exit_price: float = 0.0      # 平仓价格
    commission: float = 0.0      # 手续费
    swap: float = 0.0            # 隔夜利息
    status: str = None           # 交易状态，OPEN 或 CLOSED
    
    @property
    def pnl(self):
        """计算浮动或最终盈亏 (Quote Currency)"""
        current_price = self.exit_price if self.status == "CLOSED" else self.entry_price
        # 简化的盈亏计算，未考虑点值换算，假设是直盘
        diff = current_price - self.entry_price if self.side == Side.LONG else self.entry_price - current_price
        return (diff * self.size) - self.commission - self.swap
    
    def close(self, exit_price: float, exit_time: datetime):
        """平仓"""
        self.exit_price = exit_price
        self.exit_time = exit_time
        self.status = "CLOSED"
    
    def start(self, entry_price: float, entry_time: datetime, size: float, sl: float = 0.0, tp: float = 0.0):
        """开仓"""
        self.entry_price = entry_price
        self.entry_time = entry_time
        self.size = size
        self.sl = sl
        self.tp = tp
        self.status = "OPEN"