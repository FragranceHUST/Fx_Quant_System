import matplotlib.pyplot as plt

from models import Side

def plot_results(repo, strategy):
    df = repo.data[strategy.symbol]['1min']
    trades = strategy.trades
    
    # 只画最后回测的时间段
    if not trades:
        print("No trades to plot.")
        return
    
    start_dt = trades[0].entry_time
    end_dt = trades[-1].exit_time if trades[-1].exit_time else df.index[-1]
    
    plot_data = df.loc[(df.index >= start_dt) & (df.index <= end_dt)]
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8), sharex=True, gridspec_kw={'height_ratios': [3, 1]})
    
    # 1. 价格图
    ax1.plot(plot_data.index, plot_data['close'], label='Price', color='gray', alpha=0.5)
    
    # 标记买卖点
    long_entries = [t.entry_time for t in trades if t.side == Side.LONG]
    long_entry_prices = [t.entry_price for t in trades if t.side == Side.LONG]
    ax1.scatter(long_entries, long_entry_prices, marker='^', color='green', label='Buy')
    
    short_entries = [t.entry_time for t in trades if t.side == Side.SHORT]
    short_entry_prices = [t.entry_price for t in trades if t.side == Side.SHORT]
    ax1.scatter(short_entries, short_entry_prices, marker='v', color='red', label='Sell')
    
    ax1.set_title(f"{strategy.symbol} Backtest Results")
    ax1.legend()
    
    # 2. 权益曲线 (PnL)
    equity_data = strategy.cost_function.equity_curve
    if equity_data:
        times = [e['exit_time'] for e in equity_data]
        equities = [e['equity'] for e in equity_data]
        ax2.plot(times, equities, color='blue', label='Equity')
        ax2.set_title("Equity Curve")
        ax2.grid(True)
    
    plt.tight_layout()
    plt.savefig("backtest_result.png")
    plt.show()