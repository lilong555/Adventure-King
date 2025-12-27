# Adventure-King 赐福后端（LangChain）

这是一个用于演示的本地后端服务：游戏客户端（C++）通过 HTTP 请求该服务，由服务端使用 LangChain 调用 OpenAI 兼容接口，并用“工具调用”强约束赐福输出结构，最后返回可直接被游戏解析/应用的 JSON。

## 功能

- `POST /api/blessing/question`：生成“考验冒险决心”的问题
- `POST /api/blessing/answer`：玩家回答后返回赐福（2 条属性，覆盖旧 buff）

## 运行方式（WSL/Linux）

1. 进入目录：

   `cd tools/blessing_server`

2. 创建虚拟环境并安装依赖：

   `python3 -m venv .venv`

   `source .venv/bin/activate`

   `pip install -r requirements.txt`

3. 启动服务（默认端口 5181）：

   `python3 ak_blessing_server.py serve --host 0.0.0.0 --port 5181`

## OpenAI 兼容 Base URL

服务端默认使用公共 OpenAI 兼容网关：

- `https://elysiver.h-e.top/v1`

你可以通过环境变量覆盖（不会写入代码/仓库）：

- `AK_OPENAI_BASE_URL`：例如 `http://127.0.0.1:8000/v1`

示例：

`AK_OPENAI_BASE_URL="https://elysiver.h-e.top/v1" python3 ak_blessing_server.py serve --host 0.0.0.0 --port 5181`

## 客户端配置

游戏内赐福界面的 `baseUrl` 填写为**本服务地址**，例如：

- `http://127.0.0.1:5181`

`apiKey` 填写你的 OpenAI 兼容 Bearer Token（展示阶段手动输入）。

