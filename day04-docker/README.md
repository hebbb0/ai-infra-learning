## 10月2日：Docker Compose与Prometheus

### 服务

- FastAPI：http://127.0.0.1:8000
- API文档：http://127.0.0.1:8000/docs
- 指标接口：http://127.0.0.1:8000/metrics
- Prometheus：http://127.0.0.1:9090
- Prometheus目标：http://127.0.0.1:9090/targets

### 启动

```bash
docker compose up --build -d
### 
dockerfile描述怎么制作镜像
选择Python基础环境
        ↓
复制requirements.txt
        ↓
安装Python依赖
        ↓
复制项目代码
        ↓
指定FastAPI启动命令
###
compose.yaml描述整个应用由哪些服务组成：
API服务
Prometheus服务
容器网络
端口映射
配置文件挂载
启动顺序
#两个容器间的通讯
Windows浏览器访问API：
http://127.0.0.1:8000

Prometheus容器访问API：
http://api:8000
Prometheus容器中的localhost
└── 表示Prometheus容器自己

api
└── 表示Compose中的API容器