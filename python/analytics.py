import pandas as pd
import numpy as np

class CostFunction:
    def __init__(self):
        self.total_trades = 0
        self.win_rate = 0.0
        self.total_pnl = 0.0
        self.max_drawdown = 0.0
        self.sharpe_ratio = 0.0
        self.sortino_ratio = 0.0
        self.profit_factor = 0.0
        self.avg_win = 0.0
        self.avg_loss = 0.0
        self.equity_curve = [] # List of dicts {'time', 'equity'}

    def calc_cost_function(self, trades: list, initial_capital: float = 10000.0):
        if not trades:
            return
        
        # 转换为DataFrame方便计算
        df_trades = pd.DataFrame([t.__dict__ for t in trades])
        df_trades = df_trades[df_trades['status'] == 'CLOSED'].copy()
        
        if df_trades.empty:
            return

        # 1. 基础统计
        self.total_trades = len(df_trades)
        wins = df_trades[df_trades['pnl'] > 0]
        losses = df_trades[df_trades['pnl'] <= 0]
        
        self.win_rate = len(wins) / self.total_trades if self.total_trades > 0 else 0
        self.total_pnl = df_trades['pnl'].sum()
        
        gross_profit = wins['pnl'].sum()
        gross_loss = abs(losses['pnl'].sum())
        self.profit_factor = gross_profit / gross_loss if gross_loss != 0 else np.inf

        self.avg_win = wins['pnl'].mean() if not wins.empty else 0
        self.avg_loss = losses['pnl'].mean() if not losses.empty else 0

        # 2. 权益曲线与回撤
        df_trades = df_trades.sort_values('exit_time')
        df_trades['cum_pnl'] = df_trades['pnl'].cumsum()
        df_trades['equity'] = initial_capital + df_trades['cum_pnl']
        
        # 计算最大回撤
        running_max = df_trades['equity'].cummax()
        drawdowns = (df_trades['equity'] - running_max) / running_max
        self.max_drawdown = drawdowns.min() # 这是一个负数百分比

        # 3. 夏普率 & Sortino (假设无风险利率为0，基于交易分布而非时间分布的简化计算)
        returns = df_trades['pnl'] / initial_capital
        volatility = returns.std()
        
        if volatility != 0:
            self.sharpe_ratio = (returns.mean() / volatility) * np.sqrt(252) # 年化近似
        
        downside_returns = returns[returns < 0]
        downside_vol = downside_returns.std()
        if downside_vol != 0:
            self.sortino_ratio = (returns.mean() / downside_vol) * np.sqrt(252)

        # 生成权益曲线数据供绘图
        self.equity_curve = df_trades[['exit_time', 'equity']].to_dict('records')
        
    def to_csv(self, filepath):
        metrics = {k:v for k,v in self.__dict__.items() if k != 'equity_curve'}
        pd.DataFrame([metrics]).to_csv(filepath, index=False)
        print(f"Performance metrics saved to {filepath}")