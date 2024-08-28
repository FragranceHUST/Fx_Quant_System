#pragma once

class Trade;

class CostFunction
{
private:
    /** 胜率 */
    double winrate;
    /** 夏普比率sharpe ratio */
    double sharpe_ratio;
    /** 最大盈亏比 */
    double max_profitloss_ratio;
    /** 最大回撤 */
    double max_drawdown;
    /** 最大连续盈利次数 */
    int max_consecutive_profit;
    /** 最大连续亏损次数 */
    int max_consecutive_loss;
    /** 均方误差 */
    double mean_square_error;
    /** 绝对误差 */
    double absolute_error;
    /** 信息比率 */
    double information_ratio;
    /** 总佣金 */
    double total_commision;
    /** 总滑点 */
    int total_slippage;
    /** 交易次数 */
    int trade_cnt;
    /** 已平仓交易次数 **/
    int closed_trade_cnt;
    /** 平均持仓时间 */
    double avg_holding_time;
    /** 实现损益 Realized PnL **/
    double realized_PnL;
    /** 未实现损益 UnRealized PnL */
    double unrealized_PnL;
public:
    CostFunction();
    ~CostFunction();

    typedef std::vector<std::unique_ptr<Trade>> Trades;
    void calc_winrate(Trades&);
    void calc_sharpe(Trades&, double);
    void calc_maxPLratio(Trades&);
    void calc_maxdrawdown(Trades&);
    void calc_maxconprofit(Trades&);
    void calc_maxconloss(Trades&);
    void calc_meansqaurerror(Trades&);
    void calc_absoluterror(Trades&);
    void calc_tradecount(Trades&);
    void calc_avgtime(Trades&);
    void calc_realizedPnL(Trades&);
    void calc_unreadlizedPnL(Trades&, double);

    double get_winrate();
    double get_sharpe();
    double get_maxPLratio();
    double get_maxdrawdown();
    double get_meansquarerror();
    double get_absoluterror();
    double get_avgtime();
    double get_realizedPnL();
    double get_unrealizedPnL();
    int get_maxconprofit();
    int get_maxconloss();
    int get_tradecount();
};
