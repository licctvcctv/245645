import requests
from bs4 import BeautifulSoup
from lxml import etree
import re
import json
import csv
import os
import pandas as pd
import time
import django
from tqdm import tqdm
import numpy as np
from sklearn.preprocessing import MinMaxScaler, StandardScaler

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'CarSuv.settings')
django.setup()
from KeShiHua.models import XinXi, Info


class spider:
    def __init__(self):
        self.headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36',
            'Cookie': '''QN1=0000f08034fc5f5bb3a044a2; QN57=17152579456180.24348271966234436; fid=3aa67936-1f5d-43e8-ae3c-5660868b80b2; QN67=193517%2C36; QN300=s%3Dbing; QN99=9805; QN48=tc_b0a055a570ad7a4b_192fcb9d622_a2b2; qunar-assist={%22version%22:%2220211215173359.925%22%2C%22show%22:false%2C%22audio%22:false%2C%22speed%22:%22middle%22%2C%22zomm%22:1%2C%22cursor%22:false%2C%22pointer%22:false%2C%22bigtext%22:false%2C%22overead%22:false%2C%22readscreen%22:false%2C%22theme%22:%22default%22}; QN205=organic; QN277=organic; QN269=99A57FA3B31511EF859F4E7E5097F6A5; ctt_june=1683616182042##iK3wWRPmahPwawPwasvOEPEhE2WTaDjAW2XAWstnaRv%3DWKoGVKDNWs3sE2aAiK3siK3saKgsasX8VKPwWSjmawPwaUvt; ctf_june=1683616182042##iK3wVRDAWhPwawPwasDOVKGDE2fhaDPsX%3DjmEK3%3DWP3%2BaPfRXsgNW%3DDsaPaOiK3siK3saKgsasX8VKPwWSjmVuPwaUvt; cs_june=474f4868f711458fd6001e10fef08f8bbcc3db1f143434ce079e1084c64c3eb4025b901a651b23a5bffbdb70ccd3871e598ebe6fb3026d22e6603ebdad6dcaf6b17c80df7eee7c02a9c1a6a5b97c117997f967cd8af82d84b5b586397d6159465a737ae180251ef5be23400b098dd8ca; QN271AC=register_pc; QN271SL=1ce268778cdf9e05e656964f908cc503; QN271RC=1ce268778cdf9e05e656964f908cc503; _q=U.xotttje3065; csrfToken=dp7ctqBqkWaULxXZwtrwPeMHZDdrNtyo; _s=s_XJDJ2DWVVMB43ZCGTQXQPEMSZM; _t=28999599; _v=ehmas9SEvC5MeAb0HnhgJRMUQlT0TyQDXbNSd9prIsU8MqJfT6kvtzov22WLdJmRX-GGTpCey2wwiU9qevf8GCY4f1SQXonuL09izXRNIc0v6jbPy8sfDUPHBJIwpIjvemc2HE_VUmtUuibJfUHBCbTDTkolr9mneLEBRlv74t9c; QN43=""; QN42=%E5%8E%BB%E5%93%AA%E5%84%BF%E7%94%A8%E6%88%B7; QN44=xotttje3065; _i=VInJOQJqqqwVAZ0qMsm397UHGm3q; _vi=S6KDlKpIADRinBi_hm8crph-xiMfNu6xCudYo6kO3j0xfSH5E3hma82lzwdbW7dQaF9wl5hdrXRiwBbLnvrzkY8CDfZcjxE1ZIvgI49lgdTOzTTX51bUToYjZstPRFIXvkm9jrQKvzyExvne1thAr-sYrFMaFiahG_07fquIrz2Z; QN267=154070539131539ab4; QN58=1733409082795%7C1733409609473%7C6; QN271=92195dba-25bf-4191-81ae-87fe53eccd97; JSESSIONID=5247AD2EBE1181E812AC97BC82227903; __qt=v1%7CVTJGc2RHVmtYMTgzOUJNL1ZjVkhPTXdEMk1NTVFOT0ZLYmZHNkZUak55KzlXWnBxMVpQWHdDakhWVXVWSkpObXlhcjdSZWtwNGdTNWRjNXkwTzF0K04yay9GZHlVRWRrZG1CRWEzMnJ5dVdDNythdldVSEQ0ZmM2emNDNkE0aTJON092RFlaTW52WndiZDdpNkplV1FJb0Nxa3pUSE1rVnJkdjczZjB2bGlvPQ%3D%3D%7C1733409707495%7CVTJGc2RHVmtYMStXa2Nxb1ppbHZkNG1KdVFMZ1hRTTIzYkxNditFYUw1RmNQYTM1MVB2UE5PRkVTR0VGMjdidzdlL3czQUNTTmptWEtOYlFTWDlqUFE9PQ%3D%3D%7CVTJGc2RHVmtYMTlFc0NPOVBEV3FxMjl5Ky9SZTM2bVlrSWNWVGlFWVFoWjlZRWJSbm5kbjBTVFJjeGhtWjFNdW96VUJiTUlGRDJPc1VVbGNBdUxURnRxVGdpSGwxbDVHNTNHYVp2ejdpVVU0VmVEUFJCNmR4c3paZUhWc2h1SWRNOXM2WTVYWWtTbzM2N21WR0liWE1QalJUZXpnTjI0REpIWXNwclNCSGNQc3l0VnpqcFBUamM3MzhwUXJoWVVCc0p5VkViaEtqcklNNVlmMFBOb0FyUlpCRUt3SzJSUlNOUFRjelNBOHlvNC9ZbnowNEhPQ0RmN3lTck50UXcwc01vZXdpKzBHdTE3UWVZZ3B6OGp5TDZ1cUFQZjVCK3d0UU1EWnNBVmd0OU5QZjVtZVREMHd5TnpaNXk4ck0rZGlyS2NZbStkVG80ejJUTFpjQjNIZXRKMXJDRmdNU2NjUDlZMi83Z3BRRUM0Rmk5dlJ4dDZZaWpnVjdqZm1ETE9remhVenJodk0rTG5PdUFpUVI1Wk9zdjBaZzdHK0dlMG5yQURPZTFjZ0xOODRISTNuMGp1d3FWWVdwYmM4RVJ1T3B3dGt3dE9XWUVqczZIQlpqNk5aN2lmYk1kemdYWS9Mb1VGbFo2YnJmYjRZeHJuTmtLQ0JXNVpMQW00N0V5WGUzUjdJNnlRS2d3SDJLNXE0dWZFeFVvUUJMRDcwUmJHeXlWV3ZvTVVPZnhEM3IrVFhTMmFXUTVXbTZqL2diWk5JLzh0Wnh3cktWZ0kwRXQ2ayt6ZGVXNm1aRlhhQU5JWE8yNE9kcjkxN2RWeUxwaUFmakdJZ0xXWlhxUnphT1EzdGhwcVoyS0YwMHFvOFdQNWxCQ0xHTGRhdno4QUFuZzVLYmNZRVpzQW5qSUZaanlLaDltQVpESFRZc0NaOWxIcDVmcjVWSjFZUzJ3allRTW5CcTJ0MDNRPT0%3D; QN63=%E5%B1%B1%E8%A5%BF%7C%E5%8C%97%E4%BA%AC'''
        }

    def init(self):
        if not os.path.exists('tempData.csv'):
            with open('tempData.csv', 'w', encoding='utf8', newline='') as csvfile:
                wirter = csv.writer(csvfile)
                wirter.writerow([
                    'year',
                    'car_name',
                    'car_price',
                    'car_num',
                    'car_img',
                ])

    def send_request(self, url):
        response = requests.get(url, headers=self.headers)
        if response.status_code == 200:
            return response
        else:
            return None

    def save_to_csv(self, row):
        with open('tempData.csv', 'a', encoding='utf8', newline='') as csvfile:
            wirter = csv.writer(csvfile)
            wirter.writerow(row)

    def save_to_sql(self):
        print("读取数据")
        with open('tempData.csv', 'r', encoding='utf8') as csvfile:
            readerCsv = csv.reader(csvfile)
            next(readerCsv)
            for travel in readerCsv:
                print(travel)
                price_list = str(travel[2]).split('-')
                min_price = price_list[0]
                if len(price_list) > 1:
                    max_price = price_list[1].replace("万", "")
                else:
                    max_price = min_price

                sale = "在售"
                if "停售" in travel[1]:
                    sale = "停售"

                Info.objects.create(
                    car_year=travel[0].replace("停售", "").replace(" ", ""),
                    car_sale=sale,
                    car_name=travel[1],
                    car_price=travel[2],
                    min_price=min_price,
                    max_price=max_price,
                    car_num=str(travel[3]).replace("辆", "").replace(",", ""),
                    car_img=travel[4]
                )


    def handle_space(self, content: str):
        return content.replace("\t", "")

    def start(self):
        for year in (2022, 2023, 2024):
            res = self.send_request(f"https://car.yiche.com/newcar/salesrank/?level=8&flag={year}")
            html_data = BeautifulSoup(res.text, 'html.parser')
            total_data = html_data.find('div', class_='total pg-item')
            total = total_data.find('span').text
            print(total)
            page_num = int(int(total) / 10) + 1
            for page in tqdm(range(1, page_num + 1)):
                time.sleep(3)
                print(year, page)
                res = self.send_request(f"https://car.yiche.com/newcar/salesrank/?level=8&flag={year}&page={page}")
                html_data = BeautifulSoup(res.text, 'html.parser')
                box_list = html_data.find_all('div', class_='rk-item ka')
                for box in box_list:
                    car_name = box.find('div', class_='rk-car-name').text
                    car_price = box.find('div', class_='rk-car-price').text
                    car_num = box.find('span', class_='rk-car-num').text
                    car_img = box.find('img', class_='rk-img lazyload')['data-original']

                    # print(car_name, car_price, car_num, car_img)

                    resultData = [year, self.handle_space(car_name), self.handle_space(car_price), self.handle_space(car_num), car_img]

                    self.save_to_csv(resultData)


    def clean_data(self):
        """
        对爬取的SUV汽车数据进行全面清洗
        - 处理缺失值
        - 去除重复数据
        - 异常值处理
        - 数据类型转换
        - 特征提取
        """
        print("开始数据清洗...")
        # 读取CSV文件
        df = pd.read_csv('tempData.csv')

        # 1. 处理缺失值
        print(f"清洗前数据行数: {len(df)}")

        # 将价格字段中的"万"去掉，并转换为浮点数
        df['car_price'] = df['car_price'].str.replace('万', '')

        # 分割价格区间为最低价和最高价
        df['min_price'] = df['car_price'].apply(lambda x: x.split('-')[0] if '-' in str(x) else x)
        df['max_price'] = df['car_price'].apply(lambda x: x.split('-')[1] if '-' in str(x) else x)

        # 转换为数值类型
        df['min_price'] = pd.to_numeric(df['min_price'], errors='coerce')
        df['max_price'] = pd.to_numeric(df['max_price'], errors='coerce')

        # 处理销量字段，去掉"辆"和逗号
        df['car_num'] = df['car_num'].str.replace('辆', '').str.replace(',', '')
        df['car_num'] = pd.to_numeric(df['car_num'], errors='coerce')

        # 2. 缺失值填充
        # 使用中位数填充价格缺失值
        df['min_price'].fillna(df['min_price'].median(), inplace=True)
        df['max_price'].fillna(df['max_price'].median(), inplace=True)

        # 使用中位数填充销量缺失值
        df['car_num'].fillna(df['car_num'].median(), inplace=True)

        # 3. 去除重复数据
        df.drop_duplicates(subset=['car_name'], keep='first', inplace=True)
        print(f"去重后数据行数: {len(df)}")

        # 4. 异常值处理 (使用Z-score方法)
        # 对价格进行异常值处理
        z_scores_min = np.abs((df['min_price'] - df['min_price'].mean()) / df['min_price'].std())
        z_scores_max = np.abs((df['max_price'] - df['max_price'].mean()) / df['max_price'].std())

        # 将超过3个标准差的价格视为异常值，使用中位数替换
        df.loc[z_scores_min > 3, 'min_price'] = df['min_price'].median()
        df.loc[z_scores_max > 3, 'max_price'] = df['max_price'].median()

        # 对销量进行异常值处理
        z_scores_num = np.abs((df['car_num'] - df['car_num'].mean()) / df['car_num'].std())
        df.loc[z_scores_num > 3, 'car_num'] = df['car_num'].median()

        # 5. 特征提取：从car_name中提取品牌
        df['brand'] = df['car_name'].apply(lambda x: str(x).split(' ')[0] if ' ' in str(x) else str(x))

        # 6. 确保数据量至少为100个车型
        if len(df) < 100:
            print(f"警告：当前数据量为{len(df)}，少于要求的100个车型")
        else:
            print(f"数据清洗完成，共有{len(df)}个车型")

        # 保存清洗后的数据
        df.to_csv('cleaned_data.csv', index=False)
        return df

    def standardize_data(self, df):
        """
        对数值型特征进行标准化和归一化处理
        - 对价格使用Min-Max归一化到[0,1]区间
        - 对销量使用Z-score标准化
        """
        print("开始数据标准化...")

        # 创建一个新的DataFrame来存储标准化后的数据
        df_std = df.copy()

        # 对价格使用Min-Max归一化
        min_max_scaler = MinMaxScaler()
        df_std[['min_price_norm', 'max_price_norm']] = min_max_scaler.fit_transform(df[['min_price', 'max_price']])

        # 对销量使用Z-score标准化
        std_scaler = StandardScaler()
        df_std['car_num_std'] = std_scaler.fit_transform(df[['car_num']])

        # 保存标准化后的数据
        df_std.to_csv('standardized_data.csv', index=False)
        print("数据标准化完成")
        return df_std

    def save_cleaned_data_to_sql(self):
        """
        将清洗和标准化后的数据保存到数据库
        """
        print("开始将清洗后的数据保存到数据库...")

        # 清洗数据
        df = self.clean_data()

        # 标准化数据
        df_std = self.standardize_data(df)

        # 清空原有数据
        Info.objects.all().delete()

        # 将处理后的数据保存到数据库
        for _, row in df_std.iterrows():
            sale = "在售"
            if "停售" in str(row['car_name']):
                sale = "停售"

            Info.objects.create(
                car_year=str(row['year']).replace("停售", "").replace(" ", ""),
                car_sale=sale,
                car_name=row['car_name'],
                car_price=f"{row['min_price']}-{row['max_price']}万" if row['min_price'] != row['max_price'] else f"{row['min_price']}万",
                min_price=float(row['min_price']),
                max_price=float(row['max_price']),
                car_num=int(row['car_num']),
                car_img=row['car_img'],
                # 保存标准化和归一化字段
                min_price_norm=float(row['min_price_norm']),
                max_price_norm=float(row['max_price_norm']),
                car_num_std=float(row['car_num_std']),
                brand=row['brand']
            )

        print(f"数据保存完成，共保存{len(df_std)}条记录")

if __name__ == '__main__':
    spiderObj = spider()
    # spiderObj.init()
    # spiderObj.start()
    # spiderObj.save_to_sql()
    spiderObj.save_cleaned_data_to_sql()
