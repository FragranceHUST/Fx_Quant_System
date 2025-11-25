# main.py
import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import os

from data_repo import DataRepository
from strategy import Strategy, calc_rsi, RSIReversionStrategy
from models import Bar, Trade
from plotting import plot_results

# 导入上述类的定义 (如果在同一个文件中，无需导入)
# 假设所有类都在同一个文件中运行，或者你将它们存为模块

# --- 模拟数据生成器 ---
def generate_dummy_data(symbol, days=10):
    print(f"Generating dummy data for {symbol}...")
    dates = pd.date_range(end=datetime.now(), periods=60*24*days, freq='1min')
    
    # 随机游走生成价格
    np.random.seed(42)
    returns = np.random.normal(0, 0.0002, len(dates))
    price_path = 1.1000 * (1 + np.cumsum(returns))
    
    df = pd.DataFrame({
        'time': dates,
        'open': price_path,
        'high': price_path + 0.0005,
        'low': price_path - 0.0005,
        'close': price_path + np.random.normal(0, 0.0001, len(dates)),
        'volume': np.random.randint(100, 1000, len(dates))
    })
    
    # 确保 High/Low 正确
    df['high'] = df[['open', 'close', 'high']].max(axis=1)
    df['low'] = df[['open', 'close', 'low']].min(axis=1)
    
    filename = f"{symbol}_1min.csv"
    df.to_csv(filename, index=False)
    print(f"Saved to {filename}")
    return filename

# --- 主逻辑 ---
if __name__ == "__main__":
    symbol = "EURUSD"
    
    # 1. 准备数据 (模拟FXCM下载)
    csv_file = generate_dummy_data(symbol)
    
    # 2. 初始化数据仓库
    repo = DataRepository()
    repo.load_data_from_csv(symbol, csv_file)
    
    # 3. 初始化策略 (RSI反转: 周期14, 上界70, 下界30)
    strategy = RSIReversionStrategy(symbol, params={'rsi_period': 14, 'param1': 70, 'param2': 30})
    
    # 4. 运行回测 (最近5天)
    end_date = datetime.now()
    start_date = end_date - timedelta(days=5)
    
    strategy.run_simulation(start_date, end_date)
    
    # 5. 输出结果
    print("\n--- Performance Report ---")
    metrics = strategy.cost_function.__dict__
    for k, v in metrics.items():
        if k != 'equity_curve':
            print(f"{k}: {v}")
            
    strategy.cost_function.to_csv("performance_metrics.csv")
    
    # 6. 可视化
    try:
        plot_results(repo, strategy)
    except Exception as e:
        print(f"Visualization error: {e}")