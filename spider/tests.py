import pandas as pd
import numpy as np
from sklearn.cluster import KMeans
from sklearn.preprocessing import LabelEncoder
import matplotlib.pyplot as plt
import os
import pandas as pd
import time
import django
from tqdm import tqdm

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'CarSuv.settings')
django.setup()
from KeShiHua.models import XinXi, Info


car_infos = Info.objects.all()
car_year_list = []
car_name_list = []
car_num_list = []
car_max_price_list = []

for car_info in car_infos:
    if car_info.max_price == "暂无":
        continue
    car_year_list.append(int(car_info.car_year))
    car_name_list.append(car_info.car_name)
    car_num_list.append(int(car_info.car_num))

    car_max_price_list.append(float(str(car_info.max_price).replace("万", "")))


encoder = LabelEncoder()
car_name_list = encoder.fit_transform(car_name_list)

# 模拟生成一些SUV价格等相关特征的数据示例（实际应用中替换为真实数据）

data = {
    'price': car_max_price_list,  # 价格数据，随机生成范围示例
    'year': car_year_list,  # 其他特征示例，这里随机生成
    'name': car_name_list,
    'num': car_num_list,
}


df = pd.DataFrame(data)

# 选择要进行聚类分析的特征，这里只选择价格作为示例，实际可根据需求增加更多特征
X = df[['price']].values

# 确定聚类的数量，这里假设聚为3类，实际可根据业务需求等调整
n_clusters = 2
# 创建KMeans聚类模型对象并进行拟合
kmeans = KMeans(n_clusters=n_clusters, random_state=42)
kmeans.fit(X)

# 获取聚类的标签，标记每个数据点所属的聚类类别
labels = kmeans.labels_

# 将聚类标签添加到原始数据中
df['cluster'] = labels

# 计算各聚类中价格的平均值、中位数、范围等统计信息
stats = df.groupby('cluster')['price'].agg(['mean', 'median', 'min', 'max'])
print(stats)


# 分析不同聚类之间价格差异
for i in range(n_clusters):
    for j in range(i + 1, n_clusters):
        diff_mean = stats.loc[i, 'mean'] - stats.loc[j, 'mean']
        print(f"聚类{i}和聚类{j}的平均价格差异：{diff_mean}")

# 观察价格分布情况，可以简单绘制直方图看看每个聚类内的价格分布
for cluster in range(n_clusters):
    cluster_data = df[df['cluster'] == cluster]['price']

    plt.hist(cluster_data, alpha=0.5, label=f'Cluster {cluster}')


plt.xlabel('Price')
plt.ylabel('Frequency')
plt.title('Price Distribution in Clusters')
plt.legend()
plt.show()

# # 分析价格与其他特征的相关性（这里以简单示例展示与other_feature_1的相关性）
# for cluster in range(n_clusters):
#     cluster_df = df[df['cluster'] == cluster]
#     correlation = cluster_df['price'].corr(cluster_df['other_feature_1'])
#     print(f"聚类{cluster}中价格与其他特征1的相关性：{correlation}")