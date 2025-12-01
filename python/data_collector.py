import argparse
import pandas as pd
import datetime
import time
import os
from forexconnect import ForexConnect, fxcorepy, LiveHistoryCreator, Common

# 设置 Pandas 显示格式
pd.set_option('display.max_columns', None)
pd.set_option('display.width', 1000)

class FXCMDataCollector:
    def __init__(self, username, password, url, connection, symbol="EUR/USD", timeframe="m1"):
        self.username = username
        self.password = password
        self.url = url
        self.connection = connection
        self.symbol = symbol
        self.timeframe = timeframe
        self.fx = None
        self.history_creator = None
        self.csv_filename = f"{symbol.replace('/', '')}_{timeframe}.csv"
    
    def connect(self):
        self.fx = ForexConnect()
        try:
            print(f"Connecting to FXCM ({self.connection})...")
            self.fx.login(self.username, self.password, self.url, self.connection)
            print("Connected successfully.")
        except Exception as e:
            print(f"Connection failed: {str(e)}")
            raise e
    
    def disconnect(self):
        if self.fx:
            self.fx.logout()
            print("Disconnected.")
    
    def format_dataframe(self, df: pd.DataFrame):
        """
        将FXCM返回的 BidOpen, BidHigh 等列重命名为系统通用的 open, high, low, close
        我们默认使用 Bid 价格作为回测价格 (也可以改为 Mid)
        """
        rename_map = {
            'BidOpen': 'open',
            'BidHigh': 'high',
            'BidLow': 'low',
            'BidClose': 'close',
            'Volume': 'volume',
            'Date': 'time'
        }

        df_clean = df.copy()
        df_clean.rename(columns=rename_map, inplace=True)

        needed_cols = ['open', 'high', 'low', 'close', 'volume']
        df_clean = df_clean[needed_cols]
        return df_clean
    
    def download_years_history(self, years=5):
        """
        分页下载过去n年的数据
        """
        print(f"Starting {years}-year historical download for {self.symbol}...")

        end_date = datetime.datetime.now()
        start_date_target = end_date - datetime.timedelta(days=365*years)
        
        all_dfs = []
        current_to_date = end_date

        iteration = 0
        while True:
            try:
                df_chunk = self.fx.get_history(self.symbol, self.timeframe, None, current_to_date, 100000)
                if df_chunk.empty:
                    print("No more data received.")
                    break

                all_dfs.append(df_chunk)
                first_date_in_chunk = df_chunk.index[0].to_pydatetime()
                print(f"  Received {len(df_chunk)} bars. Earliest: {first_date_in_chunk}")
                current_to_date = first_date_in_chunk
                # 检查是否已经下载完成指定的数据量
                if current_to_date <= start_date_target:
                    print("Reached target start date.")
                    break

                time.sleep(0.1)
            
            except Exception as e:
                print(f"Error during download: {str(e)}")
                break

            iteration += 1
        
        if not all_dfs:
            print("No data downloaded.")
            return

        # 合并所有数据 (注意：all_dfs 是倒序的，通过 concat 自动按 index 排序)
        full_df = pd.concat(all_dfs).sort_index()
        
        # 去重 (因为分页边界可能重叠)
        full_df = full_df[~full_df.index.duplicated(keep='first')]
        
        # 格式化
        clean_df = self.format_dataframe(full_df)

        # 截取精确的起始时间
        clean_df = clean_df[clean_df.index >= start_date_target]
        
        # 保存到 CSV
        clean_df.to_csv(self.csv_filename, index_label='time')
        print(f"Historical data saved to {self.csv_filename}. Total rows: {len(clean_df)}")
        return clean_df
    
    def start_realtime_recording(self):
        """
        启动实时监听，利用 LiveHistoryCreator 构建 K 线并写入 CSV
        """
        print(f"Starting Realtime Recording for {self.symbol}...")

    def _append_latest_bar_to_file(self, df: pd.DataFrame):
        """
        智能追加数据到文件
        """
        # 获取 CSV 文件中最后的时间戳（避免重复读取整个文件，可以使用 seek，这里用 pandas 简化）
        if not os.path.exists(self.csv_filename):
            df.to_csv(self.csv_filename, index_label='time')
            return
    
def parse_args():
    parser = argparse.ArgumentParser(description='FXCM Data Collector')
    parser.add_argument('-u', type=str, required=True, help='Username')
    parser.add_argument('-p', type=str, required=True, help='Password')
    parser.add_argument('-url', type=str, default='http://www.fxcorporate.com/Hosts.jsp', help='URL')
    parser.add_argument('-c', type=str, default='Demo', help='Connection (Demo/Real)')
    parser.add_argument('-i', type=str, default='EUR/USD', help='Instrument')
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()
    
    collector = FXCMDataCollector(
        username=args.u,
        password=args.p,
        url=args.url,
        connection=args.c,
        symbol=args.i,
        timeframe='m1' # 强制使用 M1，方便后续合成
    )

    try:
        collector.connect()
        
        # 1. 下载历史数据 (如果文件不存在，或者你想强制更新)
        # collector.download_5_years_history()
        
        # 如果只想追加历史数据中缺失的部分，逻辑会更复杂，建议首次先全量下载
        if not os.path.exists(collector.csv_filename):
             collector.download_years_history(years=5)
        else:
            print("History file exists. Jumping to realtime stream...")

        # 2. 开启实时录制
        # collector.start_realtime_recording()
        
    except Exception as e:
        print(f"Main Error: {e}")
    finally:
        collector.disconnect()